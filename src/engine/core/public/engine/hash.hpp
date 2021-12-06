#pragma once

#include <utility>

namespace ze
{

/**
 * From boost's hash_combine
 */
template <class T, class H = std::hash<T>>
void hash_combine(size_t& seed, const T& v)
{
	seed ^= H()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

}