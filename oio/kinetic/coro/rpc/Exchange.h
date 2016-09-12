/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_KINETIC_EXCHANGE_H
#define OIO_KINETIC_EXCHANGE_H

#include <cstdint>
#include <memory>
#include <utils/utils.h>
#include "Request.h"

namespace oio {
namespace kinetic {
namespace rpc {

/* Represents any RPC to a kinetic drive */
class Exchange {
  public:
    Exchange();
    FORBID_COPY_CTOR(Exchange);
    FORBID_MOVE_CTOR(Exchange);
    
    virtual ~Exchange() {}

    void SetSequence(int64_t s);

    std::shared_ptr<oio::kinetic::rpc::Request> MakeRequest();

    bool Ok() const { return status_; }

    virtual void ManageReply(oio::kinetic::rpc::Request &rep) = 0;

  protected:
    void checkStatus (const oio::kinetic::rpc::Request &rep);

  protected:
    std::shared_ptr<oio::kinetic::rpc::Request> req_;
    bool status_;

};

} // namespace rpc
} // namespace kinetic
} // namespace oio

#endif //OIO_KINETIC_EXCHANGE_H
