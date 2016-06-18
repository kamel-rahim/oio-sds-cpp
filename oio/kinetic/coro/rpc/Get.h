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
    Get() noexcept;

    ~Get() noexcept;

    void Key(const char *k) noexcept;

    void Key(const std::string &k) noexcept;

    void Steal(std::vector<uint8_t> &v) noexcept { v.swap(val_); }

    void SetSequence(int64_t s) noexcept;

    std::shared_ptr<oio::kinetic::rpc::Request> MakeRequest() noexcept;

    void ManageReply(oio::kinetic::rpc::Request &rep) noexcept;

    bool Ok() const noexcept { return status_; }

  private:
    std::shared_ptr<oio::kinetic::rpc::Request> req_;
    std::vector<uint8_t> val_;
    bool status_;
};

} // namespace rpc
} // namespace kinetic
} // namespace oio

#endif //OIO_KINETIC_EXCHANGE_GET_H
