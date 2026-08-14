#include "TileFetcher.h"

#include <chrono>

namespace cardmesh::map {

TileFetcher::TileFetcher(TileCache cache) : cache_(std::move(cache)) {}

TileFetcher::~TileFetcher() { stop(); }

void TileFetcher::start() {
    if (running_.exchange(true)) {
        return;
    }
    thread_ = std::thread(&TileFetcher::run, this);
}

void TileFetcher::stop() {
    if (!running_.exchange(false)) {
        return;
    }
    if (thread_.joinable()) {
        thread_.join();
    }
}

void TileFetcher::request(int zoom, int32_t x, int32_t y) {
    const TileKey key{zoom, x, y};
    std::lock_guard<std::mutex> lock(mutex_);
    if (requested_.count(key) > 0) {
        return;
    }
    requested_.insert(key);
    pending_.push_back(key);
}

std::optional<std::vector<uint8_t>> TileFetcher::tryGet(int zoom, int32_t x, int32_t y) {
    const TileKey key{zoom, x, y};
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = ready_.find(key);
    if (it == ready_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void TileFetcher::run() {
    using namespace std::chrono_literals;

    while (running_) {
        std::optional<TileKey> next;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!pending_.empty()) {
                next = pending_.front();
                pending_.pop_front();
            }
        }

        if (!next.has_value()) {
            std::this_thread::sleep_for(50ms);
            continue;
        }

        auto bytes = cache_.getTile(next->zoom, next->x, next->y);
        if (bytes.has_value()) {
            std::lock_guard<std::mutex> lock(mutex_);
            ready_[*next] = std::move(*bytes);
        }
        // On failure, leave it out of `ready_`; it stays in `requested_` so
        // we don't hammer a failing/offline fetch every frame. A future
        // explicit "retry" (e.g. on reconnect) could clear `requested_`.
    }
}

}  // namespace cardmesh::map
