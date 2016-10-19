/**
 * Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/
 */

#ifndef SRC_OIO_KINETIC_CORO_RPC_GETKEYRANGE_H_
#define SRC_OIO_KINETIC_CORO_RPC_GETKEYRANGE_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "./Exchange.h"

namespace oio {
namespace kinetic {
namespace rpc {

class GetKeyRange : public oio::kinetic::rpc::Exchange {
 public:
    GetKeyRange();

    FORBID_MOVE_CTOR(GetKeyRange);
    FORBID_COPY_CTOR(GetKeyRange);

    ~GetKeyRange();

    void Start(const char *k);

    void Start(const std::string &k);

    void End(const char *k);

    void End(const std::string &k);

    void IncludeStart(bool v);

    void IncludeEnd(bool v);

    void MaxItems(int32_t v);

    void Steal(std::vector<std::string> *v);

    void ManageReply(oio::kinetic::rpc::Request *rep) override;

 private:
    std::vector<std::string> out_;
};

}  // namespace rpc
}  // namespace kinetic
}  // namespace oio

#endif  // SRC_OIO_KINETIC_CORO_RPC_GETKEYRANGE_H_
