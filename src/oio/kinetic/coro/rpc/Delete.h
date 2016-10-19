/**
 * Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/
 */

#ifndef SRC_OIO_KINETIC_CORO_RPC_DELETE_H_
#define SRC_OIO_KINETIC_CORO_RPC_DELETE_H_

#include <cstdint>
#include <string>

#include "oio/kinetic/coro/client/ClientInterface.h"

namespace oio {
namespace kinetic {
namespace rpc {

class Delete : public oio::kinetic::rpc::Exchange {
 public:
    Delete();

    FORBID_MOVE_CTOR(Delete);
    FORBID_COPY_CTOR(Delete);

    ~Delete();

    void ManageReply(oio::kinetic::rpc::Request *rep) override;

    void Key(const char *k);

    void Key(const std::string &k);
};

}  // namespace rpc
}  // namespace kinetic
}  // namespace oio

#endif  // SRC_OIO_KINETIC_CORO_RPC_DELETE_H_
