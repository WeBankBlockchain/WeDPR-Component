# coding:utf-8
import json
import os
import unittest
from enum import unique, Enum

from jsoncomment import JsonComment

from ppc_common.ppc_utils import utils
from ppc_common.ppc_utils.exception import PpcException, PpcErrorCode
from ppc_common.ppc_utils.utils import check_ppc_model_algorithm_is_homo, parse_n_class


@unique
class ModelAlgorithmType(Enum):
    HeteroLR = 1
    HomoLR = 2
    HeteroNN = 3
    HomoNN = 4
    HeteroXGB = 5


@unique
class OptimizerType(Enum):
    sgd = 1
    adam = 2


algorithm_types = [ModelAlgorithmType.HeteroLR.name, ModelAlgorithmType.HomoLR.name, ModelAlgorithmType.HeteroNN.name,
                   ModelAlgorithmType.HomoNN.name, ModelAlgorithmType.HeteroXGB.name]

optimizer_types = [OptimizerType.sgd.name, OptimizerType.adam.name]

default_epochs = 10
default_threads = 8


FILE_PATH = os.path.abspath(__file__)
CURRENT_PATH = os.path.abspath(os.path.dirname(FILE_PATH) + os.path.sep + ".")


def get_dir():
    ppc_model_template_dir = f'{CURRENT_PATH}{os.sep}..{os.sep}ppc_model_template{os.sep}'
    return ppc_model_template_dir


def parse_read_hetero_dataset_loop(participants):
    loop_start = []
    loop_end = []
    start = ''
    end = ''
    for i in range(participants):
        if i == 0 or i == participants - 1:
            if i == 0:
                start = f'{start}source{i}_feature_count'
                end = f'{start} + source{i + 1}_feature_count'
            else:
                start = f'{start} + source{i}_feature_count'
                end = f'{start} + source{i + 1}_feature_count'
        else:
            start = f'{start} + source{i}_feature_count'
            end = f'{start} + source{i + 1}_feature_count'
        loop_start.append(start)
        loop_end.append(end)
    return participants - 1, loop_start[0:participants - 1], loop_end[0:participants - 1]


def parse_read_homo_dataset_loop(participants):
    loop_start = []
    loop_end = []
    start = ''
    end = ''
    for i in range(participants):
        if i == 0 or i == participants - 1:
            if i == 0:
                start = f'{start}source{i}_record_count'
                end = f'{start} + source{i + 1}_record_count'
            else:
                start = f'{start} + source{i}_record_count'
                end = f'{start} + source{i + 1}_record_count'
        else:
            start = f'{start} + source{i}_record_count'
            end = f'{start} + source{i + 1}_record_count'
        loop_start.append(start)
        loop_end.append(end)
    return participants - 1, loop_start[0:participants - 1], loop_end[0:participants - 1]


def insert_train_record_count(layer, index, record_type):
    if 'Dense' in layer:
        layer_arr = layer.split('(')
    else:
        layer_arr = layer.split('([')
    if index == 0 and 'Dense' in layer:
        new_layer = f'{layer_arr[0]}({record_type}, total_feature_count, {layer_arr[1]}'
    else:
        if 'Dense' in layer:
            new_layer = f'{layer_arr[0]}({record_type}, {layer_arr[1]}'
        else:
            new_layer = f'{layer_arr[0]}([{record_type}, {layer_arr[1]}'
    if 'Conv2d' in new_layer:
        if ', [' in new_layer:
            layer_arr2 = new_layer.split(', [')
            new_layer = f'{layer_arr2[0]}, [{record_type}, {layer_arr2[1]}'
        if ',[' in new_layer:
            layer_arr2 = new_layer.split(',[')
            new_layer = f'{layer_arr2[0]},[{record_type}, {layer_arr2[1]}'
    return new_layer


