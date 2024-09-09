import numpy as np


class Loss:
    pass


class BinaryLoss(Loss):

    def __init__(self, objective: str = None) -> None:
        super().__init__()
        self.objective = objective

    @staticmethod
    def sigmoid(x: np.ndarray):
        return 1 / (1 + np.exp(-x))

    @staticmethod
    def compute_gradient(y_true: np.ndarray, y_pred: np.ndarray):
        return y_pred - y_true

    @staticmethod
    def compute_hessian(y_pred: np.ndarray):
        return y_pred * (1 - y_pred)

    @staticmethod
    def compute_loss(y_true: np.ndarray, y_pred: np.ndarray):
        '''binary_cross_entropy'''
        # 避免log(0)错误
        epsilon = 1e-15
        y_pred = np.clip(y_pred, epsilon, 1 - epsilon)
        return -np.mean(y_true * np.log(y_pred) + (1 - y_true) * np.log(1 - y_pred))

    @staticmethod
    def dot_product(x, theta):
        if x.ndim == 1:
            x.reshape(1, len(x))
        if theta.ndim == 1:
            theta.reshape(len(theta), 1)
        g = np.matmul(x, theta)
        return g

    @staticmethod
    def inference(g):
        # h = np.divide(np.exp(g), np.exp(g) + 1)
        # 近似
        h = 0.125 * g
        return h
