// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#pragma once

#include <GridGoblin/Model/Cell_model.hpp>
#include <GridGoblin/Model/Shape.hpp>

#include <Hobgoblin/Math/Vector.hpp>

#include <array>

namespace jbatnozic {
namespace gridgoblin {

//! Get vertices of the lines of a shape that can block visibility.
//!
//! Every pair of two consecutive vertices makes a line which blocks visibility ([0] and [1] make the
//! first line, [2] and [3] make the second line, and so on. Vertices are normalized to 0..1 range in
//! the cell's local coordinate system.
//!
//! \param aShape shape of the cell to process.
//! \param aCell cell to process (parameter used only to get flags).
//! \param aEdgesOfInterest set bit `CellModel::OBSTRUCTED_FULLY_BY_X_NEIGHBOR` to ignore vertices on the
//!                         appropriate side/edge of the cell. Leave as 0 to process all sides.
//! \param aAllEdgesOverride if `true`, ignore `aEdgesOfInterest` and process all edges.
//! \param aPaddingOffset if positive, will make the returned lines longer by roughly this amount on
//!                       each end.
//! \param aVertices[out] buffer where to store vertices.
//!
//! \returns number of vertices written to `aVertices`.
std::size_t GetVisibilityVertices(/*  in */ Shape                              aShape,
                                  /*  in */ const CellModel&                   aCell,
                                  /*  in */ std::uint16_t                      aEdgesOfInterest,
                                  /*  in */ bool                               aAllEdgesOverride,
                                  /*  in */ float                              aPaddingOffset,
                                  /* out */ std::array<hg::math::Vector2f, 8>& aVertices);

} // namespace gridgoblin
} // namespace jbatnozic
