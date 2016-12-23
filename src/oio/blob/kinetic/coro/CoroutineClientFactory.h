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

#ifndef SRC_OIO_BLOB_KINETIC_CORO_COROUTINECLIENTFACTORY_H_
#define SRC_OIO_BLOB_KINETIC_CORO_COROUTINECLIENTFACTORY_H_

#include <memory>
#include <map>
#include <string>

#include "ClientInterface.h"

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

#endif  // SRC_OIO_BLOB_KINETIC_CORO_COROUTINECLIENTFACTORY_H_
