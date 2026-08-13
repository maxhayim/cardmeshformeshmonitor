// CardputerZero device entry point (LVGL + Linux framebuffer/evdev).
//
// This is the executable packaged as `cardmesh` in the arm64 .deb. It has
// NOT been run against real CardputerZero hardware or the online emulator —
// see docs/DEVICE_BUILD.md for exactly what is and isn't verified.

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <thread>

#include "lvgl.h"

#include "storage/Settings.h"
#include "ui/DashboardModel.h"
#include "ui/DashboardScreen.h"
#include "ui/EvdevKeyboard.h"
#include "ui/NetworkWorker.h"

namespace {

std::string envOr(const char* name, const std::string& fallback) {
    const char* value = std::getenv(name);
    return (value != nullptr && value[0] != '\0') ? std::string(value) : fallback;
}

}  // namespace

int main() {
    const auto settings = cardmesh::storage::Settings::load();
    if (!settings.has_value()) {
        std::cerr << "No configuration found at " << cardmesh::storage::Settings::defaultConfigPath()
                  << "\n";
        std::cerr << "First-run setup UI is not implemented yet; write the config file manually "
                     "(see README) and relaunch.\n";
        return EXIT_FAILURE;
    }

    lv_init();

    lv_display_t* display = lv_linux_fbdev_create();
    lv_linux_fbdev_set_file(display, envOr("CARDMESH_FB_DEVICE", "/dev/fb0").c_str());

    cardmesh::ui::EvdevKeyboard keyboard(envOr("CARDMESH_INPUT_DEVICE", "/dev/input/event0"));
    if (!keyboard.isOpen()) {
        std::cerr << "Warning: could not open input device; global shortcuts (R/Q) will not work.\n";
    }

    cardmesh::ui::DashboardModel model;
    cardmesh::ui::DashboardScreen dashboard(model);

    cardmesh::ui::NetworkWorker worker(*settings, model);
    worker.start();

    bool running = true;
    while (running) {
        const std::uint32_t sleepMs = lv_timer_handler();
        dashboard.refresh();

        switch (keyboard.poll()) {
            case cardmesh::ui::GlobalKey::Refresh:
                worker.requestRefreshNow();
                break;
            case cardmesh::ui::GlobalKey::Quit:
                running = false;
                break;
            case cardmesh::ui::GlobalKey::None:
                break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs > 0 ? sleepMs : 10));
    }

    worker.stop();
    return EXIT_SUCCESS;
}
