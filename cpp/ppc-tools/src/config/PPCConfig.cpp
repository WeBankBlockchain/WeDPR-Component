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
 * @file PPCConfig.cpp
 * @author: yujiechen
 * @date 2022-11-4
 */
#include "PPCConfig.h"
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/lexical_cast.hpp>
#include <algorithm>
#include <thread>

using namespace ppc::tools;
using namespace ppc::protocol;
using namespace ppc::storage;
using namespace bcos;

void PPCConfig::loadGatewayConfig(
    NodeArch _arch, const char* _certPath, boost::property_tree::ptree const& _pt)
{
    // load the network config
    PPCConfig_LOG(INFO) << LOG_DESC("loadGatewayConfig: load the network config");
    // gateway default enable-ssl
    loadNetworkConfig(m_gatewayConfig.networkConfig, _certPath, _pt, "gateway",
        NetworkConfig::DefaultRpcListenPort, false);
    PPCConfig_LOG(INFO) << LOG_DESC("loadGatewayConfig: load the network config success");

    m_gatewayConfig.disableCache = _pt.get<bool>("gateway.disable_cache", false);

    // load the redis config
    if (_arch == NodeArch::PRO && !m_gatewayConfig.disableCache)
    {
        PPCConfig_LOG(INFO) << LOG_DESC("loadGatewayConfig: load the redis config");
        initRedisConfigForGateway(m_gatewayConfig.cacheStorageConfig, _pt);
        PPCConfig_LOG(INFO) << LOG_DESC("loadGatewayConfig: load the redis config success");
    }

    // load the agencies config
    PPCConfig_LOG(INFO) << LOG_DESC("loadGatewayConfig: load the agency config");
    auto agencyInfo = parseAgencyConfig(_pt, "agency", "agency");
    m_gatewayConfig.agencies = std::move(agencyInfo);
    PPCConfig_LOG(INFO) << LOG_DESC("loadGatewayConfig: load the agency config sucess")
                        << LOG_KV("agencySize", m_gatewayConfig.agencies.size());

    m_gatewayConfig.reconnectTime = _pt.get<int>("gateway.reconnect_time", 10000);
    // load the maxAllowedMsgSize, in MBytes
    m_gatewayConfig.maxAllowedMsgSize =
        _pt.get<uint64_t>("gateway.max_allow_msg_size", GatewayConfig::DefaultMaxAllowedMsgSize) *
        1024 * 1024;
    if (m_gatewayConfig.maxAllowedMsgSize < GatewayConfig::MinMsgSize ||
        m_gatewayConfig.maxAllowedMsgSize >= GatewayConfig::MaxMsgSize)
    {
        BOOST_THROW_EXCEPTION(
            InvalidConfig() << errinfo_comment(
                "the gateway.max_allow_msg_size must be smaller than " +
                std::to_string(GatewayConfig::MinMsgSize) + "Bytes and no larger than" +
                std::to_string(GatewayConfig::MaxMsgSize) + "Bytes, recommend to 100MBytes"));
    }
    // load the holdingMessageMinutes, in minutes
    m_holdingMessageMinutes = loadHoldingMessageMinutes(_pt, "gateway.holding_msg_minutes");
    PPCConfig_LOG(INFO) << LOG_DESC("loadGatewayConfig")
                        << LOG_KV("maxAllowedMsgSize", m_gatewayConfig.maxAllowedMsgSize)
                        << LOG_KV("reconnectTime", m_gatewayConfig.reconnectTime)
                        << LOG_KV("holdingMessageMinutes", m_holdingMessageMinutes);
}


int PPCConfig::loadHoldingMessageMinutes(
    const boost::property_tree::ptree& _pt, std::string const& _section)
{
    auto holdingMessageMinutes = _pt.get<int>(_section, 30);
    if (holdingMessageMinutes < 0)
    {
        BOOST_THROW_EXCEPTION(
            InvalidConfig() << errinfo_comment("The value of " + _section + " must positive"));
    }
    return holdingMessageMinutes;
}

