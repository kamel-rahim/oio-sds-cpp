/**
 * Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/
 */

#include "oio/kinetic/coro/client/ClientInterface.h"
#include "oio/kinetic/coro/client/CoroutineClient.h"
#include "oio/kinetic/coro/client/CoroutineClientFactory.h"

using oio::kinetic::client::ClientInterface;
using oio::kinetic::client::CoroutineClient;
using oio::kinetic::client::CoroutineClientFactory;

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
