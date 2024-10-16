from wedpr_ml_toolkit.common.base_context import BaseContext


class BaseResult:

    def __init__(self, ctx: BaseContext):

        self.ctx = ctx
