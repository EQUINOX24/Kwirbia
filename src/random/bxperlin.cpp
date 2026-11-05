#define _USE_MATH_DEFINES
#include "bxperlin.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstddef>

namespace bxperlin
{
    constexpr double PI  = 3.14159265358979323846;
    constexpr double TAU = 2.0 * PI;

    static const std::array<int32_t, PERM_HASH_LEN> initial_perm_hash = {
        39, 173,  91, 230, 226,  50, 126, 225,  41, 235, 234, 115,  67, 170,  42, 246,
        85,  11, 160,  48, 120, 241, 117, 130,  45, 25, 216, 218, 188, 138, 156, 143,
        224, 105, 113, 194, 114,  76, 250, 239,  81,  84,  21, 134, 149,  93, 159,  43,
        157,  82,  79, 128, 116, 177, 244, 161, 222, 133, 249,  49, 220,  95, 248,  34,
        124,   3,  88, 181, 119,  83,  31, 118, 121, 182,  16, 101, 153, 129,  58,  14,
        247,  73, 166,   4,   2,  64,  30, 142,  78,  59, 174, 229,  20,  44, 204, 132,
        205, 135,  66,  90, 139,  56, 185, 104,  60, 195,  37,  23, 253, 252,  27, 255,
        87,  36,  62, 190, 208, 125, 180, 210,  12, 233, 202, 145, 178, 191, 102,  29,
        106,  89, 254, 107, 152, 171, 215, 131,  17, 223,  46, 187, 183,  74,  94, 103,
        231, 169, 158, 179, 167, 219, 136,  98,  33,  57, 200, 164,  86,  63,  61,  71,
        1,   32,  40,  97, 196,  51,  53, 206,  99, 111, 209, 217, 212, 176, 100, 237,
        172, 140, 127,  69, 110, 148, 243, 163, 203,   5,   7, 232, 112, 175,  92, 238,
        242, 189,  28, 207,  68, 154, 201,  80, 162,  35,  72,  22, 155,  15,  65,  70,
        228, 141, 213,   0,  26, 186,  10, 150,   6, 165,  55,  38, 211, 199,  18,  75,
        227, 236, 168,  96,   9, 251, 214, 221, 184, 109, 245, 193,  19, 108,  47,  77,
        13, 240,  54,  52, 197, 123,  24, 144, 122, 198, 147, 137, 192, 146,   8, 151
    };

    static double fade_cubic(double w) noexcept
    {
        return w * w * (3.0 - 2.0 * w);
    }

    [[maybe_unused]]
    static double fade_quintic(double w) noexcept
    {
        return w * w * w * (w * (w * 6.0 - 15.0) + 10.0);
    }

    static double interpolate(double a0, double a1, double w) noexcept
    {
        if (w <= 0.0) return a0;
        if (w >= 1.0) return a1;
        return (a1 - a0) * fade_cubic(w) + a0;
    }

    PerlinNoise::PerlinNoise(int32_t s) noexcept
        : seed_(s), perm_hash_(initial_perm_hash)
    {
        set_seed(s);
    }

    void PerlinNoise::set_seed(int32_t s) noexcept
    {
        seed_ = s % static_cast<int32_t>(PERM_HASH_LEN);
        if (seed_ < 0) seed_ += static_cast<int32_t>(PERM_HASH_LEN);
    }

    int32_t PerlinNoise::noise2d(int32_t x, int32_t y) const noexcept
    {
        std::size_t idx_y = static_cast<std::size_t>(y + seed_) & (PERM_HASH_LEN - 1);
        int32_t tmp = perm_hash_[idx_y];
        std::size_t idx = static_cast<std::size_t>(tmp + x) & (PERM_HASH_LEN - 1);
        return perm_hash_[idx];
    }

    double PerlinNoise::dot_grid_grad(int32_t ix, int32_t iy, double x, double y) const noexcept
    {
        int32_t randint = noise2d(ix, iy);
        double dx = x - static_cast<double>(ix);
        double dy = y - static_cast<double>(iy);
        double angle = TAU * (static_cast<double>(randint) / static_cast<double>(PERM_HASH_LEN));
        return dx * std::cos(angle) + dy * std::sin(angle);
    }

    double PerlinNoise::perlin_layer(double x, double y) const noexcept
    {
        double x0f = std::floor(x);
        double y0f = std::floor(y);
        int32_t x0 = static_cast<int32_t>(x0f);
        int32_t y0 = static_cast<int32_t>(y0f);
        int32_t x1 = x0 + 1;
        int32_t y1 = y0 + 1;

        double sx = x - x0f;
        double sy = y - y0f;

        double n0 = dot_grid_grad(x0, y0, x, y);
        double n1 = dot_grid_grad(x1, y0, x, y);
        double ix0 = interpolate(n0, n1, sx);

        n0 = dot_grid_grad(x0, y1, x, y);
        n1 = dot_grid_grad(x1, y1, x, y);
        double ix1 = interpolate(n0, n1, sx);

        double value = interpolate(ix0, ix1, sy);
        return 0.5 + 0.5 * value;
    }

    double PerlinNoise::perlin(double x, double y, double freq, int32_t depth) const noexcept
    {
        double xa = x * freq;
        double ya = y * freq;
        double amp = 1.0;
        double fin = 0.0;
        double div = 0.0;

        for (int32_t i = 0; i < depth; ++i)
        {
            div += amp;
            fin += perlin_layer(xa, ya) * amp;
            amp *= 0.5;
            xa *= 2.0;
            ya *= 2.0;
        }
        return fin / div;
    }

    double PerlinNoise::mfperlin(double x, double y, double freq, int32_t depth) const noexcept
    {
        double xa = x * freq;
        double ya = y * freq;
        double xmfa = xa + 1.5;
        double ymfa = ya + 1.8;
        double amp = 1.0;
        double fin = 0.0;
        double div = 0.0;

        double H = 0.2 + 0.8 * perlin_layer(xmfa, ymfa);
        H = 0.75 * (1.0 - std::pow(1.0 - H, 3.0));

        for (int32_t i = 0; i < depth; ++i)
        {
            amp = std::pow(2.0, -H * static_cast<double>(i));
            div += amp;
            fin += perlin_layer(xa, ya) * amp;
            xa *= 2.0;
            ya *= 2.0;
        }
        return fin / div;
    }

    // Default global generator instance
    PerlinNoise default_generator;
    std::array<int32_t, PERM_HASH_LEN>& perm_hash = default_generator.perm_hash();

    void set_perlin_seed(int32_t s) noexcept
    {
        default_generator.set_seed(s);
    }

    double perlin(double x, double y, double freq, int32_t depth) noexcept
    {
        return default_generator.perlin(x, y, freq, depth);
    }

    double mfperlin(double x, double y, double freq, int32_t depth) noexcept
    {
        return default_generator.mfperlin(x, y, freq, depth);
    }
}