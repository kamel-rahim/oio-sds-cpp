/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <glog/logging.h>
#include "Upload.h"
#include <utils/utils.h>
#include <utils/MillSocket.h>

using oio::http::coro::UploadBuilder;
using oio::api::blob::Upload;

int main (int argc, char **argv) {
    if (argc != 2) {
        LOG(FATAL) << "Usage: " << argv[0] << " HOST";
        return 1;
    }

    const char *hexa = "0123456789ABCDEF";
    std::string host, chunk_id, content_path;
    host.assign(argv[1]);
    append_string_random(chunk_id, 64, hexa);
    append_string_random(content_path, 32, hexa);

    UploadBuilder builder;
    builder.Name(chunk_id);
    builder.Host(host);
    builder.Field("content-name", content_path);
    builder.Trailer("chunk-size");

    std::shared_ptr<MillSocket> socket(new MillSocket);
    if (!socket->connect(host)) {
        LOG(FATAL) << "Connection error";
        return 1;
    }

    auto ul = builder.Build(socket);
    auto rc = ul->Prepare();
    if (rc == Upload::Status::OK) {
        ul->Write("", 0);
        ul->Commit();
    } else {
        LOG(FATAL) << "Request error (prepare)";
        ul->Abort();
    }

    return 0;
}