/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <memory>

#include <utils/utils.h>
#include <oio/kinetic/client/ClientInterface.h>
#include "Put.h"

using namespace oio::kinetic::client;
using namespace oio::kinetic::rpc;
namespace proto = ::com::seagate::kinetic::proto;

Put::Put() noexcept: req_(nullptr) {
    req_.reset(new Request);
    auto h = req_->cmd.mutable_header();
    h->set_messagetype(proto::Command_MessageType_PUT);
    auto kv = req_->cmd.mutable_body()->mutable_keyvalue();
    kv->set_synchronization(proto::Command_Synchronization_WRITEBACK);
    kv->set_algorithm(proto::Command_Algorithm_SHA1);
    kv->set_force(true);
}

Put::~Put() noexcept {
    assert(nullptr != req_.get());
}

std::shared_ptr<Request> Put::MakeRequest() noexcept {
    assert(nullptr != req_.get());
    auto kv = req_->cmd.mutable_body()->mutable_keyvalue();
    kv->set_algorithm(proto::Command_Algorithm_SHA1);
    kv->set_tag("");
    return std::shared_ptr<Request>(req_);
}

void Put::ManageReply(Request &rep) noexcept {
    assert(nullptr != req_.get());
    status_ = (rep.cmd.status().code() == proto::Command_Status::SUCCESS);
}

void Put::SetSequence(int64_t s) noexcept {
    req_->cmd.mutable_header()->set_sequence(s);
}

void Put::Key(const char *k) noexcept {
    assert(nullptr != req_.get());
    req_->cmd.mutable_body()->mutable_keyvalue()->set_key(k);
}

void Put::Key(const std::string &k) noexcept {
    assert(nullptr != req_.get());
    req_->cmd.mutable_body()->mutable_keyvalue()->set_key(k);
}

void Put::PreVersion(const char *p) noexcept {
    assert(nullptr != req_.get());
    auto kv = req_->cmd.mutable_body()->mutable_keyvalue();
    bool empty = (p != nullptr) || (*p != 0);
    kv->set_force(empty);
    if (!empty)
        kv->set_dbversion(p);
}

void Put::PostVersion(const char *p) noexcept {
    assert(nullptr != req_.get());
    auto kv = req_->cmd.mutable_body()->mutable_keyvalue();
    bool empty = (p != nullptr) || (*p != 0);
    if (!empty)
        kv->set_newversion(p);
}

void Put::Value(const std::string &v) noexcept {
    assert(nullptr != req_.get());
    req_->value.assign(v.cbegin(), v.cend());
}

void Put::Value(const std::vector<uint8_t> &v) noexcept {
    assert(nullptr != req_.get());
    req_->value.assign(v.cbegin(), v.cend());
}

void Put::Value(std::vector<uint8_t> &v) noexcept {
    assert(nullptr != req_.get());
    req_->value.swap(v);
}
