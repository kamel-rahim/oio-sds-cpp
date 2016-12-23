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

#ifndef SRC_OIO_KINETIC_CORO_RPC_PUT_H_
#define SRC_OIO_KINETIC_CORO_RPC_PUT_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "./Exchange.h"

namespace oio {
namespace kinetic {
namespace rpc {

class Put : public oio::kinetic::rpc::Exchange {
 public:
    Put();

    FORBID_MOVE_CTOR(Put);
    FORBID_COPY_CTOR(Put);

    ~Put();

    void Key(const char *k);

    void Key(const std::string &k);

    void PreVersion(const char *p);

    void PostVersion(const char *p);

    /**
     * Zero copy
     * @param v
     */
    void Value(const Slice &v);

    /**
     * Copy the string
     * @param v
     */
    void Value(const std::string &v);

    /**
     * Copy the vector
     * @param v
     */
    void Value(const std::vector<uint8_t> &v);

    /**
     * Swap the vector's content
     * @param v
     */
    void Value(std::vector<uint8_t> *v);

    void ManageReply(oio::kinetic::rpc::Request *rep) override;

    /**
     *
     * @param on true for WriteThrough, false for WriteBack
     */
    void Sync(bool on);

 private:
    void rehash();

 private:
    std::vector<uint8_t> value_copy;
};


}  // namespace rpc
}  // namespace kinetic
}  // namespace oio

#endif  // SRC_OIO_KINETIC_CORO_RPC_PUT_H_
