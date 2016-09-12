/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_KINETIC_EXCHANGE_GET_H
#define OIO_KINETIC_EXCHANGE_GET_H

#include <cstdint>
#include <memory>
#include <vector>
#include "Exchange.h"
#include "Request.h"

namespace oio {
namespace kinetic {
namespace rpc {

class Get : public oio::kinetic::rpc::Exchange {
  public:
    Get();
    FORBID_MOVE_CTOR(Get);
    FORBID_COPY_CTOR(Get);

    ~Get();

    void Key(const char *k);

    void Key(const std::string &k);

    void Steal(std::vector<uint8_t> &v) { v.swap(val_); }

    void ManageReply(oio::kinetic::rpc::Request &rep) override;

  private:
    std::vector<uint8_t> val_;
};

} // namespace rpc
} // namespace kinetic
} // namespace oio

#endif //OIO_KINETIC_EXCHANGE_GET_H
