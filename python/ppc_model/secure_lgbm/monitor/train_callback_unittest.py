import time
import unittest

import numpy as np

from ppc_model_service import config
from ppc_model.secure_lgbm.monitor.callback import CallbackContainer
from ppc_model.secure_lgbm.monitor.core import Booster, fevaluation
from ppc_model.secure_lgbm.monitor.early_stopping import EarlyStopping
from ppc_model.secure_lgbm.monitor.evaluation_monitor import EvaluationMonitor

log = config.get_logger()


class TestBooster(unittest.TestCase):
    def setUp(self):
        np.random.seed(int(time.time()))
        self.y_true = np.random.randint(0, 2, 10000)
        self.test_y_true = np.random.randint(0, 2, 10000)
        self.y_pred = np.random.rand(10000)
        self.booster = Booster(self.y_true, self.test_y_true)

    def test_set_get_param(self):
        self.booster.set_param('learning_rate', 0.1)
        self.assertEqual(self.booster.get_param('learning_rate'), 0.1)

    def test_after_iteration(self):
        self.booster.after_iteration(self.y_pred, False)
        np.testing.assert_array_equal(self.booster.get_y_pred(), self.y_pred)
        self.assertEqual(self.booster.get_epoch(), 1)

    def test_eval(self):
        self.booster.after_iteration(self.y_pred)
        results = self.booster.eval(fevaluation)
        self.assertIn('auc', results)
        self.assertIn('acc', results)
        self.assertIn('recall', results)
        self.assertIn('precision', results)
        self.assertIsInstance(results['auc'], float)
        self.assertIsInstance(results['acc'], float)
        self.assertIsInstance(results['recall'], float)
        self.assertIsInstance(results['precision'], float)


class TestEarlyStopping(unittest.TestCase):
    def setUp(self):
        np.random.seed(int(time.time()))
        self.y_true = np.random.randint(0, 2, 10000)
        self.test_y_true = np.random.randint(0, 2, 10000)
        self.y_pred = np.random.rand(10000)
        self.model = Booster(self.y_true, self.test_y_true)
        self.early_stopping = EarlyStopping(
            rounds=4, metric_name='auc', maximize=True)

    def test_early_stopping(self):
        stop = False
        while not stop:
            np.random.seed(int(time.time()) + self.model.epoch)
            y_pred = np.random.rand(10000)
            self.model.after_iteration(y_pred)
            self.model.eval(fevaluation)
            stop = self.early_stopping.after_iteration(
                self.model, self.model.epoch)
            print(self.model.epoch, stop)


class TestEvaluationMonitor(unittest.TestCase):
    def setUp(self):
        np.random.seed(int(time.time()))
        self.y_true = np.random.randint(0, 2, 10000)
        self.test_y_true = np.random.randint(0, 2, 10000)
        self.y_pred = np.random.rand(10000)
        self.model = Booster(self.y_true, self.test_y_true, '/tmp/')
        self.monitor = EvaluationMonitor(log, period=2)

    def test_after_training(self):
        np.random.seed(int(time.time()))
        for i in range(10):
            np.random.seed(int(time.time()) + self.model.epoch)
            y_pred = np.random.rand(10000)
            self.model.after_iteration(y_pred)
            self.model.eval(fevaluation)
        self.monitor.after_training(self.model)


class TestCallbackContainer(unittest.TestCase):
    def setUp(self):
        np.random.seed(int(time.time()))
        self.y_true = np.random.randint(0, 2, 10000)
        self.test_y_true = np.random.randint(0, 2, 10000)
        self.y_pred = np.random.rand(10000)
        self.model = Booster(self.y_true, self.test_y_true, 'tmp')
        self.early_stopping = EarlyStopping(
            rounds=4, metric_name='auc', maximize=True)
        self.monitor = EvaluationMonitor(log, period=2)
        self.container = CallbackContainer(
            [self.early_stopping, self.monitor], fevaluation)

    def test_callback_container(self):
        stop = False
        while not stop:
            np.random.seed(int(time.time()) + self.model.epoch)
            y_pred = np.random.rand(10000)
            stop = self.container.after_iteration(self.model, y_pred, True)
            print(self.model.epoch, stop)


if __name__ == '__main__':
    unittest.main()
