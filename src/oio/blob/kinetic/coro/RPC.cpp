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

#include <oio/blob/kinetic/coro/RPC.h>

#include <sys/uio.h>
#include <netinet/in.h>

#include <libmill/libmill.h>

#include <algorithm>

#include "ClientInterface.h"

namespace proto = ::com::seagate::kinetic::proto;
using oio::kinetic::client::Request;
using oio::kinetic::client::Context;
using oio::kinetic::client::Frame;
using oio::kinetic::client::Exchange;

using oio::kinetic::client::Delete;
using oio::kinetic::client::Get;
using oio::kinetic::client::GetKeyRange;
using oio::kinetic::client::GetLog;
using oio::kinetic::client::GetNext;
using oio::kinetic::client::Put;


DEFINE_bool(dump_frames, false,
            "Dump sent/received kinetic frames");

DEFINE_bool(dump_requests, false,
            "Dump sent/received kinetic messages");

DEFINE_uint64(max_frame_size, 1024 * 1024,
              "Maximum frame size accepted from a Kinetic drive");


void Context::Reset() {
    cnx_id_ = mill_now();
    sequence_id_ = 1;
}

Context::Context() : cluster_version_{0}, identity_{1},
                     sha_salt_{"asdfasdf"} { Reset(); }


Exchange::Exchange() : cmd(), payload_(), status_{false} {}

void Exchange::SetSequence(int64_t s) {
    cmd.mutable_header()->set_sequence(s);
}

void Exchange::checkStatus(const Request *rep) {
    assert(rep != nullptr);
    status_ = (rep->cmd.status().code() == proto::Command_Status::SUCCESS);
}

void Exchange::ManageError(int errcode) {
    LOG(INFO) << "RPC failed (errno=" << errcode << "): "
              << ::strerror(errcode);
    status_ = false;
}

int Exchange::Write(net::Channel *chan, const Context &ctx, int64_t dl) {
    assert(chan != nullptr);
    auto h = cmd.mutable_header();
    h->set_priority(proto::Command_Priority::Command_Priority_NORMAL);
    h->set_clusterversion(ctx.cluster_version_);
    h->set_connectionid(ctx.cnx_id_);
    h->set_timeout(1000);

    // Finish the message
    ::com::seagate::kinetic::proto::Message msg;
    msg.set_commandbytes(cmd.SerializeAsString());
    auto hmac = compute_sha1_hmac(ctx.sha_salt_, msg.commandbytes());
    msg.set_authtype(proto::Message_AuthType::Message_AuthType_HMACAUTH);
    msg.mutable_hmacauth()->set_identity(ctx.identity_);
    msg.mutable_hmacauth()->set_hmac(hmac.data(), hmac.size());

    // Serialize the message
    std::vector<uint64_t> tmp(msg.ByteSize());
    msg.SerializeToArray(tmp.data(), tmp.size());

    DLOG_IF(INFO, FLAGS_dump_requests) << "Req> "
                                       << " V.size=" << payload_.len
                                       << " M=" << cmd.ShortDebugString();

    uint8_t hdr[9] = {'F', 0, 0, 0, 0, 0, 0, 0, 0};
    *(reinterpret_cast<uint32_t *>(hdr + 1)) = ::htonl(tmp.size());
    *(reinterpret_cast<uint32_t *>(hdr + 5)) = ::htonl(payload_.len);

    struct iovec iov[] = {
            BUFLEN_IOV(hdr, 9),
            BUFLEN_IOV(tmp.data(), tmp.size()),
            BUFLEN_IOV(payload_.buf, payload_.len)
    };
    bool rc = chan->send(iov, payload_.buf ? 3 : 2, dl);

    DLOG_IF(INFO, FLAGS_dump_frames) << "Frame> "
                                     << " V.size=" << payload_.len
                                     << " M.size=" << tmp.size();

    return rc ? 0 : errno;
}

int Frame::Read(net::Channel *chan, int64_t dl) {
    assert(chan != nullptr);
    uint8_t hdr[9] = {0};
    uint32_t lenmsg{0}, lenval{0};

    if (!chan->read_exactly(hdr, 9, dl))
        return errno;
    if (hdr[0] != 'F')
        return EBADMSG;

    lenmsg = ::ntohl(*reinterpret_cast<uint32_t *>(hdr + 1));
    lenval = ::ntohl(*reinterpret_cast<uint32_t *>(hdr + 5));
    if (lenval > FLAGS_max_frame_size || lenmsg > FLAGS_max_frame_size)
        return E2BIG;

    msg.resize(lenmsg);
    val.resize(lenval);

    if (lenmsg > 0) {
        if (!chan->read_exactly(msg.data(), msg.size(), dl))
            return errno;
    }
    if (lenval > 0) {
        if (!chan->read_exactly(val.data(), val.size(), dl))
            return errno;
    }

    DLOG_IF(INFO, FLAGS_dump_frames) << "Frame< "
                                     << " M.size=" << msg.size()
                                     << " V.size=" << val.size();
    return 0;
}


