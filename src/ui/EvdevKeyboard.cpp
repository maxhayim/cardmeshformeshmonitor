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
        if (event.code == KEY_R) {
            result = GlobalKey::Refresh;
        } else if (event.code == KEY_Q || event.code == KEY_ESC) {
            result = GlobalKey::Quit;
        }
    }

    return result;
}

}  // namespace cardmesh::ui