void PPCConfig::initRedisConfigForGateway(
    CacheStorageConfig& _redisConfig, const boost::property_tree::ptree& _pt)
{
    _redisConfig.type = ppc::protocol::CacheType(_pt.get<uint16_t>("cache.type", 0));
    if (_redisConfig.type > ppc::protocol::CacheType::Redis)
    {
        BOOST_THROW_EXCEPTION(InvalidConfig() << bcos::errinfo_comment("unsupported cache type"));
    }

    _redisConfig.proxy = _pt.get<std::string>("cache.proxy", "");
    _redisConfig.obServer = _pt.get<std::string>("cache.obServer", "");
    _redisConfig.cluster = _pt.get<std::string>("cache.cluster", "");
    _redisConfig.user = _pt.get<std::string>("cache.user", "");
    _redisConfig.host = _pt.get<std::string>("cache.host", "127.0.0.1");
    _redisConfig.port = _pt.get<uint16_t>("cache.port", 6379);
    _redisConfig.password = _pt.get<std::string>("cache.password", "");
    _redisConfig.database = _pt.get<uint16_t>("cache.database", 1);
    _redisConfig.pool = _pt.get<uint16_t>("cache.pool_size", 16);

    // the connection_timeout
    _redisConfig.connectionTimeout = _pt.get<uint16_t>("cache.connection_timeout", 500);
    // the socket_timeout
    _redisConfig.socketTimeout = _pt.get<uint16_t>("cache.socket_timeout", 500);
    PPCConfig_LOG(INFO) << LOG_DESC("initRedisConfig") << LOG_KV("type", (int)_redisConfig.type)
                        << LOG_KV("proxy", _redisConfig.proxy)
                        << LOG_KV("obServer", _redisConfig.obServer)
                        << LOG_KV("cluster", _redisConfig.cluster)
                        << LOG_KV("user", _redisConfig.user) << LOG_KV("host", _redisConfig.host)
                        << LOG_KV("port", _redisConfig.port)
                        << LOG_KV("password", _redisConfig.password)
                        << LOG_KV("database", _redisConfig.database)
                        << LOG_KV("pool", _redisConfig.pool)
                        << LOG_KV("connectionTimeout", _redisConfig.connectionTimeout)
                        << LOG_KV("socketTimeout", _redisConfig.socketTimeout);
}

std::map<std::string, std::vector<std::string>> PPCConfig::parseAgencyConfig(
    const boost::property_tree::ptree& _pt, std::string const& _sectionName,
    std::string const& _subSectionName)
{
    PPCConfig_LOG(INFO) << LOG_DESC("parseAgencyConfig") << LOG_KV("section", _sectionName)
                        << LOG_KV("subSection", _subSectionName);
    std::map<std::string, std::vector<std::string>> agencyInfo;
    // without the config
    if (!_pt.get_child_optional(_sectionName))
    {
        PPCConfig_LOG(INFO) << LOG_DESC("parseAgencyConfig return for empty config")
                            << LOG_KV("section", _sectionName)
                            << LOG_KV("subSection", _subSectionName);
        return agencyInfo;
    }
    // tranverse the child_section to parse the agencyInfo
    for (auto const& it : _pt.get_child(_sectionName))
    {
        if (it.first.find(_subSectionName) != 0)
        {
            continue;
        }
        // find and parse the agencyInfo
        auto key = it.first.data();
        std::vector<std::string> agencyIDInfo;
        boost::split(agencyIDInfo, key, boost::is_any_of("."));
        // invalid agencyID
        if (agencyIDInfo.size() < 2)
        {
            BOOST_THROW_EXCEPTION(
                InvalidConfig() << bcos::errinfo_comment("Invalid agency key " + it.first +
                                                         ", the key must be in format of " +
                                                         _sectionName + ".${agencyID}"));
        }
        std::vector<std::string> endPointInfos;
        auto value = it.second.data();
        boost::split(endPointInfos, value, boost::is_any_of(","));
        for (auto& endpoint : endPointInfos)
        {
            if (!checkEndpoint(endpoint))
            {
                BOOST_THROW_EXCEPTION(
                    InvalidConfig() << bcos::errinfo_comment("Invalid agency endpoint" + endpoint));
            }
        }
        auto& currentEndPoints = agencyInfo[agencyIDInfo.at(1)];
        currentEndPoints.reserve(currentEndPoints.size() + endPointInfos.size());
        std::move(std::begin(endPointInfos), std::end(endPointInfos),
            std::back_inserter(currentEndPoints));
        PPCConfig_LOG(INFO) << LOG_DESC("parseAgencyConfig")
                            << LOG_KV("agencyID", agencyIDInfo.at(1))
                            << LOG_KV("endPointSize", currentEndPoints.size());
    }
    PPCConfig_LOG(INFO) << LOG_DESC("parseAgencyConfig") << LOG_KV("agencySize", agencyInfo.size());
    return agencyInfo;
}

