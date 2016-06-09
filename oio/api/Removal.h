/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_API_REMOVAL_H
#define OIO_API_REMOVAL_H

namespace oio {
namespace blob {

class Removal {
  public:
    virtual ~Removal() noexcept { }

    virtual bool Prepare() noexcept = 0;

    virtual bool Commit() noexcept = 0;

    virtual bool Abort() noexcept = 0;

    virtual bool Ok() noexcept = 0;
};

}
}
#endif //OIO_API_REMOVAL_H
