#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace cardmesh::map {

// Fetches OSM raster map tiles over HTTP and caches them to disk, so the map
// screen works with no connection once tiles for an area have been viewed
// once (true "offline" maps, not a fixed pre-baked region -- see
// docs/DEVICE_BUILD.md).
//
// IMPORTANT: the default tile server (tile.openstreetmap.org) has a usage
// policy (https://operations.osmfoundation.org/policies/tiles/) that
// prohibits heavy/bulk automated use and requires a descriptive User-Agent.
// This is fine for light personal use with this cache in front of it, but a
// widely-distributed build should point `tileServerTemplate` at a
// self-hosted or commercial tile server instead -- see
// AppSettings::tileServerTemplate.
class TileCache {
public:
    TileCache(std::string cacheDir, std::string tileServerTemplate, std::string userAgent);

    // Returns PNG bytes for tile (zoom, x, y), or nullopt if not cached and
    // either offline or the fetch failed. Never blocks longer than the
    // underlying HTTP timeout; call this off the UI thread.
    std::optional<std::vector<uint8_t>> getTile(int zoom, int32_t x, int32_t y);

private:
    std::string tilePath(int zoom, int32_t x, int32_t y) const;
    std::string tileUrl(int zoom, int32_t x, int32_t y) const;

    std::string cacheDir_;
    std::string tileServerTemplate_;
    std::string userAgent_;
};

}  // namespace cardmesh::map
