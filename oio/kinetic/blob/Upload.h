/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_KINETIC_CLIENT_UPLOAD_H
#define OIO_KINETIC_CLIENT_UPLOAD_H

#include <string>
#include <memory>
#include <set>
#include <oio/api/Upload.h>
#include <oio/kinetic/client/ClientInterface.h>

namespace oio {
namespace kinetic {
namespace blob {

class UploadBuilder {
  public:
    UploadBuilder(std::shared_ptr<oio::kinetic::client::ClientFactory> f) noexcept;

    ~UploadBuilder() noexcept;

    void Target(const char *to) noexcept;
    void Target(const std::string &to) noexcept;

    void Name(const char *n) noexcept;
    void Name(const std::string &n) noexcept;

    void BlockSize(uint32_t s) noexcept;

    std::unique_ptr<oio::blob::Upload> Build() noexcept;

  private:
    std::shared_ptr<oio::kinetic::client::ClientFactory> factory;
    std::set<std::string> targets;
    std::string name;
    uint32_t block_size;
};

} // namespace rpc
} // namespace kinetic
} // namespace oio

#endif //OIO_KINETIC_CLIENT_UPLOAD_H
