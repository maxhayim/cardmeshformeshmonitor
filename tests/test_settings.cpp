#include <sys/stat.h>

#include <cstdio>
#include <filesystem>

#include "TestRunner.h"
#include "storage/Settings.h"

using cardmesh::storage::AppSettings;
using cardmesh::storage::Settings;

CARDMESH_TEST_MAIN_BEGIN()

    const std::string path = (std::filesystem::temp_directory_path() / "cardmesh_test_config.json").string();
    std::filesystem::remove(path);

    CARDMESH_CHECK(!Settings::load(path).has_value());

    AppSettings settings;
    settings.server = "mesh.example.com";
    settings.port = 443;
    settings.https = true;
    settings.apiToken = "secret-token";
    settings.preferredSource = "default";

    CARDMESH_CHECK(Settings::save(settings, path));

    struct stat info{};
    CARDMESH_CHECK(stat(path.c_str(), &info) == 0);
    CARDMESH_CHECK((info.st_mode & 0777) == 0600);

    const auto loaded = Settings::load(path);
    CARDMESH_CHECK(loaded.has_value());
    if (loaded.has_value()) {
        CARDMESH_CHECK(loaded->server == settings.server);
        CARDMESH_CHECK(loaded->port == settings.port);
        CARDMESH_CHECK(loaded->apiToken == settings.apiToken);
        CARDMESH_CHECK(loaded->preferredSource == settings.preferredSource);
    }

    std::filesystem::remove(path);

CARDMESH_TEST_MAIN_END()
