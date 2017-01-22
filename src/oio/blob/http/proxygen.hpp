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

#ifndef SRC_OIO_BLOB_HTTP_PROXYGEN_HPP__
#define SRC_OIO_BLOB_HTTP_PROXYGEN_HPP__

#include <memory>
#include <string>

#include <folly/SocketAddress.h>

#include "oio/api/blob.hpp"
#include "utils/http.hpp"
#include "utils/proxygen.hpp"


namespace oio {
namespace http {

class Slice{
public:
  Slice(){}
  Slice(Slice *slice){
    inner.insert(inner.end(),slice->data(),&slice->data()[slice->size()]);
  }
  Slice(uint8_t *data,int32_t size){
    inner.insert(inner.end(),data,&data[size]);
  }
  uint8_t *data() {
    return inner.data();
  }
  uint32_t size() {
    return inner.size();
  }
  void append(uint8_t *data,int32_t size){
    inner.insert(inner.end(),data,&data[size]);
  }
  void append(Slice& slice){
    inner.insert(inner.end(),slice.data(),&slice.data()[slice.size()]);
  }
  bool empty(){
    return inner.empty();
  }
  private:
  std::vector<uint8_t> inner;
};

namespace async{
  class HTTPUpload {
  public:
    void SetXattr(const std::string& k, const std::string& v) ;
    oio::api::Status Prepare();
    oio::api::Status WaitPrepare();  
    oio::api::Status Commit();
    oio::api::Status WaitCommit();
    oio::api::Status Abort();
    oio::api::Status WaitWrite();
    oio::api::Status Write(const uint8_t *buf, uint32_t len);
    oio::api::Status Write(std::shared_ptr<Slice> slice);
    ::http::ProxygenHTTP<Slice> ProxygenHTTP(){return proxygenHTTP_;}
  private:
    oio::api::Status AbortAndReturn(oio::api::Status status){
      Abort();
      return status;
    }
    oio::api::blob::TransactionStep step_;
    ::http::ProxygenHTTP<Slice> proxygenHTTP_ {}; 
    ::http::ControllerProxygenHTTP<Slice> controller_ (std::shared_ptr<ProxygenHTTP<Slice>>(&proxygenHTTP_));
  };

  
} // namespace async

  
namespace sync {

class UploadBuilder {
 public:
    UploadBuilder(){}

    ~UploadBuilder(){}

    void Host(const std::string &s);

    void Name(const std::string &s);

    void Field(const std::string &k, const std::string &v);

    void Trailer(const std::string &k);

    void SocketAddress(const std::unique_ptr<folly::SocketAddress> socketAddress);

    void EventBase(const std::shared_ptr<folly::EventBase> eventBase);

    void Timer(const std::shared_ptr<folly::HHWheelTimer> timer);

    std::unique_ptr<oio::api::blob::Upload> Build();

 private:
    std::string host;
    std::string name;
    std::map<std::string, std::string> fields;
    std::set<std::string> trailers;
    std::unique_ptr<folly::SocketAddress> socketAddress;
    std::shared_ptr<folly::EventBase> eventBase;
    folly::HHWheelTimer::SharedPtr timer;
};

 class HTTPUpload : public oio::api::blob::Upload {
  friend class UploadBuilder;
public:
  void SetXattr(const std::string& k, const std::string& v) override;
  oio::api::Status Prepare() override;
  oio::api::Status Commit() override;
  oio::api::Status Abort() override;
   oio::api::Status Write(std::shared_ptr<Slice> slice);
   void Write(const uint8_t *buf, uint32_t len){};
private:
  oio::api::Status AbortAndReturn(oio::api::Status status){
    Abort();
    return status;
  }
   ::oio::http::async::HTTPUpload inner;
};
  
  class ReplicatedHTTPUpload : public oio::api::blob::Upload{
  public:
    void Target(std::shared_ptr<oio::http::async::HTTPUpload> target){
      targets_.push_back(target);
    }
    
    void MinimunSuccessful(int min){
      minimunSuccessful_ = min;
    }
    oio::api::Status Prepare() override;
    oio::api::Status Commit() override;
    oio::api::Status Abort() override;
    oio::api::Status Write(std::shared_ptr<Slice> slice);
    private:
    std::vector<std::shared_ptr<oio::http::async::HTTPUpload>> targets_;
    unsigned timeout_;
    unsigned minimunSuccessful_;
  };


}  // namespace imperative
  
}  // namespace http
}  // namespace oio

#endif  // SRC_OIO_BLOB_HTTP_PROXYGEN_HPP__
