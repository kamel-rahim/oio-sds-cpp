/**
 * This file is part of the test tools for the OpenIO client libraries
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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "HTTPTransactionMocks.h"
#include "utils/http.hpp"
#include "utils/proxygen.hpp"
#include "utils/proxygen.cpp"
#include <future>
#include <string>
#include <folly/io/IOBuf.h>

using ::testing::_;
using testing::Return;
using http::Code;
using http::ProxygenHTTP;
using http::ControllerProxygenHTTP;

class TestSlice{
public:
  TestSlice(){}
  TestSlice(TestSlice *slice){
    inner.insert(inner.end(),slice->data(),&slice->data()[slice->size()]);
  }
  TestSlice(uint8_t *data,int32_t size){
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
  void append(TestSlice& slice){
    inner.insert(inner.end(),slice.data(),&slice.data()[slice.size()]);
  }
  bool empty(){
    return inner.empty();
  }
  private:
  std::vector<uint8_t> inner;
};

class ProxygenHTTPFixture: public testing::Test{
public:
  ProxygenHTTPFixture(){
  
  }
  void SetUp() override{
    proxygenHTTP = std::shared_ptr<ProxygenHTTP<TestSlice>>(new ProxygenHTTP<TestSlice>());
  
    transaction = new proxygen::MockHTTPTransaction(proxygen::TransportDirection::DOWNSTREAM,id,seqNo,egressQueue,timeout);
    proxygenHTTP->setTransaction(transaction);
    controllerProxygenHTTP =new ControllerProxygenHTTP<TestSlice>(proxygenHTTP);
  }
  void TearDown() override{
    //    delete controllerProxygenHTTP;
  }

protected:
  ControllerProxygenHTTP<TestSlice>* controllerProxygenHTTP {nullptr};
  std::shared_ptr<ProxygenHTTP<TestSlice>> proxygenHTTP ;
  proxygen::HTTPCodec::StreamID id {1};
  uint32_t seqNo {2};
  proxygen::HTTP2PriorityQueue egressQueue;
  proxygen::WheelTimerInstance timeout;
  proxygen::MockHTTPTransaction * transaction {nullptr};
};


TEST_F(ProxygenHTTPFixture,WriteSliceSend){
   EXPECT_CALL(*transaction,sendBody(_)).WillOnce(Return());
  uint8_t message[]{"TEST_MESSAGE"};
  TestSlice slice(message,13);
  proxygenHTTP->Write(std::make_shared<TestSlice>(slice));
  controllerProxygenHTTP->ReturnCode();
}

TEST_F(ProxygenHTTPFixture,ReadSliceReceive){
  std::string message{"TEST_MESSAGE"};
  std::unique_ptr<folly::IOBuf> chain =  folly::IOBuf::wrapBuffer(message.data(),message.size());
  proxygenHTTP->onBody(std::move(chain));
  std::shared_ptr<TestSlice> slice(new TestSlice());
  proxygenHTTP->Read(slice);
  controllerProxygenHTTP->ReturnCode();
  std::ostringstream to_string;
  for(unsigned int i = 0;i<slice.get()->size();i++){
    to_string << slice.get()->data()[i];
  }
  std::string s = to_string.str();
  ASSERT_EQ(message,std::string(s));
  chain =  folly::IOBuf::wrapBuffer(message.data(),message.size());
  proxygenHTTP->onBody(std::move(chain));
}

TEST_F(ProxygenHTTPFixture,onBodyAccumulatingIOBuf){
  std::string message{"TEST_MESSAGE"};
  for(int i=0;i<3;i++){
    std::unique_ptr<folly::IOBuf> chain =  folly::IOBuf::wrapBuffer(message.data(),message.size());
    proxygenHTTP->onBody(std::move(chain));
  }
  std::shared_ptr<TestSlice> slice(new TestSlice());
  proxygenHTTP->Read(slice);
  std::ostringstream to_string;
  for(unsigned int i = 0;i<slice->size();i++){
    to_string << slice->data()[i];
  }
  std::string s = to_string.str();
  ASSERT_EQ(message+message+message,s);
}

TEST_F(ProxygenHTTPFixture,OnEgressPausedAccumulateSlice){
  EXPECT_CALL(*transaction,sendBody(_)).Times(3).WillRepeatedly(Return());
  transaction->pauseEgress();
  uint8_t message[]{"TEST_MESSAGE"};
  TestSlice slice(message,13);
  for(int i=0;i<3;i++){
    proxygenHTTP->Write(std::make_shared<TestSlice>(slice));
    controllerProxygenHTTP->ReturnCode();
  }
}

TEST_F(ProxygenHTTPFixture,OnEgressResumedSendItAll){
   EXPECT_CALL(*transaction,sendBody(_)).Times(3).WillRepeatedly(Return());
  ON_CALL(*transaction,isEgressPaused()).WillByDefault(Return(true));
  proxygenHTTP->onEgressPaused();
  uint8_t message[]{"TEST_MESSAGE"};
  TestSlice slice(message,13);
  std::shared_ptr<TestSlice> sharedSlices = std::make_shared<TestSlice>(slice);
  for(int i=0;i<3;i++){
    proxygenHTTP->Write(sharedSlices);
    controllerProxygenHTTP->ReturnCode();
    slice = TestSlice(message,13);
    sharedSlices = std::make_shared<TestSlice>(slice);
  }
  
  proxygenHTTP->onEgressResumed();
}

TEST_F(ProxygenHTTPFixture,isEOFWhenEnded){
  proxygenHTTP->onEOM();
  //  ASSERT_EQ(true, proxygenHTTP->isEof());
}

TEST_F(ProxygenHTTPFixture,isEOFWhenNotEnded){
  // ASSERT_EQ(false,proxygenHTTP->isEof());
}

TEST_F(ProxygenHTTPFixture,ReadHeader){
  proxygenHTTP->ReadHeader();
}

TEST_F(ProxygenHTTPFixture,Abort){
   EXPECT_CALL(*transaction,sendAbort()).WillOnce(Return());
  proxygenHTTP->Abort();
}

int main(int argc, char **argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);
  FLAGS_logtostderr = true;
  return RUN_ALL_TESTS();
}
