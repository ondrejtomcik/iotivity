//******************************************************************
//
// Copyright 2016 Samsung Electronics All Rights Reserved.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=


#include "NSProviderService.h"
#include <cstring>
#include "NSCommon.h"
#include "NSProviderInterface.h"
#include "NSConsumer.h"
#include "NSSyncInfo.h"
#include "NSConstants.h"
#include "OCRepresentation.h"
#include "ocpayload.h"
#include "oic_string.h"
#include "oic_malloc.h"

namespace OIC
{
    namespace Service
    {
        void onConsumerSubscribedCallback(::NSConsumer *consumer)
        {
            NS_LOG(DEBUG, "onConsumerSubscribedCallback - IN");
            NSConsumer *nsConsumer = new NSConsumer(consumer);
            NSProviderService::getInstance()->getAcceptedConsumers().push_back(nsConsumer);
            if (NSProviderService::getInstance()->getProviderConfig().m_subscribeRequestCb != NULL)
            {
                NS_LOG(DEBUG, "initiating the callback for consumer subscribed");
                NSProviderService::getInstance()->getProviderConfig().m_subscribeRequestCb(nsConsumer);
            }
            NS_LOG(DEBUG, "onConsumerSubscribedCallback - OUT");
        }

        void onMessageSynchronizedCallback(::NSSyncInfo *syncInfo)
        {
            NS_LOG(DEBUG, "onMessageSynchronizedCallback - IN");
            NSSyncInfo *nsSyncInfo = new NSSyncInfo(syncInfo);
            if (NSProviderService::getInstance()->getProviderConfig().m_syncInfoCb != NULL)
            {
                NS_LOG(DEBUG, "initiating the callback for synchronized");
                NSProviderService::getInstance()->getProviderConfig().m_syncInfoCb(nsSyncInfo);
            }
            delete nsSyncInfo;
            NS_LOG(DEBUG, "onMessageSynchronizedCallback - OUT");
        }

        ::NSMessage *NSProviderService::getNSMessage(NSMessage *msg)
        {
            ::NSMessage *nsMsg = new ::NSMessage;
            nsMsg->messageId = msg->getMessageId();
            OICStrcpy(nsMsg->providerId, NS_UTILS_UUID_STRING_SIZE, msg->getProviderId().c_str());
            nsMsg->sourceName = OICStrdup(msg->getSourceName().c_str());
            nsMsg->type = (::NSMessageType) msg->getType();
            nsMsg->dateTime = OICStrdup(msg->getTime().c_str());
            nsMsg->ttl = msg->getTTL();
            nsMsg->title = OICStrdup(msg->getTitle().c_str());
            nsMsg->contentText = OICStrdup(msg->getContentText().c_str());
            nsMsg->topic = OICStrdup(msg->getTopic().c_str());

            if (msg->getMediaContents() != nullptr)
            {
                nsMsg->mediaContents = new ::NSMediaContents;
                nsMsg->mediaContents->iconImage = OICStrdup(msg->getMediaContents()->getIconImage().c_str());
            }
            else
            {
                nsMsg->mediaContents = nullptr;
            }
            nsMsg->extraInfo = msg->getExtraInfo().getPayload();
            return nsMsg;
        }

        NSProviderService::~NSProviderService()
        {
            for (auto it : getAcceptedConsumers())
            {
                delete it;
            }
            getAcceptedConsumers().clear();
        }

        NSProviderService *NSProviderService::getInstance()
        {
            static NSProviderService s_instance;
            return &s_instance;
        }

        NSResult NSProviderService::start(NSProviderService::ProviderConfig config)
        {
            NS_LOG(DEBUG, "start - IN");

            m_config = config;
            NSProviderConfig nsConfig;
            nsConfig.subRequestCallback = onConsumerSubscribedCallback;
            nsConfig.syncInfoCallback = onMessageSynchronizedCallback;
            nsConfig.subControllability = config.subControllability;
            nsConfig.userInfo = OICStrdup(config.userInfo.c_str());
            nsConfig.resourceSecurity = config.resourceSecurity;

            NSResult result = (NSResult) NSStartProvider(nsConfig);
            OICFree(nsConfig.userInfo);
            NS_LOG(DEBUG, "start - OUT");
            return result;
        }

        NSResult NSProviderService::stop()
        {
            NS_LOG(DEBUG, "stop - IN");
            NSResult result = (NSResult) NSStopProvider();
            NS_LOG(DEBUG, "stop - OUT");
            return result;
        }

        NSResult NSProviderService::enableRemoteService(const std::string &serverAddress)
        {
            NS_LOG(DEBUG, "enableRemoteService - IN");
            NS_LOG_V(DEBUG, "Server Address : %s", serverAddress.c_str());
            NSResult result = NSResult::ERROR;
#ifdef WITH_CLOUD
            result = (NSResult) NSProviderEnableRemoteService(OICStrdup(serverAddress.c_str()));
#else
            (void) serverAddress;
            NS_LOG(ERROR, "Remote Services feature is not enabled in the Build");
#endif
            NS_LOG(DEBUG, "enableRemoteService - OUT");
            return result;
        }

