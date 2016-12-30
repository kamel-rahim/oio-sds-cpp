/**
 * This file is part of the OpenIO client libraries
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

#include <cassert>
#include <string>
#include <functional>
#include <sstream>
#include <memory>
#include <vector>
#include <iomanip>
#include <cstring>
#include <iostream>
#include <fstream>

#include "utils/macros.h"
#include "utils/net.h"
#include "utils/Http.h"
#include "oio/api/blob.h"
#include "oio/sds/sds.h"
#include "oio/content/content.h"
#include "oio/blob/rawx/blob.h"

using oio::api::OioError;
using oio::content::Content;
using oio::blob::rawx::UploadBuilder;
using oio::blob::rawx::DownloadBuilder;
using oio::blob::rawx::RawxCommand;
using oio::blob::rawx::RawxCommand;
using oio::blob::rawx::Range;

OioError oio_sds::upload(std::string filepath, bool autocreate) {
    // connect to proxy
    std::shared_ptr<net::Socket> socket(new net::MillSocket);
    assert(socket->connect("127.0.0.1:6000"));
    assert(socket->setnodelay());
    assert(socket->setquickack());

    Content bucket(oio_sds_Param.GetFile());
    bucket.SetSocket(socket);

    std::ifstream file(filepath, std::ifstream::binary);
    int ret = 0;

    uint64_t fullSize = 0;
    uint32_t metachunk = 0;

    //  prime bucket
    OioError err = bucket.Prepare(autocreate);
    ChunkInfo contentSet = bucket.GetData().GetTarget(0);
    int size = contentSet.size;
    bool bfirst = true;

    do {
        std::vector<char> buffer;

        buffer.resize(contentSet.size);

        if (file.is_open())
            file.read(buffer.data(), size);

        ret = file.gcount();

        LOG(INFO) << "read = " << ret;

        if (ret > 0) {
            if (!bfirst) {
                err = bucket.Prepare(autocreate);
                if (!err.Ok())
                    LOG(INFO) << "unable to prepare content:" << err.Encode();
            } else {
                bfirst = false;
            }

            ChunkInfo contentSet;
            contentSet.Set(bucket.GetData().GetTarget(metachunk));

            RawxCommand rawx_param;
            rawx_param.SetUrl(contentSet.GetUrl());
            rawx_param.SetRange(Range(0, ret));

            contentSet.size = ret;
            contentSet.pos = metachunk;
            // TODO(ray): set HASH here
            // ChunkInfo.hash =

            bucket.GetData().UpdateTarget(&contentSet);

            // replace rawx call by router and implement xcopies, plain & EC
            std::shared_ptr<net::Socket> rawx_socket;
            rawx_socket.reset(new net::MillSocket);

            if (rawx_socket->connect(rawx_param.Url().Host_Port())) {
                UploadBuilder builder;

                builder.set_param(rawx_param);

                std::map<std::string, std::string> &system =
                        bucket.GetData().System();

                builder.ContainerId(bucket.GetUrl().ContainerId());
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
                LOG(ERROR) << "failed to connect to rawx port: "
                           << rawx_param.Url().Port();
            }

            if (ret > 0) {
                fullSize += ret;
            }
            metachunk++;
        }
    } while (ret > 0);

    if (fullSize)
// add file HASH here
        err = bucket.Create(fullSize);
    socket->close();
    return err;
}


OioError oio_sds::download(std::string filepath) {
    // connect to proxy
    std::shared_ptr<net::Socket> socket(new net::MillSocket);
    assert(socket->connect("127.0.0.1:6000"));
    assert(socket->setnodelay());
    assert(socket->setquickack());

    Content bucket(oio_sds_Param.GetFile());
    bucket.SetSocket(socket);

    std::ofstream file(filepath, std::ifstream::binary);
    OioError err = bucket.Show();

    if (err.Ok()) {
        int maxchunk = bucket.GetData().GetTargetsSize();

        for (int i = 0; i < maxchunk; i++) {
            ChunkInfo contentSet = bucket.GetData().GetTarget(i);

            std::vector<uint8_t> buffer;
            int size = contentSet.size;
            buffer.resize(contentSet.size);

            // replace rawx call by router and implement xcopies, plain & EC

            RawxCommand rawx_param;
            rawx_param.SetUrl(contentSet.GetUrl());
            rawx_param.SetRange(Range(0, contentSet.size));

            std::shared_ptr<net::Socket> rawx_socket;
            rawx_socket.reset(new net::MillSocket);

            if (rawx_socket->connect(rawx_param.Url().Host_Port())) {
                DownloadBuilder builder;

                builder.set_param(rawx_param);

                auto dl = builder.Build(rawx_socket);
                auto rc = dl->Prepare();

                if (rc.Ok()) {
                    while (!dl->IsEof()) {
                        dl->Read(&buffer);
                    }
                }
            } else {
                LOG(ERROR) << "failed to connect to rawx port: "
                           << rawx_param.Url().Port();
            }
            if (file.is_open())
                file.write((const char *) buffer.data(), size);
        }
    }
    return err;
}