void PPCConfig::loadNetworkConfig(NetworkConfig& _config, const char* _certPath,
    boost::property_tree::ptree const& _pt, std::string const& _sectionName, int _defaultListenPort,
    bool _defaultDisableSSl)
{
    // the rpcListenIp
    _config.listenIp = _pt.get<std::string>(_sectionName + ".listen_ip", "0.0.0.0");
    // the rpcListenPort
    _config.listenPort = _pt.get<int>(_sectionName + ".listen_port", _defaultListenPort);
    checkPort("rpc.listen_port", _config.listenPort);
    // the rpcThreadPoolSize
    _config.threadPoolSize = _pt.get<uint32_t>(_sectionName + ".thread_count",
        static_cast<uint32_t>(std::thread::hardware_concurrency() * 0.75));
    _config.token = _pt.get<std::string>(_sectionName + ".token", "ppcs_psi_apikey");
    // disable rpc-ssl or not
    _config.disableSsl = _pt.get<bool>(_sectionName + ".disable_ssl", _defaultDisableSSl);
    _config.protocol = _pt.get<int>(_sectionName + ".protocol", NetworkConfig::PROTOCOL_WEBSOCKET);
    if (_config.protocol == NetworkConfig::PROTOCOL_HTTP)
    {
        _config.url = _pt.get<std::string>(_sectionName + ".url", "/api/v1/interconn/invoke");
    }
    _config.minNeededMemoryGB = _pt.get<uint32_t>(_sectionName + ".min_needed_memory", 5);
    PPCConfig_LOG(INFO) << LOG_BADGE("loadNetworkConfig") << LOG_KV("section", _sectionName)
                        << LOG_KV("listenIp", _config.listenIp)
                        << LOG_KV("listenPort", _config.listenPort)
                        << LOG_KV("threadCount", _config.threadPoolSize)
                        << LOG_KV("disableSsl", _config.disableSsl)
                        << LOG_KV("protocol", _config.protocol) << LOG_KV("url", _config.url)
                        << LOG_KV("minNeededMemory", _config.minNeededMemoryGB);
    // no need to load the certificate when disable-ssl
    if (_config.disableSsl)
    {
        return;
    }
    // enable sm-rpc or not
    _config.enableSM = _pt.get<bool>(_sectionName + ".sm_ssl", false);

    // the rpc cert-path
    if (_certPath == nullptr)
    {
        _config.certPath = _pt.get<std::string>("cert.cert_path", "conf");
    }
    else
    {
        _config.certPath = _certPath;
    }
    PPCConfig_LOG(INFO) << LOG_BADGE("loadNetworkConfig") << LOG_KV("section", _sectionName)
                        << LOG_KV("certPath", _config.certPath);

    checkFileExists(_config.certPath, true);
    // load the sm-cert
    if (_config.enableSM)
    {
        _config.smCaCertPath = _config.certPath + "/" + std::string(NetworkConfig::SM_CA_CERT_NAME);
        checkFileExists(_config.smCaCertPath, false);

        _config.smSslCertPath =
            _config.certPath + "/" + std::string(NetworkConfig::SM_SSL_CERT_NAME);
        checkFileExists(_config.smSslCertPath, false);

        _config.smEnSslCertPath =
            _config.certPath + "/" + std::string(NetworkConfig::SM_SSL_EN_CERT_NAME);
        checkFileExists(_config.smEnSslCertPath, false);

        _config.smSslKeyPath = _config.certPath + "/" + std::string(NetworkConfig::SM_SSL_KEY_NAME);
        checkFileExists(_config.smSslKeyPath, false);

        _config.smEnSslKeyPath =
            _config.certPath + "/" + std::string(NetworkConfig::SM_SSL_EN_KEY_NAME);
        checkFileExists(_config.smEnSslKeyPath, false);
        PPCConfig_LOG(INFO) << LOG_BADGE("loadNetworkConfig") << LOG_DESC("load sm cert")
                            << LOG_KV("section", _sectionName)
                            << LOG_KV("caCert", _config.smCaCertPath)
                            << LOG_KV("sslCert", _config.smSslCertPath)
                            << LOG_KV("enSslCert", _config.smEnSslCertPath);
        return;
    }
    // load the non-sm-cert
    _config.caCertPath = _config.certPath + "/" + std::string(NetworkConfig::CA_CERT_NAME);
    checkFileExists(_config.caCertPath, false);

    _config.sslCertPath = _config.certPath + "/" + std::string(NetworkConfig::SSL_CERT_NAME);
    checkFileExists(_config.sslCertPath, false);

    _config.sslKeyPath = _config.certPath + "/" + std::string(NetworkConfig::SSL_KEY_NAME);
    checkFileExists(_config.sslKeyPath, false);

    PPCConfig_LOG(INFO) << LOG_BADGE("loadNetworkConfig") << LOG_DESC("load non-sm cert")
                        << LOG_KV("section", _sectionName) << LOG_KV("caCert", _config.caCertPath)
                        << LOG_KV("sslCert", _config.sslCertPath);
}

