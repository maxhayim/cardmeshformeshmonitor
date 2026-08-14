// Renders real screen code (src/ui/DashboardScreen.cpp, src/ui/MapScreen.cpp)
// natively on the host, using a custom in-memory "display driver" instead of
// the Linux framebuffer, and dumps the result as a raw RGB888 file.
//
// This is a real capture of the app's actual UI code -- not a mockup -- but
// it has NOT been confirmed to match rendering on real CardputerZero
// hardware or its emulator (font rasterization, panel gamma, etc. could
// differ). See docs/DEVICE_BUILD.md.
//
// Usage: screenshot <scenario> <out.raw>
//   scenario: "connected", "unreachable", or "map"
//   "map" fetches real OpenStreetMap tiles over the network (into
//   /tmp/cardmesh-screenshot-tiles) to render a real, not mocked, map.
// Output is raw RGB888, 320x170, row-major, no header -- convert with
// tools/screenshot/raw_to_png.py.

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>

#include "lvgl.h"

#include "map/TileFetcher.h"
#include "ui/DashboardModel.h"
#include "ui/DashboardScreen.h"
#include "ui/MapModel.h"
#include "ui/MapScreen.h"

namespace {

constexpr int32_t kWidth = 320;
constexpr int32_t kHeight = 170;

uint8_t g_rgb888[kWidth * kHeight * 3];

void flushCb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    // LV_COLOR_DEPTH is 16 (RGB565) to match the real device config.
    auto* src = reinterpret_cast<uint16_t*>(px_map);
    for (int32_t y = area->y1; y <= area->y2; y++) {
        for (int32_t x = area->x1; x <= area->x2; x++) {
            const uint16_t pixel = *src++;
            const uint8_t r5 = (pixel >> 11) & 0x1F;
            const uint8_t g6 = (pixel >> 5) & 0x3F;
            const uint8_t b5 = pixel & 0x1F;
            const uint8_t r8 = static_cast<uint8_t>((r5 * 255) / 31);
            const uint8_t g8 = static_cast<uint8_t>((g6 * 255) / 63);
            const uint8_t b8 = static_cast<uint8_t>((b5 * 255) / 31);

            uint8_t* dst = &g_rgb888[(y * kWidth + x) * 3];
            dst[0] = r8;
            dst[1] = g8;
            dst[2] = b8;
        }
    }
    lv_display_flush_ready(disp);
}

void writeRaw(const std::string& outPath) {
    FILE* f = std::fopen(outPath.c_str(), "wb");
    if (f == nullptr) {
        std::fprintf(stderr, "Could not open %s for writing\n", outPath.c_str());
        std::exit(EXIT_FAILURE);
    }
    std::fwrite(g_rgb888, 1, sizeof(g_rgb888), f);
    std::fclose(f);
    std::printf("Wrote %s (%dx%d RGB888)\n", outPath.c_str(), kWidth, kHeight);
}

void renderDashboard(const std::string& scenario, lv_display_t* disp) {
    cardmesh::ui::DashboardModel model;
    {
        std::lock_guard<std::mutex> lock(model.mutex);
        if (scenario == "connected") {
            model.connected = true;
            model.statusMessage = "CONNECTED";
            model.sourceName = "Yeraze StationG2 \xF0\x9F\x9A\x89";
            model.sourceCount = 5;
            model.nodeCount = 127;
            model.activeCount = 83;
            model.channelCount = 8;
            model.unreadCount = 4;
        } else {
            model.connected = false;
            model.statusMessage = "SERVER UNREACHABLE";
        }
    }

    cardmesh::ui::DashboardScreen dashboard(model);
    dashboard.refresh();
    lv_screen_load(dashboard.screenObj());

    lv_timer_handler();
    lv_refr_now(disp);
}

void renderMap(lv_display_t* disp) {
    // Real Twemoji-style pipeline reused here: real HTTP fetch against the
    // real OSM tile server, real disk cache -- not mocked. Mock is only the
    // node position list (this dev machine isn't near real mesh nodes).
    cardmesh::map::TileFetcher tileFetcher(cardmesh::map::TileCache(
        "/tmp/cardmesh-screenshot-tiles", "https://tile.openstreetmap.org/{z}/{x}/{y}.png",
        "CardMesh/0.1.0-screenshot-tool (+https://github.com/maxhayim/cardmeshformeshmonitor)"));
    tileFetcher.start();

    cardmesh::ui::MapModel mapModel;
    {
        std::lock_guard<std::mutex> lock(mapModel.mutex);
        mapModel.connected = true;
        // Miami-Dade area, matching the README's own example node names
        // (Doral, Brickell, Kendall).
        mapModel.nodes = {
            {"!a1", "Doral Base", 25.8195, -80.3553, true},
            {"!a2", "Brickell", 25.7617, -80.1918, false},
            {"!a3", "Kendall Solar", 25.6793, -80.3173, false},
            {"!a4", "MTEDC II", 25.7743, -80.2436, false},
        };
    }

    cardmesh::ui::MapScreen map(mapModel, tileFetcher);
    lv_screen_load(map.screenObj());

    // Tile fetches happen on TileFetcher's background thread; poll refresh()
    // for a few seconds to give them a chance to arrive before rendering.
    for (int i = 0; i < 100; i++) {
        map.refresh();
        lv_timer_handler();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    lv_refr_now(disp);

    tileFetcher.stop();
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        std::fprintf(stderr, "Usage: %s <connected|unreachable|map> <out.raw>\n", argv[0]);
        return EXIT_FAILURE;
    }
    const std::string scenario = argv[1];
    const std::string outPath = argv[2];

    lv_init();
    lv_lodepng_init();  // needed by DashboardScreen's imgfont and MapScreen's tiles

    lv_display_t* disp = lv_display_create(kWidth, kHeight);
    static uint16_t buf[kWidth * kHeight];
    lv_display_set_buffers(disp, buf, nullptr, sizeof(buf), LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(disp, flushCb);

    if (scenario == "map") {
        renderMap(disp);
    } else {
        renderDashboard(scenario, disp);
    }

    writeRaw(outPath);
    return EXIT_SUCCESS;
}
