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

#ifndef SRC_OIO_BLOB_KINETIC_CORO_RPC_H_
#define SRC_OIO_BLOB_KINETIC_CORO_RPC_H_

#include <src/kinetic.pb.h>

#include <cstdint>
#include <string>
#include <memory>
#include <vector>

#include "utils/utils.hpp"
#include "utils/net.hpp"

namespace oio {
namespace kinetic {
namespace client {

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

    virtual void ManageReply(oio::kinetic::client::Request *rep) = 0;

    void ManageError(int errcode);

 protected:
    void checkStatus(const oio::kinetic::client::Request *rep);

 protected:
    ::com::seagate::kinetic::proto::Command cmd;
    Slice payload_;
    bool status_;
};


class Delete : public oio::kinetic::client::Exchange {
 public:
    Delete();

    FORBID_MOVE_CTOR(Delete);
    FORBID_COPY_CTOR(Delete);

    ~Delete();

    void ManageReply(oio::kinetic::client::Request *rep) override;

    void Key(const char *k);

    void Key(const std::string &k);
};

class Get : public oio::kinetic::client::Exchange {
 public:
    Get();

    FORBID_MOVE_CTOR(Get);
    FORBID_COPY_CTOR(Get);

    virtual ~Get();

    void Key(const char *k);

    void Key(const std::string &k);

    void Steal(std::vector<uint8_t> &v) { v.swap(out_); }

    void ManageReply(oio::kinetic::client::Request *rep) override;

 private:
    std::vector<uint8_t> out_;
};

class GetKeyRange : public oio::kinetic::client::Exchange {
 public:
    GetKeyRange();

    FORBID_MOVE_CTOR(GetKeyRange);
    FORBID_COPY_CTOR(GetKeyRange);

    ~GetKeyRange();

    void Start(const char *k);

    void Start(const std::string &k);

    void End(const char *k);

    void End(const std::string &k);

    void IncludeStart(bool v);

    void IncludeEnd(bool v);

    void MaxItems(int32_t v);

    void Steal(std::vector<std::string> *v);

    void ManageReply(oio::kinetic::client::Request *rep) override;

 private:
    std::vector<std::string> out_;
};

class GetLog : public oio::kinetic::client::Exchange {
 public:
    GetLog();

    FORBID_MOVE_CTOR(GetLog);
    FORBID_COPY_CTOR(GetLog);

    virtual ~GetLog();

    void ManageReply(oio::kinetic::client::Request *rep) override;

    double getCpu() const;

    double getTemp() const;

    double getSpace() const;

    double getIo() const;

 private:
    double cpu, temp, space, io;
};

class GetNext : public oio::kinetic::client::Exchange {
 public:
    GetNext();
    FORBID_MOVE_CTOR(GetNext);
    FORBID_COPY_CTOR(GetNext);

    ~GetNext();

    void Key(const char *k);

    void Key(const std::string &k);

    void Steal(std::string *v);

    void ManageReply(oio::kinetic::client::Request *rep) override;

 private:
    std::string out_;
};

class Put : public oio::kinetic::client::Exchange {
 public:
    Put();

    FORBID_MOVE_CTOR(Put);
    FORBID_COPY_CTOR(Put);

    ~Put();

    void Key(const char *k);

    void Key(const std::string &k);

    void PreVersion(const char *p);

    void PostVersion(const char *p);

    /**
     * Zero copy
     * @param v
     */
    void Value(const Slice &v);

    /**
     * Copy the string
     * @param v
     */
    void Value(const std::string &v);

    /**
     * Copy the vector
     * @param v
     */
    void Value(const std::vector<uint8_t> &v);

    /**
     * Swap the vector's content
     * @param v
     */
    void Value(std::vector<uint8_t> *v);

    void ManageReply(oio::kinetic::client::Request *rep) override;

    /**
     *
     * @param on true for WriteThrough, false for WriteBack
     */
    void Sync(bool on);

 private:
    void rehash();

 private:
    std::vector<uint8_t> value_copy;
};

}  // namespace client
}  // namespace kinetic
}  // namespace oio


#endif  // SRC_OIO_BLOB_KINETIC_CORO_RPC_H_
