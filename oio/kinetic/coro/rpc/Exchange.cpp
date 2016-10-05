/** Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <sys/uio.h>
#include <netinet/in.h>
#include <glog/logging.h>

#include <libmill/libmill.h>

#include "Exchange.h"

using oio::kinetic::rpc::Context;
using oio::kinetic::rpc::Exchange;
using oio::kinetic::rpc::Request;
using oio::kinetic::rpc::Frame;
namespace proto = ::com::seagate::kinetic::proto;

DEFINE_bool(dump_frames, false,
            "Dump sent/received kinetic frames");

DEFINE_bool(dump_requests, false,
            "Dump sent/received kinetic messages");

DEFINE_uint64(max_frame_size, 1024*1024,
              "Maximum frame size accepted from a Kinetic drive");

void Context::Reset() {
    cnx_id_ = mill_now();
    sequence_id_ = 1;
}

Context::Context() : cluster_version_{0}, identity_{1}, sha_salt_{"asdfasdf"} {
    Reset();
}

Exchange::Exchange() : cmd(), payload_(), status_{false} {
}

void Exchange::SetSequence(int64_t s) {
    cmd.mutable_header()->set_sequence(s);
}

void Exchange::checkStatus(const oio::kinetic::rpc::Request &rep) {
    status_ = (rep.cmd.status().code() == proto::Command_Status::SUCCESS);
}

void Exchange::ManageError(int errcode) {
    LOG(INFO) << "RPC failed (errno=" << errcode << "): " << ::strerror(errcode);
    status_ = false;
}

int Exchange::Write(net::Channel &chan, const Context &ctx, int64_t dl) {

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
    *((uint32_t *) (hdr + 1)) = ::htonl(tmp.size());
    *((uint32_t *) (hdr + 5)) = ::htonl(payload_.len);

    struct iovec iov[] = {
            BUFLEN_IOV(hdr, 9),
            BUFLEN_IOV(tmp.data(), tmp.size()),
            BUFLEN_IOV(payload_.buf, payload_.len)
    };
    bool rc = chan.send(iov, payload_.buf ? 3 : 2, dl);

    DLOG_IF(INFO, FLAGS_dump_frames) << "Frame> "
                                     << " V.size=" << payload_.len
                                     << " M.size=" << tmp.size();

    return rc ? 0 : errno;
}

int Frame::Read(net::Channel &chan, int64_t dl) {
    uint8_t hdr[9] = {0};
    uint32_t lenmsg{0}, lenval{0};

    if (!chan.read_exactly(hdr, 9, dl))
        return errno;
    if (hdr[0] != 'F')
        return EBADMSG;

    lenmsg = ::ntohl(*(uint32_t *) (hdr + 1));
    lenval = ::ntohl(*(uint32_t *) (hdr + 5));
    if (lenval > FLAGS_max_frame_size || lenmsg > FLAGS_max_frame_size)
        return E2BIG;

    msg.resize(lenmsg);
    val.resize(lenval);

    if (lenmsg > 0) {
        if (!chan.read_exactly(msg.data(), msg.size(), dl))
            return errno;
    }
    if (lenval > 0) {
        if (!chan.read_exactly(val.data(), val.size(), dl))
            return errno;
    }

    DLOG_IF(INFO, FLAGS_dump_frames) << "Frame< "
                                     << " M.size=" << msg.size()
                                     << " V.size=" << val.size();
    return 0;
}

bool Request::Parse(Frame &f) {
    value.clear();
    if (!msg.ParseFromArray(f.msg.data(), f.msg.size()))
        return false;
    if (msg.has_commandbytes()) {
        if (!cmd.ParseFromArray(msg.commandbytes().data(),
                                msg.commandbytes().size()))
            return false;
        value.swap(f.val);
    }

    DLOG_IF(INFO, FLAGS_dump_requests)
        << "Req< "
        << " V.size=" << value.size()
                << " Auth=" << msg.authtype()
                << " HMAC=" << msg.hmacauth().ShortDebugString()
        << " " << cmd.ShortDebugString();
    return true;
}

int Request::Read(net::Channel &chan, int64_t dl) {
    Frame frame;
    int rc = frame.Read(chan, dl);
    if (rc != 0)
        return rc;
    if (!Parse(frame))
        return EINVAL;
    return 0;
}