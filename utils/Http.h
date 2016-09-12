/** Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_KINETIC_HTTP_H
#define OIO_KINETIC_HTTP_H

#include <string>
#include <map>
#include <set>
#include <vector>
#include <memory>

#include <http-parser/http_parser.h>
#include <queue>

#include "net.h"
#include "utils.h"

namespace http {

enum Code {
    OK, ClientError, ServerError, NetworkError, Done
};

/**
 * Uploads a (maybe huge) flow of bytes in the request.
 */
class Request {
  public:

    /**
     *
     */
    Request();

    /**
     *
     * @param s
     */
    Request(std::shared_ptr<net::Socket> s);

    /**
     * Destructor
     */
    ~Request();

    inline void ContentLength(int64_t l) {
        content_length = l;
    }

    inline void Selector(const std::string &sel) {
        selector.assign(sel);
    }

    inline void Method(const std::string &m) {
        method.assign(m);
    }

    inline void Query(const std::string &k, const std::string &v) {
        query[k] = v;
    }

    inline void Field(const std::string &k, const std::string &v) {
        fields[k] = v;
    }

    inline void Trailer(const std::string &k) {
        trailers.insert(k);
    }

    /**
     * Replaces the underlying socket.
     * @param s the new socket.
     */
    inline void Socket(std::shared_ptr<net::Socket> s) {
        socket = s;
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

  private:
    std::string method;
    std::string selector;
    std::map<std::string, std::string> fields;
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

        Slice() : buf{nullptr}, len{0} {
        }

        Slice(const uint8_t *b, uint32_t l) : buf{b}, len{l} {
        }
    };

    enum Step {
        Beginning, Headers, Body, Done
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
        std::queue<Slice> body_bytes;
        std::vector<uint8_t> buffer;
        struct http_parser parser;

        Context() : fields(), query(), trailers(),
                    content_length{-1}, received{0},
                    step{Beginning}, tmp(), body_bytes(), buffer(2048) {
            Init();
        }

        void Init () {
            http_parser_init(&parser, HTTP_RESPONSE);
            parser.data = this;
        }
        void Reset () {
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

    Reply();

    /**
     * Only constructor allowed.
     *
     * @param s an established socket.
     */
    Reply(std::shared_ptr<net::Socket> s);

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
    inline void Socket(std::shared_ptr<net::Socket> s) {
        socket = s;
    }

    /**
     * Consumes the input until the reply's headers have been read.
     * It is illegal to call this several times on the same object.
     * @return a status code indicating if iteratiosn might continue.
     */
    Code ReadHeaders();

    /** Get a slice of body. The data is not copied, and must be consumed by the
     * application before the next call to ReadBody(). This call invalidates all
     * the slices previously returned, because they all point to the same
     * internal buffer
     * @see consumeInput()
     * @param out the output slice modified in place
     * @return a status code indicating if iteratiosn might continue.
     */
    Code ReadBody(Slice &out);

    /**
     * Wraps ReadBody(Slice) and appends the slice into the output vector.
     * @see ReadBody()
     * @param out the output vector.
     * @return a status code indicating if iteratiosn might continue.
     */
    Code AppendBody(std::vector<uint8_t> &out);

    /**
     * Get a read-only access on the internal state of the reply.
     * @return the reply context
     */
    const Context &Get() const {
        return ctx;
    }

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
    Call(std::shared_ptr<net::Socket> s);

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

    Code Run(const std::string &in, std::string &out);

  protected:
    Request request;
    Reply reply;

  private:
	FORBID_COPY_CTOR(Call);
	FORBID_MOVE_CTOR(Call);
};

} // namespace http

#endif //OIO_KINETIC_HTTP_H
