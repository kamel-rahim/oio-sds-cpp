/**
 * This file is part of the CLI tools around the OpenIO client libraries
 * Copyright (C) 2016 OpenIO SAS
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <cassert>
#include <bin/common-header-parser.h>
#include <bin/rawx-server-headers.h>

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