def set_nn_layers(mpc_train_algorithm, layers, record_type):
    mpc_train_algorithm = f"{mpc_train_algorithm}\n"
    if layers:
        layers_str = 'layers = ['
        for i in range(len(layers)):
            new_layer = insert_train_record_count(layers[i], i, record_type)
            layers_str = f"{layers_str}{new_layer},\n"
        n_class = parse_n_class(layers[-1])
        mpc_train_algorithm = f"{mpc_train_algorithm}{layers_str}"

        if n_class == 1:
            mpc_train_algorithm = f"{mpc_train_algorithm}ml.Output({record_type}, approx=3)]\n\n"
            mpc_train_algorithm = f"{mpc_train_algorithm}test_Y = pfix.Array(test_record_count)\n"
            mpc_train_algorithm = f"{mpc_train_algorithm}test_X = pfix.Matrix(test_record_count, total_feature_count)\n"
        elif n_class > 1:
            mpc_train_algorithm = f"{mpc_train_algorithm}ml.MultiOutput({record_type}, {n_class})]\n\n"
            mpc_train_algorithm = f"{mpc_train_algorithm}total_class_count = {n_class}\n"
            mpc_train_algorithm = f"{mpc_train_algorithm}test_Y = pint.Matrix(test_record_count, total_class_count)\n"
            mpc_train_algorithm = f"{mpc_train_algorithm}test_X = pfix.Matrix(test_record_count, total_feature_count)\n"
        else:
            raise PpcException(PpcErrorCode.ALGORITHM_PPC_MODEL_OUTPUT_NUMBER_ERROR.get_code(),
                               PpcErrorCode.ALGORITHM_PPC_MODEL_OUTPUT_NUMBER_ERROR.get_msg())
    else:
        mpc_train_algorithm = f"{mpc_train_algorithm}test_Y = pfix.Array(test_record_count)\n"
        mpc_train_algorithm = f"{mpc_train_algorithm}test_X = pfix.Matrix(test_record_count, total_feature_count)\n"
        mpc_train_algorithm = f"{mpc_train_algorithm}" \
                              f"layers = [pDense({record_type}, total_feature_count, 128, " \
                              f"activation='relu'), pDense({record_type}, 128, 1), " \
                              f"ml.Output({record_type}, approx=3)]\n"
    mpc_train_algorithm = f"{mpc_train_algorithm}\n"
    return mpc_train_algorithm


def set_logreg_train_layers(mpc_algorithm):
    mpc_algorithm = f"{mpc_algorithm}\n"
    mpc_algorithm = f"{mpc_algorithm}layers = [pDense(train_record_count, total_feature_count, 1), ml.Output(train_record_count, approx=3)]\n\n"
    mpc_algorithm = f"{mpc_algorithm}test_Y = pfix.Array(test_record_count)\n"
    mpc_algorithm = f"{mpc_algorithm}test_X = pfix.Matrix(test_record_count, total_feature_count)\n"
    return mpc_algorithm


def generate_set_logreg_predict_layers(mpc_algorithm):
    mpc_algorithm = f"{mpc_algorithm}\n"
    mpc_algorithm = f"{mpc_algorithm}layers = [pDense(test_record_count, total_feature_count, 1), ml.Output(test_record_count, approx=3)]\n"
    return mpc_algorithm


def generate_homo_predict_static_template(mpc_predict_algorithm):
    homo_nn_static_template = utils.read_content_from_file(
        f'{get_dir()}homo_predict_static_template.mpc')
    mpc_predict_algorithm = f'{mpc_predict_algorithm}\n'
    mpc_predict_algorithm = f'{mpc_predict_algorithm}{homo_nn_static_template}'
    return mpc_predict_algorithm


def set_hetero_train_static_template(model_config_dict, mpc_train_algorithm):
    optimizer = model_config_dict['optimizer']
    learning_rate = model_config_dict['learning_rate']
    if optimizer == OptimizerType.sgd.name:
        hetero_train_static_template = utils.read_content_from_file(
            f'{get_dir()}hetero_train_sgd_static_template.mpc')
        hetero_train_static_template = hetero_train_static_template.replace('gamma = MemValue(cfix(.1))',
                                                                            f'gamma = MemValue(cfix({learning_rate}))')
    elif optimizer == OptimizerType.adam.name:
        hetero_train_static_template = utils.read_content_from_file(
            f'{get_dir()}hetero_train_adam_static_template.mpc')
        hetero_train_static_template = hetero_train_static_template.replace('gamma = MemValue(cfix(.001))',
                                                                            f'gamma = MemValue(cfix({learning_rate}))')
    mpc_train_algorithm = f'{mpc_train_algorithm}\n'
    mpc_train_algorithm = f'{mpc_train_algorithm}{hetero_train_static_template}'
    return mpc_train_algorithm


def generate_hetero_predict_static_template(mpc_predict_algorithm):
    hetero_logreg_static_template = utils.read_content_from_file(
        f'{get_dir()}hetero_predict_static_template.mpc')
    mpc_predict_algorithm = f'{mpc_predict_algorithm}\n'
    mpc_predict_algorithm = f'{mpc_predict_algorithm}{hetero_logreg_static_template}'
    return mpc_predict_algorithm


