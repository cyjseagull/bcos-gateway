/*
 *  Copyright (C) 2021 FISCO BCOS.
 *  SPDX-License-Identifier: Apache-2.0
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * @file AMOP.h
 * @author: octopus
 * @date 2021-10-26
 */
#pragma once
#include "Common.h"
#include <bcos-framework/interfaces/amop/AMOPInterface.h>
#include <bcos-framework/interfaces/crypto/KeyFactory.h>
#include <bcos-framework/libutilities/ThreadPool.h>
#include <bcos-framework/libutilities/Timer.h>
#include <bcos-gateway/Gateway.h>
#include <bcos-gateway/libamop/AMOPMessage.h>
#include <bcos-gateway/libamop/AMOPRequest.h>
#include <bcos-gateway/libamop/TopicManager.h>
#include <boost/asio.hpp>
namespace bcos
{
namespace amop
{
class AMOP
{
public:
    AMOP(TopicManager::Ptr _topicManager, MessageFactory::Ptr _messageFactory,
        AMOPRequestFactory::Ptr _requestFactory, bcos::gateway::P2PInterface::Ptr _network,
        bcos::gateway::P2pID const& _p2pNodeID);
    virtual ~AMOP() {}

    virtual void start();
    virtual void stop();
    virtual void asyncRegisterClient(std::string const& _clientID,
        std::string const& _clientEndPoint, std::function<void(Error::Ptr&&)> _callback);

    virtual void asyncSubscribeTopic(std::string const& _clientID, std::string const& _topicInfo,
        std::function<void(Error::Ptr&&)> _callback);
    virtual void asyncRemoveTopic(std::string const& _clientID,
        std::vector<std::string> const& _topicList, std::function<void(Error::Ptr&&)> _callback);

    /**
     * @brief: async send message to random node subscribe _topic
     * @param _topic: topic
     * @param _data: message data
     * @param _respFunc: callback
     * @return void
     */
    virtual void asyncSendMessageByTopic(const std::string& _topic, bcos::bytesConstRef _data,
        std::function<void(bcos::Error::Ptr&&, bytesPointer)> _respFunc);

    /**
     * @brief: async send message to all nodes subscribe _topic
     * @param _topic: topic
     * @param _data: message data
     * @return void
     */
    virtual void asyncSendBroadbastMessageByTopic(
        const std::string& _topic, bcos::bytesConstRef _data);

    virtual void onAMOPMessage(bcos::gateway::NetworkException const& _e,
        bcos::gateway::P2PSession::Ptr _session,
        std::shared_ptr<bcos::gateway::P2PMessage> _message);

protected:
    virtual void dispatcherAMOPMessage(bcos::gateway::NetworkException const& _e,
        bcos::gateway::P2PSession::Ptr _session,
        std::shared_ptr<bcos::gateway::P2PMessage> _message);
    /**
     * @brief: periodically send topicSeq to all other nodes
     * @return void
     */
    virtual void broadcastTopicSeq();

    /**
     * @brief: receive topicSeq from other nodes
     * @param _nodeID: the sender nodeID
     * @param _id: the message id
     * @param _msg: message
     * @return void
     */
    virtual void onReceiveTopicSeqMessage(
        bcos::gateway::P2pID const& _nodeID, AMOPMessage::Ptr _msg);

    /**
     * @brief: receive request topic message from other nodes
     * @param _nodeID: the sender nodeID
     * @param _id: the message id
     * @param _msg: message
     * @return void
     */
    void onReceiveRequestTopicMessage(bcos::gateway::P2pID const& _nodeID, AMOPMessage::Ptr _msg);

    /**
     * @brief: receive topic response message from other nodes
     * @param _nodeID: the sender nodeID
     * @param _id: the message id
     * @param _msg: message
     * @return void
     */
    virtual void onReceiveResponseTopicMessage(
        bcos::gateway::P2pID const& _nodeID, AMOPMessage::Ptr _msg);

    /**
     * @brief: receive amop message
     * @param _nodeID: the sender nodeID
     * @param _id: the message id
     * @param _msg: message
     * @return void
     */
    virtual void onReceiveAMOPMessage(bcos::gateway::P2pID const& _nodeID, AMOPMessage::Ptr _msg,
        std::function<void(bytesConstRef)> const& _responseCallback);

    /**
     * @brief: receive broadcast message
     * @param _nodeID: the sender nodeID
     * @param _id: the message id
     * @param _msg: message
     * @return void
     */
    virtual void onReceiveAMOPBroadcastMessage(
        bcos::gateway::P2pID const& _nodeID, AMOPMessage::Ptr _msg);

private:
    std::shared_ptr<bytes> buildAndEncodeMessage(uint32_t _type, bcos::bytesConstRef _data);

private:
    std::shared_ptr<TopicManager> m_topicManager;
    std::shared_ptr<MessageFactory> m_messageFactory;
    std::shared_ptr<AMOPRequestFactory> m_requestFactory;
    std::shared_ptr<Timer> m_timer;
    bcos::gateway::P2PInterface::Ptr m_network;
    bcos::gateway::P2pID m_p2pNodeID;
    ThreadPool::Ptr m_threadPool;

    unsigned const TOPIC_SYNC_PERIOD = 2000;
};
}  // namespace amop
}  // namespace bcos