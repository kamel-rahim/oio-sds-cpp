/** Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <string>
#include <memory>

#include <glog/logging.h>
#include <signal.h>

#include "utils/utils.h"
#include "MillDaemon.h"

class LocalRepository : public BlobRepository {
  public:
    LocalRepository() {}
    virtual ~LocalRepository() override {}

    BlobRepository* Clone() override {
        return new LocalRepository;
    }

    bool Configure (const std::string &cfg) override {
        (void) cfg;
        return false;
    }

    virtual std::unique_ptr<oio::api::blob::Upload> GetUpload(
            const BlobClient &client) override {
        (void) client;
        std::unique_ptr<oio::api::blob::Upload> out(nullptr);
        return out;
    }

    virtual std::unique_ptr<oio::api::blob::Download> GetDownload(
            const BlobClient &client) override {
        (void) client;
        std::unique_ptr<oio::api::blob::Download> out(nullptr);
        return out;
    }

    virtual std::unique_ptr<oio::api::blob::Removal> GetRemoval(
            const BlobClient &client) override {
        (void) client;
        std::unique_ptr<oio::api::blob::Removal> out(nullptr);
        return out;
    }
};

static volatile bool flag_running{true};

static void _sighandler_stop(int s UNUSED) {
	flag_running = 0;
}

int main(int argc, char **argv) {
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
	mill_goprepare(1024, 16384, sizeof(uint32_t));

	std::shared_ptr<BlobRepository> repo(new LocalRepository);
	BlobDaemon daemon(repo);
	for (int i = 1; i < argc; ++i) {
		if (!daemon.LoadJsonFile(argv[i]))
			return 1;
	}
	daemon.Start(flag_running);
	daemon.Join();
	return 0;
}
