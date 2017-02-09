/**
 * This file is part of the OpenIO client libraries
 * Copyright (C) 2017 OpenIO SAS
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

#include "proxygen.hpp"

#include <string>

#include "utils/utils.hpp"
#include "utils/http.hpp"
#include "utils/proxygen.hpp"
using Step = oio::api::blob::TransactionStep;

using http::Code;
using oio::http::HTTPSlice;
using std::vector;
using http::ProxygenHTTP;
using oio::api::Cause;
using oio::api::Status;
using oio::api::blob::Upload;
using std::string;
using std::unique_ptr;
using oio::api::blob::Slice;

void oio::http::async::HTTPUpload::SetXattr(
    const std::string& k, const std::string& v) {
    if (step_ == Step::Init)
        proxygenHTTP_->Field(k, v);
}

Status oio::http::async::HTTPUpload::Prepare() {
    if (step_ != Step::Init)
        return Status(Cause::InternalError);
    step_ = Step::Prepared;
    controller_.Connect();
    return Status();
}

Status oio::http::async::HTTPUpload::WaitPrepare() {
    Code code = controller_.ReturnCode();
    if (code != Code::OK) {
        if (code == Code::NetworkError)
            return Status(Cause::NetworkError);
        return Status(Cause::InternalError);
    }
    return Status();
}

Status oio::http::async::HTTPUpload::Commit() {
    if (step_ != Step::Prepared)
        return Status(Cause::InternalError);
    step_ = Step::Done;
    controller_.SendEOM();
    return Status();
}

Status oio::http::async::HTTPUpload::WaitCommit() {
    Code code = controller_.ReturnCode();
    if (code != Code::OK) {
        if (code == Code::NetworkError)
            return AbortAndReturn(Status(Cause::NetworkError));
        return AbortAndReturn(Status(Cause::InternalError));
    }
    std::shared_ptr<proxygen::HTTPMessage> header = controller_.ReturnHeader();
    if (header == nullptr) {
      DLOG(INFO) << "Header Returned is null";
        return Status(Cause::InternalError);
    }
    if (header->getStatusCode() / 100 == 2) {
      DLOG(INFO) << "Header Status Code is valid";
        return Status();
    }
    DLOG(INFO) << "The Status Code is not good" << header->getStatusCode();
    return Status(Cause::InternalError);
}

Status oio::http::async::HTTPUpload::Abort() {
    if (step_ != Step::Prepared)
        return Status(Cause::InternalError);
    step_ = Step::Done;
    controller_.Abort();
    return Status();
}

Status oio::http::async::HTTPUpload::Write(std::shared_ptr<Slice> slice) {
    if (step_ != Step::Prepared)
        return Status(Cause::InternalError);
    controller_.Write(slice);
    return Status();
}

Status oio::http::async::HTTPUpload::WaitWrite() {
    Code code = controller_.ReturnCode();
    if (code != Code::OK) {
        if (code == Code::NetworkError)
            return Status(Cause::NetworkError);
        return Status(Cause::InternalError);
    }
    return Status();
}

Status oio::http::async::HTTPDownload::SetRange(
    uint32_t offset, uint32_t size) {
    if (step_ != Step::Init)
        return Status(Cause::InternalError);
    proxygenHTTP_->Field("Range", "bytes="+std::to_string(offset)+
                         "-"+std::to_string(size));
    return Status();
}

Status oio::http::async::HTTPDownload::Read(std::shared_ptr<Slice> slice) {
    if (step_ != Step::Prepared)
        return Status(Cause::InternalError);
    controller_.Read(slice);
    return Status();
}

bool oio::http::async::HTTPDownload::IsEof() {
    return controller_.ReturnIsEof();
}

Status oio::http::async::HTTPDownload::WaitRead() {
    Code code = controller_.ReturnCode();
    if (code != Code::OK) {
        if (code == Code::NetworkError)
            return Status(Cause::NetworkError);
        return Status(Cause::InternalError);
    }
    return Status();
}

Status oio::http::async::HTTPDownload::Prepare() {
    if (step_ != Step::Init)
        return Status (Cause::InternalError);
    controller_.Connect();
    return Status();
}

Status oio::http::async::HTTPDownload::WaitPrepare() {
    Code code = controller_.ReturnCode();
    if (code != Code::OK) {
        if (code == Code::NetworkError)
            return Status(Cause::NetworkError);
        return Status(Cause::InternalError);
    }
    controller_.SendEOM();
    return Status();
}

Status oio::http::async::HTTPDownload::WaitReadHeader() {
    std::shared_ptr<proxygen::HTTPMessage> httpMessage =
            controller_.ReturnHeader();
    if (httpMessage == nullptr)
        return Status(Cause::NetworkError);
    if (httpMessage->getStatusCode() == 404)
        return Status(Cause::NotFound);
    return Status();
}

Status oio::http::async::HTTPRemoval::Prepare() {
    if (step_ != Step::Init)
        return Status(Cause::InternalError);
    step_ = Step::Prepared;
    controller_.Connect();
    return Status();
}

Status oio::http::async::HTTPRemoval::WaitPrepare() {
    Code code = controller_.ReturnCode();
    if (code != Code::OK) {
        if (code == Code::NetworkError)
            return Status(Cause::NetworkError);
        return Status(Cause::InternalError);
    }
    return Status();
}

Status oio::http::async::HTTPRemoval::Commit() {
    if (step_ != Step::Prepared)
        return Status(Cause::InternalError);
    step_ = Step::Done;
    controller_.SendEOM();
    return Status();
}

Status oio::http::async::HTTPRemoval::WaitCommit() {
    Code code = controller_.ReturnCode();
    if (code != Code::OK) {
        if (code == Code::NetworkError)
            return AbortAndReturn(Status(Cause::NetworkError));
        return AbortAndReturn(Status(Cause::InternalError));
    }
    std::shared_ptr<proxygen::HTTPMessage> header = controller_.ReturnHeader();
    if (header->getStatusCode() / 100 == 2)
        return Status();
    return Status(Cause::InternalError);
}

Status oio::http::async::HTTPRemoval::Abort() {
    if (step_ != Step::Prepared)
        return Status(Cause::InternalError);
    step_ = Step::Done;
    controller_.Abort();
    return Status();
}

void oio::http::async::HTTPBuilder::ContentLength(int64_t contentLength) {
    this->contentLength = contentLength;
}

void oio::http::async::HTTPBuilder::URL(const std::string& s) {
    url = s;
}

void oio::http::async::HTTPBuilder::Name(const std::string& s) {
    name = s;
}

void oio::http::async::HTTPBuilder::Field(
    const std::string& k, const std::string &v) {
    fields[k] = v;
}

void oio::http::async::HTTPBuilder::Trailer(const std::string& k) {
    trailers.insert(k);
}

void oio::http::async::HTTPBuilder::Host(const std::string& s) {
    host = s;
}

void oio::http::async::HTTPBuilder::SocketAddress(
    unique_ptr<folly::SocketAddress> socketAddress) {
    this->socketAddress = std::move(socketAddress);
}

void oio::http::async::HTTPBuilder::EventBase(
    const std::shared_ptr<folly::EventBase> eventBase) {
    this->eventBase = eventBase;
}

void oio::http::async::HTTPBuilder::Timeout(
    const std::chrono::milliseconds &timeout) {
    this->timeout = timeout;
}

void oio::http::async::HTTPBuilder::Timer(
    const folly::HHWheelTimer::SharedPtr timer) {
    this->timer = timer;
}

std::unique_ptr<oio::http::async::HTTPUpload>
oio::http::async::HTTPBuilder::BuildUpload() {
    auto ul = new HTTPUpload;
    if (timeout != std::chrono::milliseconds(0))
        ul->proxygenHTTP_->Timeout(timeout);
    if (contentLength > 0)
        ul->proxygenHTTP_->ContentLength(contentLength);
    ul->proxygenHTTP_->SocketAddress(std::move(socketAddress));
    ul->proxygenHTTP_->EventBase(eventBase);
    ul->proxygenHTTP_->Timer(timer);
    ul->proxygenHTTP_->URL(url);
    ul->proxygenHTTP_->Method(proxygen::HTTPMethod::PUT);
    ul->proxygenHTTP_->Field("Host", host);

    for (const auto &e : fields) {
        ul->proxygenHTTP_->Field(e.first, e.second);
    }
    return std::unique_ptr<oio::http::async::HTTPUpload>(ul);
}

std::unique_ptr<oio::http::async::HTTPDownload>
oio::http::async::HTTPBuilder::BuildDownload() {
    auto dl = new HTTPDownload;
    if (timeout !=std::chrono::milliseconds(0) )
        dl->proxygenHTTP_->Timeout(timeout);
    if (contentLength > 0)
        dl->proxygenHTTP_->ContentLength(contentLength);
    dl->proxygenHTTP_->SocketAddress(std::move(socketAddress));
    dl->proxygenHTTP_->EventBase(eventBase);
    dl->proxygenHTTP_->Timer(timer);
    dl->proxygenHTTP_->URL(url);
    dl->proxygenHTTP_->Method(proxygen::HTTPMethod::GET);
    dl->proxygenHTTP_->Field("Host", host);

    for (const auto &e : fields) {
        dl->proxygenHTTP_->Field(e.first, e.second);
    }
    return std::unique_ptr<oio::http::async::HTTPDownload>(dl);
}

std::unique_ptr<oio::http::async::HTTPRemoval>
oio::http::async::HTTPBuilder::BuildRemoval() {
    auto rm = new HTTPRemoval;
    if (timeout != std::chrono::milliseconds(0))
        rm->proxygenHTTP_->Timeout(timeout);
    if (contentLength > 0)
        rm->proxygenHTTP_->ContentLength(contentLength);
    rm->proxygenHTTP_->SocketAddress(std::move(socketAddress));
    rm->proxygenHTTP_->EventBase(eventBase);
    rm->proxygenHTTP_->Timer(timer);
    rm->proxygenHTTP_->URL(url);
    rm->proxygenHTTP_->Method(proxygen::HTTPMethod::DELETE);
    rm->proxygenHTTP_->Field("Host", host);

    for (const auto &e : fields) {
        rm->proxygenHTTP_->Field(e.first, e.second);
    }
    return std::unique_ptr<oio::http::async::HTTPRemoval>(rm);
}

Status oio::http::sync::HTTPDownload::Prepare() {
    inner->Prepare();
    return inner->WaitPrepare();
}

bool oio::http::sync::HTTPDownload::IsEof() {
    return inner->IsEof();
}

Status oio::http::sync::HTTPDownload::Read(std::shared_ptr<Slice> slice) {
    inner->Read(slice);
    return inner->WaitRead();
}

int32_t oio::http::sync::HTTPDownload::Read(std::vector<uint8_t> * buf) {
    std::shared_ptr<Slice> slice(new HTTPSlice(buf));
    Status status = Read(slice);
    if (status.Why() != Cause::OK)
        return -1;
    return buf->size();
}

Status oio::http::sync::HTTPDownload::SetRange(uint32_t offset, uint32_t size) {
    return inner->SetRange(offset, size);
}

oio::http::sync::HTTPDownload::~HTTPDownload() {}

oio::http::sync::HTTPRemoval::~HTTPRemoval() {}

std::unique_ptr<oio::api::blob::Upload>
oio::http::sync::HTTPBuilder::BuildUpload() {
    oio::http::async::HTTPBuilder builder;
    if (timeout != std::chrono::milliseconds(0))
        builder.Timeout(timeout);
    if (contentLength > 0)
        builder.ContentLength(contentLength);
    builder.SocketAddress(std::move(socketAddress));
    builder.EventBase(eventBase);
    builder.Timer(timer);
    builder.URL(url);
    builder.Host(host);
    for (const auto &e : fields) {
        builder.Field(e.first, e.second);
    }
    auto ul = new HTTPUpload(builder.BuildUpload());
    return std::unique_ptr<oio::api::blob::Upload>(ul);
}

std::unique_ptr<oio::api::blob::Download>
oio::http::sync::HTTPBuilder::BuildDownload() {
    oio::http::async::HTTPBuilder builder;
    if (timeout != std::chrono::milliseconds(0))
        builder.Timeout(timeout);
    if (contentLength > 0)
        builder.ContentLength(contentLength);
    builder.SocketAddress(std::move(socketAddress));
    builder.EventBase(eventBase);
    builder.Timer(timer);
    builder.URL(url);
    builder.Host(host);
    for (const auto &e : fields) {
        builder.Field(e.first, e.second);
    }
    auto dl = new HTTPDownload(builder.BuildDownload());
    return std::unique_ptr<oio::api::blob::Download>(dl);
}

std::unique_ptr<oio::api::blob::Removal>
oio::http::sync::HTTPBuilder::BuildRemoval() {
    oio::http::async::HTTPBuilder builder;
    if (timeout != std::chrono::milliseconds(0))
        builder.Timeout(timeout);
    if (contentLength > 0)
        builder.ContentLength(contentLength);
    builder.SocketAddress(std::move(socketAddress));
    builder.EventBase(eventBase);
    builder.Timer(timer);
    builder.URL(url);
    builder.Host(host);
    for (const auto &e : fields) {
        builder.Field(e.first, e.second);
    }
    auto rm = new HTTPRemoval(builder.BuildRemoval());
    return std::unique_ptr<oio::api::blob::Removal>(rm);
}

void oio::http::sync::HTTPBuilder::ContentLength(int64_t contentLength) {
    this->contentLength = contentLength;
}
void oio::http::sync::HTTPBuilder::Timeout(
    const std::chrono::milliseconds &timeout) {
    this->timeout = timeout;
}

void oio::http::sync::HTTPBuilder::URL(const std::string& s) {
    url = s;
}

void oio::http::sync::HTTPBuilder::Name(const std::string& s) {
    name = s;
}

void oio::http::sync::HTTPBuilder::Field(
    const std::string &k, const std::string &v) {
    fields[k] = v;
}

void oio::http::sync::HTTPBuilder::Trailer(const std::string& k) {
    trailers.insert(k);
}

void oio::http::sync::HTTPBuilder::Host(const std::string& s) {
    host = s;
}

void oio::http::sync::HTTPBuilder::SocketAddress(
    unique_ptr<folly::SocketAddress> socketAddress) {
    this->socketAddress = std::move(socketAddress);
}

void oio::http::sync::HTTPBuilder::EventBase(
    const std::shared_ptr<folly::EventBase> eventBase) {
    this->eventBase = eventBase;
}

void oio::http::sync::HTTPBuilder::Timer(
    const folly::HHWheelTimer::SharedPtr timer) {
    this->timer = timer;
}


Status oio::http::sync::HTTPRemoval::Prepare() {
    inner->Prepare();
    return inner->WaitPrepare();
}

Status oio::http::sync::HTTPRemoval::Commit() {
    inner->Commit();
    return inner->WaitCommit();
}

Status oio::http::sync::HTTPRemoval::Abort() {
    return inner->Abort();
}

Status oio::http::sync::HTTPUpload::Prepare() {
    inner->Prepare();
    return inner->WaitPrepare();
}

oio::http::sync::HTTPUpload::~HTTPUpload() {}

Status oio::http::sync::HTTPUpload::Write(std::shared_ptr<Slice> slice) {
    inner->Write(slice);
    return inner->WaitWrite();
}

Status oio::http::sync::HTTPUpload::Commit() {
    inner->Commit();
    return inner->WaitCommit();
}
Status oio::http::sync::HTTPUpload::Abort() {
    return inner->Abort();
}

Status oio::http::repli::ReplicatedHTTPUpload::Prepare() {
    for (auto &target :  targets_)
        target->Prepare();
    unsigned successful = 0;
    for (auto &target : targets_) {
        Status status = target->WaitPrepare();
        if (status.Why() == Cause::OK)
            successful++;
    }
    if (minimunSuccessful_ <= successful)
        return Status();
    return Status(Cause::InternalError);
}

Status oio::http::repli::ReplicatedHTTPUpload::Commit() {
    for (auto &target : targets_)
        target->Commit();
    unsigned successful = 0;
    Status status;
    for (auto &target : targets_) {
        status = target->WaitCommit();
        if (status.Why() == Cause::OK)
            successful++;
    }
    if (minimunSuccessful_ <= successful)
        return Status();
    return Status(Cause::InternalError);
}

Status oio::http::repli::ReplicatedHTTPUpload::Abort() {
    for (auto &target : targets_)
        target->Abort();
    return Status(Cause::OK);
}

Status oio::http::repli::ReplicatedHTTPUpload::Write(
    std::shared_ptr<Slice> slice) {
    for (auto &target : targets_)
        target->Write(slice);
    unsigned successful = 0;
    Status status;
    for (auto &target : targets_) {
        status = target->WaitWrite();
        if (status.Why() ==Cause::OK)
            successful++;
    }
    if (minimunSuccessful_ <= successful)
        return Status();
    return Status(Cause::InternalError);
}