void PPCConfig::checkPort(std::string const& _sectionName, int _port)
{
    if (_port <= 0 || _port > MAXPORT)
    {
        BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment(
                                  "The " + _sectionName + " must positive and no larger than" +
                                  std::to_string(MAXPORT)));
    }
}

void PPCConfig::checkFileExists(std::string const& _filePath, bool _dir)
{
    // check for the dir
    if (_dir)
    {
        if (!boost::filesystem::is_directory(boost::filesystem::path(_filePath)))
        {
            BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment(
                                      "Please check the existence of directory " + _filePath));
        }
        return;
    }
    // check for the file
    if (!boost::filesystem::exists(boost::filesystem::path(_filePath)))
    {
        BOOST_THROW_EXCEPTION(
            InvalidConfig() << errinfo_comment("Please check the existence of file " + _filePath));
    }
}

void PPCConfig::loadRA2018Config(boost::property_tree::ptree const& _pt)
{
    m_disableRA2018 = _pt.get<bool>("agency.disable_ra2018", false);
    if (m_disableRA2018)
    {
        PPCConfig_LOG(INFO) << LOG_DESC("The ra2018-psi has been disabled!");
        return;
    }
    // load the database
    m_ra2018PSIConfig.dbName = _pt.get<std::string>("ra2018psi.database", "ra2018");
    ///// load the cuckoo-filter-option
    auto cuckooFilterOption = std::make_shared<CuckoofilterOption>();
    // capacity, unit with MBytes
    auto capacity = _pt.get<uint64_t>(
        "ra2018psi.cuckoofilter_capacity", RA2018Config::DefaultCuckooFilterCapacity);
    if (capacity == 0)
    {
        BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment(
                                  "the ra2018psi.cuckoofilter_capacity must be larger than 0"));
    }
    cuckooFilterOption->capacity = capacity * 1024 * 1024;
    // tagBits
    cuckooFilterOption->tagBits = _pt.get<uint8_t>(
        "ra2018psi.cuckoofilter_tagBits", RA2018Config::DefaultCuckooFilterTagSize);
    if (cuckooFilterOption->tagBits == 0 || cuckooFilterOption->tagBits % 8 != 0 ||
        cuckooFilterOption->tagBits > 64)
    {
        BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment(
                                  "the ra2018psi.cuckoofilter_tagBits must be 8/16/32/64"));
    }
    // bucket-size
    cuckooFilterOption->tagNumPerBucket =
        _pt.get<uint8_t>("ra2018psi.cuckoofilter_buckets_num", RA2018Config::DefaultBucketSize);
    if (cuckooFilterOption->tagNumPerBucket == 0)
    {
        BOOST_THROW_EXCEPTION(
            InvalidConfig() << errinfo_comment(
                "the ra2018psi.cuckoofilter_buckets_num must be larger than 0, recommend to " +
                std::to_string(RA2018Config::DefaultBucketSize)));
    }
    // maxKickOutCount
    cuckooFilterOption->maxKickOutCount = _pt.get<uint16_t>(
        "ra2018psi.cuckoofilter_max_kick_out_count", RA2018Config::DefaultMaxKickoutCount);
    if (cuckooFilterOption->maxKickOutCount == 0)
    {
        BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment(
                                  "the ra2018psi.cuckoofilter_max_kick_out_count must be larger "
                                  "than 0, recommend to " +
                                  std::to_string(RA2018Config::DefaultMaxKickoutCount)));
    }
    cuckooFilterOption->trashBucketSize =
        _pt.get<uint64_t>("ra2018psi.trash_bucket_size", RA2018Config::DefaultTrashBucketSize);
    if (cuckooFilterOption->trashBucketSize == 0)
    {
        BOOST_THROW_EXCEPTION(
            InvalidConfig() << errinfo_comment(
                "the ra2018psi.trash_bucket_size must be larger than 0, recommend to " +
                boost::lexical_cast<std::string>(RA2018Config::DefaultTrashBucketSize)));
    }
    m_ra2018PSIConfig.cuckooFilterOption = std::move(cuckooFilterOption);
    ///// load the cuckoo-filter-cache-size
    auto cacheSize = _pt.get<uint64_t>(
        "ra2018psi.cuckoofilter_cache_size", RA2018Config::DefaultCuckooFilterCacheSize);
    if (cacheSize == 0)
    {
        BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment(
                                  "the ra2018psi.cuckoofilter_cache_size must be larger than 0" +
                                  std::to_string(RA2018Config::DefaultCuckooFilterCacheSize)));
    }
    m_ra2018PSIConfig.cuckooFilterCacheSize = cacheSize * 1024 * 1024;

    ///// load the psi-cache-size
    cacheSize = _pt.get<uint64_t>("ra2018psi.psi_cache_size", RA2018Config::DefaultCacheSize);
    if (cacheSize == 0)
    {
        BOOST_THROW_EXCEPTION(
            InvalidConfig() << errinfo_comment(
                "the ra2018psi.psi_cache_size must be larger than 0, recommend to " +
                std::to_string(RA2018Config::DefaultCacheSize)));
    }
    m_ra2018PSIConfig.cacheSize = cacheSize * 1024 * 1024;
    ///// load the psi-data-batch-size
    auto dataBatchSize = _pt.get<int64_t>("ra2018psi.data_batch_size", DefaultDataBatchSize);
    m_ra2018PSIConfig.dataBatchSize = getDataBatchSize("ra2018psi.data_batch_size", dataBatchSize);

    m_ra2018PSIConfig.useHDFS = _pt.get<bool>("ra2018psi.use_hdfs", false);
    // Note: the cuckooFilterOption has been moved, should not be accessed here
    PPCConfig_LOG(INFO) << LOG_DESC("loadRA2018Config")
                        << LOG_KV("database", m_ra2018PSIConfig.dbName)
                        << printCuckooFilterOption(m_ra2018PSIConfig.cuckooFilterOption)
                        << LOG_KV("cuckooFilterCacheSize", m_ra2018PSIConfig.cuckooFilterCacheSize)
                        << LOG_KV("psiCacheSize", m_ra2018PSIConfig.cacheSize)
                        << LOG_KV("dataBatchSize", m_ra2018PSIConfig.dataBatchSize)
                        << LOG_KV("useHDFS", m_ra2018PSIConfig.useHDFS);
}

