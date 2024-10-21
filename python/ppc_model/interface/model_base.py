from abc import ABC

from pandas import DataFrame
from ppc_model.network.wedpr_model_transport import ModelRouter


class ModelBase(ABC):
    mode: str

    def __init__(self, ctx):
        self.ctx = ctx
        self.ctx.model_router = ModelRouter(logger=self.ctx.components.logger(),
                                            transport=self.ctx.components.transport,
                                            participant_id_list=self.ctx.participant_id_list)

    def fit(
        self,
        *args,
        **kwargs
    ) -> None:
        pass

    def transform(self, transform_data: DataFrame) -> DataFrame:
        pass

    def predict(self, predict_data: DataFrame) -> DataFrame:
        pass

    def save_model(self, file_path):
        pass

    def load_model(self, file_path):
        pass


class VerticalModel(ModelBase):
    mode = "VERTICAL"

    def __init__(self, ctx):
        super().__init__(ctx)
        self._all_feature_name = []
