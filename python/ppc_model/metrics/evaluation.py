import gc
import time
import random
import traceback
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from typing import Dict, Union, Tuple
from sklearn import metrics
from sklearn.metrics import accuracy_score
from sklearn.metrics import precision_recall_curve
from sklearn.metrics import roc_curve, auc

from ppc_model.common.context import Context
from ppc_model.datasets.dataset import SecureDataset
from ppc_model.common.model_result import ResultFileHandling
from ppc_model.secure_lgbm.monitor.feature.feature_evaluation_info import EvaluationType
from ppc_model.secure_lgbm.monitor.feature.feature_evaluation_info import FeatureEvaluationResult


_Score = Union[float, Tuple[float, float]]


class Evaluation:

    def __init__(self,
                 ctx: Context,
                 dataset: SecureDataset,
                 train_praba: np.ndarray = None,
                 test_praba: np.ndarray = None) -> None:

        self.job_id = ctx.job_id
        self.storage_client = ctx.components.storage_client
        self.summary_evaluation_file = ctx.summary_evaluation_file
        self.remote_summary_evaluation_file = ctx.remote_summary_evaluation_file
        self.model_result_file = ctx.test_model_result_file
        self.model_output_file = ctx.test_model_output_file
        self.remote_model_output_file = ctx.remote_test_model_output_file

        self.metric_roc_file = ctx.test_metric_roc_file
        self.metric_ks_file = ctx.test_metric_ks_file
        self.metric_pr_file = ctx.test_metric_pr_file
        self.metric_acc_file = ctx.test_metric_acc_file
        self.metric_ks_table = ctx.test_metric_ks_table
        self.remote_metric_roc_file = ctx.remote_test_metric_roc_file
        self.remote_metric_ks_file = ctx.remote_test_metric_ks_file
        self.remote_metric_pr_file = ctx.remote_test_metric_pr_file
        self.remote_metric_acc_file = ctx.remote_test_metric_acc_file
        self.remote_metric_ks_table = ctx.remote_test_metric_ks_table

        # if test_praba is None or dataset.test_y is None:
        #     raise Exception('test_praba or test_y is None')

        if test_praba is not None:
            test_ks, test_auc = self.evaluation_file(
                ctx, dataset.test_idx, dataset.test_y, test_praba, 'test')
            if train_praba is not None:
                train_ks, train_auc = self.evaluation_file(
                    ctx, dataset.train_idx, dataset.train_y, train_praba, 'train')
                if dataset.train_y is not None:
                    self.summary_evaluation(
                        dataset, test_ks, test_auc, train_ks, train_auc)

    @staticmethod
    def fevaluation(
            y_true: np.ndarray,
            y_pred: np.ndarray,
            decimal_num: int = 4
    ) -> Dict[str, _Score]:
        auc = metrics.roc_auc_score(y_true, y_pred)

        y_pred_label = [0 if p <= 0.5 else 1 for p in y_pred]
        acc = metrics.accuracy_score(y_true, y_pred_label)
        recall = metrics.recall_score(y_true, y_pred_label)
        precision = metrics.precision_score(y_true, y_pred_label)

        scores_dict = {
            'auc': auc,
            'acc': acc,
            'recall': recall,
            'precision': precision
        }
        for metric_name in scores_dict:
            scores_dict[metric_name] = round(
                scores_dict[metric_name], decimal_num)
        return scores_dict

    def summary_evaluation(self, dataset, test_ks, test_auc, train_ks, train_auc):
        train_evaluation = FeatureEvaluationResult(
            type=EvaluationType.TRAIN, ks_value=train_ks, auc_value=train_auc, label_list=dataset.train_y)
        test_evaluation = FeatureEvaluationResult(
            type=EvaluationType.VALIDATION, ks_value=test_ks, auc_value=test_auc, label_list=dataset.test_y)
        FeatureEvaluationResult.store_and_upload_summary(
            [train_evaluation, test_evaluation],
            self.summary_evaluation_file, self.remote_summary_evaluation_file,
            self.storage_client)

    @staticmethod
    def calculate_ks_and_stats(predicted_proba, actual_label, num_buckets=10):
        # 合并预测概率和实际标签为一个 DataFrame
        df = pd.DataFrame({'predicted_proba': predicted_proba.reshape(-1),
                          'actual_label': actual_label.reshape(-1)})
        # 根据预测概率降序排列
        df_sorted = df.sort_values(by='predicted_proba', ascending=False)
        # 将数据划分为 num_buckets 个分组
        try:
            df_sorted['bucket'] = pd.qcut(
                df_sorted['predicted_proba'], num_buckets, retbins=True, labels=False)[0]
        except Exception:
            df_sorted['bucket'] = pd.cut(
                df_sorted['predicted_proba'], num_buckets, retbins=True, labels=False)[0]
        # 统计每个分组的信息
        stats = df_sorted.groupby('bucket').agg({
            'actual_label': ['count', 'sum'],
            'predicted_proba': ['min', 'max']
        })
        # 计算其他指标
        stats.columns = ['count', 'positive_count',
                         'predict_proba_min', 'predict_proba_max']
        stats['positive_ratio'] = stats['positive_count'] / stats['count']
        stats['negative_ratio'] = 1 - stats['positive_ratio']
        stats['count_ratio'] = stats['count'] / stats['count'].sum()
        # stats['累计坏客户占比'] = stats['坏客户数'].cumsum() / stats['坏客户数'].sum()
        # 计算累计坏客户占比，从第 9 组开始计算
        stats['cum_positive_ratio'] = stats['positive_count'].iloc[::-
                                                                   1].cumsum()[::-1] / stats['positive_count'].sum()
        stats = stats[['count_ratio', 'count', 'positive_count',
                       'positive_ratio', 'negative_ratio', 'cum_positive_ratio']].reset_index()
        stats.columns = ['分组', '样本占比', '样本数',
                         '正样本数', '正样本比例', '负样本比例', '累积正样本占比']
        return stats

    def evaluation_file(self, ctx, data_index: np.ndarray,
                        y_true: np.ndarray, y_praba: np.ndarray, label: str = 'test'):
        if label == 'train':
            self.model_result_file = ctx.train_model_result_file
            self.model_output_file = ctx.train_model_output_file
            self.remote_model_output_file = ctx.remote_train_model_output_file

            self.metric_roc_file = ctx.train_metric_roc_file
            self.metric_ks_file = ctx.train_metric_ks_file
            self.metric_pr_file = ctx.train_metric_pr_file
            self.metric_acc_file = ctx.train_metric_acc_file
            self.metric_ks_table = ctx.train_metric_ks_table
            self.remote_metric_roc_file = ctx.remote_train_metric_roc_file
            self.remote_metric_ks_file = ctx.remote_train_metric_ks_file
            self.remote_metric_pr_file = ctx.remote_train_metric_pr_file
            self.remote_metric_acc_file = ctx.remote_train_metric_acc_file
            self.remote_metric_ks_table = ctx.remote_train_metric_ks_table

        if y_true is not None:
            # metrics plot
            max_retry = 3
            retry_num = 0
            while retry_num < max_retry:
                retry_num += 1
                try:
                    with ctx.components.plot_lock:
                        ks_value, auc_value = Evaluation.plot_two_class_graph(
                            self, y_true, y_praba)
                except:
                    ctx.components.logger().info(
                        f'y_true = {len(y_true)}, {y_true[0:2]}')
                    ctx.components.logger().info(
                        f'y_praba = {len(y_praba)}, {y_praba[0:2]}')
                    err = traceback.format_exc().replace('\n', ' ; error: ')
                    # ctx.components.logger().exception(err)
                    ctx.components.logger().info(
                        f'plot metrics in times-{retry_num} failed, traceback: {err}.')
                    time.sleep(random.uniform(0.1, 3))

            ResultFileHandling._upload_file(
                self.storage_client, self.metric_roc_file, self.remote_metric_roc_file)
            ResultFileHandling._upload_file(
                self.storage_client, self.metric_ks_file, self.remote_metric_ks_file)
            ResultFileHandling._upload_file(
                self.storage_client, self.metric_pr_file, self.remote_metric_pr_file)
            ResultFileHandling._upload_file(
                self.storage_client, self.metric_acc_file, self.remote_metric_acc_file)

            # ks table
            ks_table = self.calculate_ks_and_stats(y_praba, y_true)
            ks_table.to_csv(self.metric_ks_table, header=True, index=None)
            ResultFileHandling._upload_file(
                self.storage_client, self.metric_ks_table, self.remote_metric_ks_table)
        else:
            ks_value = auc_value = None

        # predict result
        self._parse_model_result(data_index, y_true, y_praba)
        ResultFileHandling._upload_file(
            self.storage_client, self.model_output_file, self.remote_model_output_file)

        return ks_value, auc_value

    def _parse_model_result(self, data_index, y_true=None, y_praba=None):

        np.savetxt(self.model_result_file, y_praba, delimiter=',', fmt='%f')

        if y_true is None:
            df = pd.DataFrame(np.column_stack(
                (data_index, y_praba)), columns=['id', 'class_pred'])
        else:
            df = pd.DataFrame(np.column_stack((data_index, y_true, y_praba)),
                              columns=['id', 'class_label', 'class_pred'])
            df['class_label'] = df['class_label'].astype(int)

        df['id'] = df['id'].astype(int)
        df['class_pred'] = df['class_pred'].astype(float)
        df = df.sort_values(by='id')
        df.to_csv(self.model_output_file, index=None)

    def plot_two_class_graph(self, y_true, y_scores):

        y_label_probs = y_true
        y_pred_probs = y_scores
        # plt.cla()
        plt.rcParams['figure.figsize'] = (12.0, 8.0)

        # plot ROC
        fpr, tpr, thresholds = roc_curve(
            y_label_probs, y_pred_probs, pos_label=1)
        auc_value = auc(fpr, tpr)
        plt.figure(f'roc-{self.job_id}')
        plt.title('ROC Curve')  # give plot a title
        plt.xlabel('False Positive Rate (1 - Specificity)')
        plt.ylabel('True Positive Rate (Sensitivity)')
        plt.plot([0, 1], [0, 1], 'k--', lw=2)
        plt.plot(fpr, tpr, label='area = {0:0.5f}'
                 ''.format(auc_value))
        plt.legend(loc="lower right")
        plt.savefig(self.metric_roc_file, dpi=1000)
        # plt.show()

        plt.close('all')
        gc.collect()

        # plot KS
        plt.figure(f'ks-{self.job_id}')
        threshold_x = np.sort(thresholds)
        threshold_x[-1] = 1
        ks_value = max(abs(fpr - tpr))
        plt.title('KS Curve')
        plt.xlabel('Threshold')
        plt.plot(threshold_x, tpr, label='True Positive Rate')
        plt.plot(threshold_x, fpr, label='False Positive Rate')
        # 标记最大ks值
        x_index = np.argwhere(abs(fpr - tpr) == ks_value)[0, 0]
        plt.plot((threshold_x[x_index], threshold_x[x_index]), (fpr[x_index], tpr[x_index]),
                 label='ks = {:.3f}'.format(ks_value), color='r', marker='o', markerfacecolor='r', markersize=5)
        plt.legend(loc="lower right")
        plt.savefig(self.metric_ks_file, dpi=1000)
        # plt.show()

        plt.close('all')
        gc.collect()

        # plot Precision Recall
        plt.figure(f'pr-{self.job_id}')
        plt.title('Precision/Recall Curve')
        plt.xlabel('Recall')
        plt.ylabel('Precision')
        plt.xlim(0.0, 1.0)
        plt.ylim(0.0, 1.05)
        precision, recall, thresholds = precision_recall_curve(
            y_label_probs, y_pred_probs)
        plt.plot(recall, precision)
        plt.savefig(self.metric_pr_file, dpi=1000)
        # plt.show()

        plt.close('all')
        gc.collect()

        # plot accuracy
        plt.figure(f'accuracy-{self.job_id}')
        thresholds = np.linspace(0, 1, num=100)  # 在0~1之间生成100个阈值
        accuracies = []
        for threshold in thresholds:
            predicted_labels = (y_pred_probs >= threshold).astype(int)
            accuracy = accuracy_score(y_label_probs, predicted_labels)
            accuracies.append(accuracy)
        plt.title('Accuracy Curve')
        plt.xlabel('Threshold')
        plt.ylabel('Accuracy')
        plt.xlim(0.0, 1.0)
        plt.ylim(0.0, 1.05)
        plt.plot(thresholds, accuracies)
        plt.savefig(self.metric_acc_file, dpi=1000)
        # plt.show()

        plt.close('all')
        gc.collect()
        return (ks_value, auc_value)
