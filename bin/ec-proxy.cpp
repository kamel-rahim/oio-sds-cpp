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

#include <glog/logging.h>
#include <libmill.h>
#include <liberasurecode/erasurecode.h>

#include <iostream>
#include <algorithm>

#include "utils/utils.h"
#include "oio/api/blob.h"
#include "oio/blob/router/blob.h"

#include "bin/MillDaemon.h"
#include "bin/ec-proxy-headers.h"

using oio::blob::router::RemovalBuilder;
using oio::blob::router::DownloadBuilder;
using oio::blob::router::UploadBuilder;
using oio::blob::ec::EcCommand;
using oio::blob::rawx::RawxCommand;
using oio::blob::rawx::Range;
using oio::blob::rawx::RawxUrl;
using oio::blob::rawx::RawxUrlSet;

static volatile bool flag_running{true};

#define test_rawx 1

/*
 * List of possible values for the "algo" parameter of "ec" data security:
 */

static std::map<std::string, int> TheEncodingMethods = {
        {"jerasure_rs_vand",       EC_BACKEND_JERASURE_RS_VAND},
        {"jerasure_rs_cauchy",     EC_BACKEND_JERASURE_RS_CAUCHY},
        {"flat_xor_hd",            EC_BACKEND_FLAT_XOR_HD},
        {"isa_l_rs_vand",          EC_BACKEND_ISA_L_RS_VAND},
        {"shss",                   EC_BACKEND_SHSS},
        {"liberasurecode_rs_vand", EC_BACKEND_LIBERASURECODE_RS_VAND}
};

static void _sighandler_stop(int s UNUSED) {
    flag_running = 0;
}

static void transform_nondigit_to_space(std::string &str) {  // NOLINT
    for (auto &c : str) {
        if (!isdigit(c))
            c = ' ';
    }
}

struct numeric_only : std::ctype<char> {
    numeric_only() : std::ctype<char>(get_table()) {}

    static std::ctype_base::mask const *get_table() {
        static std::vector<std::ctype_base::mask>
                rc(std::ctype<char>::table_size, std::ctype_base::space);

        std::fill(&rc['0'], &rc[':'], std::ctype_base::digit);
        return &rc[0];
    }
};


class RouterHandler : public BlobHandler {
 private:
    std::map<std::string, std::string> xattrs;
    EcCommand ec_param;
    RawxCommand rawx_param;

 public:
    RouterHandler() {
        ec_param.Clear();
        rawx_param.Clear();
    }

    ~RouterHandler() {}

    std::unique_ptr<oio::api::blob::Upload> GetUpload() override {
        auto builder = UploadBuilder();

        ec_param.SetChunkSize(1024 * 1024);
        rawx_param.SetChunkSize(1024 * 1024);

        if (rawx_param.Url().ChunkId().size() > 1)
            builder.set_rawx_param(rawx_param);
        else
            builder.set_ec_param(ec_param);

        auto ul = builder.Build();
        for (const auto &e : xattrs)
            ul->SetXattr(e.first, e.second);
        return ul;
    }

    std::unique_ptr<oio::api::blob::Download> GetDownload() override {
        auto builder = DownloadBuilder();

        if (rawx_param.Url().ChunkId().size() > 1)
            builder.set_rawx_param(rawx_param);
        else
            builder.set_ec_param(ec_param);

        return builder.Build();
    }

    std::unique_ptr<oio::api::blob::Removal> GetRemoval() override {
        auto builder = RemovalBuilder();
        return NULL;
    }

    SoftError SetUrl(const std::string &u UNUSED) override {
        // Get the name, this is common to al the requests
        http_parser_url url;
        if (0 == http_parser_parse_url(u.data(), u.size(), false, &url) &&
            _http_url_has(url, UF_PATH)) {
            // Get the chunk-id, the last part of the URL path
            auto path = _http_url_field(url, UF_PATH, u.data(), u.size());
            auto sep = path.rfind('/');
            if (sep != std::string::npos && sep != path.size() - 1)
                rawx_param.Url().ChunkId().assign(path, sep + 1,
                                                std::string::npos);
        }
        return {200, 200, "OK"};
    }

