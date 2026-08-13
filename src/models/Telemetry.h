#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>

#include "JsonUtil.h"

namespace cardmesh::models {

struct Telemetry {
    std::string nodeId;
    std::optional<int> batteryLevel;
    std::optional<double> voltage;
    std::optional<double> temperature;
    std::int64_t timestamp = 0;

    static Telemetry fromJson(const nlohmann::json& j) {
        Telemetry telemetry;
        telemetry.nodeId = requiredField<std::string>(j, "nodeId", "");
        telemetry.batteryLevel = optionalField<int>(j, "batteryLevel");
        telemetry.voltage = optionalField<double>(j, "voltage");
        telemetry.temperature = optionalField<double>(j, "temperature");
        telemetry.timestamp = requiredField<std::int64_t>(j, "timestamp", 0);
        return telemetry;
    }
};

}  // namespace cardmesh::models
