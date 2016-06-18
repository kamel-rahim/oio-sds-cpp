/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_KINETIC_CLIENT_DOWNLOAD_H
#define OIO_KINETIC_CLIENT_DOWNLOAD_H

#include <string>
#include <memory>
#include <set>
#include <oio/api/blob/Download.h>
#include <oio/kinetic/coro/client/ClientInterface.h>

namespace oio {
namespace kinetic {
namespace blob {

class DownloadBuilder {
  public:
    DownloadBuilder(std::shared_ptr<oio::kinetic::client::ClientFactory> f) noexcept;

    ~DownloadBuilder() noexcept;

    void Name(const char *n) noexcept;

    void Name(const std::string &n) noexcept;

    void Target(const char *to) noexcept;

    void Target(const std::string &to) noexcept;

    std::unique_ptr<oio::api::blob::Download> Build() noexcept;

  private:
    std::string name;
    std::set<std::string> targets;
    std::shared_ptr<oio::kinetic::client::ClientFactory> factory;
};

} // namespace client
} // namespace kinetic
} // namespace oio

#endif //OIO_KINETIC_CLIENT_DOWNLOAD_H
