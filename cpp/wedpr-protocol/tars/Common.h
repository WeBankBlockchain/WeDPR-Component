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
 * @file Common.h
 * @author: shawnhe
 * @date 2022-10-20
 */

#pragma once

#include "TarsServantProxyCallback.h"
#include "ppc-framework/protocol/Task.h"
#include "ppc-tools/src/config/ParamChecker.h"
#include <bcos-utilities/Common.h>
#include <servant/Application.h>
#include <servant/Communicator.h>
#include <tup/Tars.h>
#include <util/tc_clientsocket.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/test/tools/old/interface.hpp>
#include <boost/throw_exception.hpp>
#include <cstdint>
#include <functional>
#include <memory>

namespace ppctars
{
namespace serialize
{
template <class Container>
class BufferWriter
{
protected:
    using ByteType = typename Container::value_type;
    using SizeType = typename Container::size_type;

    mutable Container _buffer;
    ByteType* _buf;
    SizeType _len;
    SizeType _buf_len;
    std::function<ByteType*(BufferWriter&, size_t)> _reserve;

private:
    BufferWriter(const BufferWriter&);
    BufferWriter& operator=(const BufferWriter& buf);

public:
    BufferWriter() : _buf(NULL), _len(0), _buf_len(0), _reserve({})
    {
#ifndef GEN_PYTHON_MASK
        _reserve = [](BufferWriter& os, size_t len) {
            os._buffer.resize(len);
            return os._buffer.data();
        };
#endif
    }

    ~BufferWriter() {}

    void reset() { _len = 0; }

    void writeBuf(const ByteType* buf, size_t len)
    {
        TarsReserveBuf(*this, _len + len);
        memcpy(_buf + _len, buf, len);
        _len += len;
    }

