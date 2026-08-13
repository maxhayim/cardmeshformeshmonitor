#pragma once

#include <string>

namespace cardmesh::ui {

enum class GlobalKey { None, Refresh, Quit };

// Minimal, non-blocking reader for global keyboard shortcuts on a Linux
// evdev device (e.g. /dev/input/event0). Deliberately bypasses LVGL's
// focus/group indev system since the MVP dashboard has no focusable
// widgets — see lv_conf.h for that tradeoff.
//
// ASSUMPTION: the device path is not confirmed against real CardputerZero
// hardware; override via the CARDMESH_INPUT_DEVICE environment variable if
// /dev/input/event0 is wrong for the target.
class EvdevKeyboard {
public:
    explicit EvdevKeyboard(const std::string& devicePath);
    ~EvdevKeyboard();

    EvdevKeyboard(const EvdevKeyboard&) = delete;
    EvdevKeyboard& operator=(const EvdevKeyboard&) = delete;

    bool isOpen() const;

    // Non-blocking: returns GlobalKey::None if nothing new was pressed.
    GlobalKey poll();

private:
    int fd_ = -1;
};

}  // namespace cardmesh::ui
