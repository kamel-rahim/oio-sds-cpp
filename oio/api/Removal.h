/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_API_REMOVAL_H
#define OIO_API_REMOVAL_H

namespace oio {
namespace blob {

class Removal {
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
    virtual ~Removal() noexcept { }

    virtual Status Prepare() noexcept = 0;

    virtual bool Commit() noexcept = 0;

    virtual bool Abort() noexcept = 0;

    virtual bool Ok() noexcept = 0;
};

}
}
#endif //OIO_API_REMOVAL_H
