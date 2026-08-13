#include <filesystem>

#include "TestRunner.h"
#include "storage/Database.h"

using cardmesh::storage::Database;

CARDMESH_TEST_MAIN_BEGIN()

    const std::string path = (std::filesystem::temp_directory_path() / "cardmesh_test.db").string();
    std::filesystem::remove(path);

    Database db(path);
    CARDMESH_CHECK(db.open());

    // Unread logic: message.timestamp > last_read_timestamp
    CARDMESH_CHECK(db.getLastRead("default", "CHANNEL", "1") == 0);
    CARDMESH_CHECK(db.markRead("default", "CHANNEL", "1", 1000));
    CARDMESH_CHECK(db.getLastRead("default", "CHANNEL", "1") == 1000);
    CARDMESH_CHECK(db.markRead("default", "CHANNEL", "1", 2000));
    CARDMESH_CHECK(db.getLastRead("default", "CHANNEL", "1") == 2000);

    CARDMESH_CHECK(!db.isFavorite("default", "!a13f829c"));
    CARDMESH_CHECK(db.setFavorite("default", "!a13f829c", true));
    CARDMESH_CHECK(db.isFavorite("default", "!a13f829c"));
    CARDMESH_CHECK(db.favoriteNodeIds("default").size() == 1);
    CARDMESH_CHECK(db.setFavorite("default", "!a13f829c", false));
    CARDMESH_CHECK(!db.isFavorite("default", "!a13f829c"));

    db.close();
    std::filesystem::remove(path);

CARDMESH_TEST_MAIN_END()
