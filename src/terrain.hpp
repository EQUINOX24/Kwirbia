#pragma once

#include <SDL3/SDL.h>
#include <cstdint>
#include <vector>
#include <string_view>

enum class WorldLayer : int32_t {
    SKY = 0,
    GROUND = 1,
    UNDERGROUND = 2,
    COUNT = 3
};

inline const char* layer_to_string(WorldLayer layer) noexcept {
    switch (layer) {
        case WorldLayer::SKY: return "Sky";
        case WorldLayer::GROUND: return "Ground";
        case WorldLayer::UNDERGROUND: return "Underground";
        default: return "Unknown";
    }
}

enum class TileType : uint8_t {
    // Sky tiles
    OPEN_SKY = 0,
    CLOUD_THIN,
    CLOUD_DENSE,
    FLOATING_ISLAND,

    // Ground tiles
    DEEP_WATER,
    SHALLOW_WATER,
    SAND,
    GRASS,
    FOREST,
    MOUNTAIN,
    SNOW_PEAK,

    // Underground tiles
    SOLID_ROCK,
    CAVERN_FLOOR,
    UNDERGROUND_WATER,
    LAVA,
    ORE_IRON,
    ORE_GOLD,
    ORE_GEM,

    COUNT
};

struct TileVisual {
    uint8_t glyph;
    SDL_Color fg;
    SDL_Color bg;
    std::string_view name;
    bool walkable;
};

class World {
public:
    World(int32_t width = 1024, int32_t height = 1024, int32_t seed = -1);

    void generate(int32_t seed = -1);

    int32_t width() const noexcept { return width_; }
    int32_t height() const noexcept { return height_; }
    int32_t seed() const noexcept { return seed_; }

    TileType get_tile(WorldLayer layer, int32_t x, int32_t y) const noexcept;
    const TileVisual& get_visual(TileType type) const noexcept;
    const TileVisual& get_tile_visual(WorldLayer layer, int32_t x, int32_t y) const noexcept;

    bool is_walkable(WorldLayer layer, int32_t x, int32_t y) const noexcept;

private:
    int32_t width_ = 1024;
    int32_t height_ = 1024;
    int32_t seed_ = 0;

    std::vector<TileType> sky_layer_;
    std::vector<TileType> ground_layer_;
    std::vector<TileType> underground_layer_;
};