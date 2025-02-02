// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#pragma once

#include <GridGoblin/Model/Cell_model.hpp>
#include <GridGoblin/Model/Shape.hpp>

#include <Hobgoblin/Math/Vector.hpp>

#include <array>

namespace jbatnozic {
namespace gridgoblin {

std::size_t GetVisibilityVertices(Shape                              aShape,
                                  const CellModel&                   aCell,
                                  std::uint16_t                      aEdgesOfInterest,
                                  bool                               aAllEdgesOverride,
                                  float                              aPaddingOffset,
                                  std::array<hg::math::Vector2f, 8>& aVertices);

} // namespace gridgoblin
} // namespace jbatnozic
