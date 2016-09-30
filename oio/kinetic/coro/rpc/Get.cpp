/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <glog/logging.h>
#include <oio/kinetic/coro/client/ClientInterface.h>
#include "Get.h"

using namespace oio::kinetic::client;
using namespace oio::kinetic::rpc;
namespace proto = ::com::seagate::kinetic::proto;

Get::Get(): Exchange(), out_() {
    auto h = cmd.mutable_header();
    h->set_messagetype(proto::Command_MessageType_GET);
    auto kv = cmd.mutable_body()->mutable_keyvalue();
    kv->set_algorithm(proto::Command_Algorithm_SHA1);
}

Get::~Get() { }

void Get::ManageReply(Request &rep) {
    checkStatus(rep);
    if (status_) {
        out_.clear();
        out_.swap(rep.value);
    }
}

void Get::Key(const char *k) {
    cmd.mutable_body()->mutable_keyvalue()->set_key(k);
}

void Get::Key(const std::string &k) {
    cmd.mutable_body()->mutable_keyvalue()->set_key(k);
}