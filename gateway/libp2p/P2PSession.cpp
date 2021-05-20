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
/** @file P2PSession.cpp
 *  @author monan
 *  @date 20181112
 */

#include "P2PSession.h"
#include "P2PMessage.h"
#include "Service.h"
#include "gateway/Gateway.h"
#include <gateway/libnetwork/ASIOInterface.h>
#include <gateway/libnetwork/Host.h>

#include <bcos-framework/libutilities/Common.h>
#include <boost/algorithm/string.hpp>

using namespace bcos;
using namespace bcos::gateway;

void P2PSession::start() {
  if (!m_run && m_session) {
    m_run = true;

    m_session->start();
    heartBeat();
  }
}

void P2PSession::stop(DisconnectReason reason) {
  if (m_run) {
    m_run = false;
    if (m_session && m_session->actived()) {
      m_session->disconnect(reason);
    }
  }
}

void P2PSession::heartBeat() {
  auto service = m_service.lock();
  if (service && service->actived()) {
    if (m_session && m_session->actived()) {

      auto message = std::dynamic_pointer_cast<P2PMessage>(
          service->messageFactory()->buildMessage());
      message->setPacketType(MessageType::Heartbeat);
      uint32_t statusSeq = 0;
      boost::asio::detail::socket_ops::host_to_network_long(
          service->statusSeq());

      message->setPayload(bytesConstRef((byte *)&statusSeq, 4));

      SESSION_LOG(DEBUG) << LOG_DESC("P2PSession onHeartBeat")
                         << LOG_KV("nodeID", m_nodeInfo.nodeID)
                         << LOG_KV("name", m_session->nodeIPEndpoint())
                         << LOG_KV("statusSeq", statusSeq);

      m_session->asyncSendMessage(message);
    }

    auto self = std::weak_ptr<P2PSession>(shared_from_this());
    m_timer = service->host()->asioInterface()->newTimer(HEARTBEAT_INTERVEL);
    m_timer->async_wait([self](boost::system::error_code e) {
      if (e) {
        SESSION_LOG(TRACE) << "Timer canceled: " << e.message();
        return;
      }

      auto s = self.lock();
      if (s) {
        s->heartBeat();
      }
    });
  }
}