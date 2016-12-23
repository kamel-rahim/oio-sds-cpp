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

#include <memory.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <gtest/gtest.h>
#include "utils/macros.h"
#include "utils/net.h"
#include "utils/Http.h"
#include "oio/directory/dir.h"
#include "oio/container/container.h"
#include "oio/content/content.h"
#include "oio/rawx/blob.h"

#include "utils/serialize_def.h"
#include "oio/rawx/command.h"
#include "oio/ec/command.h"

using user_container::container;
using user_content::content;

DEFINE_string(URL_PROXY, "127.0.0.1:6000", "URL of the oio-proxy");

static void validate(oio_err err, std::string str) {\
    if (err.status)\
        LOG(INFO) << str << err.status << ", message: " << err.message ;\
}

static void cycle(net::Socket *sptr, const char *url) {
    std::shared_ptr<net::Socket> socket(sptr);
    assert(socket->connect(url));
    assert(socket->setnodelay());
    assert(socket->setquickack());

    _file_id FileId(std::string("OPENIO"), std::string("DOVECOT"),
                    std::string("ray36"),  std::string(""),
                    std::string("Myfile"));

    directory dir(FileId);
    container Superbucket(FileId);
    content   bucket(FileId);

    Superbucket.SetSocket(socket);
    dir.SetSocket(socket);
    bucket.SetSocket(socket);

    validate(bucket.Prepare(true), "bucket.Prepare");
    std::string buffer = "This is a test";

// write to Rawx
    std::shared_ptr<net::Socket> rawx_socket;
    rawx_socket.reset(new net::MillSocket);

    rawx_cmd rawx_param;
    contentSet ContentSet = bucket.GetData().GetTarget(0);

    rawx_param = ContentSet.Rawx();
    rawx_param = _range(0, 0);  // ContentSet.Range() ;

    if (rawx_socket->connect(rawx_param.Host_Port())) {
        oio::rawx::blob::UploadBuilder builder;

        builder.set_param(rawx_param);

        std::map<std::string, std::string> &system = bucket.GetData().System();

        builder.ContainerId(bucket.GetData().ContainerId());
        builder.ContentPath(system.find("name")->second);
        builder.ContentId(system.find("id")->second);
        int64_t v;
        std::istringstream(system.find("version")->second) >> v;
        builder.ContentVersion(v);
        builder.StoragePolicy(system.find("policy")->second);
        builder.MimeType(system.find("mime-type")->second);
        builder.ChunkMethod(system.find("chunk-method")->second);
        auto ul = builder.Build(rawx_socket);
        auto rc = ul->Prepare();
        if (rc.Ok()) {
            ul->Write(buffer.data(), buffer.size());
            ul->Commit();
        } else {
            ul->Abort();
        }
        rawx_socket->close();
    } else {
        LOG(ERROR) << "Router: failed to connect to rawx port: "
                   << rawx_param.Port();
    }

    validate(bucket.Create(buffer.size()), "bucket.Create");
    validate(bucket.GetProperties(), "bucket.GetProperties");
    bucket.AddProperty("Title", "once upon a time");
    bucket.AddProperty("Prop1", "This is a property");
    validate(bucket.SetProperties(), "bucket.SetProperties");
    validate(bucket.GetProperties(), "bucket.GetProperties");

    bucket.RemoveProperty("Prop1");
    validate(bucket.DelProperties(), "bucket.DelProperties");
    // verify
    validate(bucket.GetProperties(), "bucket.GetProperties()");
    // delete all remaining properties
    bucket.RemoveProperty("Title");
    validate(bucket.DelProperties(), "bucket.DelProperties");
    // verify
    validate(bucket.GetProperties(), "bucket.GetProperties");

    bucket.ClearData();
    validate(bucket.Show(), "bucket.Show()");

    // Copy to new file & delete
    _file_id FileId2(std::string("OPENIO"), std::string("DOVECOT"),
                     std::string("ray36"),  std::string(""),
                     std::string("Myfile2"));

    validate(bucket.Copy(FileId2.URL()), "bucket.Copy");
    content   bucket2(FileId2);
    bucket2.SetSocket(socket);
    validate(bucket2.Delete(), "bucket2.Delete");
    validate(bucket.Delete(), "bucket.Delete");

    validate(dir.Create(), "dir.Create");
    validate(dir.Link(), "dir.Link");
    validate(dir.Show(), "dir.Show");

    Superbucket.AddProperty("Title", "once upon a time");
    Superbucket.AddProperty("Prop1", "This is a property");

    validate(Superbucket.Create(), "Superbucket.Create");
    validate(Superbucket.SetProperties(), "Superbucket.SetProperties");
    validate(Superbucket.GetProperties(), "Superbucket.GetProperties");

    Superbucket.RemoveProperty("Prop1");
    validate(Superbucket.DelProperties(), "Superbucket.DelProperties");
    // verify
    validate(Superbucket.GetProperties(), "Superbucket.GetProperties");
    // delete all remaining properties
    Superbucket.RemoveProperty("Title");
    validate(Superbucket.DelProperties(), "Superbucket.DelProperties");
    // verify
    validate(Superbucket.GetProperties(), "Superbucket.GetProperties");

    validate(Superbucket.Touch(), "(Superbucket.Touch");
    validate(Superbucket.Dedup(), "Superbucket.Dedup");
    validate(Superbucket.Show(), "Superbucket.Show");
    validate(Superbucket.List(), "Superbucket.List");
    validate(Superbucket.Destroy(), "Superbucket.Destroy");

    dir.AddProperties("Title", "once upon a time");
    dir.AddProperties("Prop1", "This is a property");

    validate(dir.SetProperties(), "dir.SetProperties");
    validate(dir.GetProperties(), "dir.GetProperties");
    validate(dir.DelProperties(), "dir.DelProperties");
    validate(dir.GetProperties(), "dir.GetProperties");
    validate(dir.Unlink(),        "dir.Unlink");
    validate(dir.Destroy(),       "dir.Destroy");

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
