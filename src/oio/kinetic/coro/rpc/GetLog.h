/** Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef SRC_OIO_KINETIC_CORO_RPC_GETLOG_H_
#define SRC_OIO_KINETIC_CORO_RPC_GETLOG_H_

#include <memory>
#include "Exchange.h"

namespace oio {
namespace kinetic {
namespace rpc {

class GetLog : public oio::kinetic::rpc::Exchange {
 public:
    GetLog();

    FORBID_MOVE_CTOR(GetLog);
    FORBID_COPY_CTOR(GetLog);

    virtual ~GetLog();

    void ManageReply(oio::kinetic::rpc::Request *rep) override;

    double getCpu() const;

    double getTemp() const;

    double getSpace() const;

    double getIo() const;

 private:
    double cpu, temp, space, io;
};

}  // namespace rpc
}  // namespace kinetic
}  // namespace oio

#endif  // SRC_OIO_KINETIC_CORO_RPC_GETLOG_H_
