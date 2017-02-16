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

#ifndef SRC_UTILS_PROXYGEN_HPP_
#define SRC_UTILS_PROXYGEN_HPP_

#include <proxygen/lib/http/HTTPConnector.h>
#include <proxygen/lib/http/session/HTTPTransaction.h>

#include <cstdint>
#include <future>  //  NOLINT(build/c++11)
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <string>
#include <thread>  //  NOLINT(build/c++11)
#include <utility>
#include <vector>

#include "utils.hpp"
#include "oio/api/blob.hpp"

namespace http {

enum Code {
    OK, ClientError, ServerError, NetworkError, Done ,Timeout
};

  
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

/**
 * Handle a Downstream and  Upstream Connection to a HTTPServer with Proxygen
 * The execution of the instance are in EventLoop so every function call is add
 * to the EventLoop.Most of the functions are called by the library.
 * But the Controller class can calls functions of the ProxygenHTTP.
 */
class ProxygenHTTP : public proxygen::HTTPConnector::Callback,
                     public proxygen::HTTPTransactionHandler {
    friend class ControllerProxygenHTTP;
  public:
    
    ~ProxygenHTTP();
    
    void Trailer(std::string trailer) {
        this->sendTrailers.insert(trailer);
    }

    void EventBase(const std::shared_ptr<folly::EventBase> eventBase) {
        this->eventBase = eventBase;
    }

    void Timer(folly::HHWheelTimer::SharedPtr timer) {
        this->timer = timer;
    }

    void OptionMap(const folly::AsyncSocket::OptionMap &optionMap) {
        this->optionMap = optionMap;
    }

    void SocketAddress(std::unique_ptr<folly::SocketAddress> socketAddress) {
        this->socketAddress = std::move(socketAddress);
    }

    void Timeout(const std::chrono::milliseconds &timeout) {
        this->promiseTimeout = timeout;
    }

    void SSL(std::string certPath, std::list<std::string> protocols) noexcept {
        sslCertPath = certPath;
        sslProtocols = protocols;
        sslEnabled = true;
    }

    void Field(const std::string &key, const std::string &value) noexcept {
        fields[key] = value;
    }

    void Method(const proxygen::HTTPMethod method) noexcept {
        this->method = method;
    }

    void ContentLength(int64_t contentLength) noexcept {
        sent = 0;
        this->contentLength = contentLength;
        Field("Content-Length", std::to_string(contentLength));
    }
        
    void LimitIngress(int64_t limitIngress){
        this->limitIngress = limitIngress;
    }

    void LimitEgress(int64_t limitEgress){
        this->limitEgress = limitEgress;
    }

    void Connect() noexcept;

    /**
     * Write can be called anytime by the controller even when
     * the Upstream transaction is on paused. For this reason
     * we buffered to a certain amount set by limitEgress the data we send
     * whe it is on paused.
     */
    void Write(const std::shared_ptr<::oio::api::blob::Slice> slice) noexcept;

    void SendEOM() noexcept;

    void Read(std::shared_ptr<oio::api::blob::Slice> slice) noexcept;

    void isEof() noexcept;

    void URL(std::string url) {
        url_ = url;
    }
    void Abort() noexcept;
    void initializeSSL() noexcept;
    
    void sslHandshakeFollowup(proxygen::HTTPUpstreamSession* session) noexcept;

    void setHeader();
    // function of the Callback class

    /**
     * The connection is successful and it is only after this function is called
     * that we can send the header.
     */
    void connectSuccess(proxygen::HTTPUpstreamSession* session);

    void connectError(const folly::AsyncSocketException& exception);
    // function of the HTTPTransactionHandler class

    void onEOM() noexcept;

    void setTransaction(proxygen::HTTPTransaction* txn) noexcept;

    void detachTransaction() noexcept;

    void onHeadersComplete(std::unique_ptr<proxygen::HTTPMessage> msg) noexcept;

    void onEgressPaused() noexcept;

    /**
     * onEgress Resumed is automatically called when we can send
     * It is the library that decide if we can send or not.
     * Because it is asynchrone there can have  buffered data waiting
     * to be send.
     */
    void onEgressResumed() noexcept;

    /**
     * This is called automatically by the library when data have been received.
     * So we need to buffered the data till a Read is called. But to not stock
     * too much data. We set a limit that block the reading of new data.
     */
    void onBody(std::unique_ptr<folly::IOBuf> chain) noexcept;

    void onTrailers(std::unique_ptr<proxygen::HTTPHeaders> trailers) noexcept;

    void onUpgrade(proxygen::UpgradeProtocol protocol) noexcept;

    /**
     * This function is called by the library whenever a error occured
     * but the Controller can still called function from the instance.
     * So for this reason a variable is set to know if an error happened.
     */
    void onError(const proxygen::HTTPException& error) noexcept;
  private:

    /**
     * Before a Read is called the instance may received many buffers of data
     * So when a read is called we coalesce all the buffers to one and add it
     * to the slice asked.
     */
    void CoalesceAndSetValue();

    void addFieldsOnHeader(proxygen::HTTPHeaders *headers);
    
    inline void ReturnHeader(std::shared_ptr<proxygen::HTTPMessage> msg) {
        headerPromise.set_value(msg);
    }
    
    inline void ReturnBool(bool b){
        boolPromise.set_value(b);
    }
    
    inline void ReturnCode(Code code) {
        codePromise.set_value(code);
    }
    // Common
    int64_t limitIngress {-1};
    int64_t IngressBuffered {0};
    int64_t limitEgress {-1};
    int64_t EgressBuffered {0};
    std::promise<bool> boolPromise;
    std::promise<Code> codePromise;
    std::promise<std::shared_ptr<proxygen::HTTPMessage>> headerPromise;
    std::chrono::milliseconds promiseTimeout {5000};
    std::shared_ptr<proxygen::HTTPMessage> headerReceived {nullptr};
    proxygen::HTTPMethod method;
    std::map<std::string, std::string> fields;
    bool errorHappened;
    std::shared_ptr<folly::IOBuf> readBuffer {nullptr};
    bool eof;
    std::shared_ptr<oio::api::blob::Slice> readSlice {nullptr};
    bool isEgressPaused {false};
    int64_t contentLength {-1};
    std::queue<std::shared_ptr<oio::api::blob::Slice>> writeQueue;
    int64_t sent {-1};
    // Param for setting Connection
    std::string url_;
    std::string sslCertPath;
    std::list<std::string> sslProtocols;
    std::set<std::string> sendTrailers;
    proxygen::HTTPException* error {nullptr};
    bool sslEnabled {false};
    std::shared_ptr<folly::EventBase> eventBase {nullptr};
    std::unique_ptr<proxygen::HTTPConnector> connector {nullptr};
    std::shared_ptr<folly::SSLContext> sslContext {nullptr};
    folly::HHWheelTimer::SharedPtr timer {nullptr};
    folly::AsyncSocket::OptionMap optionMap;
    std::unique_ptr<folly::SocketAddress> socketAddress {nullptr};
    std::chrono::milliseconds connectionTimeout {5000};
    proxygen::HTTPTransaction* transaction {nullptr};
    std::string serverName;
    proxygen::UpgradeProtocol protocol;
    std::shared_ptr<proxygen::HTTPHeaders> trailers {nullptr};
    folly::AsyncSocketException::AsyncSocketExceptionType exception;
};

/**
 * The Controller of a ProxygenHTTP class goal is to give a interface to call
 * functions of the ProxygenHTTP instance from any thread to the thread
 * containing the EventLoop.
 */
class ControllerProxygenHTTP{
  public:
    ControllerProxygenHTTP() {}

    explicit ControllerProxygenHTTP(std::shared_ptr<ProxygenHTTP>
                                    proxygenHTTP)
            :proxygenHTTP_{proxygenHTTP} {}

    void SetProxygenHTTP(std::shared_ptr<ProxygenHTTP> proxygenHTTP) {
        proxygenHTTP_ = proxygenHTTP;
    }

    void Abort() {
        proxygenHTTP_->eventBase->template
                runInEventBaseThread<ProxygenHTTP>
                (proxygenHTTPAbort, proxygenHTTP_.get());
    }

    void Connect() {
        if (proxygenHTTP_.get()->errorHappened)
            return;
        proxygenHTTP_->eventBase->template
                runInEventBaseThread<ProxygenHTTP>
                (proxygenHTTPConnect, proxygenHTTP_.get());
    }

    void SendEOM() {
        if (proxygenHTTP_.get()->errorHappened)
            return;
        proxygenHTTP_->eventBase->template
                runInEventBaseThread<ProxygenHTTP>
                (proxygenHTTPSendEOM, proxygenHTTP_.get());
    }

    void Read(std::shared_ptr<oio::api::blob::Slice> slice) {
        if (proxygenHTTP_.get()->errorHappened)
            return;
        auto *tmp_pair =  new std::pair<ProxygenHTTP*,
                                        std::shared_ptr<oio::api::blob::Slice>>
                (proxygenHTTP_.get(), slice);
        proxygenHTTP_->eventBase->template runInEventBaseThread
                <std::pair<ProxygenHTTP*,
                           std::shared_ptr<oio::api::blob::Slice>>>
                (proxygenHTTPRead, tmp_pair);
    }