// load the ecdh-psi config
void PPCConfig::loadEcdhPSIConfig(boost::property_tree::ptree const& _pt)
{
    auto dataBatchSize = _pt.get<int64_t>("ecdh-psi.data_batch_size", DefaultDataBatchSize);
    m_ecdhPSIConfig.dataBatchSize = getDataBatchSize("ecdh-psi.data_batch_size", dataBatchSize);
    PPCConfig_LOG(INFO) << LOG_DESC("loadEcdhPSIConfig")
                        << LOG_KV("dataBatchSize", m_ecdhPSIConfig.dataBatchSize);
}

void PPCConfig::loadCM2020PSIConfig(boost::property_tree::ptree const& _pt)
{
    m_cm2020Config.parallelism = _pt.get<uint16_t>("cm2020-psi.parallelism", DefaultParallelism);
    PPCConfig_LOG(INFO) << LOG_DESC("loadCM2020PSIConfig")
                        << LOG_KV("parallelism", m_cm2020Config.parallelism);
}

void PPCConfig::loadEcdhMultiPSIConfig(boost::property_tree::ptree const& _pt)
{
    auto dataBatchSize = _pt.get<int64_t>("ecdh-multi-psi.data_batch_size", DefaultDataBatchSize);
    m_ecdhMultiPSIConfig.dataBatchSize =
        getDataBatchSize("ecdh-multi-psi.data_batch_size", dataBatchSize);
    PPCConfig_LOG(INFO) << LOG_DESC("loadEcdhMultiPSIConfig")
                        << LOG_KV("dataBatchSize", m_ecdhMultiPSIConfig.dataBatchSize);
}

