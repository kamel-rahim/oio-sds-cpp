/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_KINETIC_GETNEXT_H
#define OIO_KINETIC_GETNEXT_H

#include <cstdint>
#include <oio/kinetic/coro/client/ClientInterface.h>

namespace oio {
namespace kinetic {
namespace rpc {

class GetNext : public oio::kinetic::rpc::Exchange {
  public:
    GetNext();

    ~GetNext();

    void Key(const char *k);

    void Key(const std::string &k);

    void Steal (std::string &v);

    void SetSequence(int64_t s);

    std::shared_ptr<oio::kinetic::rpc::Request> MakeRequest();

    void ManageReply(oio::kinetic::rpc::Request &rep);

    bool Ok() const { return status_; }

  private:
    std::shared_ptr<oio::kinetic::rpc::Request> req_;
    std::string out_;
    bool status_;
};

}
}
}

#endif //OIO_KINETIC_GETNEXT_H
