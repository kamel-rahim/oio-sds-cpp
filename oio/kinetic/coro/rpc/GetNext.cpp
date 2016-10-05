/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include "GetNext.h"

using oio::kinetic::rpc::GetNext;
namespace proto = ::com::seagate::kinetic::proto;

GetNext::GetNext(): Exchange(), out_() {
    auto h = cmd.mutable_header();
    h->set_messagetype(proto::Command_MessageType_GETNEXT);
    auto kv = cmd.mutable_body()->mutable_keyvalue();
    kv->set_algorithm(proto::Command_Algorithm_SHA1);
}

GetNext::~GetNext() { }

void GetNext::Key(const char *k) {
    cmd.mutable_body()->mutable_keyvalue()->set_key(k);
}

void GetNext::Key(const std::string &k) {
    cmd.mutable_body()->mutable_keyvalue()->set_key(k);
}

void GetNext::Steal(std::string &v) {
    out_.swap(v);
}

void GetNext::ManageReply(oio::kinetic::rpc::Request &rep) {
    checkStatus(rep);
}