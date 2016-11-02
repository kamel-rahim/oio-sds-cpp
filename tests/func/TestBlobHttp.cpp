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

#include <libmill.h>

#include <cassert>

#include "utils/macros.h"
#include "utils/utils.h"
#include "utils/net.h"
#include "oio/http/blob.h"

using oio::http::imperative::UploadBuilder;
namespace blob = oio::api::blob;
using blob::Cause;

static void _load_env(std::string *dst, const char *key) {
    assert(dst != nullptr);
    const char *v = ::getenv(key);
    if (v) {
        dst->assign(v);
    } else {
        LOG(FATAL) << "Missing environment key " << key;
    }
}

int main(int argc, char **argv) {
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = true;

    if (argc != 2) {
        LOG(FATAL) << "Usage: " << argv[0] << " HOST";
        return 1;
    }

    const char *hexa = "0123456789ABCDEF";
    std::string ns, account, user, host, chunk_id;

    _load_env(&ns, "OIO_NS");
    _load_env(&account, "OIO_ACCOUNT");
    _load_env(&user, "OIO_USER");
    host.assign(argv[1]);
    append_string_random(&chunk_id, 64, hexa);

    UploadBuilder builder;
    builder.Name(chunk_id);
    builder.Host(host);
    builder.Field("namespace", ns);
    builder.Field("account", account);
    builder.Field("user", user);
    builder.Field("container-id", generate_string_random(64, hexa));
    builder.Field("content-id", generate_string_random(20, hexa));
    builder.Field("content-path", generate_string_random(32, hexa));
    builder.Field("content-mime-type", "application/octet-steam");
    builder.Field("content-chunk-method", generate_string_random(32, hexa));
    builder.Field("content-storage-policy", "SINGLE");
    builder.Field("chunk-pos", std::to_string(0));
    builder.Field("chunk-id", chunk_id);
    builder.Field("content-version", std::to_string(mill_now()));
    builder.Trailer("chunk-size");
    builder.Trailer("chunk-hash");

    std::shared_ptr<net::Socket> socket(new net::MillSocket);
    if (!socket->connect(host.c_str())) {
        LOG(FATAL) << "Connection error";
        return 1;
    }

    auto ul = builder.Build(socket);
    auto rc = ul->Prepare();
    if (rc.Ok()) {
        ul->Write("", 0);  // no-op!
        builder.Field("chunk-size", std::to_string(0));
        builder.Field("chunk-hash", generate_string_random(32, hexa));
        if (ul->Commit().Ok()) {
            DLOG(INFO) << "LocalUpload succeeded";
        } else {
            LOG(ERROR) << "LocalUpload failed (commit)";
        }
    } else {
        LOG(FATAL) << "Request error (prepare): " << rc.Name();
        if (ul->Abort().Ok()) {
            DLOG(INFO) << "LocalUpload aborted";
        } else {
            LOG(ERROR) << "LocalUpload abortion failed";
        }
    }

    return 0;
}
