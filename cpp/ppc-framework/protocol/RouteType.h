/**
 *  Copyright (C) 2023 WeDPR.
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
 * @file RouteType.h
 * @author: yujiechen
 * @date 2024-08-22
 */

#pragma once
#include <sstream>
#include <string>

namespace ppc::protocol
{
enum class RouteType : uint8_t
{
    ROUTE_THROUGH_NODEID = 0x00,
    ROUTE_THROUGH_COMPONENT = 0x01,
    ROUTE_THROUGH_AGENCY = 0x02,
    ROUTE_THROUGH_TOPIC = 0x03
};

inline std::ostream& operator<<(std::ostream& _out, RouteType const& _type)
{
    switch (_type)
    {
    case RouteType::ROUTE_THROUGH_NODEID:
        _out << "RouteThroughNodeID";
        break;
    case RouteType::ROUTE_THROUGH_COMPONENT:
        _out << "RouteThroughComponent";
        break;
    case RouteType::ROUTE_THROUGH_AGENCY:
        _out << "RouteThroughAgency";
        break;
    case RouteType::ROUTE_THROUGH_TOPIC:
        _out << "RouteThroughTopic";
        break;
    default:
        _out << "UnknownRouteType";
        break;
    }
    return _out;
}
}  // namespace ppc::protocol