def set_homo_train_static_template(model_config_dict, mpc_train_algorithm):
    optimizer = model_config_dict['optimizer']
    learning_rate = model_config_dict['learning_rate']
    if optimizer == OptimizerType.sgd.name:
        homo_train_static_template = utils.read_content_from_file(
            f'{get_dir()}homo_train_sgd_static_template.mpc')
        homo_train_static_template = homo_train_static_template.replace('gamma = MemValue(cfix(.1))',
                                                                        f'gamma = MemValue(cfix({learning_rate}))')
    elif optimizer == OptimizerType.adam.name:
        homo_train_static_template = utils.read_content_from_file(
            f'{get_dir()}homo_train_adam_static_template.mpc')
        homo_train_static_template = homo_train_static_template.replace('gamma = MemValue(cfix(.001))',
                                                                        f'gamma = MemValue(cfix({learning_rate}))')
    mpc_train_algorithm = f'{mpc_train_algorithm}\n'
    mpc_train_algorithm = f'{mpc_train_algorithm}{homo_train_static_template}'
    return mpc_train_algorithm


def generate_set_common_code(is_psi, participants):
    mpc_algorithm = '#This file is auto generated by ppc. DO NOT EDIT!\n\n'
    if is_psi:
        mpc_algorithm = f'{mpc_algorithm}#PSI_OPTION=True'
    else:
        mpc_algorithm = f'{mpc_algorithm}#PSI_OPTION=False'
    mpc_algorithm = f'{mpc_algorithm}\n'
    mpc_algorithm = f'{mpc_algorithm}from ppc import *\n'
    mpc_algorithm = f'{mpc_algorithm}from Compiler import config\n'
    mpc_algorithm = f'{mpc_algorithm}import sys\n'
    mpc_algorithm = f'{mpc_algorithm}program.options_from_args()\n\n'
    mpc_algorithm = f'{mpc_algorithm}program.use_trunc_pr = True\n'
    mpc_algorithm = f'{mpc_algorithm}program.use_split(3)\n\n'
    # if participants == 3:
    #     mpc_algorithm = f'{mpc_algorithm}program.use_trunc_pr = True\n'
    #     mpc_algorithm = f'{mpc_algorithm}program.use_split(3)\n\n'
    for i in range(participants):
        mpc_algorithm = f'{mpc_algorithm}SOURCE{i}={i}\n'
    return mpc_algorithm


def set_hetero_feature_count(mpc_algorithm, participants):
    total_feature_count_str = 'total_feature_count='
    for i in range(participants):
        mpc_algorithm = f'{mpc_algorithm}source{i}_feature_count=$(source{i}_feature_count)\n'
        if i == participants - 1:
            total_feature_count_str = f'{total_feature_count_str}source{i}_feature_count'
        else:
            total_feature_count_str = f'{total_feature_count_str}source{i}_feature_count+'
    mpc_algorithm = f'{mpc_algorithm}{total_feature_count_str}\n'
    return mpc_algorithm


def set_homo_train_record_count(mpc_train_algorithm, participants):
    total_record_count_str = 'total_record_count='
    for i in range(participants):
        mpc_train_algorithm = f'{mpc_train_algorithm}source{i}_record_count=$(source{i}_record_count)\n'
        if i == participants - 1:
            total_record_count_str = f'{total_record_count_str}source{i}_record_count'
        else:
            total_record_count_str = f'{total_record_count_str}source{i}_record_count+'
    mpc_train_algorithm = f'{mpc_train_algorithm}{total_record_count_str}\n'
    mpc_train_algorithm = f'{mpc_train_algorithm}train_record_count=$(train_record_count)\n'
    mpc_train_algorithm = f'{mpc_train_algorithm}test_record_count=$(test_record_count)\n'
    return mpc_train_algorithm


def generate_set_homo_predict_record_count(mpc_predict_algorithm):
    mpc_predict_algorithm = f'{mpc_predict_algorithm}total_feature_count=$(total_feature_count)\n'
    mpc_predict_algorithm = f'{mpc_predict_algorithm}test_record_count=$(test_record_count)\n'
    mpc_predict_algorithm = f'{mpc_predict_algorithm}test_X = pfix.Matrix(test_record_count, total_feature_count)\n'
    return mpc_predict_algorithm


