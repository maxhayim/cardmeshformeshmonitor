#pragma once

#include <cmath>
#include <cstdint>

namespace cardmesh::map {

// Standard OSM "slippy map" Web Mercator projection.
// https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames

constexpr int kTileSizePx = 256;

inline double worldSizePx(int zoom) { return static_cast<double>(kTileSizePx) * std::pow(2.0, zoom); }

// Global pixel coordinates at the given zoom (origin at lat=85.0511, lon=-180,
// the top-left of the Mercator projection's usable range).
inline double lonToPixelX(double lonDeg, int zoom) {
    return (lonDeg + 180.0) / 360.0 * worldSizePx(zoom);
}

inline double latToPixelY(double latDeg, int zoom) {
    const double latRad = latDeg * M_PI / 180.0;
    const double y = 0.5 - std::log(std::tan(M_PI / 4.0 + latRad / 2.0)) / (2.0 * M_PI);
    return y * worldSizePx(zoom);
}

inline double pixelXToLon(double pixelX, int zoom) {
    return pixelX / worldSizePx(zoom) * 360.0 - 180.0;
}

inline double pixelYToLat(double pixelY, int zoom) {
    const double y = pixelY / worldSizePx(zoom);
    const double n = M_PI - 2.0 * M_PI * y;
    return 180.0 / M_PI * std::atan(0.5 * (std::exp(n) - std::exp(-n)));
}

inline int32_t pixelToTileIndex(double pixel) {
    return static_cast<int32_t>(std::floor(pixel / kTileSizePx));
}

// Wraps a tile X index into [0, 2^zoom) -- longitude wraps around the globe;
// latitude (tile Y) does not and is expected to already be in range.
inline int32_t wrapTileIndex(int32_t index, int zoom) {
    const int32_t count = 1 << zoom;
    int32_t wrapped = index % count;
    if (wrapped < 0) {
        wrapped += count;
    }
    return wrapped;
}

}  // namespace cardmesh::map
