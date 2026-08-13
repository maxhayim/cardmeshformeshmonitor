#include "MeshMonitorClient.h"

#include <iomanip>
#include <sstream>

#include <nlohmann/json.hpp>

namespace cardmesh::api {

namespace {

std::string urlEncodeSegment(const std::string& segment) {
    std::ostringstream encoded;
    encoded << std::hex << std::uppercase;
    for (unsigned char c : segment) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded << c;
        } else {
            encoded << '%' << std::setw(2) << std::setfill('0') << static_cast<int>(c);
        }
    }
    return encoded.str();
}

template <typename T>
ApiResult<std::vector<T>> parseArrayResponse(const HttpResponse& response) {
    if (!response.ok()) {
        return ApiResult<std::vector<T>>::failure(response.networkError ? response.errorMessage
                                                                         : response.body);
    }
    std::vector<T> items;
    try {
        const auto json = nlohmann::json::parse(response.body);
        for (const auto& item : json) {
            items.push_back(T::fromJson(item));
        }
    } catch (const nlohmann::json::exception& e) {
        return ApiResult<std::vector<T>>::failure(e.what());
    }
    return ApiResult<std::vector<T>>::success(std::move(items));
}

}  // namespace

MeshMonitorClient::MeshMonitorClient(const ConnectionConfig& config) {
    http_.setBaseUrl(config.baseUrl());
    http_.setBearerToken(config.apiToken);
    http_.setTimeoutSeconds(config.timeoutSeconds);
    http_.setVerifyTls(true);
}

std::string MeshMonitorClient::sourcePath(const std::string& sourceId, const std::string& suffix) {
    return "/api/v1/sources/" + urlEncodeSegment(sourceId) + suffix;
}

ApiResult<bool> MeshMonitorClient::testConnection() {
    const auto response = http_.get("/api/v1/sources");
    if (!response.ok()) {
        return ApiResult<bool>::failure(response.networkError ? response.errorMessage : response.body);
    }
    return ApiResult<bool>::success(true);
}

ApiResult<std::vector<models::Source>> MeshMonitorClient::getSources() {
    return parseArrayResponse<models::Source>(http_.get("/api/v1/sources"));
}

ApiResult<std::vector<models::Node>> MeshMonitorClient::getNodes(const std::string& sourceId) {
    return parseArrayResponse<models::Node>(http_.get(sourcePath(sourceId, "/nodes")));
}

ApiResult<std::vector<models::Channel>> MeshMonitorClient::getChannels(const std::string& sourceId) {
    return parseArrayResponse<models::Channel>(http_.get(sourcePath(sourceId, "/channels")));
}

ApiResult<std::vector<models::Message>> MeshMonitorClient::getMessages(const std::string& sourceId) {
    return parseArrayResponse<models::Message>(http_.get(sourcePath(sourceId, "/messages")));
}

ApiResult<bool> MeshMonitorClient::sendMessage(const std::string& sourceId, models::ConversationType type,
                                                const std::string& conversationId,
                                                const std::string& text) {
    nlohmann::json body;
    body["conversationType"] = (type == models::ConversationType::DirectMessage) ? "DM" : "CHANNEL";
    body["conversationId"] = conversationId;
    body["text"] = text;

    const auto response = http_.post(sourcePath(sourceId, "/messages"), body.dump());
    if (!response.ok()) {
        return ApiResult<bool>::failure(response.networkError ? response.errorMessage : response.body);
    }
    return ApiResult<bool>::success(true);
}

ApiResult<std::vector<models::Telemetry>> MeshMonitorClient::getTelemetry(const std::string& sourceId) {
    return parseArrayResponse<models::Telemetry>(http_.get(sourcePath(sourceId, "/telemetry")));
}

ApiResult<models::Traceroute> MeshMonitorClient::requestTraceroute(const std::string& sourceId,
                                                                    const std::string& targetNodeId) {
    nlohmann::json body;
    body["targetNodeId"] = targetNodeId;

    const auto response = http_.post(sourcePath(sourceId, "/traceroutes"), body.dump());
    if (!response.ok()) {
        return ApiResult<models::Traceroute>::failure(response.networkError ? response.errorMessage
                                                                             : response.body);
    }
    try {
        const auto json = nlohmann::json::parse(response.body);
        return ApiResult<models::Traceroute>::success(models::Traceroute::fromJson(json));
    } catch (const nlohmann::json::exception& e) {
        return ApiResult<models::Traceroute>::failure(e.what());
    }
}

}  // namespace cardmesh::api
