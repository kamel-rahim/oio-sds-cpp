/**
 * This file is part of the OpenIO client libraries
 * Copyright (C) 2016 OpenIO SAS
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.
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
