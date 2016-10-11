/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <cstdint>
#include <libmill.h>
#include "PendingExchange.h"

using oio::kinetic::client::PendingExchange;

int64_t oio::kinetic::client::rpc_default_ttl = OIO_KINETIC_RPC_DEFAULT_TTL;

PendingExchange::PendingExchange(oio::kinetic::rpc::Exchange *e) :
        exchange_(e), notification_{nullptr}, sequence_id_{0} {
    notification_ = chmake(int, 1);
    deadline_ = mill_now() + oio::kinetic::client::rpc_default_ttl;
}

PendingExchange::~PendingExchange() {
    assert(notification_ != nullptr);
    chclose(notification_);
    exchange_ = nullptr;
    notification_ = nullptr;
}

void PendingExchange::SetDeadline(int64_t dl) {
    deadline_ = dl;
}

void PendingExchange::SetSequence(int64_t s) {
    sequence_id_ = s;
    if (exchange_ != nullptr)
        exchange_->SetSequence(s);
}

void PendingExchange::Signal() {
    assert(notification_ != nullptr);
    chs(notification_, int, 0);
}

void PendingExchange::Wait() {
    assert(notification_ != nullptr);
    int rc = chr(notification_, int);
    (void) rc;
}

void PendingExchange::ManageReply(oio::kinetic::rpc::Request &rep) {
    assert(exchange_ != nullptr);
    return exchange_->ManageReply(rep);
}

void PendingExchange::ManageError(int errcode) {
    assert(exchange_ != nullptr);
    return exchange_->ManageError(errcode);
}

int PendingExchange::Write(net::Channel &chan, oio::kinetic::rpc::Context &ctx,
        int64_t dl) {
    if (exchange_ == nullptr)
        return ECANCELED;
    SetSequence(ctx.sequence_id_ ++);
    return exchange_->Write(chan, ctx, dl);
}