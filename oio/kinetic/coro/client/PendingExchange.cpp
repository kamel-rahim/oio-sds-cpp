/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <cstdint>
#include <libmill.h>
#include "PendingExchange.h"

using oio::kinetic::client::PendingExchange;

PendingExchange::PendingExchange(oio::kinetic::rpc::Exchange *e) noexcept:
        exchange_(e), notification_{nullptr} {
    notification_ = chmake(int, 1);
}

PendingExchange::~PendingExchange() noexcept {
    assert(notification_ != nullptr);
    chclose(notification_);
}

void PendingExchange::SetSequence(int64_t s) noexcept {
    assert(exchange_ != nullptr);
    seqid_ = s;
    exchange_->SetSequence(s);
}

int64_t PendingExchange::Sequence() const noexcept {
    return seqid_;
}

void PendingExchange::Signal() noexcept {
    assert(notification_ != nullptr);
    chs(notification_, int, 0);
}

void PendingExchange::Wait() noexcept {
    assert(notification_ != nullptr);
    int rc = chr(notification_, int);
    (void) rc;
}

void
PendingExchange::ManageReply(oio::kinetic::rpc::Request &rep) noexcept {
    assert(exchange_ != nullptr);
    return exchange_->ManageReply(rep);
}

std::shared_ptr<oio::kinetic::rpc::Request>
PendingExchange::MakeRequest() noexcept {
    assert(exchange_ != nullptr);
    return exchange_->MakeRequest();

}
