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

#ifndef SRC_OIO_BLOB_HTTP_PROXYGEN_HPP__
#define SRC_OIO_BLOB_HTTP_PROXYGEN_HPP__

#include <folly/SocketAddress.h>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "oio/api/blob.hpp"
#include "utils/proxygen.hpp"


namespace oio {
namespace http {

class HTTPSlice :public  oio::api::blob::Slice{
 public:
    ~HTTPSlice() {}

    HTTPSlice() {}

    explicit HTTPSlice(HTTPSlice *slice) {
        inner.insert(inner.end(), slice->data(), &slice->data()[slice->size()]);
    }

    explicit HTTPSlice(std::vector<uint8_t> *buf): inner{*buf} {}

    HTTPSlice(const uint8_t *data, uint32_t size) {
        inner.insert(inner.end(), data, &data[size]);
    }

    HTTPSlice(uint8_t *data, int32_t size) {
        inner.insert(inner.end(), data, &data[size]);
    }

    uint8_t *data() {
        return inner.data();
    }

    uint32_t size() {
        return inner.size();
    }

    void append(uint8_t *data, uint32_t size) override {
        inner.insert(inner.end(), data, &data[size]);
    }

    void append(HTTPSlice *slice) {
        inner.insert(inner.end(), slice->data(), &slice->data()[slice->size()]);
    }

    bool empty() {
        return inner.empty();
    }

 private:
    std::vector<uint8_t> inner;
};

namespace react {
   
class HTTPDownload {
    friend class HTTPBuilder;
 public:
    HTTPDownload() {
        proxygenHTTP_ = std::shared_ptr<::http::ProxygenHTTP>(
                    new ::http::ProxygenHTTP());
        controller_.SetProxygenHTTP(proxygenHTTP_);
    }
    oio::api::Status Prepare();
    oio::api::Status WaitPrepare();
    oio::api::Status Read(std::shared_ptr<oio::api::blob::Slice> slice);
    oio::api::Status WaitRead();
    oio::api::Status Abort();
    oio::api::Status WaitPrepareHeader();
    oio::api::Status SetRange(uint32_t offset, uint32_t size);
    bool IsEof();
 private:
    oio::api::blob::TransactionStep step_;
    std::shared_ptr<::http::ProxygenHTTP> proxygenHTTP_;
    ::http::ControllerProxygenHTTP controller_;
};

class HTTPUpload {
    friend class HTTPBuilder;
 public:
    HTTPUpload() {
        proxygenHTTP_ = std::shared_ptr<::http::ProxygenHTTP>(
                new ::http::ProxygenHTTP());
        controller_.SetProxygenHTTP(proxygenHTTP_);
    }
    void SetXattr(const std::string& k, const std::string& v);
    oio::api::Status Prepare();
    oio::api::Status WaitPrepare();
    oio::api::Status Commit();
    oio::api::Status WaitCommit();
    oio::api::Status Abort();
    oio::api::Status WaitWrite();
    oio::api::Status Write(const uint8_t *buf, uint32_t len);
    oio::api::Status Write(std::shared_ptr<oio::api::blob::Slice> slice);

 private:
    oio::api::Status AbortAndReturn(oio::api::Status status) {
        Abort();
        return status;
    }
    oio::api::blob::TransactionStep step_;
    std::shared_ptr<::http::ProxygenHTTP> proxygenHTTP_;
    ::http::ControllerProxygenHTTP controller_;
};

class HTTPRemoval {
    friend class HTTPBuilder;
 public:
    HTTPRemoval() {
        proxygenHTTP_ = std::shared_ptr<::http::ProxygenHTTP>(
            new ::http::ProxygenHTTP());
        controller_.SetProxygenHTTP(proxygenHTTP_);
    }
    oio::api::Status Prepare();
    oio::api::Status WaitPrepare();
    oio::api::Status Commit();
    oio::api::Status WaitCommit();
    oio::api::Status Abort();

 private:
    oio::api::Status AbortAndReturn(oio::api::Status status) {
        Abort();
        return status;
    }
    oio::api::blob::TransactionStep step_;
    std::shared_ptr<::http::ProxygenHTTP> proxygenHTTP_;
    ::http::ControllerProxygenHTTP controller_;
};


class HTTPBuilder {
 public:
    HTTPBuilder() {}

