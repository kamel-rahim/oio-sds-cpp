/** Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <string>
#include <sstream>
#include <fstream>
#include <iomanip>

#include <signal.h>

#include "utils/utils.h"
#include "MillDaemon.h"

class BlobClient;

class BlobService;

class BlobDaemon;

static volatile bool flag_running{true};

class BlobStorage {
  public:

  private:
	std::string basedir;
};


static void _sighandler_stop(int s UNUSED) noexcept {
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

	BlobDaemon daemon;
	for (int i = 1; i < argc; ++i) {
		if (!daemon.LoadFile(argv[i]))
			return 1;
	}
	daemon.Start(flag_running);
	daemon.Join();
	return 0;
}
