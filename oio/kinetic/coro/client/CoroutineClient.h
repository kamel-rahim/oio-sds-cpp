/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_KINETIC_CLIENT_COROUTINECLIENT_H
#define OIO_KINETIC_CLIENT_COROUTINECLIENT_H

#include <cstdint>
#include <string>
#include <vector>
#include <queue>
#include <cassert>
#include <utils/utils.h>
#include <utils/net.h>
#include <kinetic.pb.h>
#include <oio/kinetic/coro/rpc/Exchange.h>
#include <oio/kinetic/coro/rpc/Request.h>
#include <oio/kinetic/coro/client/ClientInterface.h>
#include <oio/kinetic/coro/client/PendingExchange.h>

#define SIGNAL_AGENT_STOP 0
#define SIGNAL_AGENT_DATA 1

struct mill_chan;

namespace oio {
namespace kinetic {
namespace client {

struct Frame {
    std::vector<uint8_t> msg;
    std::vector<uint8_t> val;

    Frame() : msg(), val() { }

    ~Frame() { }
};

namespace proto = ::com::seagate::kinetic::proto;

class CoroutineClient : public ClientInterface {

  private:
    std::string url_;
    std::shared_ptr<net::Socket> sock_;
    int64_t cnxid_;
    uint64_t seqid_;

    std::queue<std::shared_ptr<PendingExchange>> waiting_;
    std::vector<std::shared_ptr<PendingExchange>> pending_;
    struct mill_chan *to_agent_; // <int>
    struct mill_chan *stopped_;
    bool running_;

  private:
    CoroutineClient() = delete;

    CoroutineClient(CoroutineClient &o) = delete;

    CoroutineClient(const CoroutineClient &o) = delete;

    CoroutineClient(const CoroutineClient &&o) = delete;

    std::string Id () const;

    /**
     * @see ClientInterface::Start()
     */
    std::shared_ptr<Sync> Start(oio::kinetic::rpc::Exchange *ex) override;

    /**
     * Receive enough bytes from the socket and unpack them int a Framee
     * @param frame
     * @param dl the deadline for the whole operation
     * @return
     */
    int read_frame(Frame &frame, int64_t dl);

    /**
     * Manage the frame just received from the socket
     * @param frame
     * @return
     */
    bool manage(Frame &frame);

    /**
     * Send the frame over the socket and return errno
     * @param frame
     * @param dl the deadline for the whole operation
     * @return errno
     */
    int forward(Frame &frame, int64_t dl);

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
    void abort_rpc(std::shared_ptr<oio::kinetic::client::PendingExchange> pe, int errcode);

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

    /**
     * Packs req in frame, and return errno
     * @param req the request to be packed
     * @param frame the structure
     * @return the error that occured
     */
    int pack(std::shared_ptr<oio::kinetic::rpc::Request> &req, Frame &frame);

    NOINLINE void run_agent_consumer(struct mill_chan *done);

    NOINLINE void run_agent_producer(struct mill_chan *done);

    NOINLINE void run_agents();

  public:
    ~CoroutineClient();

    CoroutineClient(const std::string &u);

    std::string DebugString() const;
};

} // namespace client
} // namespace kinetic
} // namespace oio

#endif //OIO_KINETIC_CLIENT_COROUTINECLIENT_H
