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

#include "utils/macros.h"
#include "oio/kinetic/coro/client/ClientInterface.h"
#include "oio/kinetic/coro/rpc/Get.h"

using oio::kinetic::client::ClientInterface;
using oio::kinetic::rpc::Get;
namespace proto = ::com::seagate::kinetic::proto;

Get::Get() : Exchange(), out_() {
    auto h = cmd.mutable_header();
    h->set_messagetype(proto::Command_MessageType_GET);
    auto kv = cmd.mutable_body()->mutable_keyvalue();
    kv->set_algorithm(proto::Command_Algorithm_SHA1);
}

Get::~Get() {}

void Get::ManageReply(Request *rep) {
    checkStatus(rep);
    if (status_) {
        out_.clear();
        out_.swap(rep->value);
    }
}

void Get::Key(const char *k) {
    cmd.mutable_body()->mutable_keyvalue()->set_key(k);
}

void Get::Key(const std::string &k) {
    cmd.mutable_body()->mutable_keyvalue()->set_key(k);
}
