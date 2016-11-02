/**
 * This file is part of the test tools for the OpenIO client libraries
 * Copyright (C) 2016 OpenIO SAS
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.
 */

/**
 * Tests the internal HTTP client works as expected when accessing an OpenIO
 * metadata proxy.
 */

#include <signal.h>
#include <gtest/gtest.h>

#include <memory>
#include <cassert>

#include "utils/macros.h"
#include "utils/net.h"
#include "utils/Http.h"

DEFINE_string(URL_PROXY, "127.0.0.1:6000", "URL of the oio-proxy");

static void post(std::shared_ptr<net::Socket> socket) {
    std::string out;
    http::Call call;
    call.Socket(socket)
            .Method("POST")
            .Selector("/v3.0/OPENIO/conscience/unlock")
            .Field("Connection", "keep-alive");
    auto rc = call.Run("{\"addr\":\"127.0.0.1:6004\",\"type\":\"kine\"}", &out);
    LOG(INFO) << "POST rc=" << rc << " reply=" << out;
}

static void get(std::shared_ptr<net::Socket> socket) {
    std::string out;
    http::Call call;
    call.Socket(socket)
            .Method("GET")
            .Selector("/v3.0/OPENIO/conscience/list")
            .Query("type", "kine")
            .Field("Connection", "keep-alive");
    auto rc = call.Run("{}", &out);
    LOG(INFO) << "GET rc=" << rc << " reply=" << out;
}

static void cycle(net::Socket *sptr, const char *url) {
    std::shared_ptr<net::Socket> socket(sptr);
    assert(socket->connect(url));
    assert(socket->setnodelay());
    assert(socket->setquickack());
    for (int i = 0; i < 3; ++i) {
        post(socket);
        get(socket);
    }
    socket->close();
}

TEST(Http, MillSocket_CyclePostGet) {
    cycle(new net::MillSocket, FLAGS_URL_PROXY.c_str());
}

TEST(Http, RegularSocket_CyclePostGet) {
    cycle(new net::RegularSocket, FLAGS_URL_PROXY.c_str());
}

int main(int argc, char **argv) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    ::testing::InitGoogleTest(&argc, argv);
    FLAGS_logtostderr = true;
    return RUN_ALL_TESTS();
}
