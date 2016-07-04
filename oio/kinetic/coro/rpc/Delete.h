/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_KINETIC_DELETE_H
#define OIO_KINETIC_DELETE_H

#include <cstdint>
#include <oio/kinetic/coro/client/ClientInterface.h>

namespace oio {
namespace kinetic {
namespace rpc {

class Delete : public oio::kinetic::rpc::Exchange {
  public:
    Delete();

    ~Delete();

    void SetSequence(int64_t s);

    std::shared_ptr<oio::kinetic::rpc::Request> MakeRequest();

    void ManageReply(oio::kinetic::rpc::Request &rep);

    bool Ok() const { return status_; }

    void Key (const char *k);

    void Key (const std::string &k);

  private:
    std::shared_ptr<oio::kinetic::rpc::Request> req_;
    bool status_;
};

}
}
}

#endif //OIO_KINETIC_DELETE_H
