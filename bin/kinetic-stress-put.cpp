/**
 * This file is part of the CLI tools around the OpenIO client libraries
 * Copyright (C) 2016 OpenIO SAS
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <signal.h>

#include <libmill.h>

#include <cstdlib>
#include <memory>
#include <queue>
#include <chrono>  // NOLINT

#include "utils/macros.h"
#include "oio/blob/kinetic/coro/client/ClientInterface.h"
#include "oio/blob/kinetic/coro/client/CoroutineClientFactory.h"
#include "oio/blob/kinetic/coro/rpc/Put.h"
#include "oio/blob/kinetic/coro/blob.h"

using oio::kinetic::client::CoroutineClientFactory;
using oio::kinetic::client::ClientInterface;
using oio::kinetic::client::Sync;
using oio::kinetic::rpc::Exchange;
using oio::kinetic::rpc::Slice;
using oio::kinetic::rpc::Put;
using oio::kinetic::blob::UploadBuilder;

using Clock = std::chrono::steady_clock;
using Precision = std::chrono::milliseconds;

static volatile bool flag_runnning{true};

DEFINE_bool(write_through, false, "Sync before reply or not");
DEFINE_uint64(put_window, 64, "Number of frames in flight");
DEFINE_uint64(block_size, 1024 * 1024, "Number of bytes in each frame");
DEFINE_uint64(block_count, 1024, "Number of blacks sent");
DEFINE_string(naming, "rand", "Should the number be ");


class NameIterator {
 public:
    virtual ~NameIterator() {}

    virtual void Next(std::string *out) = 0;
};


class RandomNameIterator : public NameIterator {
 public:
    RandomNameIterator() {}

    virtual ~RandomNameIterator() {}

    void Next(std::string *out) override {
        out->clear();
        append_string_random(out, 64, "0123456789ABCDEF");
    }
};


class ForwardNameIterator : public NameIterator {
 private:
    int64_t next;
 public:
    ForwardNameIterator() : next{0} {}

    virtual ~ForwardNameIterator() {}

    void Next(std::string *out) override {
        out->clear();
        std::stringstream ss;
        ss << next++;
        out->assign(ss.str());
    }
};

class BackwardsNameIterator : public NameIterator {
 private:
    uint64_t next;
 public:
    BackwardsNameIterator() : next{0} {}

    virtual ~BackwardsNameIterator() {}

    void Next(std::string *out) override {
        out->clear();
        std::stringstream ss;
        ss << --next;
        out->assign(ss.str());
    }
};


static uint64_t push_loop(uint64_t max, const Slice &buf,
        std::shared_ptr<ClientInterface> client,
        std::shared_ptr<NameIterator> namer) {
    std::queue<std::shared_ptr<Exchange>> qops;
    std::queue<std::shared_ptr<Sync>> qsync;
    uint64_t count{0};
    std::string name;

    while (flag_runnning && count < max) {
        if (qops.size() < FLAGS_put_window) {
            namer->Next(&name);
            // Start a new upload
            auto op = new Put;
            op->PostVersion("0");
            op->Key(name);
            op->Value(buf);
            if (FLAGS_write_through)
                op->Sync(true);
            auto syn = client->RPC(op);
            // And queue it
            qops.push(std::shared_ptr<Exchange>(op));
            qsync.push(syn);

            ++count;
        } else if (!qops.empty()) {
            // Finish the latest upload
            auto op = qops.front();
            auto syn = qsync.front();
            qops.pop();
            qsync.pop();
            syn->Wait();
            if (!op->Ok()) {
                flag_runnning = false;
                LOG(ERROR) << "First put failed";
            }
        }
    }

    while (!qops.empty()) {
        auto op = qops.front();
        auto syn = qsync.front();
        qops.pop();
        qsync.pop();
        syn->Wait();
        LOG_IF(ERROR, !op->Ok()) << "tail put failed";
    }

    return count;
}

int main(int argc, char **argv) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = true;

    signal(SIGUSR1, SIG_IGN);
    signal(SIGUSR2, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    stdin = freopen("/dev/null", "r", stdin);
    stdout = freopen("/dev/null", "a", stdout);
    mill_goprepare(2 + 2 * FLAGS_put_window, 16384, sizeof(void *));

    std::vector<uint8_t> buf(FLAGS_block_size);

    Slice slice;
    slice.buf = buf.data();
    slice.len = buf.size();

    CoroutineClientFactory factory;

    const char *url_device = ::getenv("URL");
    if (url_device == nullptr) {
        LOG(ERROR) << "Missing env variable [URL]";
        return 1;
    }

    std::shared_ptr<NameIterator> naming;

    if (FLAGS_naming == "rand") {
        naming.reset(new RandomNameIterator);
    } else if (FLAGS_naming == "forward") {
        naming.reset(new ForwardNameIterator);
    } else if (FLAGS_naming == "backwards") {
        naming.reset(new BackwardsNameIterator);
    } else {
        LOG(ERROR) << "Invalid naming, please use (rand|forward|backwards)";
        return 1;
    }

    auto client = factory.Get(url_device);
    LOG(INFO) << "Client ready to " << client->Id();

    auto pre = Clock::now();
    uint64_t count = push_loop(FLAGS_block_count, slice, client, naming);
    auto post = Clock::now();
    auto spent_msec = std::chrono::duration_cast<Precision>(post - pre).count();

    const uint64_t total_bytes = count * uint64_t(buf.size());
    const uint64_t bytes_per_msec = total_bytes / spent_msec;
    const double MB_per_sec = static_cast<double>(bytes_per_msec) / 1000.0;

    LOG(INFO) << "Sent " << total_bytes << " in " << double(spent_msec) / 1000.0
              << " seconds "
              << MB_per_sec << " MB/s or " << (MB_per_sec * 8.0) << " Mb/s";

    return 0;
}
