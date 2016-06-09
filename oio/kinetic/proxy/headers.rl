/**
 * Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at
 * https://mozilla.org/MP:/2.0/.
 */

#include <assert.h>
#include <oio/kinetic/proxy/headers.h>

#define token(T) return (T)

struct header_lexer_s {
	const char *ts, *te;
	int cs, act;
};

%%{
machine header_lexer_s;
access lexer.;
parse_header := |*
    /content-length/i   { return HDR_CONTENT_LENGTH; };
    /content-type/i     { return HDR_CONTENT_TYPE; };
    /expect/i           { return HDR_EXPECT; };
    /connection/i       { return HDR_CONNECTION; };
    /host/i             { return HDR_HOST; };
    /accept/i           { return HDR_ACCEPT; };
    /user-agent/i       { return HDR_USERAGENT; };
    /X-oio-account/i    { return HDR_OIO_ACCOUNT; };
    /X-oio-user/i       { return HDR_OIO_USER; };
    /X-oio-path/i       { return HDR_OIO_CONTENT; };
    /X-oio-version/i    { return HDR_OIO_VERSION; };
    /X-oio-target/i     { return HDR_OIO_TARGET; };
    /X-oio-chunk-id/i   { return HDR_OIO_CHUNK_ID; };
    /X-oio-chunk-size/i { return HDR_OIO_CHUNK_SIZE; };
    /X-oio-chunk-hash/i { return HDR_OIO_CHUNK_HASH; };
*|;
}%%

%% write data;

enum http_header_e header_parse (const char *p, unsigned int len) {
	const char *pe = p + len;
	struct header_lexer_s lexer;

	assert (p != (void*)0);
%% write init;
%% write exec;
	return HDR_none_matched;
}

#define ON_HEADER(Prefix,Name) case Prefix##Name: return #Name

const char * header_to_string (enum http_header_e h) {
	switch (h) {
		ON_HEADER(HDR_,CONTENT_LENGTH);
		ON_HEADER(HDR_,CONTENT_TYPE);
		ON_HEADER(HDR_,EXPECT);
		ON_HEADER(HDR_,CONNECTION);
		ON_HEADER(HDR_,HOST);
		ON_HEADER(HDR_,ACCEPT);
		ON_HEADER(HDR_,USERAGENT);
        ON_HEADER(HDR_,OIO_ACCOUNT);
        ON_HEADER(HDR_,OIO_USER);
        ON_HEADER(HDR_,OIO_CONTENT);
        ON_HEADER(HDR_,OIO_VERSION);
        ON_HEADER(HDR_,OIO_TARGET);
        ON_HEADER(HDR_,OIO_CHUNK_ID);
        ON_HEADER(HDR_,OIO_CHUNK_SIZE);
        ON_HEADER(HDR_,OIO_CHUNK_HASH);
		default:
			return "unmanaged";
	}
}
