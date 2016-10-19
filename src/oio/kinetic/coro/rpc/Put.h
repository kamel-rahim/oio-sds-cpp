/**
 * Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/
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
