#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include "JsonUtil.h"

namespace cardmesh::models {

struct Channel {
    std::string id;
    std::string name;
    int unreadCount = 0;

    static Channel fromJson(const nlohmann::json& j) {
        Channel channel;
        channel.id = requiredField<std::string>(j, "id", "");
        channel.name = requiredField<std::string>(j, "name", channel.id);
        channel.unreadCount = requiredField<int>(j, "unreadCount", 0);
        return channel;
    }
};

}  // namespace cardmesh::models
