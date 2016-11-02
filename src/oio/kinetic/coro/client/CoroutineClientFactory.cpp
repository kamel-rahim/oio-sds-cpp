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
