#pragma once

#include <utility>
#include <robin_hood.h>

namespace ze
{

/**
 * From boost's hash_combine
 */
template <class T, class H = robin_hood::hash<T>>
void hash_combine(size_t& seed, const T& v)
{
	seed ^= H()(v) + 0x9e3779b97f4a7c15 + (seed << 12) + (seed >> 4);
}

}