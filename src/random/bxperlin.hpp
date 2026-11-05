#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <cmath>

constexpr std::size_t PERM_HASH_LEN = 256U;

namespace bxperlin
{
    class PerlinNoise
    {
    public:
        explicit PerlinNoise(int32_t seed = 0) noexcept;

        void set_seed(int32_t s) noexcept;

        double perlin(double x,
                      double y,
                      double freq = 1.0,
                      int32_t depth = 1) const noexcept;

        double mfperlin(double x,
                        double y,
                        double freq = 1.0,
                        int32_t depth = 1) const noexcept;

        std::array<int32_t, PERM_HASH_LEN>& perm_hash() noexcept { return perm_hash_; }
        const std::array<int32_t, PERM_HASH_LEN>& perm_hash() const noexcept { return perm_hash_; }

    private:
        int32_t seed_ = 0;
        std::array<int32_t, PERM_HASH_LEN> perm_hash_{};

        int32_t noise2d(int32_t x, int32_t y) const noexcept;
        double dot_grid_grad(int32_t ix, int32_t iy, double x, double y) const noexcept;
        double perlin_layer(double x, double y) const noexcept;
    };

    // Global default instance and backwards-compatible free functions
    extern PerlinNoise default_generator;
    extern std::array<int32_t, PERM_HASH_LEN>& perm_hash;

    void set_perlin_seed(int32_t s) noexcept;
    double perlin(double x, double y, double freq = 1.0, int32_t depth = 1) noexcept;
    double mfperlin(double x, double y, double freq = 1.0, int32_t depth = 1) noexcept;
}