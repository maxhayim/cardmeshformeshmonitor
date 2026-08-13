#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "JsonUtil.h"

namespace cardmesh::models {

struct Traceroute {
    std::string targetNodeId;
    std::vector<std::string> hopNodeIds;
    std::int64_t timestamp = 0;

    static Traceroute fromJson(const nlohmann::json& j) {
        Traceroute traceroute;
        traceroute.targetNodeId = requiredField<std::string>(j, "targetNodeId", "");
        traceroute.timestamp = requiredField<std::int64_t>(j, "timestamp", 0);
        auto it = j.find("hops");
        if (it != j.end() && it->is_array()) {
            for (const auto& hop : *it) {
                traceroute.hopNodeIds.push_back(hop.get<std::string>());
            }
        }
        return traceroute;
    }

    int hopCount() const { return static_cast<int>(hopNodeIds.size()); }
};

}  // namespace cardmesh::models
