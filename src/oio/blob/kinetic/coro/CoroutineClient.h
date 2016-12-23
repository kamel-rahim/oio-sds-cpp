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

#ifndef SRC_OIO_BLOB_KINETIC_CORO_COROUTINECLIENT_H_
#define SRC_OIO_BLOB_KINETIC_CORO_COROUTINECLIENT_H_

#include <src/kinetic.pb.h>

#include <cstdint>
#include <string>
#include <vector>
#include <queue>
#include <cassert>

#include "utils/utils.h"
#include "utils/net.h"

#include "oio/blob/kinetic/coro/RPC.h"
#include "ClientInterface.h"
#include "oio/blob/kinetic/coro/PendingExchange.h"

#define SIGNAL_AGENT_STOP 0
#define SIGNAL_AGENT_DATA 1

struct mill_chan;

namespace oio {
namespace kinetic {
namespace client {

namespace proto = ::com::seagate::kinetic::proto;

class CoroutineClient : public ClientInterface {
 private:
    std::string url_;
    std::shared_ptr<net::Socket> sock_;
    oio::kinetic::client::Context ctx;

    std::queue<std::shared_ptr<PendingExchange>> waiting_;
    std::vector<std::shared_ptr<PendingExchange>> pending_;
    struct mill_chan *to_agent_;  // <int>
    struct mill_chan *stopped_;
    bool running_;

 private:
    CoroutineClient() = delete;

    CoroutineClient(CoroutineClient &o) = delete;  // NOLINT

    CoroutineClient(const CoroutineClient &o) = delete;

    CoroutineClient(const CoroutineClient &&o) = delete;

    std::string Id() const;

    /**
     * @see ClientInterface::Start()
     */
    std::shared_ptr<Sync> RPC(oio::kinetic::client::Exchange *ex) override;

    /**
     * Manage the frame just received from the socket
     * @param req
     * @return
     */
    bool manage(oio::kinetic::client::Request *req);

    /**
     * Start the RPC right out of the queue
     * @param pe
     * @return if the communication was possible
     */
    bool start_rpc(std::shared_ptr<oio::kinetic::client::PendingExchange> pe);

    /**
     * Remove the RPC from the pending list and notify the caller the error
     * occured.
     * @param pe the RPC to be removed and signaled.
     * @param errcode  the error that occured.
     */
    void abort_rpc(std::shared_ptr<oio::kinetic::client::PendingExchange> pe,
            int errcode);

    /**
     * Call abort_rpc(ECONNRESET) on all the pending and waiting RPC
     */
    void abort_all_rpc();

    /**
     * Call abort_rpc(ETIMEDOUT) on all the RPC whose deadline has been reached
     * @param now the pivot time to compare to the deadlines
     */
    void abort_stalled_rpc(int64_t now);

    std::shared_ptr<oio::kinetic::client::PendingExchange> pop_rpc(int64_t id);

    NOINLINE void run_agent_consumer(struct mill_chan *done);

    NOINLINE void run_agent_producer(struct mill_chan *done);

    NOINLINE void run_agents();

 public:
    ~CoroutineClient();

    explicit CoroutineClient(const std::string &u);

    std::string DebugString() const;

    void Boot();
};

}  // namespace client
}  // namespace kinetic
}  // namespace oio

#endif  // SRC_OIO_BLOB_KINETIC_CORO_COROUTINECLIENT_H_
