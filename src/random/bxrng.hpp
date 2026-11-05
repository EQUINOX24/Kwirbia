#pragma once

#include <cstddef>
#include <cstdint>
#include <cmath>
#include <ctime>
#include <algorithm>
#include <vector>
#include <random>
#include <chrono>
#include <iostream>
#if defined(_WIN32)
    #include <windows.h>              // GetCurrentProcessId, GetCurrentThreadId
#else
    #include <unistd.h>               // getpid
    #include <pthread.h>              // pthread_self
#endif


namespace bxrng {

inline constexpr std::uint32_t BXRAND_INT_MAX = 0xffffffffu;

// ---------------------------------------------------------------------------
//  Helper functions (pure, can be inlined)
// ---------------------------------------------------------------------------
inline std::uint32_t bxhash32(std::uint32_t x) noexcept {
    constexpr std::uint32_t H1 = 0xd76609edu;
    constexpr std::uint32_t H2 = 0xc4263683u;

    x = ((x << 16) ^ x) * H1;
    x = ((x >> 16) ^ x) * H2;
    x = (x << 16) ^ x;
    return x;
}

inline std::uint32_t lcgen(std::uint32_t x) noexcept {
    constexpr std::uint32_t LCG_A = 0x41c64e6du;
    constexpr std::uint32_t LCG_C = 0x3039u;
    constexpr std::uint32_t LCG_M = 0x80000000u;   // 2^31

    return (LCG_A * x + LCG_C) % LCG_M;
}

inline std::uint32_t rdtsc() noexcept {
#if defined(__i386__) || defined(__x86_64__)
    std::uint32_t lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return hi ^ lo;
#else
    return 0xbc91da5eu;   // fallback for non‑x86 platforms
#endif
}

inline std::uint32_t random_seed() noexcept {
    std::uint32_t entropy = 0;

    // 1) OS‑provided entropy
    std::random_device rd;
    entropy ^= static_cast<std::uint32_t>(rd());

    // 2) High‑resolution timestamp
    entropy ^= static_cast<std::uint32_t>(
        std::chrono::high_resolution_clock::now()
            .time_since_epoch().count());

    // 3) Process / thread identifiers (optional, platform‑specific)
#if defined(_WIN32)
    entropy ^= static_cast<std::uint32_t>(GetCurrentProcessId());
    entropy ^= static_cast<std::uint32_t>(GetCurrentThreadId());
#else
    entropy ^= static_cast<std::uint32_t>(getpid());
    entropy ^= static_cast<std::uint32_t>(pthread_self());
#endif

    // 4) Existing fallback (time + rdtsc) for completeness
    std::uint32_t y = static_cast<std::uint32_t>(std::time(nullptr)) ^ rdtsc();
    std::uint32_t x = lcgen(y);
    y = bxhash32(y);
    
    return y ^ x ^ (x << 16) ^ entropy;
}

// ---------------------------------------------------------------------------
//  BxRng – stateful generator
// ---------------------------------------------------------------------------
class BxRng {
public:
    // -----------------------------------------------------------------------
    //  Construction / seeding
    // -----------------------------------------------------------------------
    BxRng() {
        seed_ = random_seed(); init_state();
    }
    explicit BxRng(std::uint32_t s) {
        seed_ = s; init_state();
    }

    void setSeed(std::uint32_t s) noexcept {
        seed_ = s;
        init_state();
    }

    // -----------------------------------------------------------------------
    //  Random primitives
    // -----------------------------------------------------------------------
    std::uint32_t randInt() noexcept {
        std::uint32_t x = lcgen(lcg_);
        lcg_ = x;

        std::uint32_t y = bxhash32(n_);
        n_ = (n_ + 1u) % CYCLE_BREAKER;

        return y ^ x ^ (x << 16);
    }

    double randUniform() noexcept {
        return static_cast<double>(randInt()) / static_cast<double>(BXRAND_INT_MAX);
    }

    double randNormal() noexcept {
        if (regen_) {
            double u1 = randUniform();
            double u2 = randUniform();

            amplitude_ = std::sqrt(-2.0 * std::log(u1));
            angle_     = TAU * u2;

            u1_ = u1;
            u2_ = u2;

            regen_ = false;
            return amplitude_ * std::cos(angle_);
        } else {
            regen_ = true;
            return amplitude_ * std::sin(angle_);
        }
    }

    // -----------------------------------------------------------------------
    //  Accessors (optional)
    // -----------------------------------------------------------------------
    std::uint32_t seed() const noexcept {
        return seed_;
    }

private:
    // -----------------------------------------------------------------------
    //  Internal helpers
    // -----------------------------------------------------------------------
    void init_state() noexcept {
        lcg_ = bxhash32(seed_);
        n_   = lcgen(seed_) % CYCLE_BREAKER;

        u1_ = u2_ = 0.0;
        amplitude_ = angle_ = 0.0;
        regen_ = true;
    }

    // -----------------------------------------------------------------------
    //  State
    // -----------------------------------------------------------------------
    std::uint32_t seed_      = 0u;
    std::uint32_t lcg_       = 0u;
    std::uint32_t n_         = 0u;

    double u1_               = 0.0;
    double u2_               = 0.0;
    double amplitude_        = 0.0;
    double angle_            = 0.0;
    bool   regen_            = true;

    static constexpr std::uint32_t CYCLE_BREAKER = 0xffffffffu;
    static constexpr double TAU = 2.0 * M_PI;
};

// ---------------------------------------------------------------------------
//  Utility: shuffle (generic)
// ---------------------------------------------------------------------------
template <typename T>
inline void shuffle(BxRng& rng, T* array, std::size_t n_len) noexcept {
    if (n_len <= 1) return;

    for (std::size_t i = n_len - 1; i > 0; --i) {
        std::size_t j = static_cast<std::size_t>(rng.randInt() % (i + 1));
        std::swap(array[i], array[j]);
    }
}

// Overload for std::vector (convenient)
template <typename T>
inline void shuffle(BxRng& rng, std::vector<T>& vec) noexcept {
    shuffle(rng, vec.data(), vec.size());
}

} // namespace bxrng