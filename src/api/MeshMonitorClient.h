#pragma once

#include <string>
#include <vector>

#include "HttpClient.h"
#include "models/Channel.h"
#include "models/Message.h"
#include "models/Node.h"
#include "models/Source.h"
#include "models/Telemetry.h"
#include "models/Traceroute.h"

namespace cardmesh::api {

template <typename T>
struct ApiResult {
    bool ok = false;
    T value{};
    std::string error;

    static ApiResult<T> success(T v) { return ApiResult<T>{true, std::move(v), ""}; }
    static ApiResult<T> failure(std::string err) { return ApiResult<T>{false, T{}, std::move(err)}; }
};

struct ConnectionConfig {
    std::string server;
    int port = 443;
    bool https = true;
    std::string apiToken;
    long timeoutSeconds = 10;

    std::string baseUrl() const {
        return (https ? "https://" : "http://") + server + ":" + std::to_string(port);
    }
};

// Typed wrapper over MeshMonitor's source-scoped REST API v1. Isolates the rest
// of CardMesh from the raw HTTP/JSON shape of the MeshMonitor backend.
class MeshMonitorClient {
public:
    explicit MeshMonitorClient(const ConnectionConfig& config);

    ApiResult<bool> testConnection();

    ApiResult<std::vector<models::Source>> getSources();
    ApiResult<std::vector<models::Node>> getNodes(const std::string& sourceId);
    ApiResult<std::vector<models::Channel>> getChannels(const std::string& sourceId);
    ApiResult<std::vector<models::Message>> getMessages(const std::string& sourceId);
    ApiResult<bool> sendMessage(const std::string& sourceId, models::ConversationType type,
                                 const std::string& conversationId, const std::string& text);
    ApiResult<std::vector<models::Telemetry>> getTelemetry(const std::string& sourceId);
    ApiResult<models::Traceroute> requestTraceroute(const std::string& sourceId,
                                                     const std::string& targetNodeId);

    static std::string sourcePath(const std::string& sourceId, const std::string& suffix);

private:
    HttpClient http_;
};

}  // namespace cardmesh::api
