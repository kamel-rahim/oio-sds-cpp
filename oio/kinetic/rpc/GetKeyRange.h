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
    GetKeyRange() noexcept;

    ~GetKeyRange() noexcept;

    void Start(const char *k) noexcept;

    void Start(const std::string &k) noexcept;

    void End(const char *k) noexcept;

    void End(const std::string &k) noexcept;

    void IncludeStart(bool v) noexcept;

    void IncludeEnd(bool v) noexcept;

    void MaxItems(int32_t v) noexcept;

    void Steal(std::vector<std::string> &v) noexcept;

    void SetSequence(int64_t s) noexcept;

    std::shared_ptr<oio::kinetic::rpc::Request> MakeRequest() noexcept;

    void ManageReply(oio::kinetic::rpc::Request &rep) noexcept;

    bool Ok() const noexcept { return status_; }

  private:
    std::shared_ptr<oio::kinetic::rpc::Request> req_;
    std::vector<std::string> out_;
    bool status_;
};

} // namespace rpc
} // namespace kinetic
} // namespace oio

#endif //OIO_KINETIC_GETKEYRANGE_H