    ~HTTPBuilder() {}

    void URL(const std::string &s);

    void Host(const std::string &s);

    void Name(const std::string &s);

    void Field(const std::string &k, const std::string &v);

    void Trailer(const std::string &k);

    void SocketAddress(const std::unique_ptr<folly::SocketAddress>
                       socketAddress);

    void EventBase(const std::shared_ptr<folly::EventBase> eventBase);

    void Timer(const std::shared_ptr<folly::HHWheelTimer> timer);

    void Timeout(const std::chrono::milliseconds &timeout);

    void ContentLength(int64_t contentLength);

    std::unique_ptr<oio::http::react::HTTPDownload> BuildDownload();

    std::unique_ptr<oio::http::react::HTTPUpload> BuildUpload();

    std::unique_ptr<oio::http::react::HTTPRemoval> BuildRemoval();

 private:
    int64_t contentLength {-1};
    std::chrono::milliseconds timeout {0};
    std::string url;
    std::string host;
    std::string name;
    std::map<std::string, std::string> fields;
    std::set<std::string> trailers;
    std::unique_ptr<folly::SocketAddress> socketAddress;
    std::shared_ptr<folly::EventBase> eventBase;
    folly::HHWheelTimer::SharedPtr timer;
};

}  // namespace react

namespace sync {

class HTTPUpload : public oio::api::blob::Upload {
    friend class HTTPBuilder;
 public:
    ~HTTPUpload();
    HTTPUpload() {}
    void SetXattr(const std::string& k, const std::string& v) override {
        xattrs[k] = v;
    }
    oio::api::Status Prepare() override;
    oio::api::Status Commit() override;
    oio::api::Status Abort() override;
    oio::api::Status Write(std::shared_ptr<oio::api::blob::Slice> slice) override;
    void Write(const uint8_t *buf, uint32_t len) override{
        Write(std::shared_ptr<oio::api::blob::Slice>(new HTTPSlice(buf, len)));
    };
    explicit HTTPUpload(std::unique_ptr<::oio::http::react::HTTPUpload> upload)
            :inner{std::move(upload)} { }
 private:
    oio::api::Status AbortAndReturn(oio::api::Status status) {
        Abort();
        return status;
    }
    std::map<std::string, std::string> xattrs;
    std::unique_ptr<::oio::http::react::HTTPUpload> inner;
};

class HTTPDownload : public oio::api::blob::Download {
    friend class HTTPBuilder;
 public:
    ~HTTPDownload();
    HTTPDownload() {}
    oio::api::Status Abort();
    oio::api::Status Prepare() override;
    bool IsEof();
    int32_t Read(std::vector<uint8_t> *buf);
    oio::api::Status Read(std::shared_ptr<oio::api::blob::Slice> slice) override;
    oio::api::Status SetRange(uint32_t offset, uint32_t size);
    explicit HTTPDownload(std::unique_ptr<::oio::http::react::HTTPDownload>
                          download) : inner{std::move(download)} { }
 private:
    std::unique_ptr<::oio::http::react::HTTPDownload> inner;
};

class HTTPRemoval : public oio::api::blob::Removal {
    friend class HTTPBuilder;
 public:
    ~HTTPRemoval();
    HTTPRemoval() {}
    oio::api::Status Prepare() override;
    oio::api::Status Commit() override;
    oio::api::Status Abort() override;
    explicit HTTPRemoval(std::unique_ptr<::oio::http::react::HTTPRemoval>
                         removal) : inner{std::move(removal)} {}
 private:
    std::unique_ptr<::oio::http::react::HTTPRemoval> inner;
};

class HTTPBuilder {
 public:
    HTTPBuilder() {}

    ~HTTPBuilder() {}

    void URL(const std::string &s);

    void Host(const std::string &s);

    void Name(const std::string &s);

    void Field(const std::string &k, const std::string &v);

    void Trailer(const std::string &k);

    void SocketAddress(const std::unique_ptr<folly::SocketAddress>
                       socketAddress);

    void EventBase(const std::shared_ptr<folly::EventBase> eventBase);

    void Timer(const std::shared_ptr<folly::HHWheelTimer> timer);

