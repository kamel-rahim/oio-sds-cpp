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

#ifndef SRC_OIO_KINETIC_CORO_CLIENT_CLIENTINTERFACE_H_
#define SRC_OIO_KINETIC_CORO_CLIENT_CLIENTINTERFACE_H_

#include <src/kinetic.pb.h>

#include <memory>
#include <string>
#include <vector>

#include "oio/blob/kinetic/coro/rpc/Exchange.h"

namespace oio {
namespace kinetic {
namespace client {

/* Represents a pending RPC */
class Sync {
 public:
    virtual ~Sync() {}

    virtual void Wait() = 0;
};

class ClientInterface {
 public:
    virtual std::shared_ptr<Sync> RPC(
            oio::kinetic::rpc::Exchange *ex) = 0;

    virtual std::string Id() const = 0;
};

class ClientFactory {
 public:
    ~ClientFactory() {}

    virtual std::shared_ptr<ClientInterface> Get(
            const std::string &url) = 0;
};

}  // namespace client
}  // namespace kinetic
}  // namespace oio

#endif  // SRC_OIO_KINETIC_CORO_CLIENT_CLIENTINTERFACE_H_
