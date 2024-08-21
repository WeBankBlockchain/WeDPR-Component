import numpy as np


class FeatureSelection:

    @staticmethod
    def feature_selecting(feature_name: list, train_feats: list, fr: float):
        if train_feats is not None and len(train_feats) > 0:
            feature_select = FeatureSelection._get_train_feature(
                feature_name, train_feats)
        elif fr > 0 and fr < 1:
            feature_select = FeatureSelection._get_feature_rate(
                feature_name, fr)
        else:
            feature_select = list(range(len(feature_name)))
        return feature_select

    @staticmethod
    def _get_train_feature(feature_name: list, train_feats: list):
        feature_select = []
        for i, feature in enumerate(feature_name):
            if feature in train_feats:
                feature_select.append(i)
        return feature_select

    @staticmethod
    def _get_feature_rate(feature_name: list, fr: float):
        feature_select = sorted(np.random.choice(
            range(len(feature_name)), size=int(len(feature_name) * fr), replace=False))
        return feature_select
