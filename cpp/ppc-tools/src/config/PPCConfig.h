/*
 *  Copyright (C) 2022 WeDPR.
 *  SPDX-License-Identifier: Apache-2.0
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * @file PPCConfig.h
 * @author: yujiechen
 * @date 2022-11-4
 */
#pragma once
#include "../cuckoo/Common.h"
#include "Common.h"
#include "NetworkConfig.h"
#include "ParamChecker.h"
#include "ppc-framework/storage/CacheStorage.h"
#include <bcos-utilities/Common.h>
#include <memory.h>
#include <ppc-framework/protocol/Protocol.h>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>

namespace ppc::tools
{
// lines
constexpr static uint64_t DefaultDataBatchSize = 1000000;
constexpr static uint16_t DefaultParallelism = 3;
constexpr static std::string_view NODE_PEM_NAME = "node.pem";
struct RA2018Config
{
    // MBytes
    constexpr static uint64_t DefaultCuckooFilterCacheSize = 256;
    // MBytes
    constexpr static uint64_t DefaultCacheSize = 1024;
    // the default cuckoo-filter-capacity: 1MBytes
    constexpr static int DefaultCuckooFilterCapacity = 1;
    constexpr static uint8_t DefaultCuckooFilterTagSize = 32;
    constexpr static uint8_t DefaultBucketSize = 4;
    constexpr static uint16_t DefaultMaxKickoutCount = 20;
    constexpr static uint64_t DefaultTrashBucketSize = 10000;

    std::string dbName;  // the db used to store cuckoo-filter
    ppc::tools::CuckoofilterOption::Ptr cuckooFilterOption;
    uint64_t cuckooFilterCacheSize = DefaultCuckooFilterCacheSize * 1024 * 1024;
    uint64_t cacheSize = DefaultCacheSize * 1024 * 1024;
    // default load 100w data per-time
    int64_t dataBatchSize = DefaultDataBatchSize;
    // use hdfs to store cuckoo-filter or not
    bool useHDFS = false;
};

struct StorageConfig
{
    ppc::protocol::SQLConnectionOption::Ptr sqlConnectionOpt;
    ppc::protocol::FileStorageConnectionOption::Ptr fileStorageConnectionOpt;
    ppc::protocol::RemoteStorageConnectionOption::Ptr remoteConnectionOpt;
};

struct CEMConfig
{
    std::string datasetFilePath;
    std::string datasetHDFSPath;
    std::string ciphertextSuffix;
    uint64_t readPerBatchLines;
};

struct MPCConfig
{
    std::string datasetHDFSPath;
    std::string jobPath;
    std::string mpcRootPath;
    std::string mpcRootPathNoGateway;
    uint64_t readPerBatchLines;
};

// struct AYSConfig
// {
//     std::string datasetPath;
// };

struct GatewayConfig
{
    // the max allowed message size, default is 100MBytes
    // the boostssl limit is 32MBytes
    constexpr static uint64_t DefaultMaxAllowedMsgSize = 100;
    constexpr static uint64_t MinMsgSize = 10 * 1024 * 1024;
    constexpr static uint64_t MaxMsgSize = 1024 * 1024 * 1024;

    bool disableCache;
    NetworkConfig networkConfig;
    ppc::storage::CacheStorageConfig cacheStorageConfig;
    // agencyID => endPointList
    std::map<std::string, std::vector<std::string>> agencies;
    uint64_t maxAllowedMsgSize = DefaultMaxAllowedMsgSize;
    int reconnectTime = 10000;
};

// the ecdh-psi config
struct EcdhPSIParam
{
    int64_t dataBatchSize = DefaultDataBatchSize;
};

struct EcdhMultiPSIParam
{
    int64_t dataBatchSize = DefaultDataBatchSize;
};

struct EcdhConnPSIParam
{
    int64_t dataBatchSize = DefaultDataBatchSize;
    int64_t rank;
    int64_t algos;
    int64_t protocol_families;
    int64_t curve;
    int64_t hashtype;
    int64_t hash2curve;
};

struct CM2020PSIParam
{
    uint16_t parallelism = DefaultParallelism;
};

struct OtPIRParam
{
    uint16_t parallelism = DefaultParallelism;
};

class PPCConfig
{
public:
    using Ptr = std::shared_ptr<PPCConfig>;
    using ConstPtr = std::shared_ptr<PPCConfig const>;
    PPCConfig() = default;
    virtual ~PPCConfig() = default;
    void loadConfig(std::string const& _configPath)
    {
        PPCConfig_LOG(INFO) << LOG_DESC("loadConfig") << LOG_KV("path", _configPath);
        boost::property_tree::ptree iniConfig;
        boost::property_tree::read_ini(_configPath, iniConfig);
        // Note: must load common-config firstly since some ra-configs depends on the common-config
        loadCommonConfig(iniConfig);
        loadRA2018Config(iniConfig);
        loadStorageConfig(iniConfig);
        loadEcdhPSIConfig(iniConfig);
        loadCM2020PSIConfig(iniConfig);
        loadEcdhMultiPSIConfig(iniConfig);
        loadEcdhConnPSIConfig(iniConfig);
    }

