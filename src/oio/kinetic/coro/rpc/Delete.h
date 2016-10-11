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
    FORBID_MOVE_CTOR(Delete);
    FORBID_COPY_CTOR(Delete);

    ~Delete();

    void ManageReply(oio::kinetic::rpc::Request &rep) override;

    void Key (const char *k);

    void Key (const std::string &k);
};

}
}
}

#endif //OIO_KINETIC_DELETE_H
