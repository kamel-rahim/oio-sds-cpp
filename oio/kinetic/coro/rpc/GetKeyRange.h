/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_KINETIC_GETKEYRANGE_H
#define OIO_KINETIC_GETKEYRANGE_H

#include <cstdint>
#include <memory>
#include "Request.h"
#include "Exchange.h"

namespace oio {
namespace kinetic {
namespace rpc {

class GetKeyRange : public oio::kinetic::rpc::Exchange {
  public:
    GetKeyRange();
    FORBID_MOVE_CTOR(GetKeyRange);
    FORBID_COPY_CTOR(GetKeyRange);

    ~GetKeyRange();

    void Start(const char *k);

    void Start(const std::string &k);

    void End(const char *k);

    void End(const std::string &k);

    void IncludeStart(bool v);

    void IncludeEnd(bool v);

    void MaxItems(int32_t v);

    void Steal(std::vector<std::string> &v);

    void ManageReply(oio::kinetic::rpc::Request &rep) override;

  private:
    std::vector<std::string> out_;
};

} // namespace rpc
} // namespace kinetic
} // namespace oio

#endif //OIO_KINETIC_GETKEYRANGE_H
