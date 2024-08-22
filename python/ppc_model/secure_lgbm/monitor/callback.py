from abc import ABC
from typing import (
    Callable,
    Optional,
    Sequence
)

import numpy as np

from ppc_model.secure_lgbm.monitor.core import _Model


class TrainingCallback(ABC):
    def __init__(self) -> None:
        pass

    def before_training(self, model: _Model) -> _Model:
        """Run before training starts."""
        return model

    def after_training(self, model: _Model) -> _Model:
        """Run after training is finished."""
        return model

    def before_iteration(self, model: _Model, epoch: int) -> bool:
        """Run before each iteration.  Returns True when training should stop."""
        return False

    def after_iteration(self, model: _Model, epoch: int) -> bool:
        """Run after each iteration.  Returns `True` when training should stop."""
        return False


class CallbackContainer:
    """A special internal callback for invoking a list of other callbacks."""

    def __init__(
            self,
            callbacks: Sequence[TrainingCallback],
            feval: Optional[Callable] = None
    ) -> None:
        self.callbacks = set(callbacks)
        for cb in callbacks:
            if not isinstance(cb, TrainingCallback):
                raise TypeError(
                    "callback must be an instance of `TrainingCallback`.")

        msg = (
            "feval must be callable object for monitoring. For builtin metrics"
            ", passing them in training parameter invokes monitor automatically."
        )
        if feval is not None and not callable(feval):
            raise TypeError(msg)

        self.feval = feval

    def before_training(self, model: _Model) -> _Model:
        for c in self.callbacks:
            model = c.before_training(model=model)
        return model

    def after_training(self, model: _Model) -> _Model:
        for c in self.callbacks:
            model = c.after_training(model=model)
        return model

    def before_iteration(
            self,
            model: _Model
    ) -> bool:
        return any(
            c.before_iteration(model, model.get_epoch()) for c in self.callbacks
        )

    def after_iteration(
            self,
            model: _Model,
            pred: np.ndarray,
            eval_on_test: bool = True
    ) -> bool:
        model.after_iteration(pred, eval_on_test)
        model.eval(self.feval)
        ret = any(c.after_iteration(model, model.get_epoch())
                  for c in self.callbacks)
        return ret
