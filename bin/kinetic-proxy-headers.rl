/**
 * Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at
 * https://mozilla.org/MP:/2.0/.
 */

#include <cassert>
#include "common-header-parser.h"
#include "kinetic-proxy-headers.h"

#define token(T) return (T)

%%{
machine header_lexer_s;
access lexer.;
parse_header := |*
/x-oio-target.*/i { value = Value::Target; };
/x-oio-meta-container-id/i { value = Value::ContainerId; };
/x-oio-meta-content-id/i   { value = Value::ContentId; };
/x-oio-meta-content-path/i { value = Value::ContentPath; };
/x-oio-meta-content-hash/i { value = Value::ContentHash; };
/x-oio-meta-content-size/i { value = Value::ContentSize; };
/x-oio-meta-chunk-id/i     { value = Value::ChunkId; };
/x-oio-meta-chunk-hash/i   { value = Value::ChunkHash; };
/x-oio-meta-chunk-size/i   { value = Value::ChunkSize; };
/x-oio-meta-chunk-pos/i    { value = Value::ChunkPosition; };
*|;
}%%

%% write data;

void KineticHeader::Parse(const std::string &k) {
    const char *p = k.data();
    const char *pe = p + k.size();
    const char *eof = pe;
    struct header_lexer_s lexer{nullptr, nullptr, 0, 0};
    value = Value::Unexpected;
%% write init;
%% write exec;
}

#define ON_HEADER(Name) case Value::Name: return #Name

std::string KineticHeader::Name() const {
	switch (value) {
        ON_HEADER(Unexpected);
        ON_HEADER(Target);
        ON_HEADER(ContainerId);
        ON_HEADER(ContentId);
        ON_HEADER(ContentPath);
        ON_HEADER(ContentHash);
        ON_HEADER(ContentSize);
        ON_HEADER(ChunkId);
        ON_HEADER(ChunkHash);
        ON_HEADER(ChunkSize);
        ON_HEADER(ChunkPosition);
    }
    return "invalid";
}