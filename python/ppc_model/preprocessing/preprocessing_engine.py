from ppc_model.preprocessing.local_processing.local_processing_party import LocalProcessingParty

from ppc_model.common.global_context import components
from ppc_model.common.protocol import ModelTask
from ppc_model.interface.task_engine import TaskEngine
from ppc_model.preprocessing.processing_context import ProcessingContext


class PreprocessingEngine(TaskEngine):
    task_type = ModelTask.PREPROCESSING

    @staticmethod
    def run(task_id, args):
        context = ProcessingContext(
            task_id=task_id,
            args=args,
            components=components,
        )
        lpp = LocalProcessingParty(context)
        lpp.processing()
