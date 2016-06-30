/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <cstdlib>

#include <signal.h>

#include <glog/logging.h>
#include <libmill.h>

#include <utils/utils.h>
#include <oio/api/blob.h>
#include <oio/kinetic/coro/blob.h>
#include <oio/kinetic/coro/client/CoroutineClientFactory.h>

#include "MillDaemon.h"

using oio::kinetic::blob::RemovalBuilder;
using oio::kinetic::blob::DownloadBuilder;
using oio::kinetic::blob::UploadBuilder;
using oio::kinetic::client::ClientFactory;
using oio::kinetic::client::CoroutineClientFactory;

static volatile bool flag_running{true};

static std::shared_ptr<ClientFactory> factory;

static void _sighandler_stop(int s UNUSED) noexcept {
    flag_running = 0;
}

class KineticRepository : public BlobRepository {
  public:
    KineticRepository() { }

    virtual ~KineticRepository() override { }

    BlobRepository *Clone() noexcept override {
        return new KineticRepository;
    }

    bool Configure(const std::string &cfg UNUSED) noexcept override {
        return true;
    }

    std::unique_ptr<oio::api::blob::Upload> GetUpload(
            const BlobClient &client) noexcept override {
        auto builder = UploadBuilder(factory);
        builder.BlockSize(512 * 1024);
        builder.Name(client.chunk_id);
        for (const auto &to: client.targets)
            builder.Target(to);
        return builder.Build();
    }

    std::unique_ptr<oio::api::blob::Download> GetDownload(
            const BlobClient &client) noexcept override {
        DownloadBuilder builder(factory);
        builder.Name(client.chunk_id);
        for (const auto t: client.targets)
            builder.Target(t);
        return builder.Build();
    }

    std::unique_ptr<oio::api::blob::Removal> GetRemoval(
            const BlobClient &client) noexcept override {
        RemovalBuilder builder(factory);
        builder.Name(client.chunk_id);
        for (const auto t: client.targets)
            builder.Target(t);
        return builder.Build();
    }
};

int main(int argc, char **argv) noexcept {
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = true;

    if (argc < 2) {
        LOG(ERROR) << "Usage: " << argv[0] << " FILE [FILE...]";
        return 1;
    }

    signal(SIGINT, _sighandler_stop);
    signal(SIGTERM, _sighandler_stop);
    signal(SIGUSR1, SIG_IGN);
    signal(SIGUSR2, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    freopen("/dev/null", "r", stdin);
    freopen("/dev/null", "a", stdout);
    mill_goprepare(1024, 16384, sizeof(void *));

    std::shared_ptr<BlobRepository> repo(new KineticRepository);
    BlobDaemon daemon(repo);
    for (int i = 1; i < argc; ++i) {
        if (!daemon.LoadJsonFile(argv[i]))
            return 1;
    }
    daemon.Start(flag_running);
    daemon.Join();
    return 0;
}
