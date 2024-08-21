import gc

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns
from sklearn.metrics import accuracy_score
from sklearn.metrics import confusion_matrix
from sklearn.metrics import precision_recall_curve
from sklearn.metrics import roc_curve, auc


def plot_two_class_graph(job_context, y_scores=None, y_true=None):
    y_pred_probs = job_context.y_pred_probs
    y_label_probs = job_context.y_label_probs
    if y_scores:
        y_pred_probs = y_scores
    if y_true:
        y_label_probs = y_true
    plt.rcParams['figure.figsize'] = (12.0, 8.0)

    # plot ROC
    fpr, tpr, thresholds = roc_curve(y_label_probs, y_pred_probs, pos_label=1)
    auc_value = auc(fpr, tpr)
    plt.figure(f'roc-{job_context.job_id}')
    plt.title('ROC Curve')  # give plot a title
    plt.xlabel('False Positive Rate (1 - Specificity)')
    plt.ylabel('True Positive Rate (Sensitivity)')
    plt.plot([0, 1], [0, 1], 'k--', lw=2)
    plt.plot(fpr, tpr, label='area = {0:0.5f}'
                             ''.format(auc_value))
    plt.legend(loc="lower right")
    plt.savefig(job_context.mpc_metric_roc_path, dpi=1000)
    plt.show()

    plt.close('all')
    gc.collect()

    # plot KS
    plt.figure(f'ks-{job_context.job_id}')
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
    plt.savefig(job_context.mpc_metric_ks_path, dpi=1000)
    plt.show()

    plt.close('all')
    gc.collect()

    # plot Precision Recall
    plt.figure(f'pr-{job_context.job_id}')
    plt.title('Precision/Recall Curve')
    plt.xlabel('Recall')
    plt.ylabel('Precision')
    plt.xlim(0.0, 1.0)
    plt.ylim(0.0, 1.05)
    precision, recall, thresholds = precision_recall_curve(
        y_label_probs, y_pred_probs)
    plt.plot(recall, precision)
    plt.savefig(job_context.mpc_metric_pr_path, dpi=1000)
    plt.show()

    plt.close('all')
    gc.collect()

    # plot accuracy
    plt.figure(f'accuracy-{job_context.job_id}')
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
    plt.savefig(job_context.mpc_metric_accuracy_path, dpi=1000)
    plt.show()

    plt.close('all')
    gc.collect()
    return (ks_value, auc_value)


def plot_multi_class_graph(job_context, n_class=None, y_label_value=None, y_pred_value=None):
    if not n_class:
        n_class = job_context.n_class
    if not y_label_value:
        y_label_value = job_context.y_label_value
    if not y_pred_value:
        y_pred_value = job_context.y_pred_value
    y_label_probs = job_context.y_label_probs
    y_pred_probs = job_context.y_pred_probs

    class_names = [x for x in range(n_class)]
    plt.rcParams['figure.figsize'] = (12.0, 8.0)
    plt.figure(f'roc-{job_context.job_id}')
    multi_class_roc(job_context, plt, y_label_probs, y_pred_probs, class_names)
    plt.figure(f'pr-{job_context.job_id}')
    multi_class_precision_recall(
        job_context, plt, y_label_probs, y_pred_probs, class_names)
    plt.figure(f'cm-{job_context.job_id}')
    multi_class_confusion_matrix(
        job_context, plt, y_label_value, y_pred_value, class_names)


def sigmoid(x):
    return 1 / (1 + np.exp(-x))


def softmax(x):
    x -= np.max(x, axis=1, keepdims=True)
    return np.exp(x) / np.sum(np.exp(x), axis=1, keepdims=True)


# Converts [[0.3, 0.6, 0.1], [0.1, 0.2, 0.7], [0.8, 0.1, 0.1]] to [1, 2, 0]
def get_value_from_probs(probs):
    return [np.argmax(prob) for prob in probs]


