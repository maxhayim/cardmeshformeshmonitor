#pragma once

#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

#include "JsonUtil.h"

namespace cardmesh::models {

enum class ConversationType { Channel, DirectMessage };

enum class MessageState { Queued, Sending, Sent, Ack, Failed };

inline ConversationType conversationTypeFromString(const std::string& value) {
    return value == "DM" ? ConversationType::DirectMessage : ConversationType::Channel;
}

inline MessageState messageStateFromString(const std::string& value) {
    if (value == "SENDING") return MessageState::Sending;
    if (value == "SENT") return MessageState::Sent;
    if (value == "ACK") return MessageState::Ack;
    if (value == "FAILED") return MessageState::Failed;
    return MessageState::Queued;
}

struct Message {
    std::string id;
    ConversationType conversationType = ConversationType::Channel;
    std::string conversationId;
    std::string senderId;
    std::string senderName;
    std::string text;
    std::int64_t timestamp = 0;
    MessageState state = MessageState::Sent;

    static Message fromJson(const nlohmann::json& j) {
        Message message;
        message.id = requiredField<std::string>(j, "id", "");
        message.conversationType =
            conversationTypeFromString(requiredField<std::string>(j, "conversationType", "CHANNEL"));
        message.conversationId = requiredField<std::string>(j, "conversationId", "");
        message.senderId = requiredField<std::string>(j, "senderId", "");
        message.senderName = requiredField<std::string>(j, "senderName", message.senderId);
        message.text = requiredField<std::string>(j, "text", "");
        message.timestamp = requiredField<std::int64_t>(j, "timestamp", 0);
        message.state = messageStateFromString(requiredField<std::string>(j, "state", "SENT"));
        return message;
    }
};

}  // namespace cardmesh::models
