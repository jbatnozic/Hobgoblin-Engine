// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#pragma once

#include <Hobgoblin/Graphics/Transform.hpp>

namespace jbatnozic {
namespace gridgoblin {

namespace hg = jbatnozic::hobgoblin;

//! Rotates the object 45 degrees counter-clockwise and squashes it along the
//! (new) Y-axis by 50%.
//! Note that when setting the position of the object, no further transformation
//! from the world coordinates in needed.
extern const hg::gr::Transform DIMETRIC_TRANSFORM;

} // namespace gridgoblin
} // namespace jbatnozic
