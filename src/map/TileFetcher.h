#pragma once

#include <atomic>
#include <cstdint>
#include <deque>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <thread>
#include <vector>

#include "TileCache.h"

namespace cardmesh::map {

struct TileKey {
    int zoom;
    int32_t x;
    int32_t y;

    bool operator<(const TileKey& other) const {
        if (zoom != other.zoom) return zoom < other.zoom;
        if (x != other.x) return x < other.x;
        return y < other.y;
    }
};

// Background-thread wrapper around TileCache so tile fetches (which may hit
// the network on a cache miss) never block the UI thread -- the same
// pattern NetworkWorker uses for MeshMonitor API calls.
class TileFetcher {
public:
    explicit TileFetcher(TileCache cache);
    ~TileFetcher();

    void start();
    void stop();

    // Queues a fetch if this tile hasn't already been requested (cached,
    // in-flight, or ready). Safe to call from the UI thread; returns
    // immediately.
    void request(int zoom, int32_t x, int32_t y);

    // Non-blocking. Returns the tile's PNG bytes if the fetch has
    // completed, else nullopt (still pending, or the fetch failed).
    std::optional<std::vector<uint8_t>> tryGet(int zoom, int32_t x, int32_t y);

private:
    void run();

    TileCache cache_;
    std::thread thread_;
    std::atomic<bool> running_{false};

    std::mutex mutex_;
    std::deque<TileKey> pending_;
    std::map<TileKey, std::vector<uint8_t>> ready_;
    std::set<TileKey> requested_;
};

}  // namespace cardmesh::map
