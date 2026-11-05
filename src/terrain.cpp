#include "terrain.hpp"
#include "random/bxperlin.hpp"
#include "random/bxrng.hpp"
#include <iostream>
#include <algorithm>

static const TileVisual TILE_VISUALS[static_cast<std::size_t>(TileType::COUNT)] = {
    // Sky
    { ' ',  {180, 210, 255, 255}, {15, 28, 55, 255},   "Open Sky",        true },
    { '~',  {210, 225, 255, 255}, {35, 55, 95, 255},   "Thin Clouds",     true },
    { 176,  {245, 245, 255, 255}, {70, 90, 140, 255},  "Dense Clouds",    true },
    { '^',  {120, 190, 70, 255},  {45, 75, 40, 255},   "Floating Island", true },

    // Ground
    { '~',  {15, 80, 160, 255},   {5, 25, 60, 255},    "Deep Sea",        false },
    { '~',  {35, 140, 210, 255},  {10, 45, 95, 255},   "Shallow Water",   false },
    { '.',  {225, 205, 140, 255}, {140, 120, 70, 255}, "Sand Beach",      true },
    { '"',  {110, 215, 45, 255},  {35, 90, 20, 255},   "Grassland",       true },
    { 'T',  {40, 160, 50, 255},   {20, 60, 15, 255},   "Forest",          true },
    { '^',  {170, 170, 180, 255}, {70, 70, 75, 255},   "Mountain",        false },
    { '^',  {255, 255, 255, 255}, {140, 150, 165, 255},"Snow Peak",       false },

    // Underground
    { 177,  {100, 90, 85, 255},   {35, 30, 25, 255},   "Solid Rock",      false },
    { '.',  {130, 120, 115, 255}, {20, 18, 18, 255},   "Cavern Floor",    true },
    { '~',  {45, 110, 165, 255},  {12, 28, 48, 255},   "Subterranean Pool", false },
    { '~',  {255, 210, 0, 255},   {190, 40, 0, 255},   "Magma",           false },
    { '*',  {220, 155, 125, 255}, {35, 30, 25, 255},   "Iron Ore",        false },
    { '$',  {255, 220, 0, 255},   {35, 30, 25, 255},   "Gold Ore",        false },
    { '*',  {60, 230, 255, 255},  {35, 30, 25, 255},   "Gem Vein",        false },
};

World::World(int32_t width, int32_t height, int32_t seed)
    : width_(width), height_(height)
{
    generate(seed);
}

