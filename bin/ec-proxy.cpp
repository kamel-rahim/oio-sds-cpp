/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <signal.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cstdlib>

#include <glog/logging.h>
#include <libmill.h>

#include <utils/utils.h>
#include <oio/api/blob.h>
#include <oio/ec/blob.h>

#include "./MillDaemon.h"
#include "./ec-proxy-headers.h"

using namespace std;

using oio::ec::blob::RemovalBuilder;
using oio::ec::blob::DownloadBuilder;
using oio::ec::blob::UploadBuilder;

static volatile bool flag_running{true};

static void _sighandler_stop(int s UNUSED) {
    flag_running = 0;
}

struct numeric_only: std::ctype<char>
{
    numeric_only(): std::ctype<char>(get_table()) {}

    static std::ctype_base::mask const* get_table()
    {
        static std::vector<std::ctype_base::mask>
            rc(std::ctype<char>::table_size,std::ctype_base::space);

        std::fill(&rc['0'], &rc[':'], std::ctype_base::digit);
        return &rc[0];
    }
};


class EcHandler : public BlobHandler {
  private:
    std::string chunk_id;
    std::set<oio::ec::blob::rawxSet> targets;

    std::map<std::string,std::string> xattrs;
    int kVal, mVal , nbChunks ;
    int64_t offset_pos ;
    int64_t chunkSize ;
    uint64_t offset, size_expected ;
  public:
    EcHandler(): chunk_id(), targets(), xattrs() {}
    ~EcHandler() {}

    std::unique_ptr<oio::api::blob::Upload> GetUpload() override {
        auto builder = UploadBuilder();
        builder.BlockSize(1024 * 1024);

        builder.OffsetPos(offset_pos) ;
        builder.M_Val (mVal) ;
        builder.K_Val (kVal) ;
        builder.NbChunks (nbChunks) ;

        for (const auto &to: targets)
            builder.Target(to);
        auto ul = builder.Build();
        for (const auto &e: xattrs)
            ul->SetXattr(e.first, e.second);
        return ul;
    }

    std::unique_ptr<oio::api::blob::Download> GetDownload() override {
    	auto builder = DownloadBuilder ();

        builder.ChunkSize(chunkSize) ;
        builder.M_Val (mVal) ;
        builder.K_Val (kVal) ;
        builder.NbChunks (nbChunks) ;
        builder.Offset (offset) ;
        builder.SizeExpected (size_expected ) ;

        for (const auto to: targets)
            builder.Target(to);
        return builder.Build();
        return NULL ;
    }

    std::unique_ptr<oio::api::blob::Removal> GetRemoval() override {
    	auto builder = RemovalBuilder ();

        for (const auto to: targets)
            builder.Target(to);
//        return builder.Build();
        return NULL ;
    }

    SoftError SetUrl(const std::string &u UNUSED) override {
        // Get the name, this is common to al the requests
//        http_parser_url url;
//        if (0 != http_parser_parse_url(u.data(), u.size(), false, &url))
//            return {400, 400, "URL parse error"};
//        if (!_http_url_has(url, UF_PATH))
//            return {400, 400, "URL has no path"};

//        // Get the chunk-id, the last part of the URL path
//        auto path = _http_url_field(url, UF_PATH, u.data(), u.size());
//        auto sep = path.rfind('/');
//        if (sep == std::string::npos || sep == path.size() - 1)
//            return {400, 400, "URL has no/empty basename"};

//        chunk_id.assign(path, sep + 1, std::string::npos);
        return {200,200,"OK"};
    }

    SoftError SetHeader(const std::string &k, const std::string &v) override {
        EcHeader header;
        header.Parse(k);
        if (header.Matched()) {
            switch (header.Get()) {
                case EcHeader::Value::ChunkDest:
                {
                	oio::ec::blob::rawxSet rawx;

                 	std::string str (k) ;
                 	for ( std::string::iterator it=str.begin(); it!=str.end(); ++it)
                 		if (!isdigit(*it)) *it = ' ';
                 	std::stringstream ss(str);
                 	ss >> rawx.chunk_number ;

                 	std::size_t pos = v.find(":");
                 	            pos = v.find(":", pos+1);  // we need second one

                 	str = v.substr (pos+1, 4);
                 	std::stringstream ss2 (str);
                 	ss2 >> rawx.chunk_port ;

                 	str = str = v.substr (pos+6);
                 	rawx.target = str ;

                    targets.insert(rawx);
                }
                    break;
                case EcHeader::Value::ChunkMethod:
                {
                	std::string all_numbers (v) ;
                	for ( std::string::iterator it=all_numbers.begin(); it!=all_numbers.end(); ++it)
                	    if (!isdigit(*it)) *it = ' ';
                	std::stringstream ss(all_numbers);
                	ss >> kVal ;
                	ss >> mVal ;
                }
                break ;
                case EcHeader::Value::Chunks:
                {
                	std::string all_numbers (v) ;
                	std::stringstream ss(all_numbers);
                	ss >> nbChunks ;
                }
                break;
                case EcHeader::Value::ChunkPos:
                {
                	std::string all_numbers (v) ;
                	std::stringstream ss(all_numbers);
                	ss >> offset_pos ;
                }
               	break ;
                case EcHeader::Value::ChunkSize:
                {
                	std::string all_numbers (v) ;
                	std::stringstream ss(all_numbers);
                	ss >> chunkSize ;
                }
               	break ;
                case EcHeader::Value::Range:
                {
                	std::string all_numbers (v) ;
                	for ( std::string::iterator it=all_numbers.begin(); it!=all_numbers.end(); ++it)
                	    if (!isdigit(*it)) *it = ' ';
                	std::stringstream ss(all_numbers);
                	ss >> offset;
                	ss >> size_expected ;
                	size_expected = size_expected + 1 - offset ;
                }
                break ;
               	default:
                    xattrs[k] = v;
            }
        }
        return {200,200,"OK"};
    }
};

class EcRepository : public BlobRepository {
  public:
    EcRepository() { }

    virtual ~EcRepository() override { }

    BlobRepository *Clone() override {
        return new EcRepository;
    }

    bool Configure(const std::string &cfg UNUSED) override {
        return true;
    }

    BlobHandler* Handler() override {
        return new EcHandler();
    }
};

int main(int argc, char **argv) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);

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