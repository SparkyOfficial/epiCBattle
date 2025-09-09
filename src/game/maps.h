#pragma once

#include "raylib.h"
#include <vector>
#include "core/types.h"

struct MapData {
    Vector3 arenaSize;
    Color arenaColor;
    std::vector<AABB> obstacles;
};

static inline MapData loadMapData(MapType mapType) {
    MapData data{};
    if (mapType == MapType::Green) {
        data.arenaSize = {30.0f, 1.0f, 30.0f};
        data.arenaColor = DARKGREEN;
        data.obstacles.push_back({ {-0.75f, 0.0f, -0.75f}, {0.75f, 1.5f, 0.75f} });
        data.obstacles.push_back({ {-6.5f, 0.0f,  2.5f}, {-5.5f, 1.0f, 5.5f} });
        data.obstacles.push_back({ { 5.5f, 0.0f, -5.5f}, { 6.5f, 1.0f,-2.5f} });
    } else {
        data.arenaSize = {36.0f, 1.0f, 22.0f};
        data.arenaColor = Color{200, 180, 120, 255};
        data.obstacles.push_back({ {-2.5f, 0.0f, -1.0f}, {2.5f, 1.2f, 1.0f} });
        data.obstacles.push_back({ {-12.0f,0.0f, -9.0f}, {-9.0f, 1.0f,-6.0f} });
        data.obstacles.push_back({ {  9.0f,0.0f,  6.0f}, {12.0f, 1.0f, 9.0f} });
    }
    return data;
}


