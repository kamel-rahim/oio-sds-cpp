/**
 * This file is part of the OpenIO client libraries
 * Copyright (C) 2016 OpenIO SAS
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.
 */

#ifndef SRC_OIO_API_RESOLVER_HPP_
#define SRC_OIO_API_RESOLVER_HPP_

#include <string>

namespace oio {
namespace api {

class SrvID {
 protected:
    std::string host;
    int port;

 public:
    ~SrvID() {}

    SrvID() {}

    SrvID(const std::string _host, int _port) : host(_host), port(_port) {}

    SrvID(const SrvID &o) : host(o.host), port{o.port} {}

    SrvID(SrvID &&o) : host(), port{o.port} { host.swap(o.host); }

    void Set(const SrvID &o) {
        host.assign(o.host);
        port = o.port;
    }

    bool Parse(const std::string packed) {
        auto colon = packed.rfind(':');
        if (colon == packed.npos)
            return false;
        host.assign(packed, 0, colon);
        auto strport = std::string(packed, colon + 1);
        port = ::atoi(strport.c_str());
        return true;
    }
};

namespace resolver {

}  // namespace resolver

}  // namespace api
}  // namespace oio

#endif  // SRC_OIO_API_RESOLVER_HPP_
