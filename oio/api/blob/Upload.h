/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_API_UPLOAD_H
#define OIO_API_UPLOAD_H

#include <cstdint>
#include <string>

namespace oio {
namespace api {
namespace blob {

class Upload {
  public:
    enum class Status {
        OK, Already, NetworkError, ProtocolError
    };

    static inline const char *Status2Str(Status s) {
        switch (s) {
            case Status::OK:
                return "OK";
            case Status::Already:
                return "Already";
            case Status::NetworkError:
                return "Network error";
            case Status::ProtocolError:
                return "Protocol error";
            default:
                return "***invalid status***";
        }
    }

  public:
    virtual ~Upload() { }

    virtual void SetXattr(const std::string &k, const std::string &v) = 0;

    virtual Status Prepare() = 0;

    virtual bool Commit() = 0;

    virtual bool Abort() = 0;

    // buffer copied
    virtual void Write(const uint8_t *buf, uint32_t len) = 0;

    virtual void Write(const std::string &s) = 0;

    virtual void Flush() = 0;
};

} // namespace blob
} // namespace api
} //namespace oio

#endif //OIO_API_UPLOAD_H