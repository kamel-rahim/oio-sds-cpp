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
#include <string>

#include <bin/common-server-headers.h>
#include <bin/common-header-parser.h>

#define token(T) return (T)

%%{
machine header_lexer_s;
access lexer.;
parse_header := |*
    /Content-length/i   { value = Value::ContentLength; };
    /Content-type/i     { value = Value::ContentType; };
    /Expect/i           { value = Value::Expect; };
    /Connection/i       { value = Value::Connection; };
    /Host/i             { value = Value::Host; };
    /Accept/i           { value = Value::Accept; };
    /User-agent/i       { value = Value::UserAgent; };
    /Range/i            { value = Value::Custom; };
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
