/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_KINETIC_REQUEST_H
#define OIO_KINETIC_REQUEST_H

#include <cstdint>
#include <vector>
#include <kinetic.pb.h>

namespace oio {
namespace kinetic {
namespace rpc {

struct Request {
    ::com::seagate::kinetic::proto::Command cmd;
    ::com::seagate::kinetic::proto::Message msg;
    std::vector<uint8_t> value;

    Request() : cmd(), msg(), value() { }

    Request(Request &o) = delete;

    Request(const Request &o) = delete;

    Request(Request &&o) = delete;
};

} // namespace rpc
} // namespace kinetic
} // namespace oio

#endif //OIO_KINETIC_REQUEST_H
