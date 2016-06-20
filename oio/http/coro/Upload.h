/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_CLIENT_API_OIO_HTTP_BLOB_UPLOAD_H
#define OIO_CLIENT_API_OIO_HTTP_BLOB_UPLOAD_H

#include <cstdint>
#include <map>
#include <set>
#include <memory>
#include <string>
#include <oio/api/blob.h>
#include <utils/MillSocket.h>

namespace oio {
namespace http {
namespace coro {

class UploadBuilder {
  public:
    UploadBuilder();

    ~UploadBuilder();

    void Host(const std::string &s);

    void Name(const std::string &s);

    void Field(const std::string &k, const std::string &v);

    void Trailer(const std::string &k);

    std::shared_ptr<oio::api::blob::Upload> Build(
            std::shared_ptr<MillSocket> socket);

  private:
    std::string host;
    std::string chunkid;
    int64_t content_length;
    std::map<std::string, std::string> fields;
    std::set<std::string> trailers;
};

} // namespace coro
} // namespace http
} // namespace oio

#endif //OIO_CLIENT_API_OIO_HTTP_BLOB_UPLOAD_H