    void loadRpcConfig(const char* _certPath, std::string const& _configPath)
    {
        PPCConfig_LOG(INFO) << LOG_DESC("loadRpcConfig") << LOG_KV("path", _configPath);
        boost::property_tree::ptree iniConfig;
        boost::property_tree::read_ini(_configPath, iniConfig);
        loadRpcConfig(_certPath, iniConfig);
    }

    void loadGatewayConfig(
        ppc::protocol::NodeArch _arch, const char* _certPath, std::string const& _configPath)
    {
        PPCConfig_LOG(INFO) << LOG_DESC("loadGatewayConfig") << LOG_KV("path", _configPath);
        boost::property_tree::ptree iniConfig;
        boost::property_tree::read_ini(_configPath, iniConfig);
        loadGatewayConfig(_arch, _certPath, iniConfig);
    }

    virtual void loadTarsConfig(std::string const& _configPath)
    {
        boost::property_tree::ptree iniConfig;
        boost::property_tree::read_ini(_configPath, iniConfig);
        loadTarsConfig(iniConfig);
    }

    virtual void loadRpcConfig(const char* _certPath, boost::property_tree::ptree const& _pt)
    {
        // rpc default disable-ssl
        loadNetworkConfig(
            m_rpcConfig, _certPath, _pt, "rpc", NetworkConfig::DefaultRpcListenPort, true);
    }

    virtual void loadSelfTarsEndpoint(boost::property_tree::ptree const& _pt)
    {
        m_endpoint = _pt.get<std::string>("agency.endpoint", "");
    }

    virtual void loadGatewayConfig(ppc::protocol::NodeArch _arch, const char* _certPath,
        boost::property_tree::ptree const& _pt);

    virtual void loadHDFSConfig(boost::property_tree::ptree const& _pt);

    virtual void loadSQLConfig(boost::property_tree::ptree const& _pt);

    virtual void loadCEMConfig(boost::property_tree::ptree const& _pt);

    virtual void loadMPCConfig(boost::property_tree::ptree const& _pt);

    virtual void loadTarsConfig(boost::property_tree::ptree const& _pt);

    NetworkConfig const& rpcConfig() const { return m_rpcConfig; }
    // the gateway-config
    GatewayConfig const& gatewayConfig() const { return m_gatewayConfig; }

    RA2018Config const& ra2018PSIConfig() const { return m_ra2018PSIConfig; }
    RA2018Config& mutableRA2018PSIConfig() { return m_ra2018PSIConfig; }

    StorageConfig const& storageConfig() const { return m_storageConfig; }
    CEMConfig const& cemConfig() const { return m_cemConfig; }
    MPCConfig const& mpcConfig() const { return m_mpcConfig; }
    std::string const& agencyID() const { return m_agencyID; }
    void setAgencyID(std::string const& _agencyID) { m_agencyID = _agencyID; }
    bool smCrypto() const { return m_smCrypto; }
    std::string const& endpoint() const { return m_endpoint; }

    std::string const& dataLocation() const { return m_dataLocation; }
    uint32_t const& taskTimeoutMinutes() const { return m_taskTimeoutMinutes; }
    uint32_t const& threadPoolSize() const { return m_threadPoolSize; }

    std::string const& gatewayServiceName() const { return m_gatewayServiceName; }
    std::vector<std::string> getServiceEndPointsByName(std::string const& _service)
    {
        // Note: not ensure thread-safe, since only use when init
        std::vector<std::string> endPoints;
        auto it = m_serviceToEndPoints.find(_service);
        if (it != m_serviceToEndPoints.end())
        {
            return it->second;
        }
        return endPoints;
    }

    EcdhPSIParam const& ecdhPSIConfig() const { return m_ecdhPSIConfig; }
    EcdhPSIParam& mutableEcdhPSIConfig() { return m_ecdhPSIConfig; }

    EcdhConnPSIParam const& ecdhConnPSIConfig() const { return m_ecdhConnPSIConfig; }
    EcdhConnPSIParam& mutableEcdhConnPSIConfig() { return m_ecdhConnPSIConfig; }

    EcdhMultiPSIParam const& ecdhMultiPSIConfig() const { return m_ecdhMultiPSIConfig; }
    EcdhMultiPSIParam& mutableEcdhMultiPSIConfig() { return m_ecdhMultiPSIConfig; }

