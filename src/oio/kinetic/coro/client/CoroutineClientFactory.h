/**
 * Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/
 */

#ifndef SRC_OIO_KINETIC_CORO_CLIENT_COROUTINECLIENTFACTORY_H_
#define SRC_OIO_KINETIC_CORO_CLIENT_COROUTINECLIENTFACTORY_H_

#include <memory>
#include <map>
#include <string>
#include "oio/kinetic/coro/client/ClientInterface.h"

namespace oio {
namespace kinetic {
namespace client {

class CoroutineClientFactory : public ClientFactory {
 public:
    CoroutineClientFactory() : cnx() {}

    ~CoroutineClientFactory() {}

    std::shared_ptr<ClientInterface> Get(const std::string &url);

 private:
    std::map<std::string, std::shared_ptr<ClientInterface>> cnx;
};

}  // namespace client
}  // namespace kinetic
}  // namespace oio

#endif  // SRC_OIO_KINETIC_CORO_CLIENT_COROUTINECLIENTFACTORY_H_
