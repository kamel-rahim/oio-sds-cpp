//
// Created by jfs on 01/06/16.
//
#include <cstring>
#include "http_parser.h"


int main (int argc, char **argv) {
    (void) argc, (void) argv;
    const char *request = "GET / HTTP/1.1\r\n"
            "Transfer-encoding: chunked\r\n"
            "Trailer: User-Agent, Plop\r\n"
            "\r\n"
            "4\r\nABCD\r\n"
            "0\r\n"
            "User-agent: oio-kine\r\n"
            "User-agent: oio-kine\r\n"
            "Plop: oio-kine\r\n"
            "\r\n";
    struct http_parser parser;
    http_parser_init(&parser, HTTP_REQUEST);
    struct http_parser_settings settings;
    memset(&settings, 0, sizeof(settings));
    http_parser_execute(&parser, &settings, request, strlen(request));
    return 0;
}