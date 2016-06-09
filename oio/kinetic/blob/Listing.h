/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_KINETIC_CLIENT_LISTING_H
#define OIO_KINETIC_CLIENT_LISTING_H

#include <memory>
#include <string>
#include <set>
#include <oio/api/Listing.h>
#include <oio/kinetic/client/ClientInterface.h>

namespace oio {
namespace kinetic {
namespace blob {

class ListingBuilder {
  public:
    ListingBuilder(std::shared_ptr<oio::kinetic::client::ClientFactory> f) noexcept;

    ~ListingBuilder() noexcept;

    void Name(const std::string &n) noexcept;

    void Name(const char *n) noexcept;

    void Target(const std::string &to) noexcept;

    void Target(const char *to) noexcept;

    std::unique_ptr<oio::blob::Listing> Build() noexcept;

  private:
    std::shared_ptr<oio::kinetic::client::ClientFactory> factory;
    std::set<std::string> targets;
    std::string name;
};

} // namespace client
} // namespace kinetic
} // namespace oio

#endif //OIO_KINETIC_CLIENT_LISTING_H
