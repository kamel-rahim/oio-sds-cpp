/**
 * Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/
 */

#ifndef SRC_OIO_KINETIC_CORO_RPC_EXCHANGE_H_
#define SRC_OIO_KINETIC_CORO_RPC_EXCHANGE_H_

#include <src/kinetic.pb.h>

#include <cstdint>
#include <memory>
#include <vector>

#include "utils/utils.h"
#include "utils/net.h"

namespace oio {
namespace kinetic {
namespace rpc {

/**
 * State information of the connection.
 */
struct Context {
    int64_t cnx_id_;
    int64_t sequence_id_;

    int64_t cluster_version_;
    int64_t identity_;
    const char *sha_salt_;

    Context();

    ~Context() {}

    void Reset();
};

/**
 * A bunch of bytes.
 * Onnly used when writing to a Channel.
 */
struct Slice {
    void *buf;
    size_t len;

    Slice() : buf{nullptr}, len{0} {}

    ~Slice() {}

    void Reset() {
        buf = nullptr;
        len = 0;
    }
};

/**
 * Raw bytes received.
 * Only used when reading from the channel.
 */
struct Frame {
    std::vector<uint8_t> msg;
    std::vector<uint8_t> val;

    Frame() : msg(), val() {}

    ~Frame() {}

    int Read(net::Channel *chan, int64_t dl);
};

/**
 * Unpacked form of a received frame.
 * Only used when reading from the channel.
 */
struct Request {
    ::com::seagate::kinetic::proto::Command cmd;
    ::com::seagate::kinetic::proto::Message msg;
    std::vector<uint8_t> value;

    ~Request() {}

    Request() : cmd(), msg(), value() {}

    Request(Request &o) = delete;  // NOLINT

    Request(const Request &o) = delete;

    Request(Request &&o) = delete;

    bool Parse(Frame *f);

    int Read(net::Channel *chan, int64_t dl);
};


/**
 * Represents any RPC to a kinetic drive
 */
class Exchange {
 public:
    Exchange();
    FORBID_COPY_CTOR(Exchange);

    FORBID_MOVE_CTOR(Exchange);

    virtual ~Exchange() {}

    int Write(net::Channel *chan, const Context &ctx, int64_t dl);

    void SetSequence(int64_t s);

    bool Ok() const {
        return status_;
    }

    virtual void ManageReply(oio::kinetic::rpc::Request *rep) = 0;

    void ManageError(int errcode);

 protected:
    void checkStatus(const oio::kinetic::rpc::Request *rep);

 protected:
    ::com::seagate::kinetic::proto::Command cmd;
    Slice payload_;
    bool status_;
};

}  // namespace rpc
}  // namespace kinetic
}  // namespace oio

#endif  // SRC_OIO_KINETIC_CORO_RPC_EXCHANGE_H_