    const Container& getByteBuffer() const
    {
        _buffer.resize(_len);
        return _buffer;
    }
    Container& getByteBuffer()
    {
        _buffer.resize(_len);
        return _buffer;
    }
    const ByteType* getBuffer() const { return _buf; }
    size_t getLength() const { return _len; }
    void swap(std::vector<ByteType>& v)
    {
        _buffer.resize(_len);
        v.swap(_buffer);
        _buf = NULL;
        _buf_len = 0;
        _len = 0;
    }
    void swap(BufferWriter& buf)
    {
        buf._buffer.swap(_buffer);
        std::swap(_buf, buf._buf);
        std::swap(_buf_len, buf._buf_len);
        std::swap(_len, buf._len);
    }
};

using BufferWriterByteVector = BufferWriter<std::vector<bcos::byte>>;
using BufferWriterStdByteVector = BufferWriter<std::vector<std::byte>>;
using BufferWriterString = BufferWriter<std::string>;
}  // namespace serialize

inline std::string getProxyDesc(std::string const& _servantName)
{
    std::string desc =
        tars::ServerConfig::Application + "." + tars::ServerConfig::ServerName + "." + _servantName;
    return desc;
}

inline std::string getLogPath()
{
    return tars::ServerConfig::LogPath + "/" + tars::ServerConfig::Application + "/" +
           tars::ServerConfig::ServerName;
}

inline tars::TC_Endpoint string2TarsEndPoint(const std::string& _strEndPoint)
{
    std::vector<std::string> temp;
    boost::split(temp, _strEndPoint, boost::is_any_of(":"), boost::token_compress_on);

    if (temp.size() != 2)
    {
        BOOST_THROW_EXCEPTION(bcos::InvalidParameter() << bcos::errinfo_comment(
                                  "incorrect string endpoint, it should be in IP:Port format"));
    }

    tars::TC_Endpoint ep(temp[0], boost::lexical_cast<int>(temp[1]), 60000);
    return ep;
}

inline std::vector<tars::TC_Endpoint> toTarsEndPoints(std::vector<std::string> const& _endPoints)
{
    std::vector<tars::TC_Endpoint> tarsEndPoints;
    for (auto const& it : _endPoints)
    {
        tarsEndPoints.emplace_back(string2TarsEndPoint(it));
    }
    return tarsEndPoints;
}

inline std::string endPointToString(std::string const& _serviceName, const std::string& _endpoint)
{
    std::vector<std::string> url;
    boost::split(url, _endpoint, boost::is_any_of(":"), boost::token_compress_on);
    return _serviceName + "@tcp -h " + url[0] + " -p " + url[1];
}

inline std::string endPointToString(
    std::string const& _serviceName, const std::string& _host, uint16_t _port)
{
    return _serviceName + "@tcp -h " + _host + " -p " + boost::lexical_cast<std::string>(_port);
}

inline std::string endPointToString(
    std::string const& _serviceName, tars::TC_Endpoint const& _endPoint)
{
    return endPointToString(_serviceName, _endPoint.getHost(), _endPoint.getPort());
}

inline std::string endPointToString(
    std::string const& _serviceName, const std::vector<tars::TC_Endpoint>& _eps)
{
    if (_eps.empty())
    {
        BOOST_THROW_EXCEPTION(
            bcos::InvalidParameter() << bcos::errinfo_comment(
                "vector<tars::TC_Endpoint> should not be empty in endPointToString"));
    }

    bool first = true;
    std::string endPointStr = _serviceName;
    for (const auto& _ep : _eps)
    {
        endPointStr += (first ? "@" : ":");
        endPointStr +=
            ("tcp -h " + _ep.getHost() + " -p " + boost::lexical_cast<std::string>(_ep.getPort()));

        first = false;
    }
    return endPointStr;
}

inline std::pair<bool, std::string> getEndPointDescByAdapter(tars::Application* _application,
    std::string const& _servantName, std::string const& _endpoint = "")
{
    auto adapters = _application->getEpollServer()->getBindAdapters();
    if (adapters.size() == 0)
    {
        return std::make_pair(false, "");
    }
    auto prxDesc = getProxyDesc(_servantName);
    if (!_endpoint.empty())
    {
        if (!ppc::tools::checkEndpoint(_endpoint))
        {
            BOOST_THROW_EXCEPTION(bcos::InvalidParameter() << bcos::errinfo_comment(
                                      "incorrect string endpoint, it should be in IP:Port format"));
        }
        return std::make_pair(true, endPointToString(prxDesc, _endpoint));
    }

    auto adapterName = prxDesc + "Adapter";
    for (auto const& adapter : adapters)
    {
        if (adapter->getName() == adapterName)
        {
            return std::make_pair(true, endPointToString(prxDesc, adapter->getEndpoint()));
        }
    }
    return std::make_pair(false, "");
}

template <typename S>
S createServantProxy(tars::Communicator* communicator, std::string const& _endPointStr,
    TarsServantProxyOnConnectHandler _connectHandler = TarsServantProxyOnConnectHandler(),
    TarsServantProxyOnCloseHandler _closeHandler = TarsServantProxyOnCloseHandler())
{
    auto prx = communicator->stringToProxy<S>(_endPointStr);

    BCOS_LOG(DEBUG) << LOG_DESC("createServantProxy ") << _endPointStr;

    if (!prx->tars_get_push_callback())
    {
        auto proxyCallback = new ppctars::TarsServantProxyCallback(_endPointStr, *prx);

        if (_connectHandler)
        {
            proxyCallback->setOnConnectHandler(_connectHandler);
        }

        if (_closeHandler)
        {
            proxyCallback->setOnCloseHandler(_closeHandler);
        }

        prx->tars_set_push_callback(proxyCallback);
        proxyCallback->startTimer();
    }
    return prx;
}

template <typename S>
S createServantProxy(std::string const& _endPointStr)
{
    return createServantProxy<S>(tars::Application::getCommunicator().get(), _endPointStr,
        TarsServantProxyOnConnectHandler(), TarsServantProxyOnCloseHandler());
}

template <typename S>
S createServantProxy(std::string const& _serviceName, const std::string& _host, uint16_t _port)
{
    auto endPointStr = endPointToString(_serviceName, _host, _port);
    return createServantProxy<S>(endPointStr);
}

template <typename S>
S createServantProxy(std::string const& _serviceName, const tars::TC_Endpoint& _ep)
{
    auto endPointStr = endPointToString(_serviceName, _ep);
    return createServantProxy<S>(endPointStr);
}

template <typename S>
S createServantProxy(std::string const& _serviceName, const std::vector<tars::TC_Endpoint>& _eps)
{
    std::string endPointStr = endPointToString(_serviceName, _eps);

    return createServantProxy<S>(endPointStr);
}

template <typename S>
S createServantProxy(bool _withEndPoints, std::string const& _serviceName,
    const std::vector<tars::TC_Endpoint>& _eps = std::vector<tars::TC_Endpoint>{})
{
    std::string serviceParams;
    if (_withEndPoints)
    {
        serviceParams = endPointToString(_serviceName, _eps);
    }
    else
    {
        serviceParams = _serviceName;
    }

    return createServantProxy<S>(serviceParams);
}
}  // namespace ppctars