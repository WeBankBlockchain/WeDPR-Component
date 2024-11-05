from abc import ABC

from pandas import DataFrame
from ppc_model.common.protocol import TaskRole


class ModelBase(ABC):
    mode: str

    def __init__(self, ctx):
        self.ctx = ctx
        self.ctx.model_router = self.ctx.components.model_router
        self.__handshake__()

    def __handshake__(self):
        # handshake with all passive parties
        for i in range(0, len(self.ctx.participant_id_list)):
            participant = self.ctx.participant_id_list[i]
            if participant == self.ctx.my_agency_id:
                continue
            self.ctx.components.logger().info(
                f"Send handshake to party: {participant}")
            self.ctx.model_router.handshake(self.ctx.task_id, participant)

        # wait for handshake response from the passive parties
        self.ctx.components.logger().info(
            f"Wait for handshake from all parities")
        self.ctx.model_router.wait_for_handshake(
            self.ctx.task_id, self.ctx.participant_id_list, self.ctx.my_agency_id)

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
