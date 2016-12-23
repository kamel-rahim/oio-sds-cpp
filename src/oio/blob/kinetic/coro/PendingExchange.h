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

#ifndef SRC_OIO_BLOB_KINETIC_CORO_PENDINGEXCHANGE_H_
#define SRC_OIO_BLOB_KINETIC_CORO_PENDINGEXCHANGE_H_

#include <cstdint>

#include "oio/blob/kinetic/coro/RPC.h"
#include "ClientInterface.h"

/**
 * Default TTL for RPC.
 * Expressed in the same precision than the mill_now() output(s resolution (i.e.
 * in milliseconds), it is used to compute the default deadline of new RPC.
 */
#ifndef OIO_KINETIC_RPC_DEFAULT_TTL
# define OIO_KINETIC_RPC_DEFAULT_TTL 10000
#endif

struct mill_chan;

namespace oio {
namespace kinetic {
namespace client {

extern int64_t rpc_default_ttl;

class PendingExchange : public Sync {
 public:
    explicit PendingExchange(oio::kinetic::client::Exchange *e);

    ~PendingExchange();

    void SetSequence(int64_t s);

    void SetDeadline(int64_t dl);

    inline int64_t Sequence() const { return sequence_id_; }

    inline int64_t Deadline() const { return deadline_; }

    int Write(net::Channel *chan, oio::kinetic::client::Context *ctx,
            int64_t dl);

    /**
     * A reply hass been received.
     * @param rep
     */
    void ManageReply(oio::kinetic::client::Request *rep);

    /**
     * A Network error occured. No reply could be received, and there is no clue
     * the request has been received andd managed.
     * @param errcode the errno value
     */
    void ManageError(int errcode);

    void Signal();

    void Wait();

 private:
    oio::kinetic::client::Exchange *exchange_;
    struct mill_chan *notification_;
    int64_t sequence_id_;
    int64_t deadline_;
};

}  // namespace client
}  // namespace kinetic
}  // namespace oio


#endif  // SRC_OIO_BLOB_KINETIC_CORO_PENDINGEXCHANGE_H_