    SoftError SetHeader(const std::string &k, const std::string &v) override {
        EcHeader header;
        header.Parse(k);
        if (header.Matched()) {
            switch (header.Get()) {
                case EcHeader::Value::Host: {
                    rawx_param.SetUrl(RawxUrl(v));
                }
                    break;
                case EcHeader::Value::ChunkDest: {
                    RawxUrlSet rawx_p;

                    std::string str(k);
                    transform_nondigit_to_space(str);

                    std::stringstream ss(str);
                    ss >> rawx_p.chunk_number;

                    rawx_p.Set(RawxUrl(v));

//                    LOG(ERROR) << k << ": " << v;

#ifdef test_rawx
                    if (!rawx_p.chunk_number)
                        rawx_param.SetUrl(rawx_p);
#endif
                    ec_param.AddTarget(rawx_p);
                }
                    break;
                case EcHeader::Value::ChunkMethod: {
                    ec_param.SetEncoding(EC_BACKEND_NULL);
                    for (const auto &e : TheEncodingMethods) {
                        std::size_t pos = v.find(e.first);
                        if (pos != std::string::npos) {
                            ec_param.SetEncoding(e.second);
                            break;
                        }
                    }
                    if (ec_param.Encoding() == EC_BACKEND_NULL) {
                        LOG(ERROR) << "LIBERASURECODE: "
                                   << "Could not detect encoding method";
                    }

                    std::string all_numbers(v);
                    transform_nondigit_to_space(all_numbers);
                    std::stringstream ss(all_numbers);
                    int k, m;
                    ss >> k >> m;
                    ec_param.SetK(k);
                    ec_param.SetM(m);
                }
                    break;

                case EcHeader::Value::Chunks:
                    ec_param.SetNbChunks(::atoi(v.c_str()));
                    break;

                case EcHeader::Value::ChunkPos:
                    // Not applicable here
                    break;
                case EcHeader::Value::ChunkSize: {
                    int chunkSize = ::atoi(v.c_str());
                    ec_param.SetChunkSize(chunkSize);
                    rawx_param.SetChunkSize(chunkSize);
                }
                    break;
                case EcHeader::Value::ReqId:
                    ec_param.SetReqId(v);
                    break;
                case EcHeader::Value::Range: {
                    std::string all_numbers(v);
                    transform_nondigit_to_space(all_numbers);
                    std::stringstream ss(all_numbers);
                    auto range = ec_param.GetRange();
                    int start, size;
                    ss >> start;
                    ss >> size;
                    ec_param.GetRange().Set(Range(start, size+1-start));
                    rawx_param.SetRange(ec_param.GetRange());
                }
                    break;
                default: {
                    // remove leading string OIO_HEADER_EC_PREFIX
                    std::string str_cpy = k;
                    std::string str_leading(OIO_HEADER_EC_PREFIX);
                    int Length = str_cpy.length();
                    std::size_t pos = str_cpy.find(OIO_HEADER_EC_PREFIX);
                    if (pos == 0) {
                        Length = Length - str_leading.length();
                        str_cpy.erase(str_cpy.begin(), str_cpy.end() - Length);
                    }
                    xattrs[str_cpy] = v;
                }
            }
        }
        return {200, 200, "OK"};
    }
};

class EcRepository : public BlobRepository {
 public:
    EcRepository() {}

    ~EcRepository() override {}

    BlobRepository *Clone() override { return new EcRepository; }

    bool Configure(const std::string &cfg UNUSED) override { return true; }

    BlobHandler *Handler() override { return new RouterHandler(); }
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

    std::shared_ptr<BlobRepository> repo(new EcRepository);
    BlobDaemon daemon(repo);
    for (int i = 1; i < argc; ++i) {
        if (!daemon.LoadJsonFile(argv[i]))
            return 1;
    }
    daemon.Start(&flag_running);
    daemon.Join();
    return 0;
}
