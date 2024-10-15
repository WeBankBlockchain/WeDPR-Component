from abc import ABC, abstractmethod

class WorkerEngine(ABC):
    @abstractmethod
    def run(self, *args):
        pass