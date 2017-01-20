/**
 * This file is part of the OpenIO client libraries
 * Copyright (C) 2016 OpenIO SAS
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.
 */

#ifndef SRC_UTILS_HTTP_HPP_
#define SRC_UTILS_HTTP_HPP_

#include <http-parser/http_parser.h>

#include <string>
#include <map>
#include <set>
#include <vector>
#include <queue>
#include <memory>

#include "net.hpp"
#include "utils.hpp"

namespace http {

enum Code {
  OK, ClientError, ServerError, NetworkError, Done ,Timeout
};

/**
 * Uploads a (maybe huge) flow of bytes in the request.
 */
class Request {
 public:
    /**
     * Default constructor.
     * builds a "GET /" request with no associated socket.
     */
    Request();

    /**
     * Builds a "GET /" Request on the given socket.
     * @param s the socket to be used.
     */
    explicit Request(std::shared_ptr<net::Socket> s);

    /**
     * Destructor
     */
    ~Request();

    inline void ContentLength(int64_t l) { content_length = l; }

    inline void Selector(const std::string &sel) { selector.assign(sel); }

    inline void Method(const std::string &m) { method.assign(m); }

    inline void Query(const std::string &k, const std::string &v) {
        query[k] = v;
    }

    inline void Field(const std::string &k, const std::string &v) {
        fields[k] = v;
    }

    inline void Trailer(const std::string &k) { trailers.insert(k); }

    /**
     * Replaces the underlying socket.
     * @param s the new socket.
     */
    inline void Socket(std::shared_ptr<net::Socket> s) { socket = s; }

    /**
     * Reset the connection and the parsing context
     */
    inline void Abort() { if (socket.get() != nullptr) socket->close(); }

    /**
     * Restart a new conection connected to the same target
     */
    inline bool Reconnect() {
        return socket.get() != nullptr && socket->Reconnect();
    }

    /**
     * Tells if the request is ready to work with its connection.
     * @return false if no conection has been provided.
     */
    inline bool Connected() const {
        return socket.get() != nullptr && socket->fileno() >= 0;
    }

    /**
     *
     * @return
     */
    Code WriteHeaders();

    /**
     *
     * @param buf
     * @param len
     * @return
     */
    Code Write(const uint8_t *buf, uint32_t len);

    /**
     * Send the final chunk if necessary, and the trailers if any.
     * NooP in case of a inline transfer encoding.
     * @return
     */
    Code FinishRequest();

 public:
    std::string method;
    std::string selector;
    std::map<std::string, std::string> fields;

 private:
    std::map<std::string, std::string> query;
    std::set<std::string> trailers;
    std::shared_ptr<net::Socket> socket;
    int64_t content_length;
    int64_t sent;
};

/**
 * Downloads a HTTP reply.
 */
class Reply {
 public:
    struct Slice {
        const uint8_t *buf;
        uint32_t len;

        Slice() : buf{nullptr}, len{0} {}

        Slice(const uint8_t *b, uint32_t l) : buf{b}, len{l} {}
    };

    enum Step {
        // No data read yet
        Beginning,
        // Currently reading the first batch of headers.
        // We might loop on Headers, while 100-continue are received.
        Headers,
        Body,
        Done
    };

    /**
     * Sadly, we use a HTTP parser written in C, and we have to provide C-hooks
     * that do not allow use to access the private fields of the structure. The
     * workaround chosen here is to enclose all the fields into an old-style
     * structure, give a poiter to such structure to the parser and provide a
     * getter to the caller application.
     */
    struct Context {
        std::map<std::string, std::string> fields;
        std::map<std::string, std::string> query;
        std::set<std::string> trailers;
        int64_t content_length;
        int64_t received;

        Step step;
        std::string tmp;

        // Chunks of body recognized from the reply's body. They directly point
        // to the 'buffer'. It is not allowed to consume input until the queue
        // is not empty.
        std::queue<Slice> body_bytes;

        // working buffer for the request
        std::vector<uint8_t> buffer;

        // Offset of the next byte to manage from the input.
        unsigned int buffer_offset;

        // Actual number of bytes present in the buffer
        unsigned int buffer_length;

        struct http_parser parser;

        Context() : fields(), query(), trailers(),
                    content_length{-1}, received{0},
                    step{Beginning}, tmp(), body_bytes(),
                    buffer(2048), buffer_offset{0}, buffer_length{0} { Init(); }

        void Init() {
            http_parser_init(&parser, HTTP_RESPONSE);
            parser.data = this;
        }

        void Reset() {
            step = Step::Beginning;
            fields.clear();
            query.clear();
            trailers.clear();
            while (!body_bytes.empty())
                body_bytes.pop();
            buffer.resize(2048);
            received = 0;
            content_length = -1;
            Init();
        }
    };

 public:
    /** Constructor */
    Reply();

    /**
     * Only constructor allowed.
     *
     * @param s an established socket.
     */
    explicit Reply(std::shared_ptr<net::Socket> s);

    /**
     * Destructor.
     *
     * Astonashing, isn't it ?
     */
    ~Reply();

