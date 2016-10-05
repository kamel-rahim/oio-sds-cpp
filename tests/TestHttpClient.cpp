/** Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <signal.h>

#include <memory>
#include <cassert>

#include <utils/macros.h>
#include <utils/net.h>
#include <utils/Http.h>

static void post(std::shared_ptr<net::Socket> socket) {
	std::string out;
	http::Call call;
	call.Socket(socket)
			.Method("POST")
			.Selector("/v3.0/OPENIO/conscience/unlock")
			.Field("Connection", "keep-alive");
	auto rc = call.Run("{\"addr\":\"127.0.0.1:6004\",\"type\":\"kine\"}", out);
	LOG(INFO) << "HTTP rc=" << rc << " reply=" << out;
}

static void get(std::shared_ptr<net::Socket> socket) {
    std::string out;
    http::Call call;
    call.Socket(socket)
            .Method("GET")
            .Selector("/v3.0/OPENIO/conscience/list")
            .Query("type", "kine")
            .Field("Connection", "keep-alive");
    auto rc = call.Run("{}", out);
    LOG(INFO) << "HTTP rc=" << rc << " reply=" << out;
}

static void cycle (net::Socket *sptr, const char *url) {
    std::shared_ptr<net::Socket> socket(sptr);
    assert(socket->connect(url));
    assert(socket->setnodelay());
    assert(socket->setquickack());
    for (int i=0; i<3 ;++i) {
		post(socket);
		get(socket);
    }
	socket->close();
}

int main (int argc, char **argv) {
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = true;
    FLAGS_minloglevel = google::INFO;
    FLAGS_colorlogtostderr = true;

	assert(argc > 1);
    cycle(new net::MillSocket, argv[1]);
    cycle(new net::RegularSocket, argv[1]);
    return 0;
}