    void Write(std::shared_ptr<oio::api::blob::Slice> slice) {
        if (proxygenHTTP_.get()->errorHappened)
            return;
        auto  *tmp_pair = new std::pair<ProxygenHTTP*,
                                        std::shared_ptr<oio::api::blob::Slice>>
                (proxygenHTTP_.get(), slice);
        proxygenHTTP_->eventBase->template runInEventBaseThread
                <std::pair<ProxygenHTTP*,
                           std::shared_ptr<oio::api::blob::Slice>>>
                (proxygenHTTPWrite, tmp_pair);
    }
    void IsEof(){
        if(proxygenHTTP_.get()->errorHappened){
            LOG(ERROR) << "IsEof asked when error already happened on proxygen";
            return;
        }
        proxygenHTTP_->eventBase->template
                runInEventBaseThread<ProxygenHTTP>
                (proxygenHTTPIsEof, proxygenHTTP_.get());
    }

    Code ReturnCode() {
        if (proxygenHTTP_.get()->errorHappened)
            return Code::NetworkError;
        auto codeFuture = proxygenHTTP_->codePromise.get_future();
        std::future_status status = codeFuture.wait_for(
            proxygenHTTP_->promiseTimeout);
        proxygenHTTP_->codePromise = std::promise<Code>();
        if (status == std::future_status::ready)
            return codeFuture.get();
        return Code::Timeout;
    }

    bool ReturnIsEof() {
        if (proxygenHTTP_.get()->errorHappened)
            return true;
        auto boolFuture = proxygenHTTP_.get()->boolPromise.get_future();
        std::future_status status = boolFuture.wait_for(
            proxygenHTTP_->promiseTimeout);
        proxygenHTTP_->boolPromise = std::promise<bool>();
        if (status == std::future_status::ready)
            return boolFuture.get();
        return true;
    }

    std::shared_ptr<proxygen::HTTPMessage> ReturnHeader() {
        auto headerFuture = proxygenHTTP_->headerPromise.get_future();
        std::future_status status = headerFuture.wait_for(
            proxygenHTTP_->promiseTimeout);
        proxygenHTTP_->headerPromise =  std::promise<
            std::shared_ptr<proxygen::HTTPMessage>>();
        if (status == std::future_status::ready)
            return headerFuture.get();
        return nullptr;
    }

  private:
    static void proxygenHTTPIsEof(ProxygenHTTP* proxygenHTTP) {
        proxygenHTTP->isEof();
    }

    static void proxygenHTTPConnect(ProxygenHTTP* proxygenHTTP) {
        proxygenHTTP->Connect();
    }

    static void proxygenHTTPAbort(ProxygenHTTP* proxygenHTTP) {
        proxygenHTTP->Abort();
    }

    static void proxygenHTTPSendEOM(ProxygenHTTP* proxygenHTTP) {
        proxygenHTTP->SendEOM();
    }

    static void proxygenHTTPWrite(std::pair<ProxygenHTTP*,
                                  std::shared_ptr<oio::api::blob::Slice>>
                                  *pair) {
        pair->first->Write(pair->second);
        delete pair;
    }

    static void proxygenHTTPRead(std::pair<ProxygenHTTP*,
                                 std::shared_ptr<oio::api::blob::Slice>>
                                 *pair) {
        DLOG(INFO) << pair->second.get();
        pair->first->Read(pair->second);
        delete pair;
    }
    std::shared_ptr<ProxygenHTTP> proxygenHTTP_ {nullptr};
};

}  // namespace http

#endif  // SRC_UTILS_PROXYGEN_HPP_
