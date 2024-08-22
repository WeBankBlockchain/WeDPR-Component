from typing import Optional, cast

import numpy

from ppc_model.secure_lgbm.monitor.callback import TrainingCallback
from ppc_model.secure_lgbm.monitor.core import _Score, _ScoreList, _Model, _EvalsLog


class EarlyStopping(TrainingCallback):
    """Callback function for early stopping
    Parameters
    ----------
    rounds :
        Early stopping rounds.
    metric_name :
        Name of metric that is used for early stopping.
    maximize :
        Whether to maximize evaluation metric.  None means auto (discouraged).
    min_delta :
        Minimum absolute change in score to be qualified as an improvement.
    """

    def __init__(
            self,
            rounds: int,
            metric_name: str,
            maximize: Optional[bool] = None,
            save_best: Optional[bool] = True,
            min_delta: float = 0.0,
    ) -> None:
        self.metric_name = metric_name
        assert self.metric_name in ['auc', 'acc', 'recall', 'precision']
        self.rounds = rounds
        self.maximize = maximize
        self.save_best = save_best
        self._min_delta = min_delta
        if self._min_delta < 0:
            raise ValueError("min_delta must be greater or equal to 0.")
        self.stopping_history: _EvalsLog = {}
        self.current_rounds: int = 0
        self.best_scores: dict = {}
        super().__init__()

    def before_training(self, model: _Model) -> _Model:
        return model

    def _update_rounds(
            self, score: _Score, metric_name: str, model: _Model, epoch: int
    ) -> bool:
        def get_s(value: _Score) -> float:
            """get score if it's cross validation history."""
            return value[0] if isinstance(value, tuple) else value

        def maximize(new: _Score, best: _Score) -> bool:
            """New score should be greater than the old one."""
            return numpy.greater(get_s(new) - self._min_delta, get_s(best))

        def minimize(new: _Score, best: _Score) -> bool:
            """New score should be lesser than the old one."""
            return numpy.greater(get_s(best) - self._min_delta, get_s(new))

        if self.maximize is None:
            maximize_metrics = (
                "auc",
                "aucpr",
                "pre",
                "pre@",
                "map",
                "ndcg",
                "auc@",
                "aucpr@",
                "map@",
                "ndcg@",
            )
            if metric_name != "mape" and any(metric_name.startswith(x) for x in maximize_metrics):
                self.maximize = True
            else:
                self.maximize = False

        if self.maximize:
            improve_op = maximize
        else:
            improve_op = minimize

        if not self.stopping_history:  # First round
            self.current_rounds = 0
            self.stopping_history[metric_name] = cast(_ScoreList, [score])
            self.best_scores[metric_name] = cast(_ScoreList, [score])
            if self.save_best:
                model.set_param('best_score', score)
                model.set_param('best_iteration', epoch)
        elif not improve_op(score, self.best_scores[metric_name][-1]):
            # Not improved
            self.stopping_history[metric_name].append(score)  # type: ignore
            self.current_rounds += 1
        else:  # Improved
            self.stopping_history[metric_name].append(score)  # type: ignore
            self.best_scores[metric_name].append(score)
            self.current_rounds = 0  # reset
            if self.save_best:
                model.set_param('best_score', score)
                model.set_param('best_iteration', epoch)

        if self.current_rounds >= self.rounds:
            # Should stop
            return True
        return False

    def after_iteration(
            self, model: _Model, epoch: int
    ) -> bool:
        history = model.get_history()
        if len(history.keys()) < 1:
            raise ValueError(
                "Must have at least 1 validation dataset for early stopping.")

        metric_name = self.metric_name
        # The latest score
        score = history[metric_name][-1]
        return self._update_rounds(score, metric_name, model, epoch)

    def after_training(self, model: _Model) -> _Model:
        return model
