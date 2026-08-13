#pragma once

#include <optional>
#include <string>

namespace cardmesh::storage {

struct AppSettings {
    std::string server;
    int port = 443;
    bool https = true;
    std::string apiToken;
    std::string preferredSource;
};

// Persists connection configuration at ~/.config/cardmesh/config.json (mode 0600).
// Real MeshMonitor credentials live only in this file and must never be committed.
class Settings {
public:
    static std::string defaultConfigPath();
    static std::optional<AppSettings> load(const std::string& path = defaultConfigPath());
    static bool save(const AppSettings& settings, const std::string& path = defaultConfigPath());
};

}  // namespace cardmesh::storage
