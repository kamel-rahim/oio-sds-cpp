/** Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <memory>
#include <cassert>
#include <signal.h>

#include <glog/logging.h>
#include <utils/net.h>
#include <utils/Http.h>

static void round (std::shared_ptr<net::Socket> socket) {
    std::string out;
    http::Call call;
    call.Socket(socket)
            .Method("GET")
            .Selector("/v3.0/OPENIO/conscience/list")
            .Query("type", "echo")
            .Field("Connection", "keep-alive");
    auto rc = call.Run("{}", out);
    LOG(INFO) << "HTTP rc=" << rc << " reply=" << out;
}

static void cycle (net::Socket *sptr, const char *url) {
    std::shared_ptr<net::Socket> socket(sptr);
    assert(socket->connect(url));
    socket->setnodelay();
    socket->setquickack();
    for (int i=0; i<3 ;++i) {
        round(socket);
    }
}

int main (int argc, char **argv) {
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = true;
    FLAGS_minloglevel = google::INFO;
    FLAGS_colorlogtostderr = true;

    signal(SIGUSR1, SIG_IGN);
    signal(SIGUSR2, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    assert(argc > 1);

    LOG(INFO) << "Hello!";
    cycle(new net::MillSocket, argv[1]);
    cycle(new net::RegularSocket, argv[1]);
    LOG(INFO) << "Bye!";

    return 0;
}