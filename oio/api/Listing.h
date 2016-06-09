/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_API_LISTING_H
#define OIO_API_LISTING_H

#include <string>

namespace oio {
namespace blob {

/**
 * Usage:
 * Listing *list = ...; // get an instance
 * auto rc = list->Prepare(); // mandatory step
 * if (rc != Listing::Status::Ok) {
 *   std::cerr << Listing::Status2Str(rc) << std::endl;
 * } else {
 *   std::string id, key;
 *   while (list->Next(id, key)) {
 *     std::cerr << id << " has " << key << std::endl;
 *   }
 * }
 */
class Listing {
  public:
    enum class Status {
        OK, NotFound, NetworkError, ProtocolError
    };

    static inline const char *Status2Str(Status s) {
        switch (s) {
            case Status::OK:
                return "OK";
            case Status::NotFound:
                return "Not found";
            case Status::NetworkError:
                return "Network error";
            case Status::ProtocolError:
                return "Protocol error";
            default:
                return "***invalid status***";
        }
    }

  public:
    virtual ~Listing() noexcept { }

    virtual Status Prepare() noexcept = 0;

    virtual bool Next(std::string &id, std::string &key) noexcept = 0;
};

} // namespace blob
} // namespace oio

#endif //OIO_API_LISTING_H