    /**
     * Replaces the underlying socket.
     * @param s the new socket.
     */
    inline void Socket(std::shared_ptr<net::Socket> s) { socket = s; }

    /**
     * Consumes the input until the reply's headers have been read.
     * It is illegal to call this several times on the same object.
     * @return a status code indicating if iteratiosn might continue.
     */
    Code ReadHeaders();
    /**
    * Get the reply's headers
    * @param data the output map
    * @return a status code
    */
    Code GetHeaders(std::map <std::string, std::string> *data,
                    std::string prefix);

    /** Get a slice of body. The data is not copied, and must be consumed by the
     * application before the next call to ReadBody(). This call invalidates all
     * the slices previously returned, because they all point to the same
     * internal buffer
     * @see consumeInput()
     * @param out the output slice modified in place, cannot be null
     * @return a status code indicating if iteratiosn might continue.
     */
    Code ReadBody(Slice *out);

    /**
     * Wraps ReadBody(Slice) and appends the slice into the output vector.
     * @see ReadBody()
     * @param out the output vector, cannot be null
     * @return a status code indicating if iteratiosn might continue.
     */
    Code AppendBody(std::vector<uint8_t> *out);

    /**
     * Get a read-only access on the internal state of the reply.
     * @return the reply context
     */
    const Context &Get() const { return ctx; }

    void Skip();

 private:
    /**
     * Consumes the buffers and fills the internal buffer. by the way, it
     * invalidates all the slice that have been returned.
     * @param dl a deadline as used by libmill (monotonic time in ms)
     * @return a status code indicating if iteratiosn might continue.
     */
    Code consumeInput(int64_t dl);

    /**
     * Called by the constructor.
     */
    void init();

 private:
    std::shared_ptr<net::Socket> socket;
    Context ctx;
    http_parser_settings settings;
};

/**
 * Simplifies the Request/Reply mechanisms for small requests and small replies.
 */
class Call {
 public:
    /**
     * Constructor of a HTTP call linked with no socket at all.
     * The application has to call Socket() before Run()
     */
    Call();

    /**
     * Constructor of a HTTP call linked with a specific socket.
     * @param s the socket to be used
     */
    explicit Call(std::shared_ptr<net::Socket> s);

    /**
     * Destructor
     */
    ~Call();

    /**
     * Tell the current HTTP call to work with the given socket.
     * @param s
     * @return
     */
    Call &Socket(std::shared_ptr<net::Socket> s) {
        request.Socket(s);
        reply.Socket(s);
        return *this;
    }

    Call &Method(const std::string &m) {
        request.Method(m);
        return *this;
    }

    Call &Selector(const std::string &sel) {
        request.Selector(sel);
        return *this;
    }

    Call &Query(const std::string &k, const std::string &v) {
        request.Query(k, v);
        return *this;
    }

    Call &Field(const std::string &k, const std::string &v) {
        request.Field(k, v);
        return *this;
    }

    /**
     * Play the whole HTTP Request/Reply sequence, feeding 'in' as the request's
     * body, and 'out' will be filled with the reply's body.
     * @param in the request body. Can be empty.
     * @param out cannot be null, will be erased.
     * @return a Code depicting how things happened
     */
    Code Run(const std::string &in, std::string *out);
    Code GetReplyHeaders(std::map <std::string, std::string> *data,
                         std::string prefix);

 protected:
    Request request;
    Reply reply;

 private:
    FORBID_COPY_CTOR(Call);

    FORBID_MOVE_CTOR(Call);
};

std::ostream& operator<<(std::ostream &out, const Code code);

std::ostream& operator<<(std::ostream &out, const Reply::Step step);

class Parameters {
 public:
    std::shared_ptr<net::Socket> socket;
    std::string method;
    std::string selector;
    std::string body_in;
    std::string body_out;
    std::map<std::string, std::string> header_in;
    std::map<std::string, std::string> *header_out;
    std::string header_out_filter;

    Parameters(std::shared_ptr<net::Socket> _socket,
            std::string _method, std::string _selector) :
            socket(_socket), method(_method), selector(_selector),
            header_out(NULL) {
    }

    Parameters(std::shared_ptr<net::Socket> _socket,
            std::string _method, std::string _selector,
            std::string _body_in) : socket(_socket), method(_method),
                                    selector(_selector), body_in(_body_in),
                                    header_out(NULL) {
    }

    Code DoCall() {
        Call call;
        call.Socket(socket)
                .Method(method)
                .Selector(selector)
                .Field("Connection", "keep-alive");
        if (header_in.size()) {
            for (auto& a : header_in) {
                call.Field(a.first, a.second);
            }
        }

        LOG(INFO) << method << " " << selector;
        Code rc = call.Run(body_in, &body_out);
        LOG(INFO) << " rc=" << rc << " reply=" << body_out;
        if (rc == Code::OK && header_out)
            rc = call.GetReplyHeaders(header_out, header_out_filter);
        return rc;
    }
};

}  // namespace http

#endif  // SRC_UTILS_HTTP_HPP_