def set_hetero_train_record_count(mpc_train_algorithm):
    mpc_train_algorithm = f'{mpc_train_algorithm}\n'
    mpc_train_algorithm = f'{mpc_train_algorithm}total_record_count=$(total_record_count)\n'
    mpc_train_algorithm = f'{mpc_train_algorithm}train_record_count=$(train_record_count)\n'
    mpc_train_algorithm = f'{mpc_train_algorithm}test_record_count=$(test_record_count)\n'
    mpc_train_algorithm = f'{mpc_train_algorithm}\n'
    return mpc_train_algorithm


def generate_set_hetero_predict_record_count(mpc_predict_algorithm):
    mpc_predict_algorithm = f'{mpc_predict_algorithm}\n'
    mpc_predict_algorithm = f'{mpc_predict_algorithm}test_record_count=$(test_record_count)\n'
    mpc_predict_algorithm = f'{mpc_predict_algorithm}test_X = pfix.Matrix(test_record_count, total_feature_count)\n'
    return mpc_predict_algorithm


def set_homo_feature_count(mpc_train_algorithm):
    mpc_train_algorithm = f'{mpc_train_algorithm}\n'
    mpc_train_algorithm = f'{mpc_train_algorithm}total_feature_count=$(total_feature_count)\n'
    mpc_train_algorithm = f'{mpc_train_algorithm}\n'
    return mpc_train_algorithm


def read_hetero_train_dataset(mpc_train_algorithm, participants):
    mpc_train_algorithm = f'{mpc_train_algorithm}def read_hetero_dataset():\n'
    mpc_train_algorithm = f'{mpc_train_algorithm}\tprint(f"train_record_count:{{train_record_count}}")\n'
    mpc_train_algorithm = f'{mpc_train_algorithm}\tprint(f"test_record_count:{{test_record_count}}")\n'
    mpc_train_algorithm = f'{mpc_train_algorithm}\tdo_read_hetero_y_part(SOURCE0)\n'
    for i in range(participants):
        mpc_train_algorithm = f'{mpc_train_algorithm}\tdo_read_hetero_x_part(SOURCE{i}, source{i}_feature_count)\n'
    return mpc_train_algorithm


def generate_read_hetero_predict_dataset(mpc_predict_algorithm, participants):
    mpc_predict_algorithm = f'{mpc_predict_algorithm}\n'
    mpc_predict_algorithm = f'{mpc_predict_algorithm}def read_hetero_test_dataset():\n'
    for i in range(participants):
        mpc_predict_algorithm = f'{mpc_predict_algorithm}\tdo_read_hetero_x_part(SOURCE{i}, source{i}_feature_count)\n'
    return mpc_predict_algorithm


def read_homo_dataset(mpc_train_algorithm, participants):
    mpc_train_algorithm = f'{mpc_train_algorithm}def read_homo_dataset():\n'
    mpc_train_algorithm = f'{mpc_train_algorithm}\tprint(f"total_feature_count:{{total_feature_count}}")\n'
    mpc_train_algorithm = f'{mpc_train_algorithm}\tprint(f"train_record_count:{{train_record_count}}")\n'
    mpc_train_algorithm = f'{mpc_train_algorithm}\tprint(f"test_record_count:{{test_record_count}}")\n'
    for i in range(participants):
        mpc_train_algorithm = f'{mpc_train_algorithm}\tdo_read_homo_dataset(SOURCE{i}, source{i}_record_count)\n'

    return mpc_train_algorithm


def set_parameters(model_config_dict, mpc_train_algorithm):
    mpc_train_algorithm = f'{mpc_train_algorithm}\n'
    epochs = model_config_dict['epochs']
    batch_size = model_config_dict['batch_size']
    threads = model_config_dict['threads']
    if epochs <= 0:
        mpc_train_algorithm = f'{mpc_train_algorithm}epochs={default_epochs}\n'
    else:
        mpc_train_algorithm = f'{mpc_train_algorithm}epochs={epochs}\n'
    if int(batch_size) <= 0:
        mpc_train_algorithm = f'{mpc_train_algorithm}batch_size=train_record_count\n'
    else:
        mpc_train_algorithm = f'{mpc_train_algorithm}user_batch_size={batch_size}\n'
        mpc_train_algorithm = f'{mpc_train_algorithm}batch_size=min(user_batch_size, min(128, train_record_count))\n'
    if threads <= 0:
        mpc_train_algorithm = f'{mpc_train_algorithm}threads={default_threads}\n'
    else:
        mpc_train_algorithm = f'{mpc_train_algorithm}threads={threads}\n'
    return mpc_train_algorithm


