/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_KINETIC_PROXY__HEADERS_H
#define OIO_KINETIC_PROXY__HEADERS_H 1
#if defined(__cplusplus)
extern "C" {
#endif

#define OIO_HEADER_XATTR_PREFIX "X-oio-meta-"
#define OIO_HEADER_XATTR_RAWX_PREFIX "X-oio-chunk-meta-"

enum http_header_e
{
	HDR_none_matched = 0,
	HDR_CONTENT_LENGTH,
	HDR_CONTENT_TYPE,
	HDR_EXPECT,
	HDR_CONNECTION,
	HDR_HOST,
	HDR_ACCEPT,
	HDR_USERAGENT,

    HDR_OIO_TARGET,
	HDR_OIO_XATTR,
	HDR_OIO_XATTR_RAWX,
};

enum http_header_e header_parse (const char *p, unsigned int len);

const char * header_to_string (enum http_header_e h);

# if defined(__cplusplus)
}
# endif

#endif