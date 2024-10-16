# -*- coding: utf-8 -*-
import os
import json
from ppc_model.interface.model_base import VerticalModel
from ppc_model.common.model_result import ResultFileHandling
from abc import abstractmethod


class SecureModelBooster(VerticalModel):
    def __init__(self, ctx) -> None:
        super().__init__(ctx)
        self.logger = self.ctx.components.logger()

    def save_model(self, file_path=None, model_type=None):
        if file_path is not None:
            self.ctx.model_data_file = os.path.join(
                file_path, self.ctx.MODEL_DATA_FILE)

        self.save_model_hook(file_path)
        model = {}
        model['model_type'] = model_type
        model['label_provider'] = self.ctx.participant_id_list[0]
        model['label_column'] = 'y'
        model['participant_agency_list'] = []
        for partner_index in range(0, len(self.ctx.participant_id_list)):
            agency_info = {
                'agency': self.ctx.participant_id_list[partner_index]}
            agency_info['fields'] = self._all_feature_name[partner_index]
            model['participant_agency_list'].append(agency_info)

        model['model_dict'] = self.ctx.model_params.get_all_params()
        self.merge_model_file(model)

    @abstractmethod
    def merge_model_file(self, lr_model):
        pass

    @abstractmethod
    def save_model_hook(self, model_file_path):
        pass
