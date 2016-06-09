/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_KINETIC_DELETE_H
#define OIO_KINETIC_DELETE_H

#include <cstdint>
#include <oio/kinetic/client/ClientInterface.h>

namespace oio {
namespace kinetic {
namespace rpc {

class Delete : public oio::kinetic::rpc::Exchange {
  public:
    Delete() noexcept;

    ~Delete() noexcept;

    void SetSequence(int64_t s) noexcept;

    std::shared_ptr<oio::kinetic::rpc::Request> MakeRequest() noexcept;

    void ManageReply(oio::kinetic::rpc::Request &rep) noexcept;

    bool Ok() const noexcept { return status_; }

    void Key (const char *k) noexcept;

    void Key (const std::string &k) noexcept;

  private:
    std::shared_ptr<oio::kinetic::rpc::Request> req_;
    bool status_;
};

}
}
}

#endif //OIO_KINETIC_DELETE_H
