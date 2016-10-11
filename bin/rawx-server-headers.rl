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
#include "rawx-server-headers.h"

#define token(T) return (T)

%%{
machine header_lexer_s;
access lexer.;
parse_header := |*
/X-oio-chunk-meta-container-id/i  { value = Value::ContainerId; };
/X-oio-chunk-meta-content-id/i    { value = Value::ContentId; };
/X-oio-chunk-meta-content-path/i  { value = Value::ContentPath; };
/X-oio-chunk-meta-content-hash/i  { value = Value::ContentHash; };
/X-oio-chunk-meta-content-size/i  { value = Value::ContentSize; };
/X-oio-chunk-meta-chunk-id/i      { value = Value::ChunkId; };
/X-oio-chunk-meta-chunk-hash/i    { value = Value::ChunkHash; };
/X-oio-chunk-meta-chunk-size/i    { value = Value::ChunkSize; };
/X-oio-chunk-meta-chunk-pos/i     { value = Value::ChunkPosition; };
*|;
}%%

%% write data;

void RawxHeader::Parse (const std::string &k) {
    const char *p = k.data();
    const char *pe = k.data() + k.size();
    //const char *eof = pe;
    struct header_lexer_s lexer{nullptr, nullptr, 0, 0};
%% write init;
%% write exec;
}

#define ON_HEADER(Name) case Value::Name: return #Name

std::string RawxHeader::NetworkName() const {
    switch (value) {
        ON_HEADER(Unexpected);
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

#define ON_KV(Name,V) case Value::Name: return V

std::string RawxHeader::StorageName() const {
    switch (value) {
        ON_KV(Unexpected,"");
        ON_KV(ContainerId,"container.id");
        ON_KV(ContentId, "content.id");
        ON_KV(ContentPath, "content.path");
        ON_KV(ContentHash, "content.hash");
        ON_KV(ContentSize, "content.size");
        ON_KV(ChunkId, "chunk.id");
        ON_KV(ChunkHash, "chunk.hash");
        ON_KV(ChunkSize, "chunk.size");
        ON_KV(ChunkPosition, "chunk.pos");
    }
    return "invalid";
}