void World::generate(int32_t seed)
{
    if (seed < 0) {
        seed_ = static_cast<int32_t>(bxrng::random_seed() % 2147483647);
        std::cout << "Generating world with random seed: " << seed_ << "\n";
    } else {
        seed_ = seed;
        std::cout << "Generating world with seed: " << seed_ << "\n";
    }

    bxrng::BxRng rng(static_cast<uint32_t>(seed_));
    int32_t pseed = static_cast<int32_t>(rng.randInt() % 256);

    bxperlin::PerlinNoise perlin(pseed);
    bxrng::shuffle(rng, perlin.perm_hash().data(), perlin.perm_hash().size());

    const std::size_t total_tiles = static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_);
    sky_layer_.resize(total_tiles, TileType::OPEN_SKY);
    ground_layer_.resize(total_tiles, TileType::GRASS);
    underground_layer_.resize(total_tiles, TileType::SOLID_ROCK);

    const double nscale = static_cast<double>(height_) / 16.0;
    const double mscale = static_cast<double>(width_) / 16.0;

    for (int32_t y = 0; y < height_; ++y) {
        for (int32_t x = 0; x < width_; ++x) {
            std::size_t idx = static_cast<std::size_t>(y * width_ + x);

            double nx = static_cast<double>(x) / mscale;
            double ny = static_cast<double>(y) / nscale;

            // 1) Ground Layer Generation
            double elevation = perlin.mfperlin(nx, ny, 0.25, 6);
            double moisture  = perlin.perlin(nx * 1.5 + 10.0, ny * 1.5 + 10.0, 0.2, 4);

            if (elevation < 0.38) {
                ground_layer_[idx] = TileType::DEEP_WATER;
            } else if (elevation < 0.44) {
                ground_layer_[idx] = TileType::SHALLOW_WATER;
            } else if (elevation < 0.48) {
                ground_layer_[idx] = TileType::SAND;
            } else if (elevation < 0.72) {
                if (moisture > 0.58) {
                    ground_layer_[idx] = TileType::FOREST;
                } else {
                    ground_layer_[idx] = TileType::GRASS;
                }
            } else if (elevation < 0.85) {
                ground_layer_[idx] = TileType::MOUNTAIN;
            } else {
                ground_layer_[idx] = TileType::SNOW_PEAK;
            }

            // 2) Underground Layer Generation (Caverns, Ores, Lava)
            double cave_noise = perlin.perlin(nx * 2.2 + 50.0, ny * 2.2 + 50.0, 0.3, 4);
            double sub_feature = perlin.perlin(nx * 3.0 + 80.0, ny * 3.0 + 80.0, 0.4, 3);

            if (cave_noise > 0.52) {
                if (sub_feature > 0.72) {
                    underground_layer_[idx] = TileType::LAVA;
                } else if (sub_feature < 0.28) {
                    underground_layer_[idx] = TileType::UNDERGROUND_WATER;
                } else {
                    underground_layer_[idx] = TileType::CAVERN_FLOOR;
                }
            } else {
                // Ore veins in solid rock
                double ore_noise = perlin.perlin(nx * 5.0 + 120.0, ny * 5.0 + 120.0, 0.5, 2);
                if (ore_noise > 0.82) {
                    underground_layer_[idx] = TileType::ORE_GEM;
                } else if (ore_noise > 0.76) {
                    underground_layer_[idx] = TileType::ORE_GOLD;
                } else if (ore_noise > 0.68) {
                    underground_layer_[idx] = TileType::ORE_IRON;
                } else {
                    underground_layer_[idx] = TileType::SOLID_ROCK;
                }
            }

            // 3) Sky Layer Generation (Clouds, Floating islands)
            double cloud_noise = perlin.perlin(nx * 1.8 + 200.0, ny * 1.8 + 200.0, 0.2, 4);
            if (cloud_noise > 0.78) {
                sky_layer_[idx] = TileType::FLOATING_ISLAND;
            } else if (cloud_noise > 0.60) {
                sky_layer_[idx] = TileType::CLOUD_DENSE;
            } else if (cloud_noise > 0.45) {
                sky_layer_[idx] = TileType::CLOUD_THIN;
            } else {
                sky_layer_[idx] = TileType::OPEN_SKY;
            }
        }
    }
}

TileType World::get_tile(WorldLayer layer, int32_t x, int32_t y) const noexcept
{
    // Clamp coordinates safely
    int32_t clamped_x = std::clamp(x, 0, width_ - 1);
    int32_t clamped_y = std::clamp(y, 0, height_ - 1);
    std::size_t idx = static_cast<std::size_t>(clamped_y * width_ + clamped_x);

    switch (layer) {
        case WorldLayer::SKY:
            return sky_layer_[idx];
        case WorldLayer::GROUND:
            return ground_layer_[idx];
        case WorldLayer::UNDERGROUND:
            return underground_layer_[idx];
        default:
            return TileType::GRASS;
    }
}

const TileVisual& World::get_visual(TileType type) const noexcept
{
    auto idx = static_cast<std::size_t>(type);
    if (idx >= static_cast<std::size_t>(TileType::COUNT)) {
        idx = 0;
    }
    return TILE_VISUALS[idx];
}

const TileVisual& World::get_tile_visual(WorldLayer layer, int32_t x, int32_t y) const noexcept
{
    return get_visual(get_tile(layer, x, y));
}

bool World::is_walkable(WorldLayer layer, int32_t x, int32_t y) const noexcept
{
    return get_tile_visual(layer, x, y).walkable;
}