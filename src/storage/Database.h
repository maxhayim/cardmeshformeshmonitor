#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct sqlite3;

namespace cardmesh::storage {

// SQLite-backed local cache (cardmesh.db). MeshMonitor remains authoritative
// for mesh data; this only stores CardMesh-specific state and cache.
class Database {
public:
    explicit Database(std::string path);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool open();
    void close();

    bool markRead(const std::string& sourceId, const std::string& conversationType,
                   const std::string& conversationId, std::int64_t timestamp);
    std::int64_t getLastRead(const std::string& sourceId, const std::string& conversationType,
                              const std::string& conversationId) const;

    bool setFavorite(const std::string& sourceId, const std::string& nodeId, bool favorite);
    bool isFavorite(const std::string& sourceId, const std::string& nodeId) const;
    std::vector<std::string> favoriteNodeIds(const std::string& sourceId) const;

private:
    bool createSchema();

    std::string path_;
    sqlite3* db_ = nullptr;
};

}  // namespace cardmesh::storage
