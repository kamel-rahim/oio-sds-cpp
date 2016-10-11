/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <kinetic.pb.h>
#include "GetKeyRange.h"

namespace proto = ::com::seagate::kinetic::proto;
using oio::kinetic::rpc::Exchange;
using oio::kinetic::rpc::Request;
using oio::kinetic::rpc::GetKeyRange;

GetKeyRange::GetKeyRange(): Exchange() {
    auto h = cmd.mutable_header();
    h->set_messagetype(proto::Command_MessageType_GETKEYRANGE);
    auto r = cmd.mutable_body()->mutable_range();
    r->set_startkeyinclusive(true);
    r->set_endkeyinclusive(true);
    r->set_maxreturned(200);
}

GetKeyRange::~GetKeyRange() { }

void GetKeyRange::ManageReply(Request &rep) {
    checkStatus(rep);
    if (!status_)
        return;
    for (const auto &k: rep.cmd.body().range().keys())
        out_.push_back(k);
}

void GetKeyRange::Start(const char *k) {
    cmd.mutable_body()->mutable_range()->set_startkey(k);
}

void GetKeyRange::Start(const std::string &k) {
    cmd.mutable_body()->mutable_range()->set_startkey(k);
}

void GetKeyRange::End(const char *k) {
    cmd.mutable_body()->mutable_range()->set_endkey(k);
}

void GetKeyRange::End(const std::string &k) {
    cmd.mutable_body()->mutable_range()->set_endkey(k);
}

void GetKeyRange::IncludeStart(bool v) {
    cmd.mutable_body()->mutable_range()->set_startkeyinclusive(v);
}

void GetKeyRange::IncludeEnd(bool v) {
    cmd.mutable_body()->mutable_range()->set_endkeyinclusive(v);
}

void GetKeyRange::MaxItems(int32_t v) {
    cmd.mutable_body()->mutable_range()->set_maxreturned(v);
}

void GetKeyRange::Steal (std::vector<std::string> &v) {
    out_.swap(v);
}
