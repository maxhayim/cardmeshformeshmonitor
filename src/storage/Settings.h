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

    // Defaults to OSM's own tile server, which has a usage policy
    // (https://operations.osmfoundation.org/policies/tiles/) unsuited to
    // heavy/widely-distributed use -- configurable here so a production
    // deployment can point at a self-hosted or commercial tile server
    // instead. {z}/{x}/{y} are substituted. See src/map/TileCache.h.
    std::string tileServerTemplate = "https://tile.openstreetmap.org/{z}/{x}/{y}.png";
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
