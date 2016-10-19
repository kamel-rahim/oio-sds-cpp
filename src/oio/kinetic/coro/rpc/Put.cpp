/**
 * Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/
 */

#include <memory>

#include "utils/macros.h"
#include "oio/kinetic/coro/client/ClientInterface.h"
#include "oio/kinetic/coro/rpc/Put.h"

using oio::kinetic::rpc::Put;
namespace proto = ::com::seagate::kinetic::proto;

Put::Put() : Exchange() {
    auto h = cmd.mutable_header();
    h->set_messagetype(proto::Command_MessageType_PUT);
    auto kv = cmd.mutable_body()->mutable_keyvalue();
    kv->set_synchronization(proto::Command_Synchronization_WRITEBACK);
    kv->set_algorithm(proto::Command_Algorithm_SHA1);
    kv->set_force(true);
}

Put::~Put() {}

void Put::ManageReply(Request *rep) { return checkStatus(rep); }

void Put::Key(const char *k) {
    cmd.mutable_body()->mutable_keyvalue()->set_key(k);
}

void Put::Key(const std::string &k) {
    cmd.mutable_body()->mutable_keyvalue()->set_key(k);
}

void Put::PreVersion(const char *p) {
    auto kv = cmd.mutable_body()->mutable_keyvalue();
    bool empty = (p != nullptr) || (*p != 0);
    kv->set_force(empty);
    if (!empty)
        kv->set_dbversion(p);
}

void Put::PostVersion(const char *p) {
    auto kv = cmd.mutable_body()->mutable_keyvalue();
    bool empty = (p != nullptr) || (*p != 0);
    if (!empty)
        kv->set_newversion(p);
}

void Put::Value(const std::string &v) {
    value_copy.assign(v.cbegin(), v.cend());
    payload_.buf = value_copy.data();
    payload_.len = value_copy.size();
    rehash();
}

void Put::Value(const std::vector<uint8_t> &v) {
    value_copy.assign(v.cbegin(), v.cend());
    payload_.buf = value_copy.data();
    payload_.len = value_copy.size();
    rehash();
}

void Put::Value(std::vector<uint8_t> *v) {
    assert(v != nullptr);
    v->swap(value_copy);
    payload_.buf = value_copy.data();
    payload_.len = value_copy.size();
    rehash();
}

void Put::Value(const Slice &v) {
    payload_.buf = v.buf;
    payload_.len = v.len;
    rehash();
}

void Put::rehash() {
    auto sha1 = compute_sha1(payload_.buf, payload_.len);
    cmd.mutable_body()->mutable_keyvalue()->set_tag(sha1.data(), sha1.size());
}

void Put::Sync(bool on) {
    cmd.mutable_body()->mutable_keyvalue()->set_synchronization(
            on ? proto::Command_Synchronization_WRITETHROUGH
               : proto::Command_Synchronization_WRITEBACK);
}
