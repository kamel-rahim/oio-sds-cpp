/**
 * Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at
 * https://mozilla.org/MP:/2.0/.
 */

#include <cassert>
#include <bin/common-header-parser.h>
#include <bin/ec-proxy-headers.h>

#define token(T) return (T)

%%{
machine header_lexer_s;
access lexer.;
parse_header := |*
/x-oio-req-id/i                            { value = Value::ReqId; };
/x-oio-chunk-meta-container-id/i           { value = Value::ContainerId; };
/x-oio-chunk-meta-content-path/i           { value = Value::ContentPath; };
/x-oio-chunk-meta-content-version/i        { value = Value::ContentVersion; };
/X-oio-chunk-meta-content-id/i             { value = Value::ContentId; };
/x-oio-chunk-meta-content-storage-policy/i { value = Value::StoragePolicy; };
/x-oio-chunk-meta-content-chunk-method/i   { value = Value::ChunkMethod; };
/x-oio-chunk-meta-content-mime-type/i      { value = Value::MineType; };
/x-oio-chunk-meta-chunk-*/i                { value = Value::ChunkDest; }; 
/x-oio-chunk-meta-chunks-nb/i              { value = Value::Chunks; }; 
/x-oio-chunk-meta-chunk-pos/i              { value = Value::ChunkPos; }; 
/x-oio-chunk-meta-chunk-size/i             { value = Value::ChunkSize; }; 
/Range/i                                   { value = Value::Range; }; 
/transfer-encoding/i                       { value = Value::TransferEncoding; }; 
*|;
}%%

%% write data;

void EcHeader::Parse(const std::string &k) {
    const char *p = k.data();
    const char *pe = p + k.size();
    const char *eof = pe;
    struct header_lexer_s lexer{nullptr, nullptr, 0, 0};
    value = Value::Unexpected;
%% write init;
%% write exec;
}

#define ON_HEADER(Name) case Value::Name: return #Name

std::string EcHeader::Name() const {
	switch (value) {
        ON_HEADER(Unexpected);
        ON_HEADER(ReqId);
        ON_HEADER(ContainerId);
        ON_HEADER(ContentPath);
        ON_HEADER(ContentVersion);
        ON_HEADER(ContentId);
        ON_HEADER(StoragePolicy);
        ON_HEADER(ChunkMethod);
        ON_HEADER(MineType);
        ON_HEADER(ChunkDest);
        ON_HEADER(Chunks);
        ON_HEADER(ChunkPos);
        ON_HEADER(ChunkSize);
        ON_HEADER(Range);
        ON_HEADER(TransferEncoding);
   }
    return "invalid";
}