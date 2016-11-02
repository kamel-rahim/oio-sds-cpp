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
#include <rapidjson/document.h>

#include <string>
#include <memory>

#include "utils/utils.h"
#include "oio/local/blob.h"
#include "./rawx-server-headers.h"
#include "./MillDaemon.h"

using oio::local::blob::UploadBuilder;
using oio::local::blob::DownloadBuilder;
using oio::local::blob::RemovalBuilder;

class RawxRepository;

class RawxHandler : public BlobHandler {
    friend class RawxRepository;

 private:
    std::string basedir;
    std::string filename;
    std::map<std::string, std::string> xattrs;
    unsigned int hash_depth, hash_width;

 private:
    std::string path(const std::string &f) {
        std::stringstream ss;
        ss << basedir;
        for (unsigned i = 0; i < hash_depth; ++i) {
            auto token = f.substr(i * hash_width, hash_width);
            if (token == "")
                break;
            ss << "/" << token;
        }
        ss << "/" << f;
        return ss.str();
    }

 public:
    explicit RawxHandler(const std::string &r) : basedir{r}, filename(),
                                                 xattrs(), hash_depth{0},
                                                 hash_width{0} {}

    ~RawxHandler() override {}

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

        filename.assign(path, sep + 1, std::string::npos);
        return {200, 200, "OK"};
    }

    SoftError SetHeader(const std::string &k, const std::string &v) override {
        RawxHeader header;
        header.Parse(k);
        if (header.Matched()) {
            xattrs[header.StorageName()] = v;
        } else {
        }
        return {200, 200, "OK"};
    }

    std::unique_ptr<oio::api::blob::Upload> GetUpload() override {
        UploadBuilder builder;
        builder.Path(path(filename));
        auto ul = builder.Build();
        for (const auto &e : xattrs)
            ul->SetXattr(e.first, e.second);
        return ul;
    }

    std::unique_ptr<oio::api::blob::Download> GetDownload() override {
        DownloadBuilder builder;
        builder.Path(path(filename));
        return builder.Build();
    }

    std::unique_ptr<oio::api::blob::Removal> GetRemoval() override {
        RemovalBuilder builder;
        builder.Path(path(filename));
        return builder.Build();
    }
};

class RawxRepository : public BlobRepository {
    friend class RawxHandler;

 private:
    std::string repository;
    unsigned int hash_depth, hash_width;

 public:
    RawxRepository() : repository(), hash_depth{0}, hash_width{0} {}

    ~RawxRepository() override {}

    BlobRepository *Clone() override {
        auto repo = new RawxRepository;
        repo->repository.assign(repository);
        repo->hash_depth = hash_depth;
        repo->hash_width = hash_width;
        return repo;
    }

    bool Configure(const std::string &cfg UNUSED) override {
        rapidjson::Document doc;
        if (doc.Parse<0>(cfg.c_str()).HasParseError()) {
            return false;
        }

        if (!doc.HasMember("docroot") || !doc["docroot"].IsString()) {
            LOG(ERROR) << "Missing repository.docroot (string)";
            return false;
        }
        repository.assign(doc["docroot"].GetString());
        if (doc.HasMember("hash")) {
            if (!doc["hash"].IsObject()) {
                LOG(ERROR) << "repository.hash must be an object";
                return false;
            }
            if (doc["hash"].HasMember("depth")) {
                if (!doc["hash"]["depth"].IsUint64()) {
                    LOG(ERROR) << "repository.hash.depth must be integer";
                    return false;
                }
                hash_depth = doc["hash"]["depth"].GetUint64();
            }
            if (doc["hash"].HasMember("width")) {
                if (!doc["hash"]["width"].IsUint64()) {
                    LOG(ERROR) << "repository.hash.width must be integer";
                    return false;
                }
                hash_width = doc["hash"]["width"].GetUint64();
            }
        }

        LOG(INFO) << "RAWX repository ready with"
                  << " docroot=" << repository
                  << " hash_width=" << hash_width
                  << " hash_depth=" << hash_depth;
        return true;
    }

    BlobHandler *Handler() {
        auto handler = new RawxHandler(repository);
        handler->hash_width = hash_width;
        handler->hash_depth = hash_depth;
        return handler;
    }
};

static volatile bool flag_running{true};

static void _sighandler_stop(int s UNUSED) {
    flag_running = 0;
}

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
    mill_goprepare(1024, 16384, sizeof(uint32_t));

    std::shared_ptr<BlobRepository> repo(new RawxRepository);
    BlobDaemon daemon(repo);
    for (int i = 1; i < argc; ++i) {
        if (!daemon.LoadJsonFile(argv[i]))
            return 1;
    }
    daemon.Start(&flag_running);
    daemon.Join();
    return 0;
}
