#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include "JsonUtil.h"

namespace cardmesh::models {

struct Source {
    std::string id;
    std::string name;
    bool online = false;

    static Source fromJson(const nlohmann::json& j) {
        Source source;
        source.id = requiredField<std::string>(j, "id", "");
        source.name = requiredField<std::string>(j, "name", source.id);
        source.online = requiredField<bool>(j, "online", false);
        return source;
    }
};

}  // namespace cardmesh::models
