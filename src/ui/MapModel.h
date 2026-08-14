#pragma once

#include <mutex>
#include <string>
#include <vector>

namespace cardmesh::ui {

struct MapNodeEntry {
    std::string id;
    std::string name;
    double latitude = 0.0;
    double longitude = 0.0;
    bool favorite = false;
};

// Shared state written by NetworkWorker (background thread) and read by
// MapScreen (main thread). All access must hold `mutex`. Separate from
// DashboardModel since most nodes won't have a known position and the
// Dashboard doesn't need per-node detail, just counts.
struct MapModel {
    std::mutex mutex;

    bool connected = false;
    std::vector<MapNodeEntry> nodes;  // only nodes with a known lat/lon
};

}  // namespace cardmesh::ui
