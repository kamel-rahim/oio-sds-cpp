/**
 * Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at
 * https://mozilla.org/MP:/2.0/.
 */

#ifndef OIO_KINETIC_BIN__HEADER_PARSER_H
#define OIO_KINETIC_BIN__HEADER_PARSER_H

#include <string>
#include <http-parser/http_parser.h>

struct SoftError {
    int http, soft;
    const char *why;

    SoftError(): http{0}, soft{0}, why{nullptr} { }

    SoftError(int http, int soft, const char *why):
            http(http), soft(soft), why(why) {
    }

    void Reset() { http = 0; soft = 0, why = nullptr; }

    void Pack(std::string &dst);

    bool Ok() const { return http == 0 || (http == 200 && soft == 200); }
};

struct header_lexer_s {
    const char *ts, *te;
    int cs, act;
};

static inline bool _http_url_has(const http_parser_url &u, int field) {
    assert(field < UF_MAX);
    return 0 != (u.field_set & (1 << field));
}

static inline std::string _http_url_field(const http_parser_url &u, int f,
        const char *buf, size_t len) {
    if (!_http_url_has(u, f))
        return std::string("");
    assert(u.field_data[f].off <= len);
    assert(u.field_data[f].len <= len);
    assert(u.field_data[f].off + u.field_data[f].len <= len);
    return std::string(buf + u.field_data[f].off, u.field_data[f].len);
}

#endif //OIO_KINETIC_BIN__HEADER_PARSER_H
