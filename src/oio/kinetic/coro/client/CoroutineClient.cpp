/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <netinet/in.h>

#include <utility>
#include <algorithm>
#include <sstream>

#include <libmill.h>

#include <utils/macros.h>
#include <utils/utils.h>
#include <utils/net.h>
#include "CoroutineClient.h"

using net::MillSocket;
using oio::kinetic::rpc::Request;
using oio::kinetic::rpc::Exchange;
using oio::kinetic::client::CoroutineClient;
using oio::kinetic::client::Sync;


CoroutineClient::CoroutineClient(const std::string &u) :
        url_{u}, sock_{nullptr}, ctx(), waiting_(), pending_(),
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

bool CoroutineClient::manage(oio::kinetic::rpc::Request &req) {

    const auto id = req.cmd.header().connectionid();
    if (id > 0)
        ctx.cnx_id_ = id;

    auto ack = req.cmd.header().acksequence();
    auto pe = pop_rpc(ack);
    if (pe.get() != nullptr) {
        pe->ManageReply(req);
        pe->Signal();
    } else {
        DLOG(INFO) << "K< MSG out of sequence [" << ack << "]";
    }

    return true;
}

coroutine void CoroutineClient::run_agent_consumer(chan done) {

    assert (sock_->fileno() < 0);
    int evt{0};
    int64_t handshake_deadline = mill_now() + 5000;

    DLOG(INFO) << "K< starting";
    //sock_->setcork();
    //sock_->setnodelay();
    sock_->setrcvbuf(1024*1024);
    sock_->setsndbuf(1024*1024);

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
        oio::kinetic::rpc::Request banner;
        int err = banner.Read(*(sock_.get()), handshake_deadline);
        if (err == 0) {
            if (banner.cmd.status().code() !=
                    proto::Command_Status_StatusCode_SUCCESS) {
                LOG(ERROR) << "K< device about to close the CNX";
                goto out;
            } else {
                manage(banner);
                break;
            }
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
            oio::kinetic::rpc::Request msg;
            int err = msg.Read(*(sock_.get()), mill_now() + 1000);
            if (err == 0) {
                if (!manage(msg)) {
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
        chclose(from_producer);
    }

    out:
    DLOG(INFO) << "K< exiting!";
    chs(done, int, SIGNAL_AGENT_STOP);
}

void CoroutineClient::abort_rpc(
        std::shared_ptr<oio::kinetic::client::PendingExchange> pe, int err) {
    const auto seq = pe->Sequence();
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
    pending_.push_back(pe);
    int errcode = pe->Write(*(sock_.get()), ctx, mill_now()+1000);
    if (errcode == 0) {
        return true;
    } else {
        abort_rpc(pe, errcode);
        return false;
    }
}

coroutine void CoroutineClient::run_agent_producer(chan done) {
    DLOG(INFO) << "K> starting";

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
    while (running_) {

        DLOG(INFO) << "Starting agents for " << DebugString();

        // New connection, new sequence ID!
        ctx.Reset();

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

std::shared_ptr<Sync> CoroutineClient::RPC(Exchange *ei) {
    // Ensure the agents are running
    Boot();

    // push the rpc down
    PendingExchange *ex = new PendingExchange(ei);
    ex->SetSequence(ctx.sequence_id_++);

    std::shared_ptr<PendingExchange> shex(ex);
    waiting_.push(shex);
    assert(2 == shex.use_count());
    chs(to_agent_, int, SIGNAL_AGENT_DATA);
    return shex;
}

void CoroutineClient::Boot() {
    if (!running_) {
        running_ = true;
        mill_go(run_agents());
    }
}