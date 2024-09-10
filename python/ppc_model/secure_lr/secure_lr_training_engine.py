from ppc_common.ppc_utils.exception import PpcException, PpcErrorCode
from ppc_model.common.protocol import TaskRole, ModelTask
from ppc_model.common.global_context import components
from ppc_model.interface.task_engine import TaskEngine
from ppc_model.datasets.dataset import SecureDataset
from ppc_model.metrics.evaluation import Evaluation
from ppc_model.metrics.model_plot import ModelPlot
from ppc_model.common.model_result import ResultFileHandling
from ppc_model.secure_lr.secure_lr_context import SecureLRContext
from ppc_model.secure_lr.vertical import VerticalLRActiveParty, VerticalLRPassiveParty


class SecureLRTrainingEngine(TaskEngine):
    task_type = ModelTask.LR_TRAINING

    @staticmethod
    def run(args):

        task_info = SecureLRContext(args, components)
        secure_dataset = SecureDataset(task_info)

        if task_info.role == TaskRole.ACTIVE_PARTY:
            booster = VerticalLRActiveParty(task_info, secure_dataset)
        elif task_info.role == TaskRole.PASSIVE_PARTY:
            booster = VerticalLRPassiveParty(task_info, secure_dataset)
        else:
            raise PpcException(PpcErrorCode.ROLE_TYPE_ERROR.get_code(),
                               PpcErrorCode.ROLE_TYPE_ERROR.get_message())

        booster.fit()
        booster.save_model()

        # 获取训练集和验证集的预测概率值
        train_praba = booster.get_train_praba()
        test_praba = booster.get_test_praba()

        # 获取训练集和验证集的预测值评估指标
        Evaluation(task_info, secure_dataset, train_praba, test_praba)
        ModelPlot(booster)
        ResultFileHandling(task_info)
