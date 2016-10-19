/**
 * Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/
 */

#include <src/kinetic.pb.h>

#include "oio/kinetic/coro/client/ClientInterface.h"
#include "oio/kinetic/coro/rpc/Delete.h"

namespace proto = ::com::seagate::kinetic::proto;
using oio::kinetic::rpc::Request;
using oio::kinetic::rpc::Delete;

Delete::Delete() : Exchange() {
    auto h = cmd.mutable_header();
    h->set_messagetype(proto::Command_MessageType_DELETE);
    auto kv = cmd.mutable_body()->mutable_keyvalue();
    kv->set_synchronization(proto::Command_Synchronization_WRITEBACK);
    kv->set_algorithm(proto::Command_Algorithm_SHA1);
}

Delete::~Delete() {}

void Delete::ManageReply(Request *rep) {
    checkStatus(rep);
}

void Delete::Key(const char *k) {
    assert(k != nullptr);
    return Key(std::string(k));
}

void Delete::Key(const std::string &k) {
    cmd.mutable_body()->mutable_keyvalue()->set_key(k);
}
