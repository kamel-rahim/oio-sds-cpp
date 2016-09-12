/** Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include "Exchange.h"

using oio::kinetic::rpc::Exchange;
using oio::kinetic::rpc::Request;
namespace proto = ::com::seagate::kinetic::proto;

Exchange::Exchange() : req_(new Request), status_{false} {
}

void Exchange::SetSequence(int64_t s) {
    assert(nullptr != req_.get());
    req_->cmd.mutable_header()->set_sequence(s);
}

std::shared_ptr<oio::kinetic::rpc::Request> Exchange::MakeRequest() {
    assert(nullptr != req_.get());
    return req_;
}

void Exchange::checkStatus(const oio::kinetic::rpc::Request &rep) {
    assert(nullptr != req_.get());
    status_ = (rep.cmd.status().code() == proto::Command_Status::SUCCESS);
}
