#pragma once

/** Core types */
#include <cstdint>

/** Assertions */
#include "debug/assertions.hpp"
#include "unused_parameters.hpp"

/** Warning about exported templated classes */
ZE_WARNING_DISABLE_MSVC(4275)

template<typename T>
using OwnerPtr = T*;