bool Request::Parse(Frame *f) {
    assert(f != nullptr);
    value.clear();
    if (!msg.ParseFromArray(f->msg.data(), f->msg.size()))
        return false;
    if (msg.has_commandbytes()) {
        if (!cmd.ParseFromArray(msg.commandbytes().data(),
                                msg.commandbytes().size()))
            return false;
        value.swap(f->val);
    }

    DLOG_IF(INFO, FLAGS_dump_requests)
    << "Req< "
    << " V.size=" << value.size()
    << " Auth=" << msg.authtype()
    << " HMAC=" << msg.hmacauth().ShortDebugString()
    << " " << cmd.ShortDebugString();
    return true;
}

int Request::Read(net::Channel *chan, int64_t dl) {
    Frame frame;
    int rc = frame.Read(chan, dl);
    if (rc != 0)
        return rc;
    if (!Parse(&frame))
        return EINVAL;
    return 0;
}


Delete::Delete() : Exchange() {
    auto h = cmd.mutable_header();
    h->set_messagetype(proto::Command_MessageType_DELETE);
    auto kv = cmd.mutable_body()->mutable_keyvalue();
    kv->set_synchronization(proto::Command_Synchronization_WRITEBACK);
    kv->set_algorithm(proto::Command_Algorithm_SHA1);
}

Delete::~Delete() {}

void Delete::ManageReply(Request *rep) { checkStatus(rep); }

void Delete::Key(const char *k) {
    assert(k != nullptr);
    return Key(std::string(k));
}

void Delete::Key(const std::string &k) {
    cmd.mutable_body()->mutable_keyvalue()->set_key(k);
}


Get::Get() : Exchange(), out_() {
    auto h = cmd.mutable_header();
    h->set_messagetype(proto::Command_MessageType_GET);
    auto kv = cmd.mutable_body()->mutable_keyvalue();
    kv->set_algorithm(proto::Command_Algorithm_SHA1);
}

Get::~Get() {}

void Get::ManageReply(Request *rep) {
    checkStatus(rep);
    if (status_) {
        out_.clear();
        out_.swap(rep->value);
    }
}

void Get::Key(const char *k) {
    cmd.mutable_body()->mutable_keyvalue()->set_key(k);
}

void Get::Key(const std::string &k) {
    cmd.mutable_body()->mutable_keyvalue()->set_key(k);
}


GetKeyRange::GetKeyRange() : Exchange() {
    auto h = cmd.mutable_header();
    h->set_messagetype(proto::Command_MessageType_GETKEYRANGE);
    auto r = cmd.mutable_body()->mutable_range();
    r->set_startkeyinclusive(true);
    r->set_endkeyinclusive(true);
    r->set_maxreturned(200);
}

GetKeyRange::~GetKeyRange() {}

void GetKeyRange::ManageReply(Request *rep) {
    checkStatus(rep);
    if (status_) {
        for (const auto &k : rep->cmd.body().range().keys())
            out_.push_back(k);
    }
}

void GetKeyRange::Start(const char *k) {
    cmd.mutable_body()->mutable_range()->set_startkey(k);
}

void GetKeyRange::Start(const std::string &k) {
    cmd.mutable_body()->mutable_range()->set_startkey(k);
}

void GetKeyRange::End(const char *k) {
    cmd.mutable_body()->mutable_range()->set_endkey(k);
}

void GetKeyRange::End(const std::string &k) {
    cmd.mutable_body()->mutable_range()->set_endkey(k);
}

void GetKeyRange::IncludeStart(bool v) {
    cmd.mutable_body()->mutable_range()->set_startkeyinclusive(v);
}

void GetKeyRange::IncludeEnd(bool v) {
    cmd.mutable_body()->mutable_range()->set_endkeyinclusive(v);
}

void GetKeyRange::MaxItems(int32_t v) {
    cmd.mutable_body()->mutable_range()->set_maxreturned(v);
}

void GetKeyRange::Steal(std::vector<std::string> *v) {
    assert(v != nullptr);
    v->swap(out_);
}


GetLog::GetLog() : Exchange(), cpu{0}, temp{0}, space{0}, io{0} {
    auto h = cmd.mutable_header();
    h->set_messagetype(proto::Command_MessageType_GETLOG);
    auto types = cmd.mutable_body()->mutable_getlog()->mutable_types();
    types->Add(proto::Command_GetLog_Type::Command_GetLog_Type_CAPACITIES);
    types->Add(proto::Command_GetLog_Type::Command_GetLog_Type_TEMPERATURES);
    types->Add(proto::Command_GetLog_Type::Command_GetLog_Type_UTILIZATIONS);
}

