# wedpr专家模式用户手册

## 配置

1. 左侧用户目录中新建配置文件，文件命名为：config.properties
2. 配置信息参考：

```
access_key_id=
access_key_secret=
remote_entrypoints=http://127.0.0.1:8005,http://127.0.0.1:8006

agency_name=WeBank
workspace_path=/user/ppc/webank/
user=test_user
storage_endpoint=http://127.0.0.1:50070
```

3. 通过前端页面登录，例如：http://139.159.202.235:8005/
4. 创建个人项目空间，通过【打开jupyter】按钮进入专家模式

## 基础功能

1. 支持通过launcher启动python，jupyter，终端，文本编辑等功能
2. 支持在用户目录空间创建/修改/删除配置文件，文本文件，bash，python notebook等格式文件
3. 通过launcher启动python，jupyter，终端后可以正常执行对应的代码功能

## hdfs数据功能

1. 支持注册dataset，支持两种方式: pd.Dataframe, hdfs_path
2. 支持更新dataset

* 详细使用说明参考示例文件：【test_dataset.ipynb】

## wedpr任务功能

1. 支持配置任务参数
2. 支持提交psi，建模训练，预测等任务
3. 支持获取任务结果
4. 支持对任务结果进行明文处理

* 详细使用说明参考示例文件：【test_psi.ipynb】和【test_xgboost.ipynb】
