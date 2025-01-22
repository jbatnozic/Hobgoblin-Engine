// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#include <GridGoblin/Rendering/Dimetric_transform.hpp>

namespace jbatnozic {
namespace gridgoblin {

namespace hg = jbatnozic::hobgoblin;

// clang-format off
const hg::gr::Transform DIMETRIC_TRANSFORM {
     1.0f, 1.0f, 0.f,
    -0.5f, 0.5f, 0.f,
      0.f, 0.0f, 1.f
};
// clang-format on

} // namespace gridgoblin
} // namespace jbatnozic
