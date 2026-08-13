#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>

#include "JsonUtil.h"

namespace cardmesh::models {

struct Node {
    std::string id;
    std::string shortName;
    std::string longName;
    std::int64_t lastHeard = 0;
    std::optional<int> hopCount;
    std::optional<double> rssi;
    std::optional<double> snr;
    std::optional<int> batteryLevel;
    std::optional<double> voltage;
    std::optional<double> latitude;
    std::optional<double> longitude;
    bool favorite = false;

    static Node fromJson(const nlohmann::json& j) {
        Node node;
        node.id = requiredField<std::string>(j, "id", "");
        node.shortName = requiredField<std::string>(j, "shortName", "");
        node.longName = requiredField<std::string>(j, "longName", node.shortName);
        node.lastHeard = requiredField<std::int64_t>(j, "lastHeard", 0);
        node.hopCount = optionalField<int>(j, "hopCount");
        node.rssi = optionalField<double>(j, "rssi");
        node.snr = optionalField<double>(j, "snr");
        node.batteryLevel = optionalField<int>(j, "batteryLevel");
        node.voltage = optionalField<double>(j, "voltage");
        node.latitude = optionalField<double>(j, "latitude");
        node.longitude = optionalField<double>(j, "longitude");
        node.favorite = requiredField<bool>(j, "favorite", false);
        return node;
    }

    bool isDirect() const { return hopCount.has_value() && *hopCount == 0; }
};

}  // namespace cardmesh::models
