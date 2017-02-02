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
#include "utils/proxygen.cpp"
using Step = oio::api::blob::TransactionStep;

using http::Code;
using std::vector;

using http::ProxygenHTTP;
using oio::api::Cause;
using oio::api::Status;
using oio::api::blob::Upload;
using namespace oio::http;
using std::string;
using std::unique_ptr;

void async::HTTPUpload::SetXattr(const std::string& k, const std::string& v) {
  if(step_ == Step::Init)
    proxygenHTTP_.get()->Field(k,v);
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
  if(header == nullptr)
    return Status(Cause::InternalError);
  if(header->getStatusCode() / 100 == 2)
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
   
Status async::HTTPUpload::Write(std::shared_ptr<HTTPSlice> slice){
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

Status async::HTTPDownload::SetRange(uint32_t offset, uint32_t size){
  proxygenHTTP_.get()->Field("Range","bytes="+std::to_string(offset)+"-"+std::to_string(size));
  return Status();
}

Status async::HTTPDownload::Read(std::shared_ptr<HTTPSlice> slice){
  if(step_ != Step::Prepared)
    return Status(Cause::InternalError);
  controller_.Read(slice);
  return Status();
}

bool async::HTTPDownload::IsEof(){
  return controller_.ReturnIsEof();
}

Status async::HTTPDownload::WaitRead(){
  Code code = controller_.ReturnCode();
  if(code != Code::OK){
    if(code == Code::NetworkError)
      return Status(Cause::NetworkError);
    return Status(Cause::InternalError);
  }
  return Status();
}

Status async::HTTPDownload::Prepare(){
  if(step_ != Step::Init)
    return Status (Cause::InternalError);
  controller_.Connect();
  return Status();
}

Status async::HTTPDownload::WaitPrepare(){
  Code code = controller_.ReturnCode();
  if(code != Code::OK){
    if(code == Code::NetworkError)
      return Status(Cause::NetworkError);
    return Status(Cause::InternalError);
  }
  controller_.SendEOM();
  return Status();
}

Status async::HTTPDownload::WaitReadHeader(){
  std::shared_ptr<proxygen::HTTPMessage> httpMessage = controller_.ReturnHeader();
  if(httpMessage == nullptr)
    return Status(Cause::NetworkError);
  if(httpMessage.get()->getStatusCode() == 404)
    return Status(Cause::NotFound);
  return Status();
}

Status async::HTTPRemoval::Prepare(){
  if(step_ != Step::Init)
    return Status(Cause::InternalError);
  step_ = Step::Prepared;
  controller_.Connect();
  return Status();
}

Status async::HTTPRemoval::WaitPrepare(){
  Code code = controller_.ReturnCode();
  if(code != Code::OK){
    if(code == Code::NetworkError)
      return Status(Cause::NetworkError);
    return Status(Cause::InternalError);
  }
  return Status();
}

Status async::HTTPRemoval::Commit() {
  if(step_ != Step::Prepared)
    return Status(Cause::InternalError);
  step_ = Step::Done;
  controller_.SendEOM();
  return Status();
}

Status async::HTTPRemoval::WaitCommit(){
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

Status async::HTTPRemoval::Abort() {
  if(step_ != Step::Prepared)
    return Status(Cause::InternalError);
  step_ = Step::Done;
  controller_.Abort();
  return Status();
}

void async::HTTPBuilder::URL(const std::string& s){
  url = s;
}

void async::HTTPBuilder::Name(const std::string& s){
  name = s;
}

void async::HTTPBuilder::Field(const std::string& k, const std::string &v){
  fields[k] = v;
}

void async::HTTPBuilder::Trailer(const std::string& k){
  trailers.insert(k);
}

void async::HTTPBuilder::Host(const std::string& s){
  host = s;
}

void async::HTTPBuilder::SocketAddress( unique_ptr<folly::SocketAddress> socketAddress){
  this->socketAddress = std::move(socketAddress);
}

void async::HTTPBuilder::EventBase(const std::shared_ptr<folly::EventBase> eventBase){
  this->eventBase = eventBase; 
}

void async::HTTPBuilder::Timeout(const std::chrono::milliseconds &timeout){
  this->timeout=timeout;
}

void async::HTTPBuilder::Timer(const folly::HHWheelTimer::SharedPtr timer){
  this->timer=timer;
}

std::unique_ptr<oio::http::async::HTTPUpload>  async::HTTPBuilder::BuildUpload(){
  auto ul = new HTTPUpload;
  if(timeout != std::chrono::milliseconds(0))
    ul->proxygenHTTP_->Timeout(timeout);
  ul->proxygenHTTP_.get()->SocketAddress(std::move(socketAddress));
  ul->proxygenHTTP_.get()->EventBase(eventBase);
  ul->proxygenHTTP_.get()->Timer(timer);
  ul->proxygenHTTP_.get()->URL(url);
  ul->proxygenHTTP_.get()->Method(proxygen::HTTPMethod::PUT);
  ul->proxygenHTTP_.get()->Field("Host",host);
  
  for(const auto &e : fields){
    ul->proxygenHTTP_.get()->Field(e.first,e.second);
  }
  return std::unique_ptr<oio::http::async::HTTPUpload>(ul);
}

std::unique_ptr<oio::http::async::HTTPDownload>  async::HTTPBuilder::BuildDownload(){
  auto dl = new HTTPDownload;
  if(timeout !=std::chrono::milliseconds(0) )
    dl->proxygenHTTP_->Timeout(timeout);
  dl->proxygenHTTP_.get()->SocketAddress(std::move(socketAddress));
  dl->proxygenHTTP_.get()->EventBase(eventBase);
  dl->proxygenHTTP_.get()->Timer(timer);
  dl->proxygenHTTP_.get()->URL(url);
  dl->proxygenHTTP_.get()->Method(proxygen::HTTPMethod::GET);
  dl->proxygenHTTP_.get()->Field("Host",host);

  for(const auto &e : fields){
    dl->proxygenHTTP_.get()->Field(e.first,e.second);
  }
  return std::unique_ptr<oio::http::async::HTTPDownload>(dl);
}

std::unique_ptr<oio::http::async::HTTPRemoval>  async::HTTPBuilder::BuildRemoval(){
  auto rm = new HTTPRemoval;
  if(timeout != std::chrono::milliseconds(0))
    rm->proxygenHTTP_->Timeout(timeout);
  rm->proxygenHTTP_.get()->SocketAddress(std::move(socketAddress));
  rm->proxygenHTTP_.get()->EventBase(eventBase);
  rm->proxygenHTTP_.get()->Timer(timer);
  rm->proxygenHTTP_.get()->URL(url);
  rm->proxygenHTTP_.get()->Method(proxygen::HTTPMethod::DELETE);
  rm->proxygenHTTP_.get()->Field("Host",host);

  for(const auto &e : fields){
    rm->proxygenHTTP_.get()->Field(e.first,e.second);
  }
  return std::unique_ptr<oio::http::async::HTTPRemoval>(rm);
}

Status sync::HTTPDownload::Prepare(){
  inner.get()->Prepare();
  return inner.get()->WaitPrepare();
}

bool sync::HTTPDownload::IsEof(){
  return inner.get()->IsEof();
}

Status sync::HTTPDownload::Read(std::shared_ptr<HTTPSlice> slice){
  inner.get()->Read(slice);
  return inner.get()->WaitRead();
}

int32_t sync::HTTPDownload::Read(std::vector<uint8_t> * buf){
  std::shared_ptr<HTTPSlice> slice = std::shared_ptr<HTTPSlice>(new HTTPSlice(buf));
  Status status = Read(slice);
  if(status.Why() != Cause::OK)
    return -1;
  return buf->size();
}

Status sync::HTTPDownload::SetRange(uint32_t offset, uint32_t size){
  return inner.get()->SetRange(offset,size);
}

sync::HTTPDownload::~HTTPDownload(){}

sync::HTTPRemoval::~HTTPRemoval(){}

std::unique_ptr<oio::api::blob::Upload>  sync::HTTPBuilder::BuildUpload(){
  async::HTTPBuilder builder;
  if(timeout != std::chrono::milliseconds(0))
    builder.Timeout(timeout);
  builder.SocketAddress(std::move(socketAddress));
  builder.EventBase(eventBase);
  builder.Timer(timer);
  builder.URL(url);
  builder.Host(host);
  for(const auto &e : fields){
    builder.Field(e.first,e.second);
  }
  auto ul = new HTTPUpload(builder.BuildUpload());
  return std::unique_ptr<oio::api::blob::Upload>(ul);
}

std::unique_ptr<oio::api::blob::Download>  sync::HTTPBuilder::BuildDownload(){
  async::HTTPBuilder builder;
  if(timeout != std::chrono::milliseconds(0))
    builder.Timeout(timeout);
  builder.SocketAddress(std::move(socketAddress));
  builder.EventBase(eventBase);
  builder.Timer(timer);
  builder.URL(url);
  builder.Host(host);
  for(const auto &e : fields){
    builder.Field(e.first,e.second);
  }
  auto dl = new HTTPDownload(builder.BuildDownload());
  return std::unique_ptr<oio::api::blob::Download>(dl);
}

std::unique_ptr<oio::api::blob::Removal>  sync::HTTPBuilder::BuildRemoval(){
  async::HTTPBuilder builder;
  if(timeout != std::chrono::milliseconds(0))
    builder.Timeout(timeout);
  builder.SocketAddress(std::move(socketAddress));
  builder.EventBase(eventBase);
  builder.Timer(timer);
  builder.URL(url);
  builder.Host(host);
  for(const auto &e : fields){
    builder.Field(e.first,e.second);
  }
  auto rm = new HTTPRemoval(builder.BuildRemoval());
  return std::unique_ptr<oio::api::blob::Removal>(rm);
}

void sync::HTTPBuilder::Timeout(const std::chrono::milliseconds &timeout){
  this->timeout=timeout;
}

void sync::HTTPBuilder::URL(const std::string& s){
  url = s;
}

void sync::HTTPBuilder::Name(const std::string& s){
  name = s;
}

void sync::HTTPBuilder::Field(const std::string &k, const std::string &v){
  fields[k] = v;
}

void sync::HTTPBuilder::Trailer(const std::string& k){
  trailers.insert(k);
}

void sync::HTTPBuilder::Host(const std::string& s){
  host = s;
}

void sync::HTTPBuilder::SocketAddress( unique_ptr<folly::SocketAddress> socketAddress){
  this->socketAddress = std::move(socketAddress);
}

void sync::HTTPBuilder::EventBase(const std::shared_ptr<folly::EventBase> eventBase){
  this->eventBase = eventBase; 
}

void sync::HTTPBuilder::Timer(const folly::HHWheelTimer::SharedPtr timer){
  this->timer=timer;
}


Status sync::HTTPRemoval::Prepare(){
  inner.get()->Prepare();
  return inner.get()->WaitPrepare();
}

Status sync::HTTPRemoval::Commit(){
  inner.get()->Commit();
  return inner.get()->WaitCommit();
}

Status sync::HTTPRemoval::Abort(){
  return inner.get()->Abort();
}

Status sync::HTTPUpload::Prepare(){
  inner.get()->Prepare();
  return inner.get()->WaitPrepare();
}

sync::HTTPUpload::~HTTPUpload() {}

Status sync::HTTPUpload::Write(std::shared_ptr<HTTPSlice> slice){
  inner.get()->Write(slice);
  return inner.get()->WaitWrite();
}

Status sync::HTTPUpload::Commit(){
  inner.get()->Commit();
  return inner.get()->WaitCommit();
}
Status sync::HTTPUpload::Abort(){
  return inner.get()->Abort();
}

Status repli::ReplicatedHTTPUpload::Prepare(){
  for(auto &target :  targets_)
    target.get()->Prepare();
  unsigned successful = 0;
  for(auto &target : targets_){
    Status status = target.get()->WaitPrepare();
    if(status.Why() == Cause::OK)
      successful ++;
  }
  if(minimunSuccessful_ <= successful)
    return Status();
  return Status(Cause::InternalError);
}

Status repli::ReplicatedHTTPUpload::Commit(){
  for(auto &target : targets_)
    target.get()->Commit();
  unsigned successful = 0;
  Status status;
  for(auto &target : targets_){
    status = target.get()->WaitCommit();
    if(status.Why() == Cause::OK)
      successful ++;
  }
  if(minimunSuccessful_ <= successful)
    return Status();
  return Status(Cause::InternalError);
}

Status repli::ReplicatedHTTPUpload::Abort(){
  for(auto &target : targets_)
    target.get()->Abort();
  return Status(Cause::OK);
}

Status repli::ReplicatedHTTPUpload::Write(std::shared_ptr<HTTPSlice> slice){
  for(auto &target : targets_)
    target.get()->Write(slice);
  unsigned successful = 0;
  Status status;
  for(auto &target : targets_){
    status = target.get()->WaitWrite();
    if(status.Why() ==Cause::OK)
      successful ++;
  }
  if(minimunSuccessful_ <= successful)
    return Status();
  return Status(Cause::InternalError);
}

