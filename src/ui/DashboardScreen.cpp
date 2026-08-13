#include "DashboardScreen.h"

#include <cstdio>

namespace cardmesh::ui {

namespace {

void setValueText(lv_obj_t* label, int value) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%d", value);
    lv_label_set_text(label, buf);
}

}  // namespace

DashboardScreen::DashboardScreen(DashboardModel& model) : model_(model) { build(); }

void DashboardScreen::build() {
    lv_obj_t* screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_pad_all(screen, 4, 0);

    lv_obj_t* title = lv_label_create(screen);
    lv_label_set_text(title, "CARDMESH");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 2, 2);

    statusDot_ = lv_label_create(screen);
    lv_label_set_text(statusDot_, LV_SYMBOL_WARNING);
    lv_obj_align(statusDot_, LV_ALIGN_TOP_RIGHT, -2, 2);

    lv_obj_t* divider = lv_obj_create(screen);
    lv_obj_set_size(divider, 312, 1);
    lv_obj_set_style_bg_color(divider, lv_color_white(), 0);
    lv_obj_set_style_border_width(divider, 0, 0);
    lv_obj_align(divider, LV_ALIGN_TOP_LEFT, 2, 20);

    sourceLabel_ = lv_label_create(screen);
    lv_label_set_text(sourceLabel_, "SOURCE: --");
    lv_obj_set_style_text_color(sourceLabel_, lv_color_white(), 0);
    lv_obj_align(sourceLabel_, LV_ALIGN_TOP_LEFT, 2, 26);

    struct Row {
        const char* label;
        int y;
        lv_obj_t** valueOut;
    };
    const Row rows[] = {
        {"Nodes", 46, &nodesValue_},
        {"Active", 64, &activeValue_},
        {"Channels", 82, &channelsValue_},
        {"Unread", 100, &unreadValue_},
    };

    for (const auto& row : rows) {
        lv_obj_t* nameLabel = lv_label_create(screen);
        lv_label_set_text(nameLabel, row.label);
        lv_obj_set_style_text_color(nameLabel, lv_color_white(), 0);
        lv_obj_align(nameLabel, LV_ALIGN_TOP_LEFT, 4, row.y);

        lv_obj_t* valueLabel = lv_label_create(screen);
        lv_label_set_text(valueLabel, "--");
        lv_obj_set_style_text_color(valueLabel, lv_color_white(), 0);
        lv_obj_align(valueLabel, LV_ALIGN_TOP_RIGHT, -4, row.y);
        *row.valueOut = valueLabel;
    }

    statusLabel_ = lv_label_create(screen);
    lv_label_set_text(statusLabel_, "[R] Refresh   [Q] Quit");
    lv_obj_set_style_text_color(statusLabel_, lv_color_white(), 0);
    lv_obj_align(statusLabel_, LV_ALIGN_BOTTOM_LEFT, 2, -2);
}

void DashboardScreen::refresh() {
    bool connected;
    std::string sourceName;
    std::string statusMessage;
    int nodeCount;
    int activeCount;
    int channelCount;
    int unreadCount;

    {
        std::lock_guard<std::mutex> lock(model_.mutex);
        connected = model_.connected;
        sourceName = model_.sourceName;
        statusMessage = model_.statusMessage;
        nodeCount = model_.nodeCount;
        activeCount = model_.activeCount;
        channelCount = model_.channelCount;
        unreadCount = model_.unreadCount;
    }

    if (connected != lastConnected_) {
        lv_label_set_text(statusDot_, connected ? LV_SYMBOL_OK : LV_SYMBOL_CLOSE);
        lastConnected_ = connected;
    }
    if (sourceName != lastSourceName_ || statusMessage != lastStatusMessage_) {
        const std::string text =
            connected ? ("SOURCE: " + sourceName) : (sourceName.empty() ? statusMessage : sourceName);
        lv_label_set_text(sourceLabel_, text.c_str());
        lastSourceName_ = sourceName;
        lastStatusMessage_ = statusMessage;
    }
    if (nodeCount != lastNodeCount_) {
        setValueText(nodesValue_, nodeCount);
        lastNodeCount_ = nodeCount;
    }
    if (activeCount != lastActiveCount_) {
        setValueText(activeValue_, activeCount);
        lastActiveCount_ = activeCount;
    }
    if (channelCount != lastChannelCount_) {
        setValueText(channelsValue_, channelCount);
        lastChannelCount_ = channelCount;
    }
    if (unreadCount != lastUnreadCount_) {
        setValueText(unreadValue_, unreadCount);
        lastUnreadCount_ = unreadCount;
    }
}

}  // namespace cardmesh::ui
