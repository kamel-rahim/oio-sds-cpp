/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_KINETIC_EXCHANGE_H
#define OIO_KINETIC_EXCHANGE_H

#include <cstdint>
#include <memory>
#include "Request.h"

namespace oio {
namespace kinetic {
namespace rpc {

/* Represents any RPC to a kinetic drive */
class Exchange {
  public:
    virtual void SetSequence(int64_t s) noexcept = 0;

    virtual std::shared_ptr<oio::kinetic::rpc::Request> MakeRequest() noexcept = 0;

    virtual void ManageReply(oio::kinetic::rpc::Request &rep) noexcept = 0;

    virtual bool Ok() const noexcept = 0;
};

} // namespace rpc
} // namespace kinetic
} // namespace oio

#endif //OIO_KINETIC_EXCHANGE_H
