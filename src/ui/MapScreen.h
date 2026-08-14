#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "MapModel.h"
#include "lvgl.h"
#include "map/TileFetcher.h"

namespace cardmesh::ui {

// Node map screen: an offline-capable OpenStreetMap view (tiles fetched via
// map::TileFetcher/TileCache, cached to disk so previously-viewed areas
// still render with no connection) with node position markers.
//
// Keyboard-only pan/zoom (arrows + '+'/'-'), no touch input -- see
// EvdevKeyboard::GlobalKey. Not verified against real CardputerZero
// hardware; rendering performance for repeated tile decode/redraw during
// panning in particular is unconfirmed -- see docs/DEVICE_BUILD.md.
class MapScreen {
public:
    MapScreen(MapModel& model, map::TileFetcher& tileFetcher);
    ~MapScreen();

    lv_obj_t* screenObj() const { return screen_; }

    // Call periodically from the main loop while this screen is active:
    // updates markers if the node list changed, and polls the tile fetcher
    // for newly-arrived tiles.
    void refresh();

    void panUp();
    void panDown();
    void panLeft();
    void panRight();
    void zoomIn();
    void zoomOut();

private:
    struct TileSlot {
        int zoom = -1;
        int32_t tileX = 0;
        int32_t tileY = 0;
        std::unique_ptr<std::vector<uint8_t>> pngBytes;
        std::unique_ptr<lv_image_dsc_t> dsc;
        lv_obj_t* imgObj = nullptr;
    };

    void build();
    void updateTiles();
    void updateMarkers();
    void recenterOnNodesIfNeeded();
    void pan(double dxPx, double dyPx);

    MapModel& model_;
    map::TileFetcher& tileFetcher_;

    lv_obj_t* screen_ = nullptr;
    lv_obj_t* mapLayer_ = nullptr;    // holds tile image widgets, behind markers
    lv_obj_t* markerLayer_ = nullptr;  // holds marker dots, above tiles
    lv_obj_t* hintLabel_ = nullptr;

    std::array<TileSlot, 4> tileSlots_;
    std::vector<lv_obj_t*> markerObjs_;

    double centerLatDeg_ = 0.0;
    double centerLonDeg_ = 0.0;
    int zoomLevel_ = 4;
    bool viewInitialized_ = false;

    size_t lastNodeCount_ = static_cast<size_t>(-1);
    double lastCenterLat_ = 0.0;
    double lastCenterLon_ = 0.0;
    int lastZoom_ = -1;
};

}  // namespace cardmesh::ui
