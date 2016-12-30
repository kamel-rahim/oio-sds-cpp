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

#include "utils/utils.hpp"
#include "oio/api/blob.hpp"
#include "oio/blob/kinetic/coro/blob.hpp"
#include "oio/blob/kinetic/coro/CoroutineClientFactory.h"

#include "bin/MillDaemon.h"
#include "bin/kinetic-proxy-headers.h"

using oio::kinetic::blob::RemovalBuilder;
using oio::kinetic::blob::DownloadBuilder;
using oio::kinetic::blob::UploadBuilder;
using oio::kinetic::client::ClientFactory;
using oio::kinetic::client::CoroutineClientFactory;

static volatile bool flag_running{true};

static std::shared_ptr<ClientFactory> factory(nullptr);

static void _sighandler_stop(int s UNUSED) {
    flag_running = 0;
}

class KineticHandler : public BlobHandler {
 private:
    std::string chunk_id;
    std::set<std::string> targets;
    std::map<std::string, std::string> xattrs;

 public:
    KineticHandler() : chunk_id(), targets(), xattrs() {}

    ~KineticHandler() {}

    std::unique_ptr<oio::api::blob::Upload> GetUpload() override {
        auto builder = UploadBuilder(factory);
        builder.BlockSize(1024 * 1024);
        builder.Name(chunk_id);
        for (const auto &to : targets)
            builder.Target(to);
        auto ul = builder.Build();
        for (const auto &e : xattrs)
            ul->SetXattr(e.first, e.second);
        return ul;
    }

    std::unique_ptr<oio::api::blob::Download> GetDownload() override {
        DownloadBuilder builder(factory);
        builder.Name(chunk_id);
        for (const auto to : targets)
            builder.Target(to);
        return builder.Build();
    }

    std::unique_ptr<oio::api::blob::Removal> GetRemoval() override {
        RemovalBuilder builder(factory);
        builder.Name(chunk_id);
        for (const auto to : targets)
            builder.Target(to);
        return builder.Build();
    }

    SoftError SetUrl(const std::string &u) override {
        // Get the name, this is common to al the requests
        http_parser_url url;
        if (0 != http_parser_parse_url(u.data(), u.size(), false, &url))
            return {400, 400, "URL parse error"};
        if (!_http_url_has(url, UF_PATH))
            return {400, 400, "URL has no path"};

        // Get the chunk-id, the last part of the URL path
        auto path = _http_url_field(url, UF_PATH, u.data(), u.size());
        auto sep = path.rfind('/');
        if (sep == std::string::npos || sep == path.size() - 1)
            return {400, 400, "URL has no/empty basename"};

        chunk_id.assign(path, sep + 1, std::string::npos);
        return {200, 200, "OK"};
    }

    SoftError SetHeader(const std::string &k, const std::string &v) override {
        KineticHeader header;
        header.Parse(k);
        if (header.Matched()) {
            switch (header.Get()) {
                case KineticHeader::Value::Target:
                    targets.insert(v);
                    break;
                default:
                    xattrs[k] = v;
            }
        }
        return {200, 200, "OK"};
    }
};

class KineticRepository : public BlobRepository {
 public:
    KineticRepository() {}

    ~KineticRepository() override {}

    BlobRepository *Clone() override { return new KineticRepository; }

    bool Configure(const std::string &cfg UNUSED) override { return true; }

    BlobHandler *Handler() override { return new KineticHandler(); }
};

int main(int argc, char **argv) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
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
    stdin = freopen("/dev/null", "r", stdin);
    stdout = freopen("/dev/null", "a", stdout);
    mill_goprepare(1024, 16384, sizeof(void *));

    factory.reset(new CoroutineClientFactory);
    std::shared_ptr<BlobRepository> repo(new KineticRepository);
    BlobDaemon daemon(repo);
    for (int i = 1; i < argc; ++i) {
        if (!daemon.LoadJsonFile(argv[i]))
            return 1;
    }
    daemon.Start(&flag_running);
    daemon.Join();
    return 0;
}
