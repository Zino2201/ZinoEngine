#pragma once

#include "core.hpp"

namespace ze
{

namespace detail
{

/**
 * Implements SplitMix64 RNG
 */
inline uint64_t random_SM64()
{
    static uint64_t seed = std::time(nullptr);
    uint64_t z = (seed += UINT64_C(0x9E3779B97F4A7C15));
    z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
    return z ^ (z >> 31);
}

inline double random_SM64_double()
{
    return static_cast<double>(random_SM64()) / std::pow(2.0, 64);
}

}


template<typename T>
inline T random_SM64(T A = std::numeric_limits<T>::min(), T B = std::numeric_limits<T>::max())
{
    return A + detail::random_SM64_double() * (B - A);
}

}