from ppc_model.common.base_context import BaseContext
from ppc_model.common.global_context import components
from ppc_model.common.protocol import ModelTask, TaskRole
from ppc_model.datasets.dataset import SecureDataset
from ppc_model.feature_engineering.feature_engineering_context import FeatureEngineeringContext
from ppc_model.feature_engineering.vertical.active_party import VerticalFeatureEngineeringActiveParty
from ppc_model.feature_engineering.vertical.passive_party import VerticalFeatureEngineeringPassiveParty
from ppc_model.interface.task_engine import TaskEngine


class FeatureEngineeringEngine(TaskEngine):
    task_type = ModelTask.FEATURE_ENGINEERING

    @staticmethod
    def run(task_id, args):
        input_path = BaseContext.feature_engineering_input_path(
            args['job_id'], components.config_data['JOB_TEMP_DIR'])

        if args['is_label_holder']:
            field_list, label, feature = SecureDataset.read_dataset(
                input_path, True)
            context = FeatureEngineeringContext(
                task_id=task_id,
                args=args,
                components=components,
                role=TaskRole.ACTIVE_PARTY,
                feature=feature,
                feature_name_list=field_list[1:],
                label=label
            )
            vfe = VerticalFeatureEngineeringActiveParty(context)
        else:
            field_list, _, feature = SecureDataset.read_dataset(
                input_path, False)
            context = FeatureEngineeringContext(
                task_id = task_id,
                args=args,
                components=components,
                role=TaskRole.PASSIVE_PARTY,
                feature=feature,
                feature_name_list=field_list,
                label=None
            )
            vfe = VerticalFeatureEngineeringPassiveParty(context)
        vfe.fit()
