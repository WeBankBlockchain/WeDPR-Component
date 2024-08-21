/**
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
 * @file GatewayServiceApp.cpp
 * @author: shawnhe
 * @date 2022-10-21
 */
#include "GatewayServiceApp.h"
#include "ppc-storage/src/CacheStorageFactoryImpl.h"
#include "ppc-tars-protocol/Common.h"
#include "ppc-tars-service/GatewayService/GatewayInitializer.h"
#include "ppc-tars-service/GatewayService/GatewayServiceServer.h"
#include "ppc-tools/src/config/NetworkConfig.h"

using namespace ppctars;
using namespace ppc;
using namespace ppc::tools;
using namespace ppc::gateway;
using namespace ppc::storage;
using namespace ppc::protocol;

using namespace bcos;
using namespace bcos::boostssl;
using namespace bcos::boostssl::ws;

void GatewayServiceApp::initialize()
{
    try
    {
        m_configPath = tars::ServerConfig::BasePath + "/config.ini";
        addConfig("config.ini");

        GATEWAYAPP_LOG(INFO) << LOG_DESC("initService") << LOG_KV("config", m_configPath);
        initService(m_configPath);

        GATEWAYAPP_LOG(INFO) << LOG_DESC("initService success") << LOG_KV("config", m_configPath);

        GatewayServiceParam param;
        param.gateway = m_gatewayInitializer->gateway();
        param.ppcMsgFactory = m_gatewayInitializer->messageFactory();
        addServantWithParams<GatewayServiceServer, GatewayServiceParam>(
            getProxyDesc(GATEWAY_SERVANT_NAME), param);
    }
    catch (std::exception const& e)
    {
        std::cout << "init GatewayServiceApp failed, error: " << boost::diagnostic_information(e)
                  << std::endl;
        throw e;
    }
}

void GatewayServiceApp::initService(std::string const& _configPath)
{
    boost::property_tree::ptree pt;
    boost::property_tree::read_ini(_configPath, pt);
    auto ppcConfig = std::make_shared<PPCConfig>();
    // init the log
    m_logInitializer = std::make_shared<bcos::BoostLogInitializer>();
    m_logInitializer->initLog(pt);

    // load the gatewayConfig
    auto config = std::make_shared<PPCConfig>();
    config->loadGatewayConfig(
        ppc::protocol::NodeArch::PRO, tars::ServerConfig::BasePath.c_str(), pt);
    // add the config
    auto const& networkConfig = config->gatewayConfig().networkConfig;

    if (!networkConfig.disableSsl)
    {
        GATEWAYAPP_LOG(INFO) << LOG_DESC("addConfig") << LOG_KV("enableSM", networkConfig.enableSM);
        if (!networkConfig.enableSM)
        {
            addConfig(std::string(NetworkConfig::CA_CERT_NAME));
            addConfig(std::string(NetworkConfig::SSL_CERT_NAME));
            addConfig(std::string(NetworkConfig::SSL_KEY_NAME));
        }
        else
        {
            addConfig(std::string(NetworkConfig::SM_CA_CERT_NAME));
            addConfig(std::string(NetworkConfig::SM_SSL_CERT_NAME));
            addConfig(std::string(NetworkConfig::SM_SSL_KEY_NAME));
            addConfig(std::string(NetworkConfig::SM_SSL_EN_KEY_NAME));
            addConfig(std::string(NetworkConfig::SM_SSL_EN_CERT_NAME));
        }
        GATEWAYAPP_LOG(INFO) << LOG_DESC("addConfig success")
                             << LOG_KV("enableSM", networkConfig.enableSM);
    }

    // redis cache
    storage::CacheStorage::Ptr cache;
    if (!config->gatewayConfig().disableCache)
    {
        GATEWAYAPP_LOG(INFO) << LOG_DESC("initService: buildRedisStorage")
                             << config->gatewayConfig().cacheStorageConfig.desc();
        auto cacheStorageFactory = std::make_shared<CacheStorageFactoryImpl>();
        cache = cacheStorageFactory->createCacheStorage(config->gatewayConfig().cacheStorageConfig);
        try
        {
            cache->exists("check_cache");
        }
        catch (std::exception& e)
        {
            BOOST_THROW_EXCEPTION(
                InvalidConfig() << errinfo_comment(
                    "init cache error:" + std::string(boost::diagnostic_information(e))));
        }
        GATEWAYAPP_LOG(INFO) << LOG_DESC("initService: buildRedisStorage success");
    }

    // message factory
    auto messageFactory = std::make_shared<ppc::front::PPCMessageFactory>();
    // global thread pool
    auto threadPoolSize = config->gatewayConfig().networkConfig.threadPoolSize;
    auto threadPool = std::make_shared<ThreadPool>(GATEWAY_THREAD_POOL_MODULE, threadPoolSize);

    GATEWAYAPP_LOG(INFO) << LOG_DESC("initService: build and start gateway");
    m_gatewayInitializer = std::make_shared<GatewayInitializer>(
        NodeArch::PRO, config, cache, messageFactory, threadPool);
    m_gatewayInitializer->start();
    GATEWAYAPP_LOG(INFO) << LOG_DESC("initService: build and start gateway success");
}