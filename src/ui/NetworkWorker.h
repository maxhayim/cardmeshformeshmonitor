#pragma once

#include <atomic>
#include <thread>

#include "DashboardModel.h"
#include "storage/Settings.h"

namespace cardmesh::ui {

// Polls MeshMonitor on a background thread and writes results into a
// DashboardModel, so the LVGL UI thread never blocks on network I/O
// (see the README's "Background Architecture" section).
class NetworkWorker {
public:
    NetworkWorker(storage::AppSettings settings, DashboardModel& model);
    ~NetworkWorker();

    void start();
    void stop();
    void requestRefreshNow();

private:
    void run();
    void pollOnce();

    storage::AppSettings settings_;
    DashboardModel& model_;
    std::thread thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> refreshRequested_{true};
};

}  // namespace cardmesh::ui
