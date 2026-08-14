#include "MapScreen.h"

#include <algorithm>
#include <cmath>

#include "map/WebMercator.h"

namespace cardmesh::ui {

namespace {

constexpr double kViewportW = 320.0;
constexpr double kViewportH = 170.0;
constexpr double kPanStepPx = 40.0;
constexpr int kMinZoom = 2;
constexpr int kMaxZoom = 18;

}  // namespace

MapScreen::MapScreen(MapModel& model, map::TileFetcher& tileFetcher)
    : model_(model), tileFetcher_(tileFetcher) {
    build();
}

MapScreen::~MapScreen() = default;

void MapScreen::build() {
    screen_ = lv_obj_create(nullptr);
    lv_obj_remove_style_all(screen_);
    lv_obj_set_size(screen_, static_cast<int32_t>(kViewportW), static_cast<int32_t>(kViewportH));
    lv_obj_set_style_bg_color(screen_, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen_, LV_OPA_COVER, 0);
    lv_obj_clear_flag(screen_, LV_OBJ_FLAG_SCROLLABLE);

    mapLayer_ = lv_obj_create(screen_);
    lv_obj_remove_style_all(mapLayer_);
    lv_obj_set_size(mapLayer_, static_cast<int32_t>(kViewportW), static_cast<int32_t>(kViewportH));
    lv_obj_set_pos(mapLayer_, 0, 0);
    lv_obj_clear_flag(mapLayer_, LV_OBJ_FLAG_SCROLLABLE);

    for (auto& slot : tileSlots_) {
        slot.imgObj = lv_image_create(mapLayer_);
        lv_obj_set_size(slot.imgObj, map::kTileSizePx, map::kTileSizePx);
    }

    markerLayer_ = lv_obj_create(screen_);
    lv_obj_remove_style_all(markerLayer_);
    lv_obj_set_size(markerLayer_, static_cast<int32_t>(kViewportW), static_cast<int32_t>(kViewportH));
    lv_obj_set_pos(markerLayer_, 0, 0);
    lv_obj_clear_flag(markerLayer_, LV_OBJ_FLAG_SCROLLABLE);

    hintLabel_ = lv_label_create(screen_);
    lv_label_set_text(hintLabel_, "Arrows Pan  +/- Zoom  B Back");
    lv_obj_set_style_text_color(hintLabel_, lv_color_white(), 0);
    lv_obj_set_style_bg_color(hintLabel_, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(hintLabel_, LV_OPA_70, 0);
    lv_obj_align(hintLabel_, LV_ALIGN_BOTTOM_LEFT, 2, -2);
}

void MapScreen::recenterOnNodesIfNeeded() {
    if (viewInitialized_) {
        return;
    }

    std::vector<MapNodeEntry> nodes;
    {
        std::lock_guard<std::mutex> lock(model_.mutex);
        nodes = model_.nodes;
    }
    if (nodes.empty()) {
        return;
    }

    double minLat = nodes.front().latitude;
    double maxLat = nodes.front().latitude;
    double minLon = nodes.front().longitude;
    double maxLon = nodes.front().longitude;
    for (const auto& node : nodes) {
        minLat = std::min(minLat, node.latitude);
        maxLat = std::max(maxLat, node.latitude);
        minLon = std::min(minLon, node.longitude);
        maxLon = std::max(maxLon, node.longitude);
    }
    centerLatDeg_ = (minLat + maxLat) / 2.0;
    centerLonDeg_ = (minLon + maxLon) / 2.0;

    // Pick the most zoomed-in level where every known node position still
    // fits in the viewport (with a margin) -- a single hardcoded zoom would
    // either strand widely-spread nodes off-screen or, for a tightly
    // clustered mesh, zoom out far more than necessary.
    constexpr double kFitMargin = 24.0;
    zoomLevel_ = kMinZoom;
    for (int candidate = kMaxZoom; candidate >= kMinZoom; candidate--) {
        const double spanXPx =
            map::lonToPixelX(maxLon, candidate) - map::lonToPixelX(minLon, candidate);
        const double spanYPx =
            map::latToPixelY(minLat, candidate) - map::latToPixelY(maxLat, candidate);
        if (spanXPx <= kViewportW - kFitMargin && spanYPx <= kViewportH - kFitMargin) {
            zoomLevel_ = candidate;
            break;
        }
    }
    viewInitialized_ = true;
}

void MapScreen::updateTiles() {
    const double centerPxX = map::lonToPixelX(centerLonDeg_, zoomLevel_);
    const double centerPxY = map::latToPixelY(centerLatDeg_, zoomLevel_);
    const double topLeftX = centerPxX - kViewportW / 2.0;
    const double topLeftY = centerPxY - kViewportH / 2.0;

    const int32_t anchorTileX = map::pixelToTileIndex(topLeftX);
    const int32_t anchorTileY = map::pixelToTileIndex(topLeftY);
    const int32_t maxTileIndex = (1 << zoomLevel_) - 1;

    int slotIndex = 0;
    for (int row = 0; row < 2; row++) {
        for (int col = 0; col < 2; col++) {
            TileSlot& slot = tileSlots_[slotIndex++];

            const int32_t tileX = map::wrapTileIndex(anchorTileX + col, zoomLevel_);
            int32_t tileY = anchorTileY + row;
            if (tileY < 0) tileY = 0;
            if (tileY > maxTileIndex) tileY = maxTileIndex;

            const double objX = (anchorTileX + col) * static_cast<double>(map::kTileSizePx) - topLeftX;
            const double objY = (anchorTileY + row) * static_cast<double>(map::kTileSizePx) - topLeftY;
            lv_obj_set_pos(slot.imgObj, static_cast<int32_t>(std::lround(objX)),
                           static_cast<int32_t>(std::lround(objY)));

            if (slot.zoom == zoomLevel_ && slot.tileX == tileX && slot.tileY == tileY && slot.dsc) {
                continue;  // already showing the right tile
            }

            tileFetcher_.request(zoomLevel_, tileX, tileY);
            auto bytes = tileFetcher_.tryGet(zoomLevel_, tileX, tileY);
            if (!bytes.has_value()) {
                continue;  // not ready yet -- leave whatever was there (stale beats blank while panning)
            }

            slot.zoom = zoomLevel_;
            slot.tileX = tileX;
            slot.tileY = tileY;
            slot.pngBytes = std::make_unique<std::vector<uint8_t>>(std::move(*bytes));

            // A fresh lv_image_dsc_t per tile change (not mutating the
            // previous one in place) so LVGL's image cache -- keyed by this
            // pointer's address -- never serves stale decoded pixels for
            // what looks like the same source but is now different content.
            slot.dsc = std::make_unique<lv_image_dsc_t>();
            slot.dsc->header.magic = LV_IMAGE_HEADER_MAGIC;
            slot.dsc->header.cf = LV_COLOR_FORMAT_RAW;
            slot.dsc->header.flags = 0;
            slot.dsc->header.w = 0;
            slot.dsc->header.h = 0;
            slot.dsc->header.stride = 0;
            slot.dsc->header.reserved_2 = 0;
            slot.dsc->data_size = static_cast<uint32_t>(slot.pngBytes->size());
            slot.dsc->data = slot.pngBytes->data();
            slot.dsc->reserved = nullptr;
            lv_image_set_src(slot.imgObj, slot.dsc.get());
        }
    }
}

void MapScreen::updateMarkers() {
    for (auto* obj : markerObjs_) {
        lv_obj_delete(obj);
    }
    markerObjs_.clear();

    std::vector<MapNodeEntry> nodes;
    {
        std::lock_guard<std::mutex> lock(model_.mutex);
        nodes = model_.nodes;
    }

    const double centerPxX = map::lonToPixelX(centerLonDeg_, zoomLevel_);
    const double centerPxY = map::latToPixelY(centerLatDeg_, zoomLevel_);
    const double topLeftX = centerPxX - kViewportW / 2.0;
    const double topLeftY = centerPxY - kViewportH / 2.0;

    for (const auto& node : nodes) {
        const double px = map::lonToPixelX(node.longitude, zoomLevel_) - topLeftX;
        const double py = map::latToPixelY(node.latitude, zoomLevel_) - topLeftY;
        if (px < -6 || px > kViewportW + 6 || py < -6 || py > kViewportH + 6) {
            continue;
        }

        lv_obj_t* dot = lv_obj_create(markerLayer_);
        lv_obj_remove_style_all(dot);
        lv_obj_set_size(dot, 7, 7);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(
            dot, node.favorite ? lv_palette_main(LV_PALETTE_YELLOW) : lv_palette_main(LV_PALETTE_RED), 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(dot, 1, 0);
        lv_obj_set_style_border_color(dot, lv_color_white(), 0);
        lv_obj_set_style_border_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_pos(dot, static_cast<int32_t>(std::lround(px)) - 3,
                       static_cast<int32_t>(std::lround(py)) - 3);
        markerObjs_.push_back(dot);
    }
}

void MapScreen::refresh() {
    recenterOnNodesIfNeeded();

    size_t nodeCount;
    {
        std::lock_guard<std::mutex> lock(model_.mutex);
        nodeCount = model_.nodes.size();
    }

    const bool viewChanged =
        (centerLatDeg_ != lastCenterLat_) || (centerLonDeg_ != lastCenterLon_) || (zoomLevel_ != lastZoom_);
    const bool nodesChanged = (nodeCount != lastNodeCount_);

    updateTiles();  // cheap when nothing changed; also polls for late tile arrivals

    if (viewChanged || nodesChanged) {
        updateMarkers();
        lastCenterLat_ = centerLatDeg_;
        lastCenterLon_ = centerLonDeg_;
        lastZoom_ = zoomLevel_;
        lastNodeCount_ = nodeCount;
    }
}

void MapScreen::pan(double dxPx, double dyPx) {
    const double centerPxX = map::lonToPixelX(centerLonDeg_, zoomLevel_) + dxPx;
    const double centerPxY = map::latToPixelY(centerLatDeg_, zoomLevel_) + dyPx;
    centerLonDeg_ = map::pixelXToLon(centerPxX, zoomLevel_);
    centerLatDeg_ = map::pixelYToLat(centerPxY, zoomLevel_);
    viewInitialized_ = true;  // manual pan overrides auto-recenter-on-nodes
}

void MapScreen::panUp() { pan(0.0, -kPanStepPx); }
void MapScreen::panDown() { pan(0.0, kPanStepPx); }
void MapScreen::panLeft() { pan(-kPanStepPx, 0.0); }
void MapScreen::panRight() { pan(kPanStepPx, 0.0); }

void MapScreen::zoomIn() {
    if (zoomLevel_ < kMaxZoom) {
        zoomLevel_++;
        viewInitialized_ = true;
    }
}

void MapScreen::zoomOut() {
    if (zoomLevel_ > kMinZoom) {
        zoomLevel_--;
        viewInitialized_ = true;
    }
}

}  // namespace cardmesh::ui
