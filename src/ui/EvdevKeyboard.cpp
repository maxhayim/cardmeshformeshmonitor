#include "EvdevKeyboard.h"

#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

namespace cardmesh::ui {

EvdevKeyboard::EvdevKeyboard(const std::string& devicePath) {
    fd_ = ::open(devicePath.c_str(), O_RDONLY | O_NONBLOCK);
}

EvdevKeyboard::~EvdevKeyboard() {
    if (fd_ >= 0) {
        ::close(fd_);
    }
}

bool EvdevKeyboard::isOpen() const { return fd_ >= 0; }

GlobalKey EvdevKeyboard::poll() {
    if (fd_ < 0) {
        return GlobalKey::None;
    }

    struct input_event event {};
    GlobalKey result = GlobalKey::None;

    while (true) {
        const ssize_t bytesRead = ::read(fd_, &event, sizeof(event));
        if (bytesRead != static_cast<ssize_t>(sizeof(event))) {
            break;
        }
        if (event.type != EV_KEY || event.value != 1) {
            continue;
        }
        switch (event.code) {
            case KEY_R:
                result = GlobalKey::Refresh;
                break;
            case KEY_Q:
                result = GlobalKey::Quit;
                break;
            case KEY_M:
                result = GlobalKey::ToggleMap;
                break;
            case KEY_ESC:
            case KEY_B:
                // KEY_B as a fallback: CardputerZero's chiclet keyboard may
                // not have a reliable dedicated Esc key -- unconfirmed on
                // real hardware, see docs/DEVICE_BUILD.md.
                result = GlobalKey::Back;
                break;
            case KEY_UP:
                result = GlobalKey::PanUp;
                break;
            case KEY_DOWN:
                result = GlobalKey::PanDown;
                break;
            case KEY_LEFT:
                result = GlobalKey::PanLeft;
                break;
            case KEY_RIGHT:
                result = GlobalKey::PanRight;
                break;
            case KEY_EQUAL:
            case KEY_KPPLUS:
                result = GlobalKey::ZoomIn;
                break;
            case KEY_MINUS:
            case KEY_KPMINUS:
                result = GlobalKey::ZoomOut;
                break;
            default:
                break;
        }
    }

    return result;
}

}  // namespace cardmesh::ui
