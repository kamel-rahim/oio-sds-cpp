#include "proxygen.hpp"

#include <folly/io/async/SSLContext.h>
#include <proxygen/lib/http/session/HTTPUpstreamSession.h>
#include <wangle/ssl/SSLContextConfig.h>

using namespace http;
using proxygen::HTTPConnector;
using proxygen::HTTPTransaction;
using proxygen::HTTPUpstreamSession;

template<class Slice>
void ProxygenHTTP<Slice>::initializeSSL() noexcept{
  sslContext = std::make_shared<folly::SSLContext>();
  sslContext->setOptions(SSL_OP_NO_COMPRESSION);
  wangle::SSLContextConfig config;
  sslContext->ciphers(config.sslCiphers);
  sslContext->loadTrustedCertificates(sslCertPath.c_str());
  sslContext->setAdvertisedNextProtocols(sslProtocols); 
}

template<class Slice>
void ProxygenHTTP<Slice>::sslHandshakeFollowup(HTTPUpstreamSession* session) noexcept{
  folly::AsyncSSLSocket * sslSocket = dynamic_cast<folly::AsyncSSLSocket*>(session->getTransport());
  const unsigned char* nextProto = nullptr;
  unsigned nextProtoLength = 0;
  sslSocket->getSelectedNextProtocol(&nextProto,&nextProtoLength);
}
template<class Slice>	
void ProxygenHTTP<Slice>::Connect() noexcept{
  connector = new HTTPConnector((Callback*) this,timer.get());
  if(!sslEnabled){
    connector->connect(eventBase.get(),*socketAddress.get(),connectionTimeout,optionMap);
  }else{
    initializeSSL();
    connector->connectSSL(eventBase.get(),*socketAddress.get(),sslContext,
			  nullptr,connectionTimeout,optionMap,
			  folly::AsyncSocket::anyAddress(),serverName);
  }
}
template<class Slice>
void ProxygenHTTP<Slice>::connectSuccess(HTTPUpstreamSession* session){
  if(sslEnabled)
    sslHandshakeFollowup(session);
  if(transaction !=  nullptr)
    delete transaction;
  transaction = session->newTransaction(this);
  // TODO create and send the header
  proxygen::HTTPMessage message{};
  
  message.setMethod(method);
  addFieldsOnHeader(message.getHeaders());

  transaction->sendHeaders(message);
  codePromise.set_value(Code::OK);
}
template<class Slice>
void ProxygenHTTP<Slice>::addFieldsOnHeader(proxygen::HTTPHeaders &headers){
  for(auto field : fields){
    headers.add(field.first,field.second);
  }
}
template<class Slice>
void ProxygenHTTP<Slice>::onEgressPaused() noexcept{
  // not needed because transaction.isEgressPaused() already
  // give us this information
}
template<class Slice>
void ProxygenHTTP<Slice>::onEgressResumed() noexcept{
  while(!writeQueue.empty()){
    if(transaction->isEgressPaused())
      return;
    std::shared_ptr<Slice> slice = writeQueue.front();
    writeQueue.pop();
    transaction->sendBody(folly::IOBuf::wrapBuffer(slice.get()->data(),
    						   (unsigned int)slice.get()->size()));
  }
}
template<class Slice>
void ProxygenHTTP<Slice>::Abort() noexcept{
  transaction->sendAbort();
}
template<class Slice>
void ProxygenHTTP<Slice>::SendEOM() noexcept{
  transaction->sendEOM();
}
template<class Slice>
void ProxygenHTTP<Slice>::Write(const std::shared_ptr<Slice> slice) noexcept{
  if(transaction->isEgressPaused()){
    writeQueue.push(slice);
  }else{
    transaction->sendBody(folly::IOBuf::wrapBuffer(slice.get()->data(),slice.get()->size()));
  }
  codePromise.set_value(Code::OK);
}
template<class Slice>
void ProxygenHTTP<Slice>::connectError(const folly::AsyncSocketException& exception){
  codePromise.set_value(Code::NetworkError);
  this->exception = exception.getType();
}
template<class Slice>
void ProxygenHTTP<Slice>::onEOM() noexcept{
  eof=true;
}
template<class Slice>
void ProxygenHTTP<Slice>::ReadHeader() noexcept{
  headerPromise.set_value(headerReceived);
}
template<class Slice>	
void ProxygenHTTP<Slice>::setTransaction(proxygen::HTTPTransaction* txn) noexcept{
  if(transaction != nullptr)
    delete transaction;
  transaction = txn;
}
template<class Slice>
void ProxygenHTTP<Slice>::onHeadersComplete(std::unique_ptr<proxygen::HTTPMessage> msg) noexcept{
  headerReceived = std::move(msg);
}
template<class Slice>
void ProxygenHTTP<Slice>::detachTransaction() noexcept{
  // transaction use  unique_ptr so with RAII explicit destruction
  // is not needed
}
template<class Slice>
void ProxygenHTTP<Slice>::onBody(std::unique_ptr<folly::IOBuf> chain) noexcept{
  if(!readSlice){
    if(readBuffer)
      readBuffer.get()->prependChain(std::move(chain));
    else
      readBuffer = std::move(chain);
  }else{
    CoalesceAndSetValue();
  }
}
template<class Slice>
void ProxygenHTTP<Slice>::CoalesceAndSetValue(){
  readBuffer.get()->coalesce();
  readSlice->append(std::move(readBuffer.get()->writableData()),readBuffer.get()->length());
  codePromise.set_value(Code::OK);
  readSlice = nullptr;
  readBuffer = nullptr;
}
template<class Slice>      
void ProxygenHTTP<Slice>::Read(Slice *slice) noexcept{
  readSlice = slice;
  if(readBuffer){
    CoalesceAndSetValue();
  }
}
template<class Slice>	
void ProxygenHTTP<Slice>::onTrailers(std::unique_ptr<proxygen::HTTPHeaders> trailers) noexcept{
  this->trailers = std::move(trailers);
}
template<class Slice>
void ProxygenHTTP<Slice>::onUpgrade(proxygen::UpgradeProtocol protocol) noexcept{
  this->protocol = protocol;
}
template<class Slice>	
bool ProxygenHTTP<Slice>::isEof() noexcept{
  return eof;
}
template<class Slice>
void ProxygenHTTP<Slice>::onError(const proxygen::HTTPException& error) noexcept{
  codePromise.set_value(Code::NetworkError);
  this->error= new proxygen::HTTPException(error);
}
template<class Slice>
ProxygenHTTP<Slice>::~ProxygenHTTP(){
  if(connector)
    delete connector;
}
template<class Slice>
std::shared_ptr<Slice> ProxygenHTTP<Slice>::getReturnSlice(){
  auto sliceFuture =  slicePromise.get_future();
  std::future_status status = sliceFuture.wait_for(promiseTimeout);
  slicePromise = std::promise<std::shared_ptr<Slice>>();
  if(status == std::future_status::ready)
    return sliceFuture.get();
  return nullptr;
}
template<class Slice>
Code ProxygenHTTP<Slice>::getReturnCode(){
  auto codeFuture = codePromise.get_future();
  std::future_status status = codeFuture.wait_for(promiseTimeout);
  codePromise= std::promise<Code>();
  if(status == std::future_status::ready)
    return codeFuture.get();
  return Code::Timeout;
}
template<class Slice>
std::shared_ptr<proxygen::HTTPMessage> ProxygenHTTP<Slice>::getReturnHeader(){
  auto headerFuture = headerPromise.get_future();
  std::future_status status = headerFuture.wait_for(promiseTimeout);
  headerPromise =  std::promise<std::shared_ptr<proxygen::HTTPMessage>>();
  if(status == std::future_status::ready)
    return headerFuture.get();
  return nullptr;
}
