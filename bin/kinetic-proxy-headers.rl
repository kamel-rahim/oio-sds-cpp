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
#include <bin/kinetic-proxy-headers.h>

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