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
#include <utils/MillSocket.h>
#include <kinetic.pb.h>
#include <oio/kinetic/rpc/Exchange.h>
#include <oio/kinetic/rpc/Request.h>
#include <oio/kinetic/client/ClientInterface.h>
#include <oio/kinetic/client/PendingExchange.h>

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
    MillSocket sock_;
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

    std::string Id () const noexcept;

    std::shared_ptr<Sync> Start(oio::kinetic::rpc::Exchange *ex) noexcept;

    // returns errno
    int recv(Frame &frame, int64_t dl) noexcept;

    bool manage(Frame &frame) noexcept;

    bool forward(Frame &frame) noexcept;

    // Packs req in frame, and return errno
    int pack(std::shared_ptr<oio::kinetic::rpc::Request> &req, Frame &frame) noexcept;

    NOINLINE void run_agent_consumer(struct mill_chan *done) noexcept;

    NOINLINE void run_agent_producer(struct mill_chan *done) noexcept;

    NOINLINE void run_agents() noexcept;

  public:
    ~CoroutineClient() noexcept;

    CoroutineClient(const std::string &u) noexcept;

    std::string debug_string() const noexcept;
};

} // namespace client
} // namespace kinetic
} // namespace oio

#endif //OIO_KINETIC_CLIENT_COROUTINECLIENT_H
