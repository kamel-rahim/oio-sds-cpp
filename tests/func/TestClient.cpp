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

#include <string>
#include <signal.h>
#include <gtest/gtest.h>

#include <memory>
#include <cassert>

#include "utils/macros.h"
#include "utils/net.h"
#include "utils/Http.h"

#include "oio/directory/dir.h"
#include "oio/container/container.h"
using user_container::container;

DEFINE_string(URL_PROXY, "127.0.0.1:6000", "URL of the oio-proxy");

#define CONTAINER_NAME "ray36"

static void cycle(net::Socket *sptr, const char *url) {
    std::shared_ptr<net::Socket> socket(sptr);
    assert(socket->connect(url));
    assert(socket->setnodelay());
    assert(socket->setquickack());

    oio_err err ;

    container bucket (std::string ("DOVECOT"), std::string(CONTAINER_NAME), std::string("mail") )  ;
    directory dir (std::string ("DOVECOT"), std::string(CONTAINER_NAME), std::string("mail") )  ;

    bucket.SetSocket (socket) ;
    dir.SetSocket (socket) ;

// test
    err = dir.Create() ;
    if (err.status)
    	LOG(INFO) << "dir.Create status: " << err.status << ", message: " << err.message ;

    err = dir.Link() ;
    if (err.status)
    	LOG(INFO) << "dir.Link status: " << err.status << ", message: " << err.message ;

    err = dir.Show() ;
    if (err.status)
    	LOG(INFO) << "dir.Show status: " << err.status << ", message: " << err.message ;

    bucket.AddProperties ("Title", "once upon a time");
    bucket.AddProperties ("Prop1", "This is a property");

    err = bucket.Create() ;
       if (err.status)
       	LOG(INFO) << "bucket.Create status: " << err.status << ", message: " << err.message ;

    err = bucket.SetProperties() ;
    if (err.status)
     	LOG(INFO) << "bucket.SetProperties status: " << err.status << ", message: " << err.message ;

    err = bucket.GetProperties() ;
    if (err.status)
    	LOG(INFO) << "bucket.GetProperties status: " << err.status << ", message: " << err.message ;

    err = bucket.DelProperties() ;
    if (err.status)
     	LOG(INFO) << "bucket.DelProperties status: " << err.status << ", message: " << err.message ;

    err = bucket.GetProperties() ;
    if (err.status)
    	LOG(INFO) << "bucket.GetProperties status: " << err.status << ", message: " << err.message ;


//    container bucket2 (std::string ("DOVECOT"), std::string("raymond"), std::string("") )  ;
//    bucket2.SetSocket (socket) ;

    err = bucket.Touch() ;
    if (err.status)
    	LOG(INFO) << "bucket.Touch status: " << err.status << ", message: " << err.message ;

    err = bucket.Dedup() ;
    if (err.status)
    	LOG(INFO) << "bucket.Dedup status: " << err.status << ", message: " << err.message ;

    err = bucket.Show() ;
    if (err.status)
    	LOG(INFO) << "bucket.Show status: " << err.status << ", message: " << err.message ;

    err = bucket.List() ;
    if (err.status)
    	LOG(INFO) << "bucket.Lis status: " << err.status << ", message: " << err.message ;


    err = bucket.Destroy() ;
    if (err.status)
    	LOG(INFO) << "bucket.Destroy status: " << err.status << ", message: " << err.message ;

    dir.AddProperties ("Title", "once upon a time");
    dir.AddProperties ("Prop1", "This is a property");

    err = dir.SetProperties() ;
    if (err.status)
    	LOG(INFO) << "dir.SetProperties status: " << err.status << ", message: " << err.message ;

    err = dir.GetProperties() ;
    if (err.status)
    	LOG(INFO) << "dir.GetProperties status: " << err.status << ", message: " << err.message ;

    err = dir.DelProperties() ;
    if (err.status)
    	LOG(INFO) << "dir.DelProperties status: " << err.status << ", message: " << err.message ;

    err = dir.GetProperties() ;
    if (err.status)
    	LOG(INFO) << "dir.GetProperties status: " << err.status << ", message: " << err.message ;


    err = dir.Unlink() ;
    if (err.status)
    	LOG(INFO) << "dir.Unlink status: " << err.status << ", message: " << err.message ;

    err = dir.Destroy() ;
    if (err.status)
    	LOG(INFO) << "dir.Destroy status: " << err.status << ", message: " << err.message ;

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
