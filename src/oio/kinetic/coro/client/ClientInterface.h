/**
 * Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/
 */

#ifndef SRC_OIO_KINETIC_CORO_CLIENT_CLIENTINTERFACE_H_
#define SRC_OIO_KINETIC_CORO_CLIENT_CLIENTINTERFACE_H_

#include <src/kinetic.pb.h>

#include <memory>
#include <string>
#include <vector>

#include "oio/kinetic/coro/rpc/Exchange.h"

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
