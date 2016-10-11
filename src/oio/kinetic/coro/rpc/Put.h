/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_KINETIC_PUTEXCHANGE_H
#define OIO_KINETIC_PUTEXCHANGE_H

#include <cstdint>
#include <memory>
#include "Exchange.h"

namespace oio {
namespace kinetic {
namespace rpc {

class Put : public oio::kinetic::rpc::Exchange {
  public:
    Put();
    FORBID_MOVE_CTOR(Put);
    FORBID_COPY_CTOR(Put);

    ~Put();

    void Key(const char *k);

    void Key(const std::string &k);

    void PreVersion(const char *p);

    void PostVersion(const char *p);

    void Value(const Slice &v); // zero copy
    void Value(const std::string &v); // copy
    void Value(const std::vector<uint8_t> &v); // copy
    void Value(std::vector<uint8_t> &v); // swap

    void ManageReply(oio::kinetic::rpc::Request &rep) override;

    /**
     *
     * @param on true for WriteThrough, false for WriteBack
     */
    void Sync(bool on);

  private:
	void rehash();

  private:
    std::vector<uint8_t> value_copy;
};


} // namespace rpc
} // namespace kinetic
} // namespace oio

#endif //OIO_KINETIC_PUTEXCHANGE_H