GetLog::~GetLog() {}

void GetLog::ManageReply(Request *rep) {
    checkStatus(rep);
    const auto &gl = rep->cmd.body().getlog();

    if (gl.has_capacity()) {
        const double usage = gl.capacity().portionfull();
        space = 100.0 * (1.0 - usage);
    } else {
        LOG(ERROR) << "no capacity returned";
    }

    // TODO(jfs) replace the check on size() by empty()
    // as soon as empty() is available on all target distros
    if (0 < gl.temperatures().size()) {
        double t0 = 100.0;
        for (const auto &temperature : gl.temperatures()) {
            const double min = temperature.minimum();
            const double max = temperature.maximum() - min;
            const double cur = temperature.current() - min;
            const double t = 100.0 * (1.0 - (cur / max));
            t0 = std::min(t0, t);
        }
        temp = t0;
    } else {
        LOG(ERROR) << "no temperature returned";
    }

    // TODO(jfs) replace the check on size() by empty()
    // as soon as empty() is available on all target distros
    if (0 < gl.utilizations().size()) {
        for (const auto &u : gl.utilizations()) {
            const double val = 100.0 * (1.0 - u.value());
            if (0 == u.name().compare(0, 3, "CPU"))
                cpu = val;
            if (0 == u.name().compare(0, 2, "HD"))
                io = val;
        }
    } else {
        LOG(ERROR) << "no cpu/disk utilization returned";
    }
}

double GetLog::getCpu() const { return cpu; }

double GetLog::getTemp() const { return temp; }

double GetLog::getSpace() const { return space; }

double GetLog::getIo() const { return io; }


GetNext::GetNext() : Exchange(), out_() {
    auto h = cmd.mutable_header();
    h->set_messagetype(proto::Command_MessageType_GETNEXT);
    auto kv = cmd.mutable_body()->mutable_keyvalue();
    kv->set_algorithm(proto::Command_Algorithm_SHA1);
}

GetNext::~GetNext() {}

void GetNext::Key(const char *k) {
    cmd.mutable_body()->mutable_keyvalue()->set_key(k);
}

void GetNext::Key(const std::string &k) {
    cmd.mutable_body()->mutable_keyvalue()->set_key(k);
}

void GetNext::Steal(std::string *v) {
    assert(v != nullptr);
    v->swap(out_);
}

void GetNext::ManageReply(Request *rep) { checkStatus(rep); }

Put::Put() : Exchange() {
    auto h = cmd.mutable_header();
    h->set_messagetype(proto::Command_MessageType_PUT);
    auto kv = cmd.mutable_body()->mutable_keyvalue();
    kv->set_synchronization(proto::Command_Synchronization_WRITEBACK);
    kv->set_algorithm(proto::Command_Algorithm_SHA1);
    kv->set_force(true);
}

Put::~Put() {}

void Put::ManageReply(Request *rep) { return checkStatus(rep); }

void Put::Key(const char *k) {
    cmd.mutable_body()->mutable_keyvalue()->set_key(k);
}

void Put::Key(const std::string &k) {
    cmd.mutable_body()->mutable_keyvalue()->set_key(k);
}

void Put::PreVersion(const char *p) {
    auto kv = cmd.mutable_body()->mutable_keyvalue();
    bool empty = (p != nullptr) || (*p != 0);
    kv->set_force(empty);
    if (!empty)
        kv->set_dbversion(p);
}

void Put::PostVersion(const char *p) {
    auto kv = cmd.mutable_body()->mutable_keyvalue();
    bool empty = (p != nullptr) || (*p != 0);
    if (!empty)
        kv->set_newversion(p);
}

void Put::Value(const std::string &v) {
    value_copy.assign(v.cbegin(), v.cend());
    payload_.buf = value_copy.data();
    payload_.len = value_copy.size();
    rehash();
}

void Put::Value(const std::vector<uint8_t> &v) {
    value_copy.assign(v.cbegin(), v.cend());
    payload_.buf = value_copy.data();
    payload_.len = value_copy.size();
    rehash();
}

void Put::Value(std::vector<uint8_t> *v) {
    assert(v != nullptr);
    v->swap(value_copy);
    payload_.buf = value_copy.data();
    payload_.len = value_copy.size();
    rehash();
}

void Put::Value(const Slice &v) {
    payload_.buf = v.buf;
    payload_.len = v.len;
    rehash();
}

void Put::rehash() {
    auto sha1 = compute_sha1(payload_.buf, payload_.len);
    cmd.mutable_body()->mutable_keyvalue()->set_tag(sha1.data(), sha1.size());
}

void Put::Sync(bool on) {
    cmd.mutable_body()->mutable_keyvalue()->set_synchronization(
            on ? proto::Command_Synchronization_WRITETHROUGH
               : proto::Command_Synchronization_WRITEBACK);
}