    void Timeout(const std::chrono::milliseconds &timeout);

    void ContentLength(int64_t contentLength);

    std::unique_ptr<oio::api::blob::Upload> BuildUpload();
    std::unique_ptr<oio::api::blob::Download> BuildDownload();
    std::unique_ptr<oio::api::blob::Removal> BuildRemoval();

 private:
    int64_t contentLength {-1};
    std::chrono::milliseconds timeout {0};
    std::string url;
    std::string host;
    std::string name;
    std::map<std::string, std::string> fields;
    std::set<std::string> trailers;
    std::unique_ptr<folly::SocketAddress> socketAddress;
    std::shared_ptr<folly::EventBase> eventBase;
    folly::HHWheelTimer::SharedPtr timer;
};

}  // namespace sync

namespace repli {

class ReplicatedHTTPUpload : public oio::api::blob::Upload {
 public:
    void Timeout(unsigned timeout) {
        timeout_ = timeout;
    }
    void SetXattr(const std::string& k, const std::string& v) override {
        xattrs[k] = v;
    }

    void Target(std::unique_ptr<oio::http::react::HTTPUpload> target) {
        targets_.push_back(std::move(target));
    }

    void MinimunSuccessful(int min) {
        minimunSuccessful_ = min;
    }
    oio::api::Status Prepare() override;
    oio::api::Status Commit() override;
    oio::api::Status Abort() override;
    void Write(const uint8_t *buf, uint32_t len) override {
        Write(std::shared_ptr<oio::api::blob::Slice>(new HTTPSlice(buf, len)));
    };
    oio::api::Status Write(std::shared_ptr<oio::api::blob::Slice> slice);

 private:
    std::vector<std::unique_ptr<oio::http::react::HTTPUpload>> targets_;
    unsigned timeout_;
    unsigned minimunSuccessful_;
    std::map<std::string, std::string> xattrs;
};

class ReplicatedHTTPRemoval : public oio::api::blob::Removal {
  public:
    ~ReplicatedHTTPRemoval() {}
    oio::api::Status Prepare() override;
    oio::api::Status Commit() override;
    oio::api::Status Abort() override;
    void Target(std::unique_ptr<oio::http::react::HTTPRemoval> target){
        targets_.push_back(std::move(target));
    }
  private:
    std::vector<std::unique_ptr<oio::http::react::HTTPRemoval>> targets_;
    unsigned timeout;
    unsigned minimunSuccessful_;
};

}  // namespace repli

namespace ec{

class ECHTTPUpload : public oio::api::blob::Upload{
  public:
    void SetXattr(const std::string& k, const std::string& v) override{
        xattrs[k] = v;
    }
    
    void Target(std::unique_ptr<oio::http::react::HTTPUpload> target){
        targets_.push_back(std::move(target));
    }
    
    void MinimunSuccessful(int min){
        minimunSuccessful_ = min;
    }
    oio::api::Status Prepare() override;
    oio::api::Status Commit() override;
    oio::api::Status Abort() override;
    oio::api::Status Write(std::vector<std::shared_ptr<HTTPSlice>> slices);
    
  private:
    std::vector<std::unique_ptr<oio::http::react::HTTPUpload>> targets_;
    unsigned timeout_;
    unsigned minimunSuccessful_;
    std::map<std::string,std::string> xattrs;  
};

class ECHTTPDownload : public oio::api::blob::Download{
  public:
    void Target(std::unique_ptr<oio::http::react::HTTPDownload> target){
        targets_.push_back(std::move(target));
    }
    
    void MinimunSuccessful(int min){
        minimunSuccessful_ = min;
    }
    oio::api::Status Prepare() override;
    oio::api::Status Read(std::vector<std::shared_ptr<HTTPSlice>> slices);
    oio::api::Status Abort();
    
  private:
    std::vector<std::unique_ptr<oio::http::react::HTTPDownload>> targets_;
    unsigned timeout_;
    unsigned minimunSuccessful_;
    std::map<std::string,std::string> xattrs;      
};

}  // namespace ec
}  // namespace http
}  // namespace oio

#endif  // SRC_OIO_BLOB_HTTP_PROXYGEN_HPP__
