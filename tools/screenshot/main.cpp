// Renders the real DashboardScreen (src/ui/DashboardScreen.cpp) natively on
// the host, using a custom in-memory "display driver" instead of the Linux
// framebuffer, and dumps the result as a raw RGB888 file.
//
// This is a real capture of the app's actual UI code -- not a mockup -- but
// it has NOT been confirmed to match rendering on real CardputerZero
// hardware or its emulator (font rasterization, panel gamma, etc. could
// differ). See docs/DEVICE_BUILD.md.
//
// Usage: screenshot <scenario> <out.raw>
//   scenario: "connected" or "unreachable"
// Output is raw RGB888, 320x170, row-major, no header -- convert with
// tools/screenshot/raw_to_png.py.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "lvgl.h"

#include "ui/DashboardModel.h"
#include "ui/DashboardScreen.h"

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

}  // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        std::fprintf(stderr, "Usage: %s <connected|unreachable> <out.raw>\n", argv[0]);
        return EXIT_FAILURE;
    }
    const std::string scenario = argv[1];
    const std::string outPath = argv[2];

    lv_init();
    lv_lodepng_init();  // needed by DashboardScreen's imgfont (emoji rendering)

    lv_display_t* disp = lv_display_create(kWidth, kHeight);
    static uint16_t buf[kWidth * kHeight];
    lv_display_set_buffers(disp, buf, nullptr, sizeof(buf), LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(disp, flushCb);

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

    lv_timer_handler();
    lv_refr_now(disp);

    FILE* f = std::fopen(outPath.c_str(), "wb");
    if (f == nullptr) {
        std::fprintf(stderr, "Could not open %s for writing\n", outPath.c_str());
        return EXIT_FAILURE;
    }
    std::fwrite(g_rgb888, 1, sizeof(g_rgb888), f);
    std::fclose(f);

    std::printf("Wrote %s (%dx%d RGB888)\n", outPath.c_str(), kWidth, kHeight);
    return EXIT_SUCCESS;
}
