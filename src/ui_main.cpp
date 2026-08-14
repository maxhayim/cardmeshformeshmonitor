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

#include "map/TileFetcher.h"
#include "storage/Settings.h"
#include "ui/DashboardModel.h"
#include "ui/DashboardScreen.h"
#include "ui/EvdevKeyboard.h"
#include "ui/MapModel.h"
#include "ui/MapScreen.h"
#include "ui/NetworkWorker.h"

namespace {

std::string envOr(const char* name, const std::string& fallback) {
    const char* value = std::getenv(name);
    return (value != nullptr && value[0] != '\0') ? std::string(value) : fallback;
}

enum class ActiveScreen { Dashboard, Map };

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
    lv_lodepng_init();  // needed by DashboardScreen's imgfont and MapScreen's tiles

    lv_display_t* display = lv_linux_fbdev_create();
    lv_linux_fbdev_set_file(display, envOr("CARDMESH_FB_DEVICE", "/dev/fb0").c_str());

    cardmesh::ui::EvdevKeyboard keyboard(envOr("CARDMESH_INPUT_DEVICE", "/dev/input/event0"));
    if (!keyboard.isOpen()) {
        std::cerr << "Warning: could not open input device; global shortcuts will not work.\n";
    }

    const std::filesystem::path stateDir =
        std::filesystem::path(std::getenv("HOME") != nullptr ? std::getenv("HOME") : ".") /
        ".local" / "state" / "cardmesh";
    std::error_code ec;
    std::filesystem::create_directories(stateDir / "tiles", ec);

    cardmesh::map::TileFetcher tileFetcher(cardmesh::map::TileCache(
        (stateDir / "tiles").string(), settings->tileServerTemplate,
        "CardMesh/0.1.0 (+https://github.com/maxhayim/cardmeshformeshmonitor)"));
    tileFetcher.start();

    cardmesh::ui::DashboardModel model;
    cardmesh::ui::MapModel mapModel;
    cardmesh::ui::DashboardScreen dashboard(model);
    cardmesh::ui::MapScreen map(mapModel, tileFetcher);

    cardmesh::ui::NetworkWorker worker(*settings, model, mapModel);
    worker.start();

    ActiveScreen active = ActiveScreen::Dashboard;
    lv_screen_load(dashboard.screenObj());

    bool running = true;
    while (running) {
        const std::uint32_t sleepMs = lv_timer_handler();

        if (active == ActiveScreen::Dashboard) {
            dashboard.refresh();
        } else {
            map.refresh();
        }

        switch (keyboard.poll()) {
            case cardmesh::ui::GlobalKey::Refresh:
                worker.requestRefreshNow();
                break;
            case cardmesh::ui::GlobalKey::Quit:
                running = false;
                break;
            case cardmesh::ui::GlobalKey::ToggleMap:
                if (active == ActiveScreen::Dashboard) {
                    active = ActiveScreen::Map;
                    lv_screen_load(map.screenObj());
                }
                break;
            case cardmesh::ui::GlobalKey::Back:
                if (active == ActiveScreen::Map) {
                    active = ActiveScreen::Dashboard;
                    lv_screen_load(dashboard.screenObj());
                }
                break;
            case cardmesh::ui::GlobalKey::PanUp:
                if (active == ActiveScreen::Map) map.panUp();
                break;
            case cardmesh::ui::GlobalKey::PanDown:
                if (active == ActiveScreen::Map) map.panDown();
                break;
            case cardmesh::ui::GlobalKey::PanLeft:
                if (active == ActiveScreen::Map) map.panLeft();
                break;
            case cardmesh::ui::GlobalKey::PanRight:
                if (active == ActiveScreen::Map) map.panRight();
                break;
            case cardmesh::ui::GlobalKey::ZoomIn:
                if (active == ActiveScreen::Map) map.zoomIn();
                break;
            case cardmesh::ui::GlobalKey::ZoomOut:
                if (active == ActiveScreen::Map) map.zoomOut();
                break;
            case cardmesh::ui::GlobalKey::None:
                break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs > 0 ? sleepMs : 10));
    }

    tileFetcher.stop();
    worker.stop();
    return EXIT_SUCCESS;
}
