/**
 * Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/
 */

#include <http-parser/http_parser.h>

#include <iomanip>
#include <cstring>
#include <vector>

#include "utils/Http.h"
#include "oio/http/blob.h"

using http::Code;
using oio::http::imperative::RemovalBuilder;
using oio::http::imperative::UploadBuilder;
using oio::http::imperative::DownloadBuilder;
using oio::api::blob::Removal;
using oio::api::blob::Upload;
using oio::api::blob::Download;
using oio::api::blob::Status;
using oio::api::blob::Errno;
using oio::api::blob::Cause;
using Step = oio::api::blob::TransactionStep;

DEFINE_bool(blob_http_trace, false, "Log all calls to the blob HTTP client");

#define HTTP_LOG() DLOG_IF(INFO, FLAGS_blob_http_trace) << __FUNCTION__ << ' '

/**
 *
 */
class HttpRemoval : public Removal {
    friend class RemovalBuilder;
 private:
    http::Request request;
    http::Reply reply;
    Step step_;

    Status skipAndReturn(Status s) {
        reply.Skip();
        request.Abort();
        return s;
    }

 public:
    HttpRemoval(): step_{Step::Init} { HTTP_LOG(); }

    ~HttpRemoval() override {}

    Status Prepare() override {
        if (step_ != Step::Init)
            return Status(Cause::InternalError);

        // If the connection encountered a problem, then re-establish it.
        if (!request.Connected()) {
            if (!request.Reconnect()) {
                return Errno();
            }
        }

        auto code = request.WriteHeaders();
        HTTP_LOG() << "WriteHeaders code=" << code;
        if (code != Code::OK) {
            if (code == Code::NetworkError)
                return skipAndReturn(Status(Cause::NetworkError));
            return skipAndReturn(Status(Cause::InternalError));
        }

        code = reply.ReadHeaders();
        const auto status = reply.Get().parser.status_code;
        HTTP_LOG() << "ReadHeaders(100) code=" << code << " status=" << status;

        if (code != Code::OK && code != Code::ServerError) {
            if (code == Code::NetworkError)
                return skipAndReturn(Status(Cause::NetworkError));
            if (code == Code::ClientError)
                return skipAndReturn(Status(Cause::InternalError));
            return skipAndReturn(Status(Cause::InternalError));
        }

        switch (status) {
            case 100:
                step_ = Step::Prepared;
                reply.Skip();
                return Status();
            case 400:
                return skipAndReturn(Status(Cause::ProtocolError));
            case 403:
                return skipAndReturn(Status(Cause::Forbidden));
            case 404:
                return skipAndReturn(Status(Cause::NotFound));
            default:
                return skipAndReturn(Status(Cause::InternalError));
        }
    }

    Status Commit() override {
        if (step_ != Step::Prepared)
            return Status(Cause::InternalError);
        if (!request.Connected())
            return Status(Cause::NetworkError);

        step_ = Step::Done;

        auto code = request.FinishRequest();
        HTTP_LOG() << "FinishRequest code=" << code;
        if (code != Code::OK) {
            if (code == Code::NetworkError)
                return skipAndReturn(Status(Cause::NetworkError));
            if (code == Code::ClientError)
                return skipAndReturn(Status(Cause::InternalError));
            return skipAndReturn(Status(Cause::InternalError));
        }

        code = reply.ReadHeaders();
        HTTP_LOG() << "ReadHeaders(Final) code=" << code;
        if (code != Code::OK) {
            if (code == Code::NetworkError)
                return skipAndReturn(Status(Cause::NetworkError));
            if (code == Code::ClientError)
                return skipAndReturn(Status(Cause::InternalError));
            return skipAndReturn(Status(Cause::InternalError));
        }

        const auto status = reply.Get().parser.status_code;
        switch (status) {
            case 200:
            case 201:
            case 204:
                reply.Skip();
                return Status();
            case 403:
                return skipAndReturn(Status(Cause::Forbidden));
            case 404:
                return skipAndReturn(Status(Cause::NotFound));
            default:
                return skipAndReturn(Status(Cause::InternalError));
        }
    }

    Status Abort() override {
        if (step_ != Step::Prepared)
            return Status(Cause::InternalError);
        if (!request.Connected())
            return Status(Cause::NetworkError);

        request.Abort();
        step_ = Step::Done;
        return Status();
    }
};

RemovalBuilder::RemovalBuilder() {}

RemovalBuilder::~RemovalBuilder() {}

void RemovalBuilder::Name(const std::string &s) { name.assign(s); }

void RemovalBuilder::Host(const std::string &s) { host.assign(s); }

void RemovalBuilder::Field(const std::string &k, const std::string &v) {
    fields[k] = v;
}

void RemovalBuilder::Trailer(const std::string &k) { trailers.insert(k); }

std::unique_ptr<oio::api::blob::Removal> RemovalBuilder::Build(
        std::shared_ptr<net::Socket> socket) {
    auto rm = new HttpRemoval;
    rm->reply.Socket(socket);
    rm->request.Socket(socket);
    rm->request.Method("DELETE");
    rm->request.Selector(name);
    rm->request.Field("Host", host);
    rm->request.ContentLength(-1);
    rm->request.Field("Expect", "100-continue");
    for (const auto &t : trailers)
        rm->request.Trailer(t);
    for (const auto &e : fields)
        rm->request.Field(e.first, e.second);
    return std::unique_ptr<Removal>(rm);
}


/**
 *
 */
class HttpUpload : public Upload {
    friend class UploadBuilder;
 private:
    http::Request request;
    http::Reply reply;
    Step step_;

 public:
    HttpUpload(): request(), reply(), step_{Step::Init} { HTTP_LOG(); }

