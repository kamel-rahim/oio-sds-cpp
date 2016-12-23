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