        NSResult NSProviderService::disableRemoteService(const std::string &serverAddress)
        {
            NS_LOG(DEBUG, "disableRemoteService - IN");
            NS_LOG_V(DEBUG, "Server Address : %s", serverAddress.c_str());
            NSResult result = NSResult::ERROR;
#ifdef WITH_CLOUD
            result = (NSResult) NSProviderDisableRemoteService(OICStrdup(serverAddress.c_str()));
#else
            (void) serverAddress;
            NS_LOG(ERROR, "Remote Services feature is not enabled in the Build");
#endif
            NS_LOG(DEBUG, "disableRemoteService - OUT");
            return result;
        }

        NSResult NSProviderService::subscribeMQService(const std::string &serverAddress,
                const std::string &topicName)
        {
            NS_LOG(DEBUG, "subscribeMQService - IN");
            NS_LOG_V(DEBUG, "Server Address : %s", serverAddress.c_str());
            NSResult result = NSResult::ERROR;
#ifdef WITH_MQ
            result = (NSResult) NSProviderSubscribeMQService(
                         serverAddress.c_str(), topicName.c_str());
#else
            NS_LOG(ERROR, "MQ Services feature is not enabled in the Build");
            (void) serverAddress;
            (void) topicName;
#endif
            NS_LOG(DEBUG, "subscribeMQService - OUT");
            return result;
        }

        NSResult NSProviderService::sendMessage(NSMessage *msg)
        {
            NS_LOG(DEBUG, "sendMessage - IN");
            NSResult result = NSResult::ERROR;
            if (msg != nullptr)
            {
                ::NSMessage *nsMsg = getNSMessage(msg);

                NS_LOG_V(DEBUG, "nsMsg->providerId : %s", nsMsg->providerId);
                result = (NSResult) NSSendMessage(nsMsg);
                OICFree(nsMsg->dateTime);
                OICFree(nsMsg->title);
                OICFree(nsMsg->contentText);
                OICFree(nsMsg->sourceName);
                OICFree(nsMsg->topic);
                if (nsMsg->mediaContents != NULL)
                {
                    if (nsMsg->mediaContents->iconImage != NULL)
                    {
                        OICFree(nsMsg->mediaContents->iconImage);
                    }
                    delete nsMsg->mediaContents;
                }
                OCPayloadDestroy((OCPayload *) nsMsg->extraInfo);
                delete nsMsg;
            }
            else
            {
                NS_LOG(DEBUG, "Empty Message");
            }
            NS_LOG(DEBUG, "sendMessage - OUT");
            return result;
        }

        void NSProviderService::sendSyncInfo(uint64_t messageId,
                                             NSSyncInfo::NSSyncType type)
        {
            NS_LOG(DEBUG, "sendSyncInfo - IN");
            NSProviderSendSyncInfo(messageId, (NSSyncType)type);
            NS_LOG(DEBUG, "sendSyncInfo - OUT");
            return;
        }

        NSMessage *NSProviderService::createMessage()
        {
            NS_LOG(DEBUG, "createMessage - IN");

            ::NSMessage *message = NSCreateMessage();
            NSMessage *nsMessage = new NSMessage(message);

            NS_LOG_V(DEBUG, "Message ID : %lld", (long long int) nsMessage->getMessageId());
            NS_LOG_V(DEBUG, "Provider ID : %s", nsMessage->getProviderId().c_str());
            NS_LOG(DEBUG, "createMessage - OUT");

            OICFree(message);
            return nsMessage;
        }

        NSResult NSProviderService::registerTopic(const std::string &topicName)
        {
            NS_LOG(DEBUG, "registerTopic - IN");
            NSResult result = (NSResult) NSProviderRegisterTopic(topicName.c_str());
            NS_LOG(DEBUG, "registerTopic - OUT");
            return result;
        }

        NSResult NSProviderService::unregisterTopic(const std::string &topicName)
        {
            NS_LOG(DEBUG, "unregisterTopic - IN");
            NSResult result = (NSResult) NSProviderUnregisterTopic(topicName.c_str());
            NS_LOG(DEBUG, "unregisterTopic - OUT");
            return result;
        }

        NSTopicsList *NSProviderService::getRegisteredTopicList()
        {
            NS_LOG(DEBUG, "getRegisteredTopicList - IN");
            ::NSTopicLL *topics = NSProviderGetTopics();

            NSTopicsList *nsTopics = new NSTopicsList(topics);
            NS_LOG(DEBUG, "getRegisteredTopicList - OUT");
            return nsTopics;
        }

        NSProviderService::ProviderConfig NSProviderService::getProviderConfig()
        {
            return m_config;
        }

        NSConsumer *NSProviderService::getConsumer(const std::string &id)
        {
            for (auto it : getAcceptedConsumers())
            {
                if (it->getConsumerId() == id)
                {
                    NS_LOG(DEBUG, "getConsumer : Found Consumer with given ID");
                    return it;
                }
            }
            NS_LOG(DEBUG, "getConsumer : Not Found Consumer with given ID");
            return NULL;
        }

        std::list<NSConsumer *> &NSProviderService::getAcceptedConsumers()
        {
            return m_acceptedConsumers;
        }
    }
}
