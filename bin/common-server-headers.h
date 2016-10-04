/**
 * Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at
 * https://mozilla.org/MP:/2.0/.
 */

#ifndef OIO_KINETIC_BIN__COMMON_SERVER_HEADERS_H
#define OIO_KINETIC_BIN__COMMON_SERVER_HEADERS_H

class HeaderCommon {
  public:
    enum Value {
        Unexpected = 0,
        ContentLength,
        ContentType,
        Expect,
        Connection,
        Host,
        Accept,
        UserAgent,
        Custom
    };

    HeaderCommon(): value{Value::Unexpected} {}
    ~HeaderCommon() {}

    void Parse(const std::string &k);
    std::string Name();
    inline bool Matched() const { return value != Value::Unexpected; }
    inline bool IsCustom() const { return value == Value::Custom; }
    inline Value Get() const { return value; }

  private:
    Value value;
};

#endif //OIO_KINETIC_BIN__COMMON_SERVER_HEADERS_H