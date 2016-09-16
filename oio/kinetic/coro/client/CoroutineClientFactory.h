/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_KINETIC_CLIENT_COROUTINECLIENTFACTORY_H
#define OIO_KINETIC_CLIENT_COROUTINECLIENTFACTORY_H

#include <memory>
#include <string>
#include "ClientInterface.h"

namespace oio {
namespace kinetic {
namespace client {

class CoroutineClientFactory : public ClientFactory {
  public:
    CoroutineClientFactory(): cnx() { }

    ~CoroutineClientFactory() { }

    std::shared_ptr<ClientInterface> Get(const std::string &url);

  private:
    std::map<std::string, std::shared_ptr<ClientInterface>> cnx;
};

} // namespace client
} // namespace kinetic
} // namespace oio

#endif //OIO_KINETIC_CLIENT_COROUTINECLIENTFACTORY_H
