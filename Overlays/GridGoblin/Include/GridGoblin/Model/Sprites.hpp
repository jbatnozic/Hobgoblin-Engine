// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#pragma once

#include <cstdint>

namespace jbatnozic {
namespace gridgoblin {

//! Identifies a sprite within the GridGoblin system.
//!
//! \warning despite this being an alias for a 32-bit type, GridGoblin will use only the lower
//!          16 bits, so don't use IDs over 65,535!
using SpriteId = std::uint16_t;

} // namespace gridgoblin
} // namespace jbatnozic
