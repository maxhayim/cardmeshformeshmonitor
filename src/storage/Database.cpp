#include "Database.h"

#include <sqlite3.h>

namespace cardmesh::storage {

namespace {

constexpr const char* kSchema = R"SQL(
CREATE TABLE IF NOT EXISTS settings (
    key TEXT PRIMARY KEY,
    value TEXT
);

CREATE TABLE IF NOT EXISTS read_state (
    source_id TEXT NOT NULL,
    conversation_type TEXT NOT NULL,
    conversation_id TEXT NOT NULL,
    last_read_timestamp INTEGER NOT NULL,
    PRIMARY KEY (source_id, conversation_type, conversation_id)
);

CREATE TABLE IF NOT EXISTS favorites (
    source_id TEXT NOT NULL,
    node_id TEXT NOT NULL,
    PRIMARY KEY (source_id, node_id)
);

CREATE TABLE IF NOT EXISTS recent_nodes (
    source_id TEXT NOT NULL,
    node_id TEXT NOT NULL,
    last_viewed_timestamp INTEGER NOT NULL,
    PRIMARY KEY (source_id, node_id)
);

CREATE TABLE IF NOT EXISTS cached_sources (
    source_id TEXT PRIMARY KEY,
    name TEXT,
    online INTEGER
);

CREATE TABLE IF NOT EXISTS cached_nodes (
    source_id TEXT NOT NULL,
    node_id TEXT NOT NULL,
    json TEXT NOT NULL,
    updated_at INTEGER NOT NULL,
    PRIMARY KEY (source_id, node_id)
);

CREATE TABLE IF NOT EXISTS cached_channels (
    source_id TEXT NOT NULL,
    channel_id TEXT NOT NULL,
    json TEXT NOT NULL,
    updated_at INTEGER NOT NULL,
    PRIMARY KEY (source_id, channel_id)
);

CREATE TABLE IF NOT EXISTS cached_messages (
    source_id TEXT NOT NULL,
    message_id TEXT NOT NULL,
    conversation_type TEXT NOT NULL,
    conversation_id TEXT NOT NULL,
    json TEXT NOT NULL,
    timestamp INTEGER NOT NULL,
    PRIMARY KEY (source_id, message_id)
);

CREATE TABLE IF NOT EXISTS field_sessions (
    session_id TEXT PRIMARY KEY,
    start_time INTEGER NOT NULL,
    end_time INTEGER,
    selected_source TEXT,
    target_node TEXT,
    notes TEXT
);

CREATE TABLE IF NOT EXISTS field_measurements (
    session_id TEXT NOT NULL,
    timestamp INTEGER NOT NULL,
    node_id TEXT NOT NULL,
    rssi REAL,
    snr REAL,
    hop_count INTEGER,
    last_heard INTEGER,
    battery INTEGER,
    voltage REAL,
    latitude REAL,
    longitude REAL
);
)SQL";

}  // namespace

Database::Database(std::string path) : path_(std::move(path)) {}

Database::~Database() { close(); }

bool Database::open() {
    if (sqlite3_open(path_.c_str(), &db_) != SQLITE_OK) {
        close();
        return false;
    }
    return createSchema();
}

void Database::close() {
    if (db_ != nullptr) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool Database::createSchema() { return sqlite3_exec(db_, kSchema, nullptr, nullptr, nullptr) == SQLITE_OK; }

bool Database::markRead(const std::string& sourceId, const std::string& conversationType,
                         const std::string& conversationId, std::int64_t timestamp) {
    static constexpr const char* kSql =
        "INSERT INTO read_state (source_id, conversation_type, conversation_id, last_read_timestamp) "
        "VALUES (?, ?, ?, ?) "
        "ON CONFLICT(source_id, conversation_type, conversation_id) "
        "DO UPDATE SET last_read_timestamp = excluded.last_read_timestamp;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, sourceId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, conversationType.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, conversationId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, timestamp);

    const bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

std::int64_t Database::getLastRead(const std::string& sourceId, const std::string& conversationType,
                                    const std::string& conversationId) const {
    static constexpr const char* kSql =
        "SELECT last_read_timestamp FROM read_state "
        "WHERE source_id = ? AND conversation_type = ? AND conversation_id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }

    sqlite3_bind_text(stmt, 1, sourceId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, conversationType.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, conversationId.c_str(), -1, SQLITE_TRANSIENT);

    std::int64_t lastRead = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        lastRead = sqlite3_column_int64(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return lastRead;
}

bool Database::setFavorite(const std::string& sourceId, const std::string& nodeId, bool favorite) {
    sqlite3_stmt* stmt = nullptr;
    bool success = false;

    if (favorite) {
        static constexpr const char* kSql =
            "INSERT OR IGNORE INTO favorites (source_id, node_id) VALUES (?, ?);";
        if (sqlite3_prepare_v2(db_, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
            return false;
        }
        sqlite3_bind_text(stmt, 1, sourceId.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, nodeId.c_str(), -1, SQLITE_TRANSIENT);
        success = sqlite3_step(stmt) == SQLITE_DONE;
    } else {
        static constexpr const char* kSql = "DELETE FROM favorites WHERE source_id = ? AND node_id = ?;";
        if (sqlite3_prepare_v2(db_, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
            return false;
        }
        sqlite3_bind_text(stmt, 1, sourceId.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, nodeId.c_str(), -1, SQLITE_TRANSIENT);
        success = sqlite3_step(stmt) == SQLITE_DONE;
    }

    sqlite3_finalize(stmt);
    return success;
}

bool Database::isFavorite(const std::string& sourceId, const std::string& nodeId) const {
    static constexpr const char* kSql =
        "SELECT 1 FROM favorites WHERE source_id = ? AND node_id = ? LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, sourceId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, nodeId.c_str(), -1, SQLITE_TRANSIENT);

    const bool found = sqlite3_step(stmt) == SQLITE_ROW;
    sqlite3_finalize(stmt);
    return found;
}

std::vector<std::string> Database::favoriteNodeIds(const std::string& sourceId) const {
    static constexpr const char* kSql = "SELECT node_id FROM favorites WHERE source_id = ?;";

    sqlite3_stmt* stmt = nullptr;
    std::vector<std::string> nodeIds;
    if (sqlite3_prepare_v2(db_, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
        return nodeIds;
    }

    sqlite3_bind_text(stmt, 1, sourceId.c_str(), -1, SQLITE_TRANSIENT);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const auto* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        nodeIds.emplace_back(text != nullptr ? text : "");
    }
    sqlite3_finalize(stmt);
    return nodeIds;
}

}  // namespace cardmesh::storage
