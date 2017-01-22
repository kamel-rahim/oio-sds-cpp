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

#include "proxygen.hpp"

#include <string>

#include "utils/utils.hpp"
#include "utils/http.hpp"
#include "utils/proxygen.hpp"

using Step = oio::api::blob::TransactionStep;

using http::Code;


using http::ProxygenHTTP;
using oio::api::Cause;
using oio::api::Status;
using oio::api::blob::Upload;
using namespace oio::http;
using std::string;
using std::unique_ptr;

void async::HTTPUpload::SetXattr(const std::string& k, const std::string& v) {
  if(step_ == Step::Init)
    proxygenHTTP_.Field(k,v);
}
  
Status async::HTTPUpload::Prepare() {
  if(step_ != Step::Init)
    return Status(Cause::InternalError);
  step_ = Step::Prepared;
  controller_.Connect();
  return Status();
}

Status async::HTTPUpload::WaitPrepare(){
  Code code = controller_.ReturnCode();
  if(code != Code::OK){
    if(code == Code::NetworkError)
      return Status(Cause::NetworkError);
    return Status(Cause::InternalError);
  }
  return Status();
}

Status async::HTTPUpload::Commit() {
  if(step_ != Step::Prepared)
    return Status(Cause::InternalError);
  step_ = Step::Done;
  controller_.SendEOM();
  return Status();
}

Status async::HTTPUpload::WaitCommit(){
  Code code = controller_.ReturnCode();
  if(code != Code::OK){
    if(code == Code::NetworkError)
      return AbortAndReturn(Status(Cause::NetworkError));
    return AbortAndReturn(Status(Cause::InternalError));
  }
  std::shared_ptr<proxygen::HTTPMessage> header = controller_.ReturnHeader();
  if(header.get()->getStatusCode() / 100 == 2)
    return Status();
  return Status(Cause::InternalError);
}

Status async::HTTPUpload::Abort() {
  if(step_ != Step::Prepared)
    return Status(Cause::InternalError);
  step_ = Step::Done;
  controller_.Abort();
  return Status();
}
   
Status async::HTTPUpload::Write(std::shared_ptr<Slice> slice){
  if(step_ != Step::Prepared)
    return Status(Cause::InternalError);
  controller_.Write(slice);
  return Status();
}

Status async::HTTPUpload::WaitWrite(){
  Code code = controller_.ReturnCode();
  if(code != Code::OK){
    if(code == Code::NetworkError)
      return Status(Cause::NetworkError);
    return Status(Cause::InternalError);
  }
  return Status();
  
}

void sync::UploadBuilder::Name(const std::string& s){
  name = s;
}

void sync::UploadBuilder::Field(const std::string& k, const std::string &v){
  fields[k] = v;
}

void sync::UploadBuilder::Trailer(const std::string& k){
  trailers.insert(k);
}

void sync::UploadBuilder::Host(const std::string& s){
  host = s;
}

void sync::UploadBuilder::SocketAddress( unique_ptr<folly::SocketAddress> socketAddress){
  this->socketAddress = std::move(socketAddress);
}

void sync::UploadBuilder::EventBase(const std::shared_ptr<folly::EventBase> eventBase){
  this->eventBase = eventBase; 
}

void sync::UploadBuilder::Timer(const folly::HHWheelTimer::SharedPtr timer){
  this->timer=timer;
}

std::unique_ptr<oio::api::blob::Upload>  sync::UploadBuilder::Build(){
  auto ul = new HTTPUpload;
  ul->inner.ProxygenHTTP().SocketAddress(std::move(socketAddress));
  ul->inner.ProxygenHTTP().EventBase(eventBase);
  ul->inner.ProxygenHTTP().Timer(timer); 
  ul->inner.ProxygenHTTP().Method(proxygen::HTTPMethod::PUT);
  ul->inner.ProxygenHTTP().Field("Host",host);
  ul->inner.ProxygenHTTP().Field("Content-Length","-1");
  for(const auto &e : fields){
    ul->inner.ProxygenHTTP().Field(e.first,e.second);
  }
  return std::unique_ptr<oio::api::blob::Upload>(ul);
}

Status sync::HTTPUpload::Prepare(){
  inner.Prepare();
  return inner.WaitPrepare();
}

Status sync::HTTPUpload::Write(std::shared_ptr<Slice> slice){
  inner.Write(slice);
  return inner.WaitWrite();
}
Status sync::HTTPUpload::Commit(){
  inner.Commit();
  return inner.WaitCommit();
}
Status sync::HTTPUpload::Abort(){
  return inner.Abort();
}

Status sync::ReplicatedHTTPUpload::Prepare(){
  for(auto target :  targets_)
    target.get()->Prepare();
  unsigned successful;
  for(auto target : targets_){
    Status status = target.get()->WaitPrepare();
    if(status.Why() == Cause::OK)
      successful ++;
  }
  if(minimunSuccessful_ > successful)
    return Status();
  return Status(Cause::InternalError);
}

Status sync::ReplicatedHTTPUpload::Commit(){
  for(auto target : targets_)
    target.get()->Commit();
  unsigned successful;
  Status status;
  for(auto target : targets_){
    status = target.get()->WaitCommit();
    if(status.Why() ==Cause::OK)
      successful ++;
  }
  if(minimunSuccessful_ > successful)
    return Status();
  return Status(Cause::InternalError);
}

Status sync::ReplicatedHTTPUpload::Abort(){
  for(auto target : targets_)
    target.get()->Abort();
  return Status(Cause::OK);
}

Status sync::ReplicatedHTTPUpload::Write(std::shared_ptr<Slice> slice){
  for(auto target : targets_)
    target.get()->Write(slice);
  unsigned successful;
  Status status;
  for(auto target : targets_){
    status = target.get()->WaitWrite();
    if(status.Why() ==Cause::OK)
      successful ++;
  }
  if(minimunSuccessful_ > successful)
    return Status();
  return Status(Cause::InternalError);
}


