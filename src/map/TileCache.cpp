#include "TileCache.h"

#include <filesystem>
#include <fstream>

#include "api/HttpClient.h"

namespace cardmesh::map {

namespace fs = std::filesystem;

TileCache::TileCache(std::string cacheDir, std::string tileServerTemplate, std::string userAgent)
    : cacheDir_(std::move(cacheDir)),
      tileServerTemplate_(std::move(tileServerTemplate)),
      userAgent_(std::move(userAgent)) {}

std::string TileCache::tilePath(int zoom, int32_t x, int32_t y) const {
    return (fs::path(cacheDir_) / std::to_string(zoom) / std::to_string(x) /
            (std::to_string(y) + ".png"))
        .string();
}

std::string TileCache::tileUrl(int zoom, int32_t x, int32_t y) const {
    std::string url = tileServerTemplate_;
    const auto replace = [&url](const std::string& token, const std::string& value) {
        const auto pos = url.find(token);
        if (pos != std::string::npos) {
            url.replace(pos, token.size(), value);
        }
    };
    replace("{z}", std::to_string(zoom));
    replace("{x}", std::to_string(x));
    replace("{y}", std::to_string(y));
    return url;
}

std::optional<std::vector<uint8_t>> TileCache::getTile(int zoom, int32_t x, int32_t y) {
    const std::string path = tilePath(zoom, x, y);

    if (fs::exists(path)) {
        std::ifstream file(path, std::ios::binary);
        if (file.is_open()) {
            return std::vector<uint8_t>(std::istreambuf_iterator<char>(file),
                                        std::istreambuf_iterator<char>());
        }
    }

    const auto response =
        cardmesh::api::fetchUrl(tileUrl(zoom, x, y), {"User-Agent: " + userAgent_}, 10);
    if (!response.ok() || response.body.empty()) {
        return std::nullopt;
    }

    std::error_code ec;
    fs::create_directories(fs::path(path).parent_path(), ec);
    if (!ec) {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        if (out.is_open()) {
            out.write(response.body.data(), static_cast<std::streamsize>(response.body.size()));
        }
    }

    return std::vector<uint8_t>(response.body.begin(), response.body.end());
}

}  // namespace cardmesh::map
