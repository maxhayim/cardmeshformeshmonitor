#pragma once

#include <string>

#include "DashboardModel.h"
#include "lvgl.h"

namespace cardmesh::ui {

// The MVP dashboard screen (see the README's "Dashboard" mock). Other
// screens (Channels, Chat, Nodes, Direct Messages, Sources) are not yet
// implemented — see docs/DEVICE_BUILD.md.
class DashboardScreen {
public:
    explicit DashboardScreen(DashboardModel& model);

    // Call periodically from the LVGL timer/main loop; cheap no-op if the
    // model hasn't changed since the last call.
    void refresh();

private:
    void build();

    DashboardModel& model_;

    lv_obj_t* statusDot_ = nullptr;
    lv_obj_t* statusLabel_ = nullptr;
    lv_obj_t* sourceLabel_ = nullptr;
    lv_obj_t* nodesValue_ = nullptr;
    lv_obj_t* activeValue_ = nullptr;
    lv_obj_t* channelsValue_ = nullptr;
    lv_obj_t* unreadValue_ = nullptr;

    bool lastConnected_ = false;
    int lastNodeCount_ = -1;
    int lastActiveCount_ = -1;
    int lastChannelCount_ = -1;
    int lastUnreadCount_ = -1;
    std::string lastSourceName_;
    std::string lastStatusMessage_;
};

}  // namespace cardmesh::ui