def generate_mpc_train_algorithm(model_config_dict, algorithm_name, is_psi):
    participants = model_config_dict['participants']
    mpc_train_algorithm = generate_set_common_code(is_psi, participants)
    if algorithm_name == ModelAlgorithmType.HeteroLR.name or algorithm_name == ModelAlgorithmType.HeteroNN.name:
        mpc_train_algorithm = set_hetero_feature_count(
            mpc_train_algorithm, participants)
        mpc_train_algorithm = set_hetero_train_record_count(
            mpc_train_algorithm)
        mpc_train_algorithm = read_hetero_train_dataset(
            mpc_train_algorithm, participants)
        mpc_train_algorithm = set_parameters(
            model_config_dict, mpc_train_algorithm)
        if algorithm_name == ModelAlgorithmType.HeteroLR.name:
            if 'layers' in model_config_dict.keys():
                raise PpcException(PpcErrorCode.ALGORITHM_PPC_MODEL_LAYERS_ERROR.get_code(),
                                   PpcErrorCode.ALGORITHM_PPC_MODEL_LAYERS_ERROR.get_msg())
            mpc_train_algorithm = set_logreg_train_layers(mpc_train_algorithm)
            mpc_train_algorithm = set_hetero_train_static_template(
                model_config_dict, mpc_train_algorithm)
        else:
            layers = []
            if 'layers' in model_config_dict.keys():
                layers = model_config_dict['layers']
                if layers and 'Conv2d' in layers[0]:
                    raise PpcException(PpcErrorCode.ALGORITHM_PPC_MODEL_LAYERS_ERROR2.get_code(),
                                       PpcErrorCode.ALGORITHM_PPC_MODEL_LAYERS_ERROR2.get_msg())
            mpc_train_algorithm = set_nn_layers(
                mpc_train_algorithm, layers, 'train_record_count')
            mpc_train_algorithm = set_hetero_train_static_template(
                model_config_dict, mpc_train_algorithm)
    elif algorithm_name == ModelAlgorithmType.HomoLR.name or algorithm_name == ModelAlgorithmType.HomoNN.name:
        mpc_train_algorithm = set_homo_train_record_count(
            mpc_train_algorithm, participants)
        mpc_train_algorithm = set_parameters(
            model_config_dict, mpc_train_algorithm)
        mpc_train_algorithm = set_homo_feature_count(mpc_train_algorithm)
        mpc_train_algorithm = read_homo_dataset(
            mpc_train_algorithm, participants)
        if algorithm_name == ModelAlgorithmType.HomoLR.name:
            if 'layers' in model_config_dict.keys():
                raise PpcException(PpcErrorCode.ALGORITHM_PPC_MODEL_LAYERS_ERROR.get_code(),
                                   PpcErrorCode.ALGORITHM_PPC_MODEL_LAYERS_ERROR.get_msg())
            mpc_train_algorithm = set_logreg_train_layers(mpc_train_algorithm)
            mpc_train_algorithm = set_homo_train_static_template(
                model_config_dict, mpc_train_algorithm)
        else:
            layers = []
            if 'layers' in model_config_dict.keys():
                layers = model_config_dict['layers']
            mpc_train_algorithm = set_nn_layers(
                mpc_train_algorithm, layers, 'train_record_count')
            mpc_train_algorithm = set_homo_train_static_template(
                model_config_dict, mpc_train_algorithm)

    return mpc_train_algorithm


