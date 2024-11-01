# -*- coding: utf-8 -*-
import numpy as np
import sys
import os
import argparse
from enum import Enum
import random


class DataType(Enum):
    TRAIN = "train",
    PREDICT = "predict",
    ALL = "all"

    @classmethod
    def has_value(cls, value):
        return value in cls._value2member_map_

    @classmethod
    def value_of(cls, label):
        if label in cls.TRAIN.value:
            return cls.TRAIN
        elif label in cls.PREDICT.value:
            return cls.PREDICT
        elif label in cls.ALL.value:
            return cls.ALL
        else:
            raise NotImplementedError


def parse_args():
    parser = argparse.ArgumentParser(prog=sys.argv[0])
    parser.add_argument("-f", '--feature_size',
                        help='the feature size', required=True)
    parser.add_argument("-s", '--sample_capacity',
                        help='the faked data size(in MB, default 1MB)', default=1, required=False)
    parser.add_argument("-d", '--id_file',
                        help='the id file', required=False)
    parser.add_argument("-S", '--sample_file',
                        help='the file to store the faked data', required=True)
    parser.add_argument("-i", "--start_id",
                        help="the start id", required=False, default=0)
    parser.add_argument("-I", "--ignore_header",
                        help="ignore the file header", required=False, default=False)
    parser.add_argument("-m", "--with_missing_value", help="with missing value",
                        required=False, default=False)
    parser.add_argument("-p", "--missing_percent", required=False,
                        default=10, help="the missing value percentage")
    parser.add_argument("-t", "--data_type", required=False,
                        default=DataType.ALL.value, help="the dataType, now support 'all'/'predict'/'train'")
    parser.add_argument("-F", "--feature_prefix", required=False,
                        default="x", help="the feature prefix")
    args = parser.parse_args()
    return args


def generate_id_list(id_fp, granularity, id):
    if id_fp is None:
        return (granularity, np.array(range(id, id + granularity), dtype=str))
    lines = id_fp.readlines(granularity * 1024)
    data = [line.split(",")[0].strip() for line in lines]
    return (len(data), data)


class FileInfo:
    def __init__(self, fp, fmt_setting):
        self.fp = fp
        self.fmt_setting = fmt_setting


def generate_header_for_given_data(file_name, ignore_header, data_type, feature_size, feature_prefix):
    if ignore_header:
        return
    # write the header
    f = open(file_name, "a")
    # write the header
    fmt_setting = "%s"
    if data_type == DataType.PREDICT:
        f.write("id")
    if data_type == DataType.TRAIN:
        f.write("id,y")
        fmt_setting = "%s,%s"
    for j in range(feature_size):
        fmt_setting = fmt_setting + ",%s"
        f.write(f",{feature_prefix}{str(j)}")
    f.write("\n")
    return FileInfo(f, fmt_setting)


def generate_header(args, data_type, feature_size):
    train_file_info = None
    predict_file_info = None
    if data_type == DataType.PREDICT:
        predict_file_info = generate_header_for_given_data(
            args.sample_file, args.ignore_header, data_type, feature_size, args.feature_prefix)
    if data_type == DataType.TRAIN:
        train_file_info = generate_header_for_given_data(
            args.sample_file, args.ignore_header, data_type, feature_size, args.feature_prefix)
    if data_type == DataType.ALL:
        train_file_info = generate_header_for_given_data(
            args.sample_file, args.ignore_header, DataType.TRAIN, feature_size, args.feature_prefix)
        predict_file_info = generate_header_for_given_data(
            args.sample_file + ".predict", args.ignore_header, DataType.PREDICT, feature_size, args.feature_prefix)
    return (train_file_info, predict_file_info)


def fake_data(args):
    data_type = DataType.value_of(args.data_type)
    sample_capacity_bytes = None
    if args.sample_capacity is not None:
        sample_capacity_bytes = int(args.sample_capacity) * 1024 * 1024
    feature_size = int(args.feature_size)
    granularity = 100
    id = 0
    (train_file_info, predict_file_info) = generate_header(
        args, data_type, feature_size)
    if args.start_id is not None:
        id = int(args.start_id)
    id_fp = None
    if args.id_file is not None:
        id_fp = open(args.id_file, "r")
        # skip the header
        id_fp.readline()
    # write the header
    while True:
        (rows, id_list) = generate_id_list(id_fp, granularity, id)
        if rows == 0:
            break
        id += rows
        feature_sample = np.random.standard_normal(
            (rows, feature_size))
        if args.with_missing_value:
            for i in range(int(args.missing_percent)):
                selected_line = random.randrange(rows)
                selected_feature = random.randrange(feature_size)
                feature_sample[selected_line, selected_feature] = np.nan
        if train_file_info is not None:
            train_sample = feature_sample.astype("str")
            y_list = np.random.randint(0, 2, rows)
            # insert the y_list
            train_sample = np.insert(train_sample, 0, values=y_list, axis=1)
            # insert the id
            train_sample = np.insert(train_sample.astype(
                "str"), 0, values=id_list, axis=1)
            np.set_printoptions(suppress=True, threshold=np.inf)
            np.savetxt(train_file_info.fp, train_sample.astype(
                "str"), fmt=train_file_info.fmt_setting)
        if predict_file_info is not None:
            predict_sample = feature_sample.astype("str")
            # insert the id
            predict_sample = np.insert(predict_sample.astype(
                "str"), 0, values=id_list, axis=1)
            np.set_printoptions(suppress=True, threshold=np.inf)
            np.savetxt(predict_file_info.fp, predict_sample.astype(
                "str"), fmt=predict_file_info.fmt_setting)
        # check the file size
        if args.id_file is None:
            file_size = os.stat(args.sample_file).st_size
            if file_size >= sample_capacity_bytes:
                print(f"#### final id: {id}")
                break
    if train_file_info is not None:
        train_file_info.fp.close()
    if predict_file_info is not None:
        predict_file_info.fp.close()


if __name__ == "__main__":
    args = parse_args()
    fake_data(args)