# Converts [1, 2, 0] to [[0, 1, 0], [0, 0, 1], [1, 0, 0]]
def get_probs_from_value(values, n_classes):
    probs = np.zeros((len(values), n_classes), int)
    for p, v in zip(probs, values):
        p[v] = 1

    return probs


# This need one-hot encoding
def multi_class_roc(job_context, plt, y_label, y_pred, class_names):
    n_classes = len(class_names)

    fpr = dict()
    tpr = dict()
    roc_auc = dict()
    for i in range(n_classes):
        fpr[i], tpr[i], _ = roc_curve(y_label[:, i], y_pred[:, i])
        roc_auc[i] = auc(fpr[i], tpr[i])

    # micro
    fpr["micro"], tpr["micro"], _ = roc_curve(y_label.ravel(), y_pred.ravel())
    roc_auc["micro"] = auc(fpr["micro"], tpr["micro"])

    # macro
    # First aggregate all false positive rates
    all_fpr = np.unique(np.concatenate([fpr[i] for i in range(n_classes)]))
    # Then interpolate all ROC curves at this points
    mean_tpr = np.zeros_like(all_fpr)
    for i in range(n_classes):
        mean_tpr += np.interp(all_fpr, fpr[i], tpr[i])
    # Finally average it and compute AUC
    mean_tpr /= n_classes
    fpr["macro"] = all_fpr
    tpr["macro"] = mean_tpr
    roc_auc["macro"] = auc(fpr["macro"], tpr["macro"])

    # Plot all ROC curves
    lw = 2
    plt.plot(fpr["micro"], tpr["micro"],
             label='Micro-averaging (area = {0:0.5f})'
                   ''.format(roc_auc["micro"]),
             linestyle=':', linewidth=4)

    plt.plot(fpr["macro"], tpr["macro"],
             label='Macro-averaging (area = {0:0.5f})'
                   ''.format(roc_auc["macro"]),
             linestyle=':', linewidth=4)

    for i in range(n_classes):
        plt.plot(fpr[i], tpr[i], lw=lw,
                 label='Class {0} (area = {1:0.5f})'
                       ''.format(class_names[i], roc_auc[i]))

    plt.xlim([0.0, 1.0])
    plt.ylim([0.0, 1.05])
    plt.xlabel('False Positive Rate (1 - Specificity)')
    plt.ylabel('True Positive Rate (Sensitivity)')
    plt.title('Multi-class ROC Curve')
    plt.legend(loc="lower right")
    plt.plot([0, 1], [0, 1], 'k--', lw=lw)
    plt.savefig(job_context.mpc_metric_roc_path, dpi=1000)


# This need one-hot encoding
def multi_class_precision_recall(job_context, plt, y_label, y_pred, class_names):
    n_classes = len(class_names)
    precision = dict()
    recall = dict()
    for i in range(n_classes):
        precision[i], recall[i], _ = precision_recall_curve(
            y_label[:, i], y_pred[:, i])

    # Plot all ROC curves
    lw = 2
    for i in range(n_classes):
        plt.plot(recall[i], precision[i], lw=lw,
                 label='Class {0}'.format(class_names[i]))
    plt.xlim([0.0, 1.0])
    plt.ylim([0.0, 1.05])
    plt.xlabel('Recall')
    plt.ylabel('Precision')
    plt.title('Multi-class Precision/Recall Curve')
    plt.legend(loc="lower right")
    plt.savefig(job_context.mpc_metric_pr_path, dpi=1000)


# This need value encoding
def multi_class_confusion_matrix(job_context, plt, y_label, y_pred, class_names):
    cm = confusion_matrix(y_label, y_pred)
    conf_matrix = pd.DataFrame(cm, index=class_names, columns=class_names)

    sns.heatmap(conf_matrix, annot=True, annot_kws={
                "size": 19}, cmap="Blues", fmt='d')
    plt.ylabel('True label')
    plt.xlabel('Predicted label')
    plt.title('Confusion Matrix')
    plt.savefig(job_context.mpc_metric_confusion_matrix_path, dpi=1000)