void PPCConfig::loadEcdhConnPSIConfig(boost::property_tree::ptree const& _pt)
{
    m_ecdhConnPSIConfig.rank = _pt.get<int64_t>("ecdh-conn-psi.rank", 0);
    m_ecdhConnPSIConfig.algos = _pt.get<int64_t>("ecdh-conn-psi.algos", 1);
    m_ecdhConnPSIConfig.protocol_families = _pt.get<int64_t>("ecdh-conn-psi.protocol_families", 1);
    m_ecdhConnPSIConfig.curve = _pt.get<int64_t>("ecdh-conn-psi.curve", 2);
    m_ecdhConnPSIConfig.hashtype = _pt.get<int64_t>("ecdh-conn-psi.hashtype", 11);
    m_ecdhConnPSIConfig.hash2curve = _pt.get<int64_t>("ecdh-conn-psi.hash2curve", 2);
    auto dataBatchSize = _pt.get<int64_t>("ecdh-conn-psi.data_batch_size", DefaultDataBatchSize);
    m_ecdhConnPSIConfig.dataBatchSize =
        getDataBatchSize("ecdh-conn-psi.data_batch_size", dataBatchSize);
    PPCConfig_LOG(INFO) << LOG_DESC("loadEcdhConnPSIConfig Success");
}

int64_t PPCConfig::getDataBatchSize(std::string const& _section, int64_t _dataBatchSize)
{
    int64_t dataBatchSize = _dataBatchSize;
    if (dataBatchSize != -1 && dataBatchSize <= 0)
    {
        BOOST_THROW_EXCEPTION(
            InvalidConfig() << errinfo_comment("the " + _section +
                                               " must be larger than 0 or equal to -1(load all "
                                               "data into memory at-once) recommend to " +
                                               std::to_string(DefaultDataBatchSize)));
    }
    // in-case of the data over the max-allowed-message size
    int64_t maxNetworkItems = (GatewayConfig::MaxMsgSize - 1024 * 1024) / 32;
    if (dataBatchSize == -1 || dataBatchSize > maxNetworkItems)
    {
        dataBatchSize = std::min((uint64_t)maxNetworkItems, (uint64_t)INT_MAX);
    }
    PPCConfig_LOG(INFO) << LOG_DESC("getDataBatchSize") << LOG_KV("section", _section)
                        << LOG_KV("maxNetworkItems", maxNetworkItems)
                        << LOG_KV("dataBatchSize", dataBatchSize);
    return dataBatchSize;
}

void PPCConfig::loadCommonConfig(boost::property_tree::ptree const& _pt)
{
    m_agencyID = _pt.get<std::string>("agency.id", "");
    if (m_agencyID.empty())
    {
        BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment("Must set agency.id"));
    }
    if (m_privateKeyPath.empty())
    {
        m_privateKeyPath =
            _pt.get<std::string>("agency.private_key_path", "conf/" + std::string(NODE_PEM_NAME));
        checkFileExists(m_privateKeyPath, false);
    }
    // load the crypto config
    m_smCrypto = _pt.get<bool>("crypto.sm_crypto", false);

    m_dataLocation = _pt.get<std::string>("agency.data_location", "data");

    m_taskTimeoutMinutes = _pt.get<uint32_t>("agency.task_timeout_minutes", 180);

    m_threadPoolSize = _pt.get<uint32_t>(
        "agency.thread_count", static_cast<uint32_t>(std::thread::hardware_concurrency() * 0.75));

    PPCConfig_LOG(INFO) << LOG_DESC("loadCommonConfig success") << LOG_KV("agencyID", m_agencyID)
                        << LOG_KV("dataLocation", m_dataLocation) << LOG_KV("smCrypto", m_smCrypto)
                        << LOG_KV("taskTimeoutMinutes", m_taskTimeoutMinutes)
                        << LOG_KV("threadPoolSize", m_threadPoolSize);
}