    CM2020PSIParam const& cm2020PSIConfig() const { return m_cm2020Config; }
    CM2020PSIParam& mutableCM2020PSIConfig() { return m_cm2020Config; }

    OtPIRParam const& otPIRParam() const { return m_otPIRConfig; }
    OtPIRParam& mutableOtPIRParam() { return m_otPIRConfig; }

    bcos::bytes const& privateKey() const { return m_privateKey; }
    void setPrivateKey(bcos::bytes const& _privateKey) { m_privateKey = _privateKey; }

    std::string const& privateKeyPath() const { return m_privateKeyPath; }
    // for pro-mode
    void setPrivateKeyPath(std::string const& _privateKeyPath)
    {
        m_privateKeyPath = _privateKeyPath;
    }

    bool disableRA2018() const { return m_disableRA2018; }

    int holdingMessageMinutes() const { return m_holdingMessageMinutes; }

private:
    virtual void loadRA2018Config(boost::property_tree::ptree const& _pt);
    virtual void loadEcdhPSIConfig(boost::property_tree::ptree const& _pt);
    virtual void loadCM2020PSIConfig(boost::property_tree::ptree const& _pt);
    virtual void loadEcdhMultiPSIConfig(boost::property_tree::ptree const& _pt);
    virtual void loadEcdhConnPSIConfig(boost::property_tree::ptree const& _pt);
    virtual void loadCommonConfig(boost::property_tree::ptree const& _pt);
    virtual void loadStorageConfig(boost::property_tree::ptree const& _pt);
    // Note: the gateway/rpc can share the loadNetworkConfig
    void loadNetworkConfig(NetworkConfig& _config, const char* _certPath,
        boost::property_tree::ptree const& _pt, std::string const& _sectionName,
        int _defaultListenPort, bool _defaultDisableSSl);
    void checkPort(std::string const& _sectionName, int _port);
    void checkFileExists(std::string const& _filePath, bool _dir);

    inline void checkNonEmptyField(std::string const& _section, std::string const& _value)
    {
        if (_value.empty())
        {
            BOOST_THROW_EXCEPTION(InvalidConfig() << bcos::errinfo_comment("Must set " + _section));
        }
    }

    void initRedisConfigForGateway(
        ppc::storage::CacheStorageConfig& _redisConfig, const boost::property_tree::ptree& _pt);

    std::map<std::string, std::vector<std::string>> parseAgencyConfig(
        const boost::property_tree::ptree& _pt, std::string const& _sectionName,
        std::string const& _subSectionName);

    // load the tars-config for the given service, e.g:
    /*
    [tars_gateway]
    name = agencyAGateway
    proxy.0 = "192.168.0.1:3000"
    proxy.0 = "192.168.0.2:3002"
    proxy.0 = "192.168.0.3:3003"
     */
    virtual void loadServiceTarsConfig(boost::property_tree::ptree const& _pt,
        std::string const& _serviceName, std::string const& _sectionName);

    std::string getServiceName(boost::property_tree::ptree const& _pt,
        std::string const& _configSection, std::string const& _objName);
    void checkService(std::string const& _serviceType, std::string const& _serviceName);

    int64_t getDataBatchSize(std::string const& _section, int64_t _dataBatchSize);

    int loadHoldingMessageMinutes(
        const boost::property_tree::ptree& _pt, std::string const& _section);

private:
    // the rpc-config
    NetworkConfig m_rpcConfig;
    // the gateway-config
    GatewayConfig m_gatewayConfig;
    // the gateway holding message time, in minutes, default 30min
    int m_holdingMessageMinutes = 30;

    // the ra2018-psi config
    RA2018Config m_ra2018PSIConfig;
    // the storage config
    StorageConfig m_storageConfig;
    // the cem config
    CEMConfig m_cemConfig;
    // the mpc config
    MPCConfig m_mpcConfig;

    // the ecdh config
    EcdhPSIParam m_ecdhPSIConfig;

    EcdhMultiPSIParam m_ecdhMultiPSIConfig;

    EcdhConnPSIParam m_ecdhConnPSIConfig;

    CM2020PSIParam m_cm2020Config;

    OtPIRParam m_otPIRConfig;

    // the agencyID/partyID
    std::string m_agencyID;
    bool m_smCrypto = false;
    bcos::bytes m_privateKey;
    std::string m_privateKeyPath;

    // the tars config
    std::map<std::string, std::vector<std::string>> m_serviceToEndPoints;
    std::string m_gatewayServiceName;

    bool m_disableRA2018 = false;

    std::string m_endpoint;
    std::string m_dataLocation;
    uint32_t m_taskTimeoutMinutes;
    uint32_t m_threadPoolSize;
};
}  // namespace ppc::tools