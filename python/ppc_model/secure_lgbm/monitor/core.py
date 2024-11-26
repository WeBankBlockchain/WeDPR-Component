import collections
from typing import Dict, Any, Union, Tuple, List, Callable
from ppc_model.common.base_context import BaseContext
import numpy as np
from sklearn import metrics

_Score = Union[float, Tuple[float, float]]
_ScoreList = Union[List[float], List[Tuple[float, float]]]

_BoosterParams = Dict[str, Any]
_Metric = Callable[[np.ndarray, np.ndarray], Dict[str, _Score]]

"""
A dictionary containing the evaluation history:
{"metric_name": [0.5, ...]}
"""
_EvalsLog = Dict[str, _ScoreList]


class Booster:
    """A Booster of XGBoost.

    Booster is the model of xgboost, that contains low level routines for
    training, prediction and evaluation.
    """

    def __init__(
            self,
            y_true: np.ndarray,
            test_y_true: np.ndarray,
            ctx: BaseContext = None,
            workspace: str = None,
            job_id: str = None,
            storage_client: str = None
    ) -> None:
        self.params: _BoosterParams = {}
        self.ctx = ctx
        self.y_true = y_true
        self.test_y_true = test_y_true
        self.y_pred = None
        self.test_y_pred = None
        self.eval_on_test = True
        self.epoch = 0
        self.workspace = workspace
        self.job_id = job_id

        self.storage_client = storage_client
        self.history: _EvalsLog = collections.OrderedDict()

    def get_y_true(self) -> np.ndarray:
        return self.y_true

    def get_test_y_true(self) -> np.ndarray:
        return self.test_y_true

    def get_y_pred(self) -> np.ndarray:
        return self.y_pred

    def get_test_y_pred(self) -> np.ndarray:
        return self.test_y_pred

    def get_epoch(self) -> int:
        return self.epoch

    def get_workspace(self) -> str:
        return self.workspace

    def get_job_id(self) -> str:
        return self.job_id

    def get_storage_client(self):
        return self.storage_client

    def set_param(
            self,
            key: str,
            value: Any,
    ) -> None:
        self.params[key] = value

    def get_param(
            self,
            key: str
    ) -> Any:
        return self.params[key]

    def get_history(self) -> _EvalsLog:
        return self.history

    def after_iteration(
            self,
            pred: np.ndarray,
            eval_on_test: bool = True
    ) -> None:
        if eval_on_test:
            self.test_y_pred = pred
        else:
            self.y_pred = pred
        self.eval_on_test = eval_on_test
        self.epoch += 1

    def _update_history(
            self,
            scores: Dict[str, _Score]
    ) -> None:
        for key, value in scores.items():
            if key in self.history:
                self.history[key].append(value)
            else:
                self.history[key] = [value]

    def eval(
            self,
            feval: _Metric
    ) -> Dict[str, _Score]:
        if self.eval_on_test:
            scores = feval(self.test_y_true, self.test_y_pred)
        else:
            scores = feval(self.y_true, self.y_pred)
        self._update_history(scores)
        return scores


_Model = Booster


def fevaluation(
        y_true: np.ndarray,
        y_pred: np.ndarray,
        decimal_num: int = 4
) -> Dict[str, _Score]:
    auc = metrics.roc_auc_score(y_true, y_pred)

    y_pred_label = [0 if p <= 0.5 else 1 for p in y_pred]
    acc = metrics.accuracy_score(y_true, y_pred_label)
    recall = metrics.recall_score(y_true, y_pred_label)
    precision = metrics.precision_score(y_true, y_pred_label)

    scores_dict = {
        'auc': auc,
        'acc': acc,
        'recall': recall,
        'precision': precision
    }
    for metric_name in scores_dict:
        scores_dict[metric_name] = round(scores_dict[metric_name], decimal_num)
    return scores_dict