void PPCConfig::loadHDFSConfig(boost::property_tree::ptree const& _pt)
{
    SQLConnectionOption opt;
    opt.host = _pt.get<std::string>("storage.host", "127.0.0.1");
    // checkNonEmptyField("storage.host", opt.host);

    opt.port = _pt.get<int>("storage.port", 3306);

    opt.user = _pt.get<std::string>("storage.user", "ppcs");
    // checkNonEmptyField("storage.user", opt.user);

    opt.password = _pt.get<std::string>("storage.password", "ppcs");
    // checkNonEmptyField("storage.password", opt.password);

    opt.database = _pt.get<std::string>("storage.database", "");
    // checkNonEmptyField("storage.database", opt.database);

    m_storageConfig.sqlConnectionOpt = std::make_shared<SQLConnectionOption>(opt);
    PPCConfig_LOG(INFO) << LOG_DESC("loadStorageConfig: load sql connection option success")
                        << m_storageConfig.sqlConnectionOpt->desc();

    auto option = std::make_shared<FileStorageConnectionOption>();
    // the name node
    option->nameNode = _pt.get<std::string>("hdfs_storage.name_node", "");
    // checkNonEmptyField("hdfs_storage.name_node", option->nameNode);
    // the name node port
    option->nameNodePort = _pt.get<int>("hdfs_storage.name_node_port", 8020);
    // the user
    option->userName = _pt.get<std::string>("hdfs_storage.user", "root");
    // checkNonEmptyField("hdfs_storage.user", option->userName);
    // the token
    option->token = _pt.get<std::string>("hdfs_storage.token", "");
    // replace-datanode-on-failure
    option->replaceDataNodeOnFailure =
        _pt.get<bool>("hdfs_storage.replace-datanode-on-failure", false);
    // connection-timeout
    option->connectionTimeout = _pt.get<uint16_t>("hdfs_storage.connection-timeout", 1000);
    m_storageConfig.fileStorageConnectionOpt = option;
    PPCConfig_LOG(INFO) << LOG_DESC("loadStorageConfig: load hdfs connection option success")
                        << option->desc();
}

void PPCConfig::loadSQLConfig(boost::property_tree::ptree const& _pt)
{
    SQLConnectionOption opt;
    opt.host = _pt.get<std::string>("storage.host", "127.0.0.1");
    // checkNonEmptyField("storage.host", opt.host);

    opt.port = _pt.get<int>("storage.port", 3306);

    opt.user = _pt.get<std::string>("storage.user", "wedpr");
    // checkNonEmptyField("storage.user", opt.user);

    opt.password = _pt.get<std::string>("storage.password", "");
    // checkNonEmptyField("storage.password", opt.password);

    opt.database = _pt.get<std::string>("storage.database", "");
    // checkNonEmptyField("storage.database", opt.database);

    m_storageConfig.sqlConnectionOpt = std::make_shared<SQLConnectionOption>(opt);
    PPCConfig_LOG(INFO) << LOG_DESC("loadStorageConfig: load sql connection option success")
                        << m_storageConfig.sqlConnectionOpt->desc();
}


void PPCConfig::loadStorageConfig(boost::property_tree::ptree const& _pt)
{
    loadSQLConfig(_pt);
    loadHDFSConfig(_pt);
}

void PPCConfig::loadCEMConfig(boost::property_tree::ptree const& _pt)
{
    m_cemConfig.datasetFilePath = _pt.get<std::string>("cem.dataset_file_path", "./");
    m_cemConfig.datasetHDFSPath = _pt.get<std::string>("cem.dataset_hdfs_path", "./");
    m_cemConfig.ciphertextSuffix = _pt.get<std::string>("cem.ciphertext_suffix", "-encrypted");
    m_cemConfig.readPerBatchLines = _pt.get<uint64_t>("cem.read_per_batch_lines", 100000);
    loadHDFSConfig(_pt);
}

