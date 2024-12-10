# WeDPR-Component

![](./static/images/wedpr_logo.png)


[![CodeFactor](https://www.codefactor.io/repository/github/webankblockchain/wedpr-component/badge?s=a4c3fb6ffd39e7618378fe13b6bd06c5846cc103)](https://www.codefactor.io/repository/github/webankblockchain/wedpr-component)
[![contributors](https://img.shields.io/github/contributors/WeBankBlockchain/WeDPR)](https://github.com/WeBankBlockchain/WeDPR-Component/graphs/contributors)
[![GitHub activity](https://img.shields.io/github/commit-activity/m/WeBankBlockchain/WeDPR-Component)](https://github.com/WeBankBlockchain/WeDPR-Component/pulse)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg?style=flat-square)](http://makeapullrequest.com)

微众银行多方大数据隐私计算平台[WeDPR-Component](https://github.com/WeBankBlockchain/WeDPR-Component)核心组件库，包括：


**丰富的隐私计算任务支持**

- **隐私求交集**: 包括两方隐私求交集任务和多方隐私求交集任务，并从性能、网络带宽、使用场景等多方面考虑，实现了多种隐私求交集算法，包括CM2020(性能高), RA2018(非平衡PSI算法，适用于CS模式), ECDH-PSI;
- **匿踪查询**: 基于OT算法构建匿踪查询，可将数据集发布为匿踪查询服务开放给相关用户使用;
- **联合建模**: 基于SecureLGB和SecureLR算法支持多方数据联合建模，并可将建模结果发布为模型用于预测，满足了80%多方数据联合建模需求;
- **联合分析**: 基于安全多方计算算法，提供了类SQL/Python的隐私数据联合分析语法，可在不引入额外学习成本的前提下，满足数据开发人员基于多方数据进行联合分析的需求;


**统一网关**

- 支持基于最短路径的消息路由转发
- 支持按节点ID、服务名、机构名进行路由寻址
- 支持服务注册和服务发现

**统一网关SDK**

- 提供Java/Python网关SDK，支持接入网关与其他节点、服务或者机构进行通信
- 可向网关注册服务
- 可从网关拉取服务信息


## 技术文档

- [文档](https://wedpr-document.readthedocs.io/zh-cn/latest/)
- [代码](https://github.com/WeBankBlockchain/WeDPR-Component)


## License

WeDPR-Component的开源协议为Apache License 2.0, 详情参见[LICENSE](LICENSE)。