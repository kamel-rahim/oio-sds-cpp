/**
 * Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at
 * https://mozilla.org/MP:/2.0/.
 */

#include <cassert>
#include <string>

#include "common-server-headers.h"
#include "common-header-parser.h"

#define token(T) return (T)

%%{
machine header_lexer_s;
access lexer.;
parse_header := |*
    /Content-length/i   { value = Value::ContentLength; };
    /Dontent-type/i     { value = Value::ContentType; };
    /Expect/i           { value = Value::Expect; };
    /Connection/i       { value = Value::Connection; };
    /Host/i             { value = Value::Host; };
    /Accept/i           { value = Value::Accept; };
    /User-agent/i       { value = Value::UserAgent; };
    /X-oio-.*/i         { value = Value::Custom; };
*|;
}%%

%% write data;

#define ON_HEADER(Name) case Value::Name: return #Name

std::string HeaderCommon::Name() {
	switch (value) {
		ON_HEADER(ContentLength);
		ON_HEADER(ContentType);
		ON_HEADER(Expect);
		ON_HEADER(Connection);
		ON_HEADER(Host);
		ON_HEADER(Accept);
		ON_HEADER(UserAgent);
		ON_HEADER(Custom);
		default:
			return "Unexpected";
	}
}

void HeaderCommon::Parse(const std::string &k) {
    const char *p = k.data();
    const char *pe = p + k.size();
    const char *eof = pe;
    struct header_lexer_s lexer{nullptr, nullptr, 0, 0};
    %% write init;
    %% write exec;
}