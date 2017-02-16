/**
 * This file is part of the test tools for the OpenIO client libraries
 * Copyright (C) 2017 OpenIO SAS
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

#include <folly/SocketAddress.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/SSLContext.h>
#include <cassert>
#include <proxygen/lib/http/HTTPConnector.h>
#include <proxygen/lib/http/HTTPMessage.h>
#include "utils/macros.h"
#include "utils/net.hpp"
#include "utils/proxygen.hpp"
#include "oio/blob/http/blob.hpp"
#include "oio/blob/http/proxygen.hpp"
#include <proxygen/lib/http/session/HTTPTransaction.h>
#include <proxygen/lib/http/session/HTTPUpstreamSession.h>

using namespace folly;
using namespace proxygen;
using oio::api::Cause;
using oio::http::sync::HTTPBuilder;
using oio::http::sync::HTTPUpload;
namespace blob = oio::api::blob;

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

  // if (argc != 2) {
  //   LOG(FATAL) << "Usage: " << argv[0] << " HOST";
  //   return 1;
  // }
  
  const char *hexa = "0123456789ABCDEF";
  std::string ns, account, user, host, chunk_id;

  if(argc > 5)
    return -1;
  _load_env(&ns, "OIO_NS");
  _load_env(&account, "OIO_ACCOUNT");
  _load_env(&user, "OIO_USER");
  //host.assign(argv[1]);
  append_string_random(&chunk_id, 64, hexa);

  HTTPBuilder builder;
  builder.Name(chunk_id);
  builder.Host("127.0.0.1:6010");
  builder.URL("//"+chunk_id);
  builder.Timeout(std::chrono::milliseconds(8000));
  builder.Field("namespace", ns);
  builder.Field("account", account);
  builder.Field("user", user);
  builder.Field("Accept-Encoding","identity");
  builder.Field("transfer-encoding","chunked");
  builder.Field("X-oio-chunk-meta-container-id", generate_string_random(64, hexa));
  builder.Field("X-oio-chunk-meta-content-id", generate_string_random(20, hexa));
  builder.Field("X-oio-chunk-meta-content-path", generate_string_random(32, hexa));
  builder.Field("X-oio-chunk-meta-content-chunk-method", generate_string_random(32, hexa));
  builder.Field("X-oio-chunk-meta-content-storage-policy", "SINGLE");
  builder.Field("X-oio-chunk-meta-content-chunk-method","plain/nb-copy=1");
  builder.Field("X-oio-chunk-meta-chunk-pos", std::to_string(0));
  builder.Field("X-oio-chunk-meta-chunk-id", chunk_id);
  builder.Field("X-oio-chunk-meta-content-version", std::to_string(123123));
  builder.Field("content-length","0");
  builder.Trailer("chunk-size");
  builder.Trailer("chunk-hash");

  std::shared_ptr<folly::EventBase> evb = std::shared_ptr<folly::EventBase>(new folly::EventBase());
  HHWheelTimer::SharedPtr timer{HHWheelTimer::newTimer(
						       evb.get(),
						       std::chrono::milliseconds(HHWheelTimer::DEFAULT_TICK_INTERVAL),
						       AsyncTimeout::InternalEnum::NORMAL,
						       std::chrono::milliseconds(8000))};
  std::unique_ptr<folly::SocketAddress> socketAddr1 =  std::unique_ptr<folly::SocketAddress>(new SocketAddress("127.0.0.1",6008,true));

  builder.EventBase(evb);
  builder.Timer(timer);
  std::thread t([&] () {
      evb.get()->loopForever();
    });

  builder.SocketAddress(std::move(socketAddr1));
  std::unique_ptr<oio::api::blob::Upload> ul1 = builder.BuildUpload();
  
  auto rc1 =  ul1->Prepare();
  if (rc1.Ok()) {
    // ul1->Write("", 0); // no-op!
    builder.Field("chunk-size", std::to_string(0));
    builder.Field("chunk-hash", generate_string_random(32, hexa));
    if (ul1->Commit().Ok()) {
      DLOG(INFO) << "LocalUpload succeeded";
    } else {
      LOG(ERROR) << "LocalUpload failed (commit)";
    }
  } else {
    LOG(FATAL) << "Request error (prepare): " << rc1.Name();
    if (ul1->Abort().Ok()) {
      DLOG(INFO) << "LocalUpload aborted";
    } else {
      LOG(ERROR) << "LocalUpload abortion failed";
    }
  }

  //  evb.get()->terminateLoopSoon();
  return 0;
}
