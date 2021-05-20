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
/** @file P2PInterface.h
 *  @author chaychen
 *  @date 20180911
 */

#pragma once

#include "Common.h"
#include "P2PMessage.h"
#include <gateway/libnetwork/Host.h>
#include <gateway/libnetwork/SessionFace.h>
#include <memory>

namespace bcos {
namespace stat {
class NetworkStatHandler;
class ChannelNetworkStatHandler;
} // namespace stat

namespace gateway {
class P2PMessage;
class MessageFactory;
class P2PSession;
using CallbackFuncWithSession =
    std::function<void(NetworkException, std::shared_ptr<P2PSession>,
                       std::shared_ptr<P2PMessage>)>;
using DisconnectCallbackFuncWithSession =
    std::function<void(NetworkException, std::shared_ptr<P2PSession>)>;

class P2PInterface {
public:
  using Ptr = std::shared_ptr<P2PInterface>;
  virtual ~P2PInterface(){};

  virtual P2PNodeID id() const = 0;

  virtual std::shared_ptr<P2PMessage>
  sendMessageByNodeID(P2PNodeID nodeID,
                      std::shared_ptr<P2PMessage> message) = 0;

  virtual void asyncSendMessageByNodeID(P2PNodeID nodeID,
                                        std::shared_ptr<P2PMessage> message,
                                        CallbackFuncWithSession callback,
                                        Options options = Options()) = 0;

  virtual void
  asyncMulticastMessageByNodeIDList(P2PNodeIDs nodeIDs,
                                    std::shared_ptr<P2PMessage> message) = 0;

  virtual void asyncBroadcastMessage(std::shared_ptr<P2PMessage> message,
                                     Options options) = 0;

  virtual P2PSessionInfos sessionInfos() = 0;

  virtual bool isConnected(P2PNodeID const &_nodeID) const = 0;

  virtual std::shared_ptr<Host> host() = 0;

  virtual std::shared_ptr<MessageFactory> messageFactory() = 0;

  virtual std::shared_ptr<P2PSession>
  getP2PSessionByNodeId(P2PNodeID const &_nodeID) = 0;
};

} // namespace gateway

} // namespace bcos