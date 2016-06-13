/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_API_UPLOAD_H
#define OIO_API_UPLOAD_H

#include <cstdint>
#include <string>

namespace oio {
namespace blob {

class Upload {
  public:
    virtual ~Upload() { }

    virtual void SetXattr (const std::string &k, const std::string &v) = 0;

    virtual bool Prepare() = 0;

    virtual bool Commit() = 0;

    virtual bool Abort() = 0;

    // buffer copied
    virtual void Write(const uint8_t *buf, uint32_t len) = 0;

    virtual void Write(const std::string &s) = 0;

    virtual void Flush() = 0;
};

} // namespace blob
} //namespace oio

#endif //OIO_API_UPLOAD_H