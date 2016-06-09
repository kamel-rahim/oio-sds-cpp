/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_API_DOWNLOAD_H
#define OIO_API_DOWNLOAD_H

#include <vector>
#include <cstdint>

namespace oio {
namespace blob {

class Download {
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
    virtual ~Download() noexcept { }

    virtual Status Prepare() noexcept = 0;

    virtual bool IsEof() noexcept = 0;

    virtual int32_t Read(std::vector<uint8_t> &buf) noexcept = 0;
};

} // namespace blob
} // oio


#endif //OIO_API_DOWNLOAD_H
