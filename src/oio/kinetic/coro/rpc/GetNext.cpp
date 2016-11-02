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

#include "oio/kinetic/coro/rpc/GetNext.h"

using oio::kinetic::rpc::GetNext;
namespace proto = ::com::seagate::kinetic::proto;

GetNext::GetNext(): Exchange(), out_() {
    auto h = cmd.mutable_header();
    h->set_messagetype(proto::Command_MessageType_GETNEXT);
    auto kv = cmd.mutable_body()->mutable_keyvalue();
    kv->set_algorithm(proto::Command_Algorithm_SHA1);
}

GetNext::~GetNext() {}

void GetNext::Key(const char *k) {
    cmd.mutable_body()->mutable_keyvalue()->set_key(k);
}

void GetNext::Key(const std::string &k) {
    cmd.mutable_body()->mutable_keyvalue()->set_key(k);
}

void GetNext::Steal(std::string *v) {
    assert(v != nullptr);
    v->swap(out_);
}

void GetNext::ManageReply(oio::kinetic::rpc::Request *rep) { checkStatus(rep); }
