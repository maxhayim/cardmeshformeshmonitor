#include "Settings.h"

#include <sys/stat.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>

#include <nlohmann/json.hpp>

namespace cardmesh::storage {

namespace fs = std::filesystem;

std::string Settings::defaultConfigPath() {
    const char* home = std::getenv("HOME");
    const fs::path base = (home != nullptr) ? fs::path(home) : fs::path(".");
    return (base / ".config" / "cardmesh" / "config.json").string();
}

std::optional<AppSettings> Settings::load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return std::nullopt;
    }

    try {
        nlohmann::json json;
        file >> json;

        AppSettings settings;
        settings.server = json.value("server", "");
        settings.port = json.value("port", 443);
        settings.https = json.value("https", true);
        settings.apiToken = json.value("token", "");
        settings.preferredSource = json.value("preferredSource", "");
        return settings;
    } catch (const nlohmann::json::exception&) {
        return std::nullopt;
    }
}

bool Settings::save(const AppSettings& settings, const std::string& path) {
    const fs::path filePath(path);

    std::error_code ec;
    fs::create_directories(filePath.parent_path(), ec);
    if (ec) {
        return false;
    }

    nlohmann::json json;
    json["server"] = settings.server;
    json["port"] = settings.port;
    json["https"] = settings.https;
    json["token"] = settings.apiToken;
    json["preferredSource"] = settings.preferredSource;

    std::ofstream file(path, std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }
    file << json.dump(2);
    file.close();

    return chmod(path.c_str(), S_IRUSR | S_IWUSR) == 0;
}

}  // namespace cardmesh::storage
