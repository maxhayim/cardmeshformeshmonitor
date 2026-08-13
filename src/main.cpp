// Headless entry point for CardMesh's core (settings, MeshMonitor API client,
// local cache). The CardputerZero LVGL UI layer (src/ui/) requires the vendor
// CardputerZero AppBuilder SDK and is not implemented here; this binary lets
// the API/storage core be built, run, and tested independently of that SDK.

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include "api/MeshMonitorClient.h"
#include "storage/Database.h"
#include "storage/Settings.h"

namespace {

using cardmesh::api::ConnectionConfig;
using cardmesh::api::MeshMonitorClient;
using cardmesh::storage::AppSettings;
using cardmesh::storage::Database;
using cardmesh::storage::Settings;

AppSettings runInteractiveConfigure() {
    AppSettings settings;

    std::cout << "CARDMESH\nfor MeshMonitor\n\n";

    std::cout << "MeshMonitor Server: ";
    std::getline(std::cin, settings.server);

    std::cout << "Port [443]: ";
    std::string portInput;
    std::getline(std::cin, portInput);
    settings.port = portInput.empty() ? 443 : std::stoi(portInput);

    std::cout << "HTTPS [Y/n]: ";
    std::string httpsInput;
    std::getline(std::cin, httpsInput);
    settings.https = httpsInput.empty() || httpsInput == "Y" || httpsInput == "y";

    std::cout << "API Token: ";
    std::getline(std::cin, settings.apiToken);

    std::cout << "Preferred Source [default]: ";
    std::getline(std::cin, settings.preferredSource);
    if (settings.preferredSource.empty()) {
        settings.preferredSource = "default";
    }

    return settings;
}

int printDashboard(const AppSettings& settings) {
    ConnectionConfig config;
    config.server = settings.server;
    config.port = settings.port;
    config.https = settings.https;
    config.apiToken = settings.apiToken;

    MeshMonitorClient client(config);

    const auto sources = client.getSources();
    if (!sources.ok) {
        std::cerr << "SERVER UNREACHABLE\n" << sources.error << "\n";
        return EXIT_FAILURE;
    }

    std::cout << "CARDMESH\n\n";
    std::cout << "Sources: " << sources.value.size() << "\n";

    for (const auto& source : sources.value) {
        const auto nodes = client.getNodes(source.id);
        const auto channels = client.getChannels(source.id);
        std::cout << "  " << source.name << " [" << (source.online ? "online" : "offline") << "]"
                  << "  nodes=" << (nodes.ok ? std::to_string(nodes.value.size()) : "?")
                  << "  channels=" << (channels.ok ? std::to_string(channels.value.size()) : "?") << "\n";
    }

    return EXIT_SUCCESS;
}

}  // namespace

int main(int argc, char** argv) {
    const bool configureRequested = argc > 1 && std::string(argv[1]) == "--configure";

    if (configureRequested) {
        const AppSettings settings = runInteractiveConfigure();
        if (!Settings::save(settings)) {
            std::cerr << "Failed to save configuration to " << Settings::defaultConfigPath() << "\n";
            return EXIT_FAILURE;
        }
        std::cout << "Saved configuration to " << Settings::defaultConfigPath() << "\n";
        return printDashboard(settings);
    }

    const auto settings = Settings::load();
    if (!settings.has_value()) {
        std::cerr << "No configuration found at " << Settings::defaultConfigPath() << "\n";
        std::cerr << "Run: cardmesh --configure\n";
        return EXIT_FAILURE;
    }

    const std::filesystem::path dbDir =
        std::filesystem::path(std::getenv("HOME") != nullptr ? std::getenv("HOME") : ".") /
        ".local" / "state" / "cardmesh";
    std::error_code ec;
    std::filesystem::create_directories(dbDir, ec);

    Database db((dbDir / "cardmesh.db").string());
    if (!db.open()) {
        std::cerr << "Failed to open local cache database\n";
    }

    return printDashboard(*settings);
}
