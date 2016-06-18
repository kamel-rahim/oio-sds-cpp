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

Get::Get() noexcept: req_(), val_(), status_{false} {
    req_.reset(new Request);
    auto h = req_->cmd.mutable_header();
    h->set_messagetype(proto::Command_MessageType_GET);
    auto kv = req_->cmd.mutable_body()->mutable_keyvalue();
    kv->set_algorithm(proto::Command_Algorithm_SHA1);
}

Get::~Get() { }

void Get::SetSequence(int64_t s) noexcept {
    req_->cmd.mutable_header()->set_sequence(s);
}

std::shared_ptr<Request> Get::MakeRequest() noexcept {
    assert(nullptr != req_.get());
    return req_;
}

void Get::ManageReply(Request &rep) noexcept {
    auto code = rep.cmd.status().code();
    status_ = code == proto::Command_Status_StatusCode_SUCCESS;
    val_.clear();
    val_.swap(rep.value);
    DLOG(INFO) << val_.size();
}

void Get::Key(const char *k) noexcept {
    req_->cmd.mutable_body()->mutable_keyvalue()->set_key(k);
}

void Get::Key(const std::string &k) noexcept {
    req_->cmd.mutable_body()->mutable_keyvalue()->set_key(k);
}