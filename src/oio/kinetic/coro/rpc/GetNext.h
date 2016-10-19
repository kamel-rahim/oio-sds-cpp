/**
 * Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/
 */

#ifndef SRC_OIO_KINETIC_CORO_RPC_GETNEXT_H_
#define SRC_OIO_KINETIC_CORO_RPC_GETNEXT_H_

#include <cstdint>
#include <string>
#include "./Exchange.h"

namespace oio {
namespace kinetic {
namespace rpc {

class GetNext : public oio::kinetic::rpc::Exchange {
 public:
    GetNext();
    FORBID_MOVE_CTOR(GetNext);
    FORBID_COPY_CTOR(GetNext);

    ~GetNext();

    void Key(const char *k);

    void Key(const std::string &k);

    void Steal(std::string *v);

    void ManageReply(oio::kinetic::rpc::Request *rep) override;

 private:
    std::string out_;
};

}  // namespace rpc
}  // namespace kinetic
}  // namespace oio

#endif  // SRC_OIO_KINETIC_CORO_RPC_GETNEXT_H_
