import os
import time
import random
import traceback
from typing import Optional

import matplotlib.pyplot as plt

from ppc_common.ppc_utils.utils import METRICS_OVER_ITERATION_FILE
from ppc_model.secure_lgbm.monitor.callback import TrainingCallback
from ppc_model.secure_lgbm.monitor.core import _Model


def _draw_figure(model: _Model):
    scores = model.get_history()
    path = model.get_workspace()

    iterations = [i + 1 for i in range(len(next(iter(scores.values()))))]

    # plt.cla()
    plt.figure(figsize=(int(10 + len(iterations) / 5), 10))

    for metric, values in scores.items():
        plt.plot(iterations, values, label=metric)
        max_index = values.index(max(values))
        plt.scatter(max_index + 1, values[max_index], color='green')
        plt.text(max_index + 1, values[max_index],
                 f'{values[max_index]:.4f}', fontsize=9, ha='right')

    plt.legend()
    plt.title('Metrics Over Iterations')
    plt.xlabel('Iteration')
    plt.ylabel('Metric Value')
    plt.grid(True)
    if len(iterations) <= 60:
        plt.xticks(iterations, fontsize=10, rotation=45)
    else:
        plt.xticks(range(0, len(iterations), 5), fontsize=10, rotation=45)
    plt.yticks(fontsize=12)

    file_path = os.path.join(path, METRICS_OVER_ITERATION_FILE)
    plt.savefig(file_path, format='svg', dpi=300)
    plt.close('all')


def _upload_figure(model: _Model):
    storage_client = model.get_storage_client()
    if storage_client is not None:
        path = model.get_workspace()
        job_id = model.get_job_id()
        metrics_file_path = os.path.join(path, METRICS_OVER_ITERATION_FILE)
        unique_file_path = os.path.join(job_id, METRICS_OVER_ITERATION_FILE)
        storage_client.upload_file(metrics_file_path, unique_file_path)


def _fmt_metric(
        metric_name: str, score: float
) -> str:
    msg = f"\t{metric_name}:{score:.5f}"
    return msg


class EvaluationMonitor(TrainingCallback):
    """Print the evaluation result after each period iteration.
    Parameters
    ----------
    period :
        How many epoches between printing.
    """

    def __init__(self, logger, period: int = 1) -> None:
        self.logger = logger
        self.period = period
        assert period > 0
        # last error message, useful when early stopping and period are used together.
        self._latest: Optional[str] = None
        super().__init__()

    def after_iteration(
            self, model: _Model, epoch: int
    ) -> bool:
        history = model.get_history()
        if not history:
            return False

        msg: str = f"[{model.get_job_id()}, epoch(iter): {epoch}]"
        for metric_name, scores in history.items():
            if isinstance(scores[-1], tuple):
                score = scores[-1][0]
            else:
                score = scores[-1]
            msg += _fmt_metric(metric_name, score)
        msg += "\n"

        if (epoch % self.period) == 0 or self.period == 1:
            self.logger.info(msg)
            self._latest = None
        else:
            # There is skipped message
            self._latest = msg

        return False

    def after_training(self, model: _Model) -> _Model:
        if self._latest is not None:
            self.logger.info(self._latest)
        max_retry = 3
        retry_num = 0
        while retry_num < max_retry:
            retry_num += 1
            try:
                _draw_figure(model)
            except:
                self.logger.info(f'scores = {model.get_history()}')
                self.logger.info(f'path = {model.get_workspace()}')
                err = traceback.format_exc().replace('\n', ' ; error ')
                # self.logger.exception(err)
                self.logger.info(
                    f'plot moniter in times-{retry_num} failed, traceback: {err}.')
                time.sleep(random.uniform(0.1, 3))
        _upload_figure(model)
        return model
