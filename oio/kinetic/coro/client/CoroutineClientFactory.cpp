/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include "ClientInterface.h"
#include "CoroutineClient.h"
#include "CoroutineClientFactory.h"

using namespace oio::kinetic::client;

std::shared_ptr<ClientInterface>
CoroutineClientFactory::Get(const std::string &url) {
    auto it = cnx.find(url);
    if (it != cnx.end())
        return it->second;

    CoroutineClient *client = new CoroutineClient(url);
    std::shared_ptr<ClientInterface> shared(client);
    cnx[url] = shared;
    return shared;
}
