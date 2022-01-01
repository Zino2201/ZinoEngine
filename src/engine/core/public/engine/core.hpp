#pragma once

/** Core types */
#include <cstdint>

/** Assertions */
#include "debug/assertions.hpp"
#include "unused_parameters.hpp"

#pragma warning(disable: 4275)

template<typename T>
using OwnerPtr = T*;
