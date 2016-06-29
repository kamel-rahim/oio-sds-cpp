/** Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_KINETIC_OIO_HTTP_CORO_BLOB_H
#define OIO_KINETIC_OIO_HTTP_CORO_BLOB_H

#include <string>
#include <memory>
#include <map>
#include <set>
#include <utils/MillSocket.h>
#include <oio/api/blob.h>

namespace oio {
namespace http {
namespace coro {

#ifndef RAWX_HDR_PREFIX
#define RAWX_HDR_PREFIX "X-oio-chunk-meta-"
#endif

class DownloadBuilder {
  public:
    DownloadBuilder() noexcept;

    ~DownloadBuilder() noexcept;

    void Host(const std::string &s) noexcept ;

    void Name(const std::string &s) noexcept;

    void Field(const std::string &k, const std::string &v) noexcept;

    std::shared_ptr<oio::api::blob::Download> Build(
            std::shared_ptr<MillSocket> socket) noexcept;

  private:
    std::string host;
    std::string name;
    std::map<std::string, std::string> fields;
};


class UploadBuilder {
  public:
    UploadBuilder() noexcept;

    ~UploadBuilder() noexcept;

    void Host(const std::string &s) noexcept;

    void Name(const std::string &s) noexcept;

    void Field(const std::string &k, const std::string &v) noexcept;

    void Trailer(const std::string &k) noexcept;

    std::shared_ptr<oio::api::blob::Upload> Build(
            std::shared_ptr<MillSocket> socket) noexcept;

  private:
    std::string host;
    std::string name;
    int64_t content_length;
    std::map<std::string, std::string> fields;
    std::set<std::string> trailers;
};

} // namespace coro
} // namespace http
} // namespace oio

#endif //OIO_KINETIC_OIO_HTTP_CORO_BLOB_H
