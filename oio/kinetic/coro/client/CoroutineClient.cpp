/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <utility>
#include <algorithm>
#include <sstream>
#include <netinet/in.h>

#include <glog/logging.h>
#include <libmill.h>

#include "utils/utils.h"
#include "utils/net.h"
#include "CoroutineClient.h"

using net::MillSocket;
using oio::kinetic::rpc::Request;
using oio::kinetic::rpc::Exchange;
using oio::kinetic::client::CoroutineClient;
using oio::kinetic::client::Sync;

CoroutineClient::CoroutineClient(const std::string &u) :
        url_{u}, sock_{nullptr}, cnxid_{0}, seqid_{2},
        waiting_(), pending_(),
        to_agent_{nullptr}, stopped_{nullptr}, running_{false} {
    to_agent_ = chmake(int, 64);
    stopped_ = chmake(int, 2);
    sock_.reset(new MillSocket());
}

CoroutineClient::~CoroutineClient() {
    if (running_) {
        running_ = false;
        chs(to_agent_, int, SIGNAL_AGENT_STOP);
        (void) chr(stopped_, int);
    }
    chclose(stopped_);
    chclose(to_agent_);
}

std::string CoroutineClient::Id() const {
    return url_;
}

std::string CoroutineClient::DebugString() const {
    std::stringstream ss;
    ss << "CoroKC{url:"<< url_ << ",sock:" << sock_->Debug() << '}';
    return ss.str();
}

int CoroutineClient::read_frame(Frame &frame, int64_t dl) {

    constexpr uint32_t max = 1024 * 1024;
    uint8_t hdr[9];

    if (!sock_->read_exactly(hdr, 9, dl))
        return errno;
    if (hdr[0] != 'F')
        return EBADMSG;

    uint32_t lenmsg = ::ntohl(*(uint32_t *) (hdr + 1));
    uint32_t lenval = ::ntohl(*(uint32_t *) (hdr + 5));
    if (lenval > max || lenmsg > max)
        return E2BIG;

    frame.msg.resize(lenmsg);
    frame.val.resize(lenval);

    if (!sock_->read_exactly(frame.msg.data(), frame.msg.size(), dl))
        return errno;
    if (!sock_->read_exactly(frame.val.data(), frame.val.size(), dl))
        return errno;

    return 0;
}

std::shared_ptr<oio::kinetic::client::PendingExchange>
CoroutineClient::pop_rpc(int64_t seqid) {
    auto cb = [seqid](const std::shared_ptr<PendingExchange> &ex) -> bool {
        return seqid == ex->Sequence();
    };
    auto it = std::find_if(pending_.begin(), pending_.end(), cb);
    if (it == pending_.end()) {
        return std::shared_ptr<oio::kinetic::client::PendingExchange>(nullptr);
    }

    auto pe = *it;
    pending_.erase(it);
    return pe;
}

bool CoroutineClient::manage(Frame &frame) {
    Request req;

    req.msg.ParseFromArray(frame.msg.data(), frame.msg.size());
    if (req.msg.has_commandbytes()) {
        req.cmd.ParseFromArray(req.msg.commandbytes().data(),
                               req.msg.commandbytes().size());
        req.value.swap(frame.val);
        DLOG(INFO) << "K< " << DebugString()
                   << " " << req.cmd.ShortDebugString()
                   << " V.size=" << req.value.size();

        const auto id = req.cmd.header().connectionid();
        if (id > 0)
            cnxid_ = id;

        const auto seqid = req.cmd.header().acksequence();
        auto pe = pop_rpc(seqid);
        if (pe.get() != nullptr) {
            pe->ManageReply(req);
            pe->Signal();
        }
    }

    return true;
}

int CoroutineClient::forward(Frame &frame, int64_t dl) {
    uint8_t hdr[9] = {'F', 0, 0, 0, 0, 0, 0, 0, 0};
    *((uint32_t *) (hdr + 1)) = ::htonl(frame.msg.size());
    *((uint32_t *) (hdr + 5)) = ::htonl(frame.val.size());
    struct iovec iov[] = {
            BUFLEN_IOV(hdr, 9),
            BUFLEN_IOV(frame.msg.data(), frame.msg.size()),
            BUFLEN_IOV(frame.val.data(), frame.val.size())
    };
    return sock_->send(iov, 3, dl) ? 0 : errno;
}

int CoroutineClient::pack(std::shared_ptr<Request> &req,
        Frame &frame) {
    assert (req != nullptr);

    // Finish the command
    auto h = req->cmd.mutable_header();
    h->set_priority(proto::Command_Priority::Command_Priority_NORMAL);
    h->set_clusterversion(0);
    h->set_connectionid(cnxid_);
    h->set_timeout(1000);

    // Finish the message
    req->msg.set_commandbytes(req->cmd.SerializeAsString());
    auto hmac = compute_sha1_hmac("asdfasdf", req->msg.commandbytes());
    req->msg.set_authtype(proto::Message_AuthType::Message_AuthType_HMACAUTH);
    req->msg.mutable_hmacauth()->set_identity(1);
    req->msg.mutable_hmacauth()->set_hmac(hmac.data(), hmac.size());

    DLOG(INFO) << "K> " << DebugString()
               << " " << req->cmd.ShortDebugString()
               << " V.size=" << req->value.size();

    // Serialize the message
    frame.msg.clear();
    frame.msg.resize(req->msg.ByteSize());
    req->msg.SerializeToArray(frame.msg.data(), frame.msg.size());
    frame.val.clear();
    frame.val.swap(req->value);
    return 0;
}

