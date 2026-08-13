#pragma once

#include <optional>
#include <string>

#include <nlohmann/json.hpp>

namespace cardmesh::models {

template <typename T>
std::optional<T> optionalField(const nlohmann::json& j, const std::string& key) {
    auto it = j.find(key);
    if (it == j.end() || it->is_null()) {
        return std::nullopt;
    }
    return it->get<T>();
}

template <typename T>
T requiredField(const nlohmann::json& j, const std::string& key, T fallback) {
    auto it = j.find(key);
    if (it == j.end() || it->is_null()) {
        return fallback;
    }
    return it->get<T>();
}

}  // namespace cardmesh::models
