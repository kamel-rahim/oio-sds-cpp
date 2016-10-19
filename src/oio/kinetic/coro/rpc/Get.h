/**
 * Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/
 */

#ifndef SRC_OIO_KINETIC_CORO_RPC_GET_H_
#define SRC_OIO_KINETIC_CORO_RPC_GET_H_

#include <cstdint>
#include <memory>
#include <vector>
#include <string>

#include "./Exchange.h"

namespace oio {
namespace kinetic {
namespace rpc {

class Get : public oio::kinetic::rpc::Exchange {
 public:
    Get();

    FORBID_MOVE_CTOR(Get);
    FORBID_COPY_CTOR(Get);

    virtual ~Get();

    void Key(const char *k);

    void Key(const std::string &k);

    void Steal(std::vector<uint8_t> &v) { v.swap(out_); }

    void ManageReply(oio::kinetic::rpc::Request *rep) override;

 private:
    std::vector<uint8_t> out_;
};

}  // namespace rpc
}  // namespace kinetic
}  // namespace oio

#endif  // SRC_OIO_KINETIC_CORO_RPC_GET_H_