coroutine void CoroutineClient::run_agent_consumer(chan done) {

    assert (sock_->fileno() < 0);
    int evt{0};
    int64_t handshake_deadline = mill_now() + 5000;

    // Wait for an established connection
    if (0 > (sock_->connect(url_.c_str()))) {
        DLOG(ERROR) << "K< connection error (url)";
        goto out;
    }
    evt = sock_->PollOut(handshake_deadline);
    if (evt & MILLSOCKET_ERROR) {
        DLOG(ERROR) << "K< connection error (network)";
        goto out;
    }
    if (!(evt & MILLSOCKET_EVENT)) { // timeout
        DLOG(ERROR) << "K< connection timeout";
        goto out;
    }

    // wait for a banner
    while (running_) {
        Frame banner;
        int err = read_frame(banner, handshake_deadline);
        if (err == 0) {
            manage(banner);
            break;
        }
        if (err != EAGAIN)
            goto out;
    }

    if (running_) {
        // Ok, start the producer coroutine
        chan from_producer = chmake(int, 0);
        mill_go(run_agent_producer(from_producer));

        // consume frames from the device
        while (running_) {
            Frame frame;
            int err = read_frame(frame, mill_now() + 1000);
            if (err == 0) {
                if (!manage(frame)) {
                    DLOG(INFO) << "K< Frame management error";
                    break;
                }
            } else if (err != EAGAIN) {
                DLOG(ERROR) << "K< Frame reading error: (" << errno << ") "
                            << ::strerror(errno);
                break;
            }
        }

        DLOG(INFO) << "K< waiting for the producer";
        ::shutdown(sock_->fileno(), SHUT_RDWR);
        (void) chr(from_producer, int);
    }

    out:
    DLOG(INFO) << "K< exiting!";
    chs(done, int, SIGNAL_AGENT_STOP);
}

void CoroutineClient::abort_rpc(
        std::shared_ptr<oio::kinetic::client::PendingExchange> pe, int err) {
    const auto seq = pe->SeqId();
    pe->ManageError(err);
    pe->Signal();
    pop_rpc(seq);
}

void CoroutineClient::abort_all_rpc() {

    LOG(INFO) << "Aborting waiting & pending RPC";
    while (!waiting_.empty()) {
        auto pe = waiting_.front();
        waiting_.pop();
        pending_.push_back(pe);
    }

    decltype(pending_) tmp;
    tmp.swap(pending_);
    for (auto pe: tmp)
        pe->ManageError(ECONNRESET);
    for (auto pe: tmp)
        pe->Signal();
    tmp.clear();
}

void CoroutineClient::abort_stalled_rpc(int64_t now) {
    for (auto pe: pending_) {
        if (now > pe->Deadline()) {
            abort_rpc(pe, ETIMEDOUT);
        }
    }
}

bool CoroutineClient::start_rpc(std::shared_ptr<PendingExchange> pe) {

    Frame frame;
    auto spe = pe->MakeRequest();
    pack(spe, frame);

    // the RPC must be registered before any I/O op, because those ops might
    // cause other coroutines to be yield, and the reply be managed before the
    // return of the forward().
    pending_.push_back(pe);

    int errcode = forward(frame, mill_now() + 1000);
    if (errcode == 0) {
        return true;
    } else {
        abort_rpc(pe, errcode);
        return false;
    }
}

coroutine void CoroutineClient::run_agent_producer(chan done) {
    int64_t now = mill_now(), next_check = now + 1000;
    while (running_) {
        mill_choose {
                mill_in(to_agent_, int, sig):
                    if (SIGNAL_AGENT_STOP == sig) {
                        DLOG(INFO) << "K> Explicit shutdown requested";
                        goto out;
                    } else {
                        if (!waiting_.empty()) {
                            auto pe = waiting_.front();
                            waiting_.pop();
                            if (!start_rpc(pe)) {
                                DLOG(ERROR) << "K> Failed to send RPC";
                                goto out;
                            }
                        }
                    }
                mill_deadline(mill_now() + 1000):
            mill_end
        }

        now = mill_now();
        // check for stalled pending jobs
        if (now > next_check) {
            next_check = now + 1000;
            abort_stalled_rpc(now);
        }
    }
out:
    // shut the reading so that the consumer ends (the producer is already
    // being stopped)
    // TODO make shutdown available as a socket method
    ::shutdown(sock_->fileno(), SHUT_RD);

    DLOG(INFO) << "K> exiting!";
    chs(done, int, SIGNAL_AGENT_STOP);
}

coroutine void CoroutineClient::run_agents() {
    DLOG(INFO) << "Starting agents for " << DebugString();
    while (running_) {

        // New connection, new sequence ID
        seqid_ = 0;

        chan from_consumer = chmake(int, 0);
        mill_go(run_agent_consumer(from_consumer));
        (void) chr(from_consumer, int);
        sock_->close();
        chclose(from_consumer);

        // At this point, both producer/consumer coroutines have been stopped.
        // Abort all the pending operations.
        abort_all_rpc();

        if (running_)
            msleep(mill_now() + 500);
    }
    DLOG(INFO) << "Exited agents for " << DebugString();
    chs(stopped_, int, SIGNAL_AGENT_STOP);
}

std::shared_ptr<Sync> CoroutineClient::Start(Exchange *ei) {
    // Ensure the agents are running
    if (!running_) {
        running_ = true;
        mill_go(run_agents());
    }

    // push the rpc down
    PendingExchange *ex = new PendingExchange(ei);
    ex->SetSequence(seqid_++);

    std::shared_ptr<PendingExchange> shex(ex);
    waiting_.push(shex);
    assert(2 == shex.use_count());
    chs(to_agent_, int, SIGNAL_AGENT_DATA);
    return shex;
}