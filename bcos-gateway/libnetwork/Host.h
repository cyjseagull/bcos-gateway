/*
 * @CopyRight:
 * FISCO-BCOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * FISCO-BCOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FISCO-BCOS.  If not, see <http://www.gnu.org/licenses/>
 * (c) 2016-2018 fisco-dev contributors.
 */
/** @file Host.h
 * @author monan <651932351@qq.com>
 * @date 2018
 */
#pragma once

#include <bcos-framework/libutilities/Common.h> // for Guard, Mutex
#include <bcos-framework/libutilities/ThreadPool.h>
#include <bcos-gateway/libnetwork/Common.h>  // for  NodeIP...
#include <bcos-gateway/libnetwork/Message.h> // for Message
#include <boost/asio/deadline_timer.hpp>     // for deadline_timer
#include <boost/system/error_code.hpp>       // for error_code
#include <openssl/x509.h>
#include <set>     // for set
#include <string>  // for string
#include <thread>  // for thread
#include <utility> // for swap, move
#include <vector>  // for vector

namespace boost {
namespace asio {
namespace ssl {
class verify_context;
}
} // namespace asio
} // namespace boost
namespace bcos {
class ThreadPool;

namespace gateway {
class SessionFactory;
class SessionFace;
class SocketFace;
class ASIOInterface;

using x509PubHandler = std::function<bool(X509 *x509, std::string &pubHex)>;

class Host : public std::enable_shared_from_this<Host> {
public:
  Host(){};
  virtual ~Host() { stop(); };

  using Ptr = std::shared_ptr<Host>;

  virtual uint16_t listenPort() const { return m_listenPort; }

  virtual void start();
  virtual void stop();

  virtual void
  asyncConnect(NodeIPEndpoint const &_nodeIPEndpoint,
               std::function<void(NetworkException, P2PInfo const &,
                                  std::shared_ptr<SessionFace>)>
                   callback);

  virtual bool haveNetwork() const { return m_run; }

  virtual std::string listenHost() const { return m_listenHost; }
  virtual void setHostPort(std::string host, uint16_t port) {
    m_listenHost = host;
    m_listenPort = port;
  }

  virtual std::function<void(NetworkException, P2PInfo const &,
                             std::shared_ptr<SessionFace>)>
  connectionHandler() const {
    return m_connectionHandler;
  }
  virtual void
  setConnectionHandler(std::function<void(NetworkException, P2PInfo const &,
                                          std::shared_ptr<SessionFace>)>
                           connectionHandler) {
    m_connectionHandler = connectionHandler;
  }

  virtual std::function<bool(X509 *x509, std::string &pubHex)>
  sslContextPubHandler() {
    return m_sslContextPubHandler;
  }

  virtual void
  setSSLContextPubHandler(std::function<bool(X509 *x509, std::string &pubHex)>
                              _sslContextPubHandler) {
    m_sslContextPubHandler = _sslContextPubHandler;
  }

  virtual std::shared_ptr<bcos::ThreadPool> threadPool() const {
    return m_threadPool;
  }
  virtual void setThreadPool(std::shared_ptr<bcos::ThreadPool> threadPool) {
    m_threadPool = threadPool;
  }

  virtual std::shared_ptr<ASIOInterface> asioInterface() const {
    return m_asioInterface;
  }
  virtual void setASIOInterface(std::shared_ptr<ASIOInterface> asioInterface) {
    m_asioInterface = asioInterface;
  }

  virtual std::shared_ptr<SessionFactory> sessionFactory() const {
    return m_sessionFactory;
  }
  virtual void
  setSessionFactory(std::shared_ptr<SessionFactory> sessionFactory) {
    m_sessionFactory = sessionFactory;
  }

  virtual MessageFactory::Ptr messageFactory() const {
    return m_messageFactory;
  }
  virtual void setMessageFactory(MessageFactory::Ptr _messageFactory) {
    m_messageFactory = _messageFactory;
  }

  virtual P2PInfo p2pInfo();

private:
  /// obtain the common name from the subject:
  /// the subject format is: /CN=xx/O=xxx/OU=xxx/ commonly
  std::string obtainCommonNameFromSubject(std::string const &subject);

  /// called by 'startedWorking' to accept connections
  void startAccept(boost::system::error_code ec = boost::system::error_code());
  /// functions called after openssl handshake,
  /// maily to get node id and verify whether the certificate has been expired
  /// @return: node id of the connected peer
  std::function<bool(bool, boost::asio::ssl::verify_context &)>
  newVerifyCallback(std::shared_ptr<std::string> nodeIDOut);

  /// obtain nodeInfo from given vector
  void obtainNodeInfo(P2PInfo &info, std::string const &node_info);

  /// server calls handshakeServer to after handshake, mainly calls
  /// RLPxHandshake to obtain informations(client version, caps, etc),start peer
  /// session and start accepting procedure repeatedly
  void handshakeServer(const boost::system::error_code &error,
                       std::shared_ptr<std::string> endpointPublicKey,
                       std::shared_ptr<SocketFace> socket);

  void startPeerSession(P2PInfo const &p2pInfo,
                        std::shared_ptr<SocketFace> const &socket,
                        std::function<void(NetworkException, P2PInfo const &,
                                           std::shared_ptr<SessionFace>)>
                            handler);

  void handshakeClient(const boost::system::error_code &error,
                       std::shared_ptr<SocketFace> socket,
                       std::shared_ptr<std::string> endpointPublicKey,
                       std::function<void(NetworkException, P2PInfo const &,
                                          std::shared_ptr<SessionFace>)>
                           callback,
                       NodeIPEndpoint _nodeIPEndpoint,
                       std::shared_ptr<boost::asio::deadline_timer> timerPtr);

  void erasePendingConns(NodeIPEndpoint const &_nodeIPEndpoint) {
    bcos::Guard l(x_pendingConns);
    if (m_pendingConns.count(_nodeIPEndpoint))
      m_pendingConns.erase(_nodeIPEndpoint);
  }

  void insertPendingConns(NodeIPEndpoint const &_nodeIPEndpoint) {
    bcos::Guard l(x_pendingConns);
    if (!m_pendingConns.count(_nodeIPEndpoint))
      m_pendingConns.insert(_nodeIPEndpoint);
  }

  std::shared_ptr<bcos::ThreadPool> m_threadPool;

  /// representing to the network state
  std::shared_ptr<ASIOInterface> m_asioInterface;
  std::shared_ptr<SessionFactory> m_sessionFactory;
  int m_connectTimeThre = 50000;
  std::set<NodeIPEndpoint> m_pendingConns;
  bcos::Mutex x_pendingConns;

  MessageFactory::Ptr m_messageFactory;

  std::string m_listenHost = "";
  uint16_t m_listenPort = 0;

  std::function<void(NetworkException, P2PInfo const &,
                     std::shared_ptr<SessionFace>)>
      m_connectionHandler;

  // get the hex public key of the peer from the the SSL connection
  std::function<bool(X509 *x509, std::string &pubHex)> m_sslContextPubHandler;

  bool m_run = false;

  std::shared_ptr<std::thread> m_hostThread;

  P2PInfo m_p2pInfo;
};
} // namespace gateway

} // namespace bcos