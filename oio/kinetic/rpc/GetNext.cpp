/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include "GetNext.h"

using oio::kinetic::rpc::GetNext;
namespace proto = ::com::seagate::kinetic::proto;

GetNext::GetNext() noexcept: req_(), out_(), status_{false} {
    req_.reset(new Request);
    auto h = req_->cmd.mutable_header();
    h->set_messagetype(proto::Command_MessageType_GETNEXT);
    auto kv = req_->cmd.mutable_body()->mutable_keyvalue();
    kv->set_algorithm(proto::Command_Algorithm_SHA1);
}

GetNext::~GetNext() noexcept { }

void GetNext::Key(const char *k) noexcept {
    assert(nullptr != req_.get());
    req_->cmd.mutable_body()->mutable_keyvalue()->set_key(k);
}

void GetNext::Key(const std::string &k) noexcept {
    assert(nullptr != req_.get());
    req_->cmd.mutable_body()->mutable_keyvalue()->set_key(k);
}

void GetNext::Steal(std::string &v) noexcept {
    out_.swap(v);
}

void GetNext::SetSequence(int64_t s) noexcept {
    assert(nullptr != req_.get());
    req_->cmd.mutable_header()->set_sequence(s);
}

std::shared_ptr<oio::kinetic::rpc::Request> GetNext::MakeRequest() noexcept {
    assert(nullptr != req_.get());
    return req_;
}

void GetNext::ManageReply(oio::kinetic::rpc::Request &rep) noexcept {
    status_ =
            rep.cmd.status().code() == proto::Command_Status_StatusCode_SUCCESS;
}