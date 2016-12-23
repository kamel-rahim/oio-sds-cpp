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

#include "PendingExchange.h"

#include <libmill.h>

using oio::kinetic::client::PendingExchange;

int64_t oio::kinetic::client::rpc_default_ttl = OIO_KINETIC_RPC_DEFAULT_TTL;

PendingExchange::PendingExchange(oio::kinetic::client::Exchange *e) :
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

void PendingExchange::ManageReply(oio::kinetic::client::Request *rep) {
    assert(exchange_ != nullptr);
    return exchange_->ManageReply(rep);
}

void PendingExchange::ManageError(int errcode) {
    assert(exchange_ != nullptr);
    return exchange_->ManageError(errcode);
}

int PendingExchange::Write(net::Channel *chan,
        oio::kinetic::client::Context *ctx, int64_t dl) {
    if (exchange_ == nullptr)
        return ECANCELED;
    SetSequence(ctx->sequence_id_ ++);
    return exchange_->Write(chan, *ctx, dl);
}
