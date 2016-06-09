/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_KINETIC_CLIENT_PENDINGEXCHANGE_H
#define OIO_KINETIC_CLIENT_PENDINGEXCHANGE_H

#include <cstdint>
#include <oio/kinetic/rpc/Exchange.h>
#include <oio/kinetic/rpc/Request.h>
#include "ClientInterface.h"

struct mill_chan;

namespace oio {
namespace kinetic {
namespace client {

class PendingExchange : public Sync {
  public:
    PendingExchange(oio::kinetic::rpc::Exchange *e) noexcept;

    ~PendingExchange() noexcept;

    void SetSequence(int64_t s) noexcept;

    int64_t Sequence() const noexcept;

    void ManageReply (oio::kinetic::rpc::Request &rep) noexcept;

    std::shared_ptr<oio::kinetic::rpc::Request> MakeRequest() noexcept;

    void Signal() noexcept;

    void Wait() noexcept;

  private:
    oio::kinetic::rpc::Exchange *exchange_;
    struct mill_chan *notification_;
    int64_t seqid_;
};

} // namespace client
} // namespace kinetic
} // namespace oio


#endif //OIO_KINETIC_CLIENT_PENDINGEXCHANGE_H
