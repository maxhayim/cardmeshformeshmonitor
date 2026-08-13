#include "NetworkWorker.h"

#include <chrono>
#include <ctime>

#include "api/MeshMonitorClient.h"

namespace cardmesh::ui {

namespace {

// MeshMonitor does not define an "active" node status; this is CardMesh's
// own heuristic for the dashboard's "Active" tile.
constexpr std::int64_t kActiveWindowSeconds = 15 * 60;

using cardmesh::api::ConnectionConfig;
using cardmesh::api::MeshMonitorClient;

}  // namespace

NetworkWorker::NetworkWorker(storage::AppSettings settings, DashboardModel& model)
    : settings_(std::move(settings)), model_(model) {}

NetworkWorker::~NetworkWorker() { stop(); }

void NetworkWorker::start() {
    if (running_.exchange(true)) {
        return;
    }
    thread_ = std::thread(&NetworkWorker::run, this);
}

void NetworkWorker::stop() {
    if (!running_.exchange(false)) {
        return;
    }
    if (thread_.joinable()) {
        thread_.join();
    }
}

void NetworkWorker::requestRefreshNow() { refreshRequested_ = true; }

void NetworkWorker::run() {
    using namespace std::chrono_literals;

    while (running_) {
        if (refreshRequested_.exchange(false)) {
            pollOnce();
        }

        for (int waited = 0; waited < 10000 && running_; waited += 200) {
            std::this_thread::sleep_for(200ms);
            if (refreshRequested_) {
                break;
            }
        }
    }
}

void NetworkWorker::pollOnce() {
    ConnectionConfig config;
    config.server = settings_.server;
    config.port = settings_.port;
    config.https = settings_.https;
    config.apiToken = settings_.apiToken;

    MeshMonitorClient client(config);

    const auto sources = client.getSources();
    if (!sources.ok || sources.value.empty()) {
        std::lock_guard<std::mutex> lock(model_.mutex);
        model_.connected = false;
        model_.statusMessage = sources.ok ? "NO SOURCES" : "SERVER UNREACHABLE";
        return;
    }

    const models::Source* selected = nullptr;
    for (const auto& source : sources.value) {
        if (source.id == settings_.preferredSource) {
            selected = &source;
            break;
        }
    }
    if (selected == nullptr) {
        selected = &sources.value.front();
    }

    const auto nodes = client.getNodes(selected->id);
    const auto channels = client.getChannels(selected->id);

    const auto now = static_cast<std::int64_t>(std::time(nullptr));
    int activeCount = 0;
    if (nodes.ok) {
        for (const auto& node : nodes.value) {
            if (node.lastHeard > 0 && (now - node.lastHeard) <= kActiveWindowSeconds) {
                ++activeCount;
            }
        }
    }

    int unreadCount = 0;
    if (channels.ok) {
        for (const auto& channel : channels.value) {
            unreadCount += channel.unreadCount;
        }
    }

    std::lock_guard<std::mutex> lock(model_.mutex);
    model_.connected = true;
    model_.statusMessage = "CONNECTED";
    model_.sourceName = selected->name;
    model_.sourceCount = static_cast<int>(sources.value.size());
    model_.nodeCount = nodes.ok ? static_cast<int>(nodes.value.size()) : model_.nodeCount;
    model_.activeCount = activeCount;
    model_.channelCount = channels.ok ? static_cast<int>(channels.value.size()) : model_.channelCount;
    model_.unreadCount = unreadCount;
}

}  // namespace cardmesh::ui