    ~HttpUpload() override {}

    void SetXattr(const std::string &k, const std::string &v) override {
        if (step_ != Step::Done)
            request.Field(k, v);
    }

    Status Prepare() override {
        if (step_ != Step::Init)
            return Status(Cause::InternalError);

        auto rc = request.WriteHeaders();
        if (rc != Code::OK) {
            if (rc == Code::NetworkError)
                return Status(Cause::NetworkError);
            return Status(Cause::InternalError);
        } else {
            step_ = Step::Prepared;
            return Status();
        }
    }

    Status Commit() override {
        HTTP_LOG() << " (" << __FILE__ << ") " << step_;
        if (step_ != Step::Prepared)
            return Status(Cause::InternalError);
        step_ = Step::Done;

        auto rc = request.FinishRequest();
        if (rc != Code::OK) {
            if (rc == Code::NetworkError)
                return Status(Cause::NetworkError);
            return Status(Cause::InternalError);
        }

        rc = reply.ReadHeaders();
        while (rc == http::Code::OK) {
            std::vector<uint8_t> out;
            rc = reply.AppendBody(&out);
        }

        if (reply.Get().parser.status_code / 100 == 2)
            return Status();
        return Status(Cause::InternalError);
    }

    Status Abort() override {
        if (step_ != Step::Prepared)
            return Status(Cause::InternalError);
        request.Abort();
        step_ = Step::Done;
        return Status();
    }

    void Write(const uint8_t *buf, uint32_t len) override;
};

void HttpUpload::Write(const uint8_t *buf, uint32_t len) {
    if (buf == nullptr || len == 0)
        return;
    request.Write(buf, len);
}

UploadBuilder::UploadBuilder() {}

UploadBuilder::~UploadBuilder() {}

void UploadBuilder::Name(const std::string &s) { name.assign(s); }

void UploadBuilder::Host(const std::string &s) { host.assign(s); }

void UploadBuilder::Field(const std::string &k, const std::string &v) {
    fields[k] = v;
}

void UploadBuilder::Trailer(const std::string &k) { trailers.emplace(k); }

std::unique_ptr<oio::api::blob::Upload> UploadBuilder::Build(
        std::shared_ptr<net::Socket> socket) {
    auto ul = new HttpUpload;
    ul->reply.Socket(socket);
    ul->request.Socket(socket);
    ul->request.Method("PUT");
    ul->request.Selector(name);
    ul->request.Field("Host", host);
    ul->request.ContentLength(-1);
    for (const auto &t : trailers)
        ul->request.Trailer(t);
    for (const auto &e : fields)
        ul->request.Field(e.first, e.second);
    return std::unique_ptr<Upload>(ul);
}


/**
 *
 */
class HttpDownload : public Download {
    friend class DownloadBuilder;

 private:
    http::Request request;
    http::Reply reply;
    Step step_;

    Status skipAndReturn(Status s) {
        request.Abort();
        reply.Skip();
        return s;
    }

 public:
    HttpDownload() : request(), reply(), step_{Step::Init} { HTTP_LOG(); }

    ~HttpDownload() override {}

    bool IsEof() override {
        return reply.Get().step == http::Reply::Step::Done;
    }

    Status Prepare() override  {
        if (step_ != Step::Init)
            return Status(Cause::InternalError);

        // If the connection encountered a problem, then re-establish it.
        if (!request.Connected()) {
            if (!request.Reconnect()) {
                return Errno();
            }
        }

        auto code = request.WriteHeaders();
        HTTP_LOG() << "WriteHeaders code=" << code;
        if (code != http::Code::OK && code != http::Code::Done) {
            if (code == http::Code::NetworkError)
                return skipAndReturn(Status(Cause::NetworkError));
            return skipAndReturn(Status(Cause::InternalError));
        }

        code = request.FinishRequest();
        HTTP_LOG() << "FinishRequest code=" << code;
        if (code != http::Code::OK && code != http::Code::Done) {
            if (code == http::Code::NetworkError)
                return skipAndReturn(Status(Cause::NetworkError));
            return skipAndReturn(Status(Cause::InternalError));
        }

        code = reply.ReadHeaders();
        HTTP_LOG() << "ReadHeaders code=" << code;
        if (code != http::Code::OK && code != http::Code::Done) {
            if (code == http::Code::NetworkError)
                return skipAndReturn(Status(Cause::NetworkError));
            return skipAndReturn(Status(Cause::InternalError));
        }

        const auto status = reply.Get().parser.status_code;
        if (status == 404)
            return skipAndReturn(Status(Cause::NotFound));

        step_ = Step::Prepared;
        return Status(Cause::OK);
    }

    int32_t Read(std::vector<uint8_t> *buf) override  {
        if (step_ != Step::Prepared)
            return -1;
        buf->clear();
        reply.AppendBody(buf);
        return buf->size();
    }
};

DownloadBuilder::DownloadBuilder() {}

DownloadBuilder::~DownloadBuilder() {}

void DownloadBuilder::Field(const std::string &k, const std::string &v) {
    fields[k] = v;
}

void DownloadBuilder::Host(const std::string &s) { host.assign(s); }

void DownloadBuilder::Name(const std::string &s) { name.assign(s); }

std::unique_ptr<oio::api::blob::Download> DownloadBuilder::Build(
        std::shared_ptr<net::Socket> socket) {
    auto dl = new HttpDownload;
    dl->request.Socket(socket);
    dl->request.Method("GET");
    dl->request.Selector(name);
    dl->request.Field("Host", host);
    dl->request.ContentLength(0);
    for (const auto &e : fields)
        dl->request.Field(e.first, e.second);
    dl->reply.Socket(socket);
    return std::unique_ptr<Download>(dl);
}