void PPCConfig::loadMPCConfig(boost::property_tree::ptree const& _pt)
{
    m_mpcConfig.datasetHDFSPath = _pt.get<std::string>("mpc.dataset_hdfs_path", "./");
    m_mpcConfig.jobPath = _pt.get<std::string>("mpc.job_path", "./");
    m_mpcConfig.mpcRootPath = _pt.get<std::string>("mpc.mpc_root_path", "./");
    m_mpcConfig.mpcRootPathNoGateway = _pt.get<std::string>("mpc.mpc_root_path_no_gateway", "./");
    m_mpcConfig.readPerBatchLines = _pt.get<std::uint64_t>("mpc.read_per_batch_lines", 100000);
    loadHDFSConfig(_pt);
}

//////// the tars config
void PPCConfig::loadTarsConfig(boost::property_tree::ptree const& _pt)
{
    PPCConfig_LOG(INFO) << LOG_DESC("loadTarsConfig");
    // load the service name
    std::string gatewayServiceConfigSection = "tars_gateway";
    m_gatewayServiceName =
        getServiceName(_pt, gatewayServiceConfigSection + ".name", GATEWAY_SERVANT_NAME);
    // load the holding_msg_minutes
    m_holdingMessageMinutes =
        loadHoldingMessageMinutes(_pt, gatewayServiceConfigSection + ".holding_msg_minutes");
    // config the gateway information
    loadServiceTarsConfig(_pt, m_gatewayServiceName, gatewayServiceConfigSection);

    PPCConfig_LOG(INFO) << LOG_DESC("loadTarsConfig success")
                        << LOG_KV("gatewayService", m_gatewayServiceName)
                        << LOG_KV("endPoints", m_serviceToEndPoints.at(m_gatewayServiceName).size())
                        << LOG_KV("holdingMessageMinutes", m_holdingMessageMinutes);
}

void PPCConfig::loadServiceTarsConfig(boost::property_tree::ptree const& _pt,
    std::string const& _serviceName, std::string const& _sectionName)
{
    PPCConfig_LOG(INFO) << LOG_DESC("loadServiceTarsConfig for ") << _sectionName
                        << LOG_KV("service", _serviceName);
    if (!_pt.get_child_optional(_sectionName))
    {
        BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment("Must config " + _serviceName));
    }
    for (auto const& it : _pt.get_child(_sectionName))
    {
        if (it.first.find("proxy.") != 0)
        {
            continue;
        }
        std::string endpoint = it.second.data();
        PPCConfig_LOG(INFO) << LOG_BADGE("loadServiceTarsConfig")
                            << LOG_KV("serviceName", _serviceName) << LOG_KV("endpoint", endpoint);
        if (!checkEndpoint(endpoint))
        {
            BOOST_THROW_EXCEPTION(
                InvalidConfig() << errinfo_comment("Invalid endpoint: " + endpoint));
        }
        m_serviceToEndPoints[_serviceName].emplace_back(std::move(endpoint));
    }
}

std::string PPCConfig::getServiceName(boost::property_tree::ptree const& _pt,
    std::string const& _configSection, std::string const& _objName)
{
    auto serviceName = _pt.get<std::string>(_configSection, "");
    checkService(_configSection, serviceName);
    return getPrxDesc(serviceName, _objName);
}

void PPCConfig::checkService(std::string const& _serviceType, std::string const& _serviceName)
{
    if (_serviceName.empty())
    {
        BOOST_THROW_EXCEPTION(
            InvalidConfig() << errinfo_comment("Must set service name for " + _serviceType + "!"));
    }
    std::vector<std::string> serviceNameList;
    boost::split(serviceNameList, _serviceName, boost::is_any_of("."));
    std::string errorMsg =
        "Must set service name in format of application_name.server_name with only include letters "
        "and numbers for " +
        _serviceType + ", invalid config now is:" + _serviceName;
    if (serviceNameList.size() != 2)
    {
        BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment(errorMsg));
    }
    for (const auto& serviceName : serviceNameList)
    {
        if (!isalNumStr(serviceName))
        {
            BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment(errorMsg));
        }
    }
}