def generate_mpc_predict_algorithm(algorithm_name, layers, participants, is_psi):
    mpc_predict_algorithm = generate_set_common_code(is_psi, participants)
    if algorithm_name == ModelAlgorithmType.HeteroLR.name or algorithm_name == ModelAlgorithmType.HeteroNN.name:
        mpc_predict_algorithm = set_hetero_feature_count(
            mpc_predict_algorithm, participants)
        mpc_predict_algorithm = generate_set_hetero_predict_record_count(
            mpc_predict_algorithm)
        mpc_predict_algorithm = generate_read_hetero_predict_dataset(
            mpc_predict_algorithm, participants)
        if algorithm_name == ModelAlgorithmType.HeteroLR.name:
            mpc_predict_algorithm = generate_set_logreg_predict_layers(
                mpc_predict_algorithm)
        else:
            mpc_predict_algorithm = set_nn_layers(
                mpc_predict_algorithm, layers, 'test_record_count')
        mpc_predict_algorithm = generate_hetero_predict_static_template(
            mpc_predict_algorithm)

    elif algorithm_name == ModelAlgorithmType.HomoLR.name or algorithm_name == ModelAlgorithmType.HomoNN.name:
        mpc_predict_algorithm = generate_set_homo_predict_record_count(
            mpc_predict_algorithm)
        if algorithm_name == ModelAlgorithmType.HomoLR.name:
            mpc_predict_algorithm = generate_set_logreg_predict_layers(
                mpc_predict_algorithm)
        else:
            mpc_predict_algorithm = set_nn_layers(
                mpc_predict_algorithm, layers, 'test_record_count')
        mpc_predict_algorithm = generate_homo_predict_static_template(
            mpc_predict_algorithm)

    return mpc_predict_algorithm

    # try:
    #     test_dataset_percentage = float(model_config_dict['test_dataset_percentage'])
    #     model_config_dict['test_dataset_percentage'] = test_dataset_percentage
    #     if not 0 < test_dataset_percentage <= 0.5:
    #         raise PpcException(PpcErrorCode.ALGORITHM_PPC_MODEL_TEST_DATASET_PERCENTAGE_ERROR.get_code(),
    #                            PpcErrorCode.ALGORITHM_PPC_MODEL_TEST_DATASET_PERCENTAGE_ERROR.get_msg())
    # except BaseException as e:
    #     raise PpcException(PpcErrorCode.ALGORITHM_PPC_MODEL_TEST_DATASET_PERCENTAGE_ERROR.get_code(),
    #                        PpcErrorCode.ALGORITHM_PPC_MODEL_TEST_DATASET_PERCENTAGE_ERROR.get_msg())

    # try:
    #     learning_rate = float(model_config_dict['learning_rate'])
    #     model_config_dict['learning_rate'] = learning_rate
    #     if not 0 < learning_rate <= 1:
    #         raise PpcException(PpcErrorCode.ALGORITHM_PPC_MODEL_LEARNING_RATE_ERROR.get_code(),
    #                            PpcErrorCode.ALGORITHM_PPC_MODEL_LEARNING_RATE_ERROR.get_msg())
    # except BaseException as e:
    #     raise PpcException(PpcErrorCode.ALGORITHM_PPC_MODEL_LEARNING_RATE_ERROR.get_code(),
    #                        PpcErrorCode.ALGORITHM_PPC_MODEL_LEARNING_RATE_ERROR.get_msg())

    # try:
    #     num_trees = int(model_config_dict['num_trees'])
    #     model_config_dict['num_trees'] = num_trees
    #     if not 0 < num_trees <= 300:
    #         raise PpcException(PpcErrorCode.ALGORITHM_PPC_MODEL_TREES_ERROR.get_code(),
    #                            PpcErrorCode.ALGORITHM_PPC_MODEL_TREES_ERROR.get_msg())
    # except BaseException as e:
    #     raise PpcException(PpcErrorCode.ALGORITHM_PPC_MODEL_TREES_ERROR.get_code(),
    #                        PpcErrorCode.ALGORITHM_PPC_MODEL_TREES_ERROR.get_msg())

    # try:
    #     max_depth = int(model_config_dict['max_depth'])
    #     model_config_dict['max_depth'] = max_depth
    #     if not 0 < max_depth <= 10:
    #         raise PpcException(PpcErrorCode.ALGORITHM_PPC_MODEL_DEPTH_ERROR.get_code(),
    #                            PpcErrorCode.ALGORITHM_PPC_MODEL_DEPTH_ERROR.get_msg())
    # except BaseException as e:
    #     raise PpcException(PpcErrorCode.ALGORITHM_PPC_MODEL_DEPTH_ERROR.get_code(),
    #                        PpcErrorCode.ALGORITHM_PPC_MODEL_DEPTH_ERROR.get_msg())

    # try:
    #     threads = int(model_config_dict['threads'])
    #     model_config_dict['threads'] = threads
    #     if not (0 < threads <= 8):
    #         raise PpcException(PpcErrorCode.ALGORITHM_PPC_MODEL_THREADS_ERROR.get_code(),
    #                            PpcErrorCode.ALGORITHM_PPC_MODEL_THREADS_ERROR.get_msg())
    # except BaseException as e:
    #     raise PpcException(PpcErrorCode.ALGORITHM_PPC_MODEL_THREADS_ERROR.get_code(),
    #                        PpcErrorCode.ALGORITHM_PPC_MODEL_THREADS_ERROR.get_msg())
