/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <glog/logging.h>
#include <libmill.h>
#include <utils/utils.h>
#include <utils/net.h>
#include <oio/http/imperative/blob.h>

using oio::http::imperative::UploadBuilder;
using oio::api::blob::Upload;

static void _load_env (std::string &dst, const char *key) {
    const char *v = ::getenv(key);
    if (v)
        dst.assign(v);
    else {
        LOG(FATAL) << "Missing environment key " << key;
    }
}

int main (int argc, char **argv) {
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = true;

    if (argc != 2) {
        LOG(FATAL) << "Usage: " << argv[0] << " HOST";
        return 1;
    }

    const char *hexa = "0123456789ABCDEF";
    std::string ns, account, user, host, chunk_id;

    _load_env(ns, "OIO_NS");
    _load_env(account, "OIO_ACCOUNT");
    _load_env(user, "OIO_USER");
    host.assign(argv[1]);
    append_string_random(chunk_id, 64, hexa);

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
    auto rc_prepare = ul->Prepare();
    if (rc_prepare == Upload::Status::OK) {
        ul->Write("", 0); // no-op!
        builder.Field("chunk-size", std::to_string(0));
        builder.Field("chunk-hash", generate_string_random(32, hexa));
        if (ul->Commit()) {
            DLOG(INFO) << "LocalUpload succeeded";
        } else {
            LOG(ERROR) << "LocalUpload failed (commit)";
        }
    } else {
        LOG(FATAL) << "Request error (prepare): " << Upload::Status2Str(rc_prepare);
        if (ul->Abort()) {
            DLOG(INFO) << "LocalUpload aborted";
        } else {
            LOG(ERROR) << "LocalUpload abortion failed";
        }
    }

    return 0;
}