/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_KINETIC_CLIENT_PENDINGEXCHANGE_H
#define OIO_KINETIC_CLIENT_PENDINGEXCHANGE_H

/**
 * Default TTL for RPC.
 * Expressed in the same precision than the mill_now() output(s resolution (i.e.
 * in milliseconds), it is used to compute the default deadline of new RPC.
 */
#ifndef OIO_KINETIC_RPC_DEFAULT_TTL
# define OIO_KINETIC_RPC_DEFAULT_TTL 10000
#endif

#include <cstdint>
#include <oio/kinetic/coro/rpc/Exchange.h>
#include "ClientInterface.h"

struct mill_chan;

namespace oio {
namespace kinetic {
namespace client {

extern int64_t rpc_default_ttl;

class PendingExchange : public Sync {
  public:
    PendingExchange(oio::kinetic::rpc::Exchange *e);

    ~PendingExchange();

    void SetSequence(int64_t s);

    void SetDeadline(int64_t dl);

    inline int64_t Sequence() const { return sequence_id_; }

    inline int64_t Deadline() const { return deadline_; }

    int Write(net::Channel &chan, oio::kinetic::rpc::Context &ctx, int64_t dl);

    /**
     * A reply hass been received.
     * @param rep
     */
    void ManageReply (oio::kinetic::rpc::Request &rep);

    /**
     * A Network error occured. No reply could be received, and there is no clue
     * the request has been received andd managed.
     * @param errcode the errno value
     */
    void ManageError (int errcode);

    void Signal();

    void Wait();

  private:
    oio::kinetic::rpc::Exchange *exchange_;
    struct mill_chan *notification_;
    int64_t sequence_id_;
    int64_t deadline_;
};

} // namespace client
} // namespace kinetic
} // namespace oio


#endif //OIO_KINETIC_CLIENT_PENDINGEXCHANGE_H
