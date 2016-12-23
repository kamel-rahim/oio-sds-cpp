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

#include <src/kinetic.pb.h>

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

GetKeyRange::~GetKeyRange() {}

void GetKeyRange::ManageReply(Request *rep) {
    checkStatus(rep);
    if (status_) {
        for (const auto &k : rep->cmd.body().range().keys())
            out_.push_back(k);
    }
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

void GetKeyRange::Steal(std::vector<std::string> *v) {
    assert(v != nullptr);
    v->swap(out_);
}
