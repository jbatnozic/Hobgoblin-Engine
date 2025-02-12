// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#include <GridGoblin/Model/Cell_model.hpp>
#include <GridGoblin/Model/Shape.hpp>
#include <GridGoblin/Model/Shape_vertices.hpp>

#include <Hobgoblin/HGExcept.hpp>
#include <Hobgoblin/Math/Vector.hpp>

#include <array>
#include <cstdint>

namespace jbatnozic {
namespace gridgoblin {

namespace hg = ::jbatnozic::hobgoblin;

template <Shape taShape>
std::size_t GetVisibilityVertices(const CellModel&                   aCell,
                                  std::uint16_t                      aEdgesOfInterest,
                                  bool                               aAllEdgesOverride,
                                  float                              aPaddingOffset,
                                  std::array<hg::math::Vector2f, 8>& aVertices) {
    return 0; // Dummy
}

#define GG_EAST(_flags_)  (((_flags_) & CellModel::OBSTRUCTED_FULLY_BY_EAST_NEIGHBOR) == 0)
#define GG_NORTH(_flags_) (((_flags_) & CellModel::OBSTRUCTED_FULLY_BY_NORTH_NEIGHBOR) == 0)
#define GG_WEST(_flags_)  (((_flags_) & CellModel::OBSTRUCTED_FULLY_BY_WEST_NEIGHBOR) == 0)
#define GG_SOUTH(_flags_) (((_flags_) & CellModel::OBSTRUCTED_FULLY_BY_SOUTH_NEIGHBOR) == 0)

// MARK: FULL_SQUARE

template <>
std::size_t GetVisibilityVertices<Shape::FULL_SQUARE>(const CellModel& aCell,
                                                      std::uint16_t    aEdgesOfInterest,
                                                      bool             aAllEdgesOverride,
                                                      float            aPaddingOffset,
                                                      std::array<hg::math::Vector2f, 8>& aVertices) {
    std::size_t cnt = 0;

    const auto flags = aCell.getFlags() | aEdgesOfInterest;

    if (aAllEdgesOverride || GG_EAST(flags)) {
        aVertices[cnt + 0] = {1.f, 1.f + aPaddingOffset};
        aVertices[cnt + 1] = {1.f, 0.f - aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_NORTH(flags)) {
        aVertices[cnt + 0] = {1.f + aPaddingOffset, 0.f};
        aVertices[cnt + 1] = {0.f - aPaddingOffset, 0.f};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_WEST(flags)) {
        aVertices[cnt + 0] = {0.f, 0.f - aPaddingOffset};
        aVertices[cnt + 1] = {0.f, 1.f + aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_SOUTH(flags)) {
        aVertices[cnt + 0] = {0.f - aPaddingOffset, 1.f};
        aVertices[cnt + 1] = {1.f + aPaddingOffset, 1.f};
        cnt += 2;
    }

    return cnt;
}

template <>
std::size_t GetVisibilityVertices<Shape::FULL_SQUARE | Shape::HFLIP>(
    const CellModel&                   aCell,
    std::uint16_t                      aEdgesOfInterest,
    bool                               aAllEdgesOverride,
    float                              aPaddingOffset,
    std::array<hg::math::Vector2f, 8>& aVertices) {
    return GetVisibilityVertices<Shape::FULL_SQUARE>(aCell,
                                                     aEdgesOfInterest,
                                                     aAllEdgesOverride,
                                                     aPaddingOffset,
                                                     aVertices);
}

template <>
std::size_t GetVisibilityVertices<Shape::FULL_SQUARE | Shape::VFLIP>(
    const CellModel&                   aCell,
    std::uint16_t                      aEdgesOfInterest,
    bool                               aAllEdgesOverride,
    float                              aPaddingOffset,
    std::array<hg::math::Vector2f, 8>& aVertices) {
    return GetVisibilityVertices<Shape::FULL_SQUARE>(aCell,
                                                     aEdgesOfInterest,
                                                     aAllEdgesOverride,
                                                     aPaddingOffset,
                                                     aVertices);
}

template <>
std::size_t GetVisibilityVertices<Shape::FULL_SQUARE | Shape::HVFLIP>(
    const CellModel&                   aCell,
    std::uint16_t                      aEdgesOfInterest,
    bool                               aAllEdgesOverride,
    float                              aPaddingOffset,
    std::array<hg::math::Vector2f, 8>& aVertices) {
    return GetVisibilityVertices<Shape::FULL_SQUARE>(aCell,
                                                     aEdgesOfInterest,
                                                     aAllEdgesOverride,
                                                     aPaddingOffset,
                                                     aVertices);
}

// MARK: LARGE_TRIANGLE

template <>
std::size_t GetVisibilityVertices<Shape::LARGE_TRIANGLE>(const CellModel& aCell,
                                                         std::uint16_t    aEdgesOfInterest,
                                                         bool             aAllEdgesOverride,
                                                         float            aPaddingOffset,
                                                         std::array<hg::math::Vector2f, 8>& aVertices) //
{
    std::size_t cnt = 0;

    const auto flags = aCell.getFlags() | aEdgesOfInterest;

    if (aAllEdgesOverride || GG_EAST(flags) || GG_SOUTH(flags)) {
        aVertices[cnt + 0] = {0.f - aPaddingOffset, 1.f + aPaddingOffset};
        aVertices[cnt + 1] = {1.f + aPaddingOffset, 0.f - aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_NORTH(flags)) {
        aVertices[cnt + 0] = {1.f + aPaddingOffset, 0.f};
        aVertices[cnt + 1] = {0.f - aPaddingOffset, 0.f};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_WEST(flags)) {
        aVertices[cnt + 0] = {0.f, 0.f - aPaddingOffset};
        aVertices[cnt + 1] = {0.f, 1.f + aPaddingOffset};
        cnt += 2;
    }

    return cnt;
}

template <>
std::size_t GetVisibilityVertices<Shape::LARGE_TRIANGLE | Shape::HFLIP>(
    const CellModel&                   aCell,
    std::uint16_t                      aEdgesOfInterest,
    bool                               aAllEdgesOverride,
    float                              aPaddingOffset,
    std::array<hg::math::Vector2f, 8>& aVertices) //
{
    std::size_t cnt = 0;

    const auto flags = aCell.getFlags() | aEdgesOfInterest;

    if (aAllEdgesOverride || GG_EAST(flags)) {
        aVertices[cnt + 0] = {1.f, 1.f + aPaddingOffset};
        aVertices[cnt + 1] = {1.f, 0.f - aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_NORTH(flags)) {
        aVertices[cnt + 0] = {1.f + aPaddingOffset, 0.f};
        aVertices[cnt + 1] = {0.f - aPaddingOffset, 0.f};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_WEST(flags) || GG_SOUTH(flags)) {
        aVertices[cnt + 0] = {0.f - aPaddingOffset, 0.f - aPaddingOffset};
        aVertices[cnt + 1] = {1.f + aPaddingOffset, 1.f + aPaddingOffset};
        cnt += 2;
    }

    return cnt;
}

template <>
std::size_t GetVisibilityVertices<Shape::LARGE_TRIANGLE | Shape::VFLIP>(
    const CellModel&                   aCell,
    std::uint16_t                      aEdgesOfInterest,
    bool                               aAllEdgesOverride,
    float                              aPaddingOffset,
    std::array<hg::math::Vector2f, 8>& aVertices) //
{
    std::size_t cnt = 0;

    const auto flags = aCell.getFlags() | aEdgesOfInterest;

    if (aAllEdgesOverride || GG_EAST(flags) || GG_NORTH(flags)) {
        aVertices[cnt + 0] = {1.f + aPaddingOffset, 1.f + aPaddingOffset};
        aVertices[cnt + 1] = {0.f - aPaddingOffset, 0.f - aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_WEST(flags)) {
        aVertices[cnt + 0] = {0.f, 0.f - aPaddingOffset};
        aVertices[cnt + 1] = {0.f, 1.f + aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_SOUTH(flags)) {
        aVertices[cnt + 0] = {0.f - aPaddingOffset, 1.f};
        aVertices[cnt + 1] = {1.f + aPaddingOffset, 1.f};
        cnt += 2;
    }

    return cnt;
}

template <>
std::size_t GetVisibilityVertices<Shape::LARGE_TRIANGLE | Shape::HVFLIP>(
    const CellModel&                   aCell,
    std::uint16_t                      aEdgesOfInterest,
    bool                               aAllEdgesOverride,
    float                              aPaddingOffset,
    std::array<hg::math::Vector2f, 8>& aVertices) //
{
    std::size_t cnt = 0;

    const auto flags = aCell.getFlags() | aEdgesOfInterest;

    if (aAllEdgesOverride || GG_EAST(flags)) {
        aVertices[cnt + 0] = {1.f, 1.f + aPaddingOffset};
        aVertices[cnt + 1] = {1.f, 0.f - aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_NORTH(flags) || GG_WEST(flags)) {
        aVertices[cnt + 0] = {1.f + aPaddingOffset, 0.f - aPaddingOffset};
        aVertices[cnt + 1] = {0.f - aPaddingOffset, 1.f + aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_SOUTH(flags)) {
        aVertices[cnt + 0] = {0.f - aPaddingOffset, 1.f};
        aVertices[cnt + 1] = {1.f + aPaddingOffset, 1.f};
        cnt += 2;
    }

    return cnt;
}

// MARK: SMALL_TRIANGLE_HOR

template <>
std::size_t GetVisibilityVertices<Shape::SMALL_TRIANGLE_HOR>(
    const CellModel&                   aCell,
    std::uint16_t                      aEdgesOfInterest,
    bool                               aAllEdgesOverride,
    float                              aPaddingOffset,
    std::array<hg::math::Vector2f, 8>& aVertices) //
{
    std::size_t cnt = 0;

    const auto flags = aCell.getFlags() | aEdgesOfInterest;

    if (aAllEdgesOverride || GG_EAST(flags) || GG_SOUTH(flags)) {
        aVertices[cnt + 0] = {0.f - aPaddingOffset, 0.5f + aPaddingOffset};
        aVertices[cnt + 1] = {1.f + aPaddingOffset, 0.0f - aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_NORTH(flags)) {
        aVertices[cnt + 0] = {1.f + aPaddingOffset, 0.f};
        aVertices[cnt + 1] = {0.f - aPaddingOffset, 0.f};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_WEST(flags)) {
        aVertices[cnt + 0] = {0.f, 0.0f - aPaddingOffset};
        aVertices[cnt + 1] = {0.f, 0.5f + aPaddingOffset};
        cnt += 2;
    }

    return cnt;
}

template <>
std::size_t GetVisibilityVertices<Shape::SMALL_TRIANGLE_HOR | Shape::HFLIP>(
    const CellModel&                   aCell,
    std::uint16_t                      aEdgesOfInterest,
    bool                               aAllEdgesOverride,
    float                              aPaddingOffset,
    std::array<hg::math::Vector2f, 8>& aVertices) //
{
    std::size_t cnt = 0;

    const auto flags = aCell.getFlags() | aEdgesOfInterest;

    if (aAllEdgesOverride || GG_EAST(flags)) {
        aVertices[cnt + 0] = {1.f, 0.5f + aPaddingOffset};
        aVertices[cnt + 1] = {1.f, 0.0f - aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_NORTH(flags)) {
        aVertices[cnt + 0] = {1.f + aPaddingOffset, 0.f};
        aVertices[cnt + 1] = {0.f - aPaddingOffset, 0.f};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_WEST(flags) || GG_SOUTH(flags)) {
        aVertices[cnt + 0] = {0.f - aPaddingOffset, 0.0f - aPaddingOffset};
        aVertices[cnt + 1] = {1.f + aPaddingOffset, 0.5f + aPaddingOffset};
        cnt += 2;
    }

    return cnt;
}

template <>
std::size_t GetVisibilityVertices<Shape::SMALL_TRIANGLE_HOR | Shape::VFLIP>(
    const CellModel&                   aCell,
    std::uint16_t                      aEdgesOfInterest,
    bool                               aAllEdgesOverride,
    float                              aPaddingOffset,
    std::array<hg::math::Vector2f, 8>& aVertices) //
{
    std::size_t cnt = 0;

    const auto flags = aCell.getFlags() | aEdgesOfInterest;

    if (aAllEdgesOverride || GG_EAST(flags) || GG_NORTH(flags)) {
        aVertices[cnt + 0] = {1.f + aPaddingOffset, 1.0f + aPaddingOffset};
        aVertices[cnt + 1] = {0.f - aPaddingOffset, 0.5f - aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_WEST(flags)) {
        aVertices[cnt + 0] = {0.f, 0.5f - aPaddingOffset};
        aVertices[cnt + 1] = {0.f, 1.0f + aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_SOUTH(flags)) {
        aVertices[cnt + 0] = {0.f - aPaddingOffset, 1.f};
        aVertices[cnt + 1] = {1.f + aPaddingOffset, 1.f};
        cnt += 2;
    }

    return cnt;
}

template <>
std::size_t GetVisibilityVertices<Shape::SMALL_TRIANGLE_HOR | Shape::HVFLIP>(
    const CellModel&                   aCell,
    std::uint16_t                      aEdgesOfInterest,
    bool                               aAllEdgesOverride,
    float                              aPaddingOffset,
    std::array<hg::math::Vector2f, 8>& aVertices) //
{
    std::size_t cnt = 0;

    const auto flags = aCell.getFlags() | aEdgesOfInterest;

    if (aAllEdgesOverride || GG_EAST(flags)) {
        aVertices[cnt + 0] = {1.f, 1.0f + aPaddingOffset};
        aVertices[cnt + 1] = {1.f, 0.5f - aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_NORTH(flags) || GG_WEST(flags)) {
        aVertices[cnt + 0] = {1.f + aPaddingOffset, 0.5f - aPaddingOffset};
        aVertices[cnt + 1] = {0.f - aPaddingOffset, 1.0f + aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_SOUTH(flags)) {
        aVertices[cnt + 0] = {0.f - aPaddingOffset, 1.f};
        aVertices[cnt + 1] = {1.f + aPaddingOffset, 1.f};
        cnt += 2;
    }

    return cnt;
}

// MARK: TALL_SMALL_TRIANGLE_HOR

template <>
std::size_t GetVisibilityVertices<Shape::TALL_SMALL_TRIANGLE_HOR>(
    const CellModel&                   aCell,
    std::uint16_t                      aEdgesOfInterest,
    bool                               aAllEdgesOverride,
    float                              aPaddingOffset,
    std::array<hg::math::Vector2f, 8>& aVertices) //
{
    std::size_t cnt = 0;

    const auto flags = aCell.getFlags() | aEdgesOfInterest;

    if (aAllEdgesOverride || GG_EAST(flags) || GG_SOUTH(flags)) {
        aVertices[cnt + 0] = {0.f - aPaddingOffset, 1.0f + aPaddingOffset};
        aVertices[cnt + 1] = {1.f + aPaddingOffset, 0.5f - aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_EAST(flags)) {
        aVertices[cnt + 0] = {1.f, 0.5f + aPaddingOffset};
        aVertices[cnt + 1] = {1.f, 0.0f - aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_NORTH(flags)) {
        aVertices[cnt + 0] = {1.f + aPaddingOffset, 0.0f};
        aVertices[cnt + 1] = {0.f - aPaddingOffset, 0.0f};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_WEST(flags)) {
        aVertices[cnt + 0] = {0.f, 0.f - aPaddingOffset};
        aVertices[cnt + 1] = {0.f, 1.f + aPaddingOffset};
        cnt += 2;
    }

    return cnt;
}

template <>
std::size_t GetVisibilityVertices<Shape::TALL_SMALL_TRIANGLE_HOR | Shape::HFLIP>(
    const CellModel&                   aCell,
    std::uint16_t                      aEdgesOfInterest,
    bool                               aAllEdgesOverride,
    float                              aPaddingOffset,
    std::array<hg::math::Vector2f, 8>& aVertices) //
{
    std::size_t cnt = 0;

    const auto flags = aCell.getFlags() | aEdgesOfInterest;

    if (aAllEdgesOverride || GG_EAST(flags)) {
        aVertices[cnt + 0] = {1.f, 1.f + aPaddingOffset};
        aVertices[cnt + 1] = {1.f, 0.f - aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_NORTH(flags)) {
        aVertices[cnt + 0] = {1.f + aPaddingOffset, 0.f};
        aVertices[cnt + 1] = {0.f - aPaddingOffset, 0.f};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_WEST(flags)) {
        aVertices[cnt + 0] = {0.f, 0.0f - aPaddingOffset};
        aVertices[cnt + 1] = {0.f, 0.5f + aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_WEST(flags) || GG_SOUTH(flags)) {
        aVertices[cnt + 0] = {0.f - aPaddingOffset, 0.5f - aPaddingOffset};
        aVertices[cnt + 1] = {1.f + aPaddingOffset, 1.0f + aPaddingOffset};
        cnt += 2;
    }

    return cnt;
}

template <>
std::size_t GetVisibilityVertices<Shape::TALL_SMALL_TRIANGLE_HOR | Shape::VFLIP>(
    const CellModel&                   aCell,
    std::uint16_t                      aEdgesOfInterest,
    bool                               aAllEdgesOverride,
    float                              aPaddingOffset,
    std::array<hg::math::Vector2f, 8>& aVertices) //
{
    std::size_t cnt = 0;

    const auto flags = aCell.getFlags() | aEdgesOfInterest;

    if (aAllEdgesOverride || GG_EAST(flags)) {
        aVertices[cnt + 0] = {1.f, 1.0f + aPaddingOffset};
        aVertices[cnt + 1] = {1.f, 0.5f - aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_EAST(flags) || GG_NORTH(flags)) {
        aVertices[cnt + 0] = {1.f + aPaddingOffset, 0.5f + aPaddingOffset};
        aVertices[cnt + 1] = {0.f - aPaddingOffset, 0.f - aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_WEST(flags)) {
        aVertices[cnt + 0] = {0.f, 0.f - aPaddingOffset};
        aVertices[cnt + 1] = {0.f, 1.f + aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_SOUTH(flags)) {
        aVertices[cnt + 0] = {0.f - aPaddingOffset, 1.f};
        aVertices[cnt + 1] = {1.f + aPaddingOffset, 1.f};
        cnt += 2;
    }

    return cnt;
}

template <>
std::size_t GetVisibilityVertices<Shape::TALL_SMALL_TRIANGLE_HOR | Shape::HVFLIP>(
    const CellModel&                   aCell,
    std::uint16_t                      aEdgesOfInterest,
    bool                               aAllEdgesOverride,
    float                              aPaddingOffset,
    std::array<hg::math::Vector2f, 8>& aVertices) //
{
    std::size_t cnt = 0;

    const auto flags = aCell.getFlags() | aEdgesOfInterest;

    if (aAllEdgesOverride || GG_EAST(flags)) {
        aVertices[cnt + 0] = {1.f, 1.f + aPaddingOffset};
        aVertices[cnt + 1] = {1.f, 0.f - aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_NORTH(flags) || GG_WEST(flags)) {
        aVertices[cnt + 0] = {1.f + aPaddingOffset, 0.0f - aPaddingOffset};
        aVertices[cnt + 1] = {0.f - aPaddingOffset, 0.5f + aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_WEST(flags)) {
        aVertices[cnt + 0] = {0.f, 0.5f - aPaddingOffset};
        aVertices[cnt + 1] = {0.f, 1.0f + aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_SOUTH(flags)) {
        aVertices[cnt + 0] = {0.f - aPaddingOffset, 1.f};
        aVertices[cnt + 1] = {1.f + aPaddingOffset, 1.f};
        cnt += 2;
    }

    return cnt;
}

// MARK: HALF_SQUARE_HOR

template <>
std::size_t GetVisibilityVertices<Shape::HALF_SQUARE_HOR>(const CellModel& aCell,
                                                          std::uint16_t    aEdgesOfInterest,
                                                          bool             aAllEdgesOverride,
                                                          float            aPaddingOffset,
                                                          std::array<hg::math::Vector2f, 8>& aVertices) {
    std::size_t cnt = 0;

    const auto flags = aCell.getFlags() | aEdgesOfInterest;

    if (aAllEdgesOverride || GG_EAST(flags)) {
        aVertices[cnt + 0] = {1.f, 0.5f + aPaddingOffset};
        aVertices[cnt + 1] = {1.f, 0.0f - aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_NORTH(flags)) {
        aVertices[cnt + 0] = {1.f + aPaddingOffset, 0.f};
        aVertices[cnt + 1] = {0.f - aPaddingOffset, 0.f};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_WEST(flags)) {
        aVertices[cnt + 0] = {0.f, 0.0f - aPaddingOffset};
        aVertices[cnt + 1] = {0.f, 0.5f + aPaddingOffset};
        cnt += 2;
    }
    // TODO: optimize handling of these middle lines for all HALF_SQUARE_* variants.
    // if (aAllEdgesOverride || GG_SOUTH(flags)) {
    aVertices[cnt + 0] = {0.f - aPaddingOffset, 0.5f};
    aVertices[cnt + 1] = {1.f + aPaddingOffset, 0.5f};
    cnt += 2;
    // }

    return cnt;
}

template <>
std::size_t GetVisibilityVertices<Shape::HALF_SQUARE_HOR | Shape::HFLIP>(
    const CellModel&                   aCell,
    std::uint16_t                      aEdgesOfInterest,
    bool                               aAllEdgesOverride,
    float                              aPaddingOffset,
    std::array<hg::math::Vector2f, 8>& aVertices) //
{
    return GetVisibilityVertices<Shape::HALF_SQUARE_HOR>(aCell,
                                                         aEdgesOfInterest,
                                                         aAllEdgesOverride,
                                                         aPaddingOffset,
                                                         aVertices);
}

template <>
std::size_t GetVisibilityVertices<Shape::HALF_SQUARE_HOR | Shape::VFLIP>(
    const CellModel&                   aCell,
    std::uint16_t                      aEdgesOfInterest,
    bool                               aAllEdgesOverride,
    float                              aPaddingOffset,
    std::array<hg::math::Vector2f, 8>& aVertices) //
{
    std::size_t cnt = 0;

    const auto flags = aCell.getFlags() | aEdgesOfInterest;

    if (aAllEdgesOverride || GG_EAST(flags)) {
        aVertices[cnt + 0] = {1.f, 1.0f + aPaddingOffset};
        aVertices[cnt + 1] = {1.f, 0.5f - aPaddingOffset};
        cnt += 2;
    }
    // if (aAllEdgesOverride || GG_NORTH(flags)) {
    aVertices[cnt + 0] = {1.f + aPaddingOffset, 0.5f};
    aVertices[cnt + 1] = {0.f - aPaddingOffset, 0.5f};
    cnt += 2;
    // }
    if (aAllEdgesOverride || GG_WEST(flags)) {
        aVertices[cnt + 0] = {0.f, 0.5f - aPaddingOffset};
        aVertices[cnt + 1] = {0.f, 1.0f + aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_SOUTH(flags)) {
        aVertices[cnt + 0] = {0.f - aPaddingOffset, 1.f};
        aVertices[cnt + 1] = {1.f + aPaddingOffset, 1.f};
        cnt += 2;
    }

    return cnt;
}

template <>
std::size_t GetVisibilityVertices<Shape::HALF_SQUARE_HOR | Shape::HVFLIP>(
    const CellModel&                   aCell,
    std::uint16_t                      aEdgesOfInterest,
    bool                               aAllEdgesOverride,
    float                              aPaddingOffset,
    std::array<hg::math::Vector2f, 8>& aVertices) //
{
    return GetVisibilityVertices < Shape::HALF_SQUARE_HOR |
           Shape::VFLIP > (aCell, aEdgesOfInterest, aAllEdgesOverride, aPaddingOffset, aVertices);
}

// MARK: SMALL_TRIANGLE_VER

template <>
std::size_t GetVisibilityVertices<Shape::SMALL_TRIANGLE_VER>(
    const CellModel&                   aCell,
    std::uint16_t                      aEdgesOfInterest,
    bool                               aAllEdgesOverride,
    float                              aPaddingOffset,
    std::array<hg::math::Vector2f, 8>& aVertices) //
{
    std::size_t cnt = 0;

    const auto flags = aCell.getFlags() | aEdgesOfInterest;

    if (aAllEdgesOverride || GG_EAST(flags) || GG_SOUTH(flags)) {
        aVertices[cnt + 0] = {0.0f - aPaddingOffset, 1.0f + aPaddingOffset};
        aVertices[cnt + 1] = {0.5f + aPaddingOffset, 0.0f - aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_NORTH(flags)) {
        aVertices[cnt + 0] = {0.5f + aPaddingOffset, 0.f};
        aVertices[cnt + 1] = {0.0f - aPaddingOffset, 0.f};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_WEST(flags)) {
        aVertices[cnt + 0] = {0.f, 0.0f - aPaddingOffset};
        aVertices[cnt + 1] = {0.f, 1.0f + aPaddingOffset};
        cnt += 2;
    }

    return cnt;
}

template <>
std::size_t GetVisibilityVertices<Shape::SMALL_TRIANGLE_VER | Shape::HFLIP>(
    const CellModel&                   aCell,
    std::uint16_t                      aEdgesOfInterest,
    bool                               aAllEdgesOverride,
    float                              aPaddingOffset,
    std::array<hg::math::Vector2f, 8>& aVertices) //
{
    std::size_t cnt = 0;

    const auto flags = aCell.getFlags() | aEdgesOfInterest;

    if (aAllEdgesOverride || GG_EAST(flags)) {
        aVertices[cnt + 0] = {1.f, 1.0f + aPaddingOffset};
        aVertices[cnt + 1] = {1.f, 0.0f - aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_NORTH(flags)) {
        aVertices[cnt + 0] = {1.0f + aPaddingOffset, 0.f};
        aVertices[cnt + 1] = {0.5f - aPaddingOffset, 0.f};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_WEST(flags) || GG_SOUTH(flags)) {
        aVertices[cnt + 0] = {0.5f - aPaddingOffset, 0.0f - aPaddingOffset};
        aVertices[cnt + 1] = {1.0f + aPaddingOffset, 1.0f + aPaddingOffset};
        cnt += 2;
    }

    return cnt;
}

template <>
std::size_t GetVisibilityVertices<Shape::SMALL_TRIANGLE_VER | Shape::VFLIP>(
    const CellModel&                   aCell,
    std::uint16_t                      aEdgesOfInterest,
    bool                               aAllEdgesOverride,
    float                              aPaddingOffset,
    std::array<hg::math::Vector2f, 8>& aVertices) //
{
    std::size_t cnt = 0;

    const auto flags = aCell.getFlags() | aEdgesOfInterest;

    if (aAllEdgesOverride || GG_EAST(flags) || GG_NORTH(flags)) {
        aVertices[cnt + 0] = {0.5f + aPaddingOffset, 1.0f + aPaddingOffset};
        aVertices[cnt + 1] = {0.0f - aPaddingOffset, 0.0f - aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_WEST(flags)) {
        aVertices[cnt + 0] = {0.f, 0.0f - aPaddingOffset};
        aVertices[cnt + 1] = {0.f, 1.0f + aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_SOUTH(flags)) {
        aVertices[cnt + 0] = {0.0f - aPaddingOffset, 1.f};
        aVertices[cnt + 1] = {0.5f + aPaddingOffset, 1.f};
        cnt += 2;
    }

    return cnt;
}

template <>
std::size_t GetVisibilityVertices<Shape::SMALL_TRIANGLE_VER | Shape::HVFLIP>(
    const CellModel&                   aCell,
    std::uint16_t                      aEdgesOfInterest,
    bool                               aAllEdgesOverride,
    float                              aPaddingOffset,
    std::array<hg::math::Vector2f, 8>& aVertices) //
{
    std::size_t cnt = 0;

    const auto flags = aCell.getFlags() | aEdgesOfInterest;

    if (aAllEdgesOverride || GG_EAST(flags)) {
        aVertices[cnt + 0] = {1.f, 1.0f + aPaddingOffset};
        aVertices[cnt + 1] = {1.f, 0.0f - aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_NORTH(flags) || GG_WEST(flags)) {
        aVertices[cnt + 0] = {1.0f + aPaddingOffset, 0.0f - aPaddingOffset};
        aVertices[cnt + 1] = {0.5f - aPaddingOffset, 1.0f + aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_SOUTH(flags)) {
        aVertices[cnt + 0] = {0.5f - aPaddingOffset, 1.f};
        aVertices[cnt + 1] = {1.0f + aPaddingOffset, 1.f};
        cnt += 2;
    }

    return cnt;
}

// MARK: TALL_SMALL_TRIANGLE_VER

template <>
std::size_t GetVisibilityVertices<Shape::TALL_SMALL_TRIANGLE_VER>(
    const CellModel&                   aCell,
    std::uint16_t                      aEdgesOfInterest,
    bool                               aAllEdgesOverride,
    float                              aPaddingOffset,
    std::array<hg::math::Vector2f, 8>& aVertices) //
{
    std::size_t cnt = 0;

    const auto flags = aCell.getFlags() | aEdgesOfInterest;

    if (aAllEdgesOverride || GG_EAST(flags) || GG_SOUTH(flags)) {
        aVertices[cnt + 0] = {0.5f - aPaddingOffset, 1.f + aPaddingOffset};
        aVertices[cnt + 1] = {1.0f + aPaddingOffset, 0.f - aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_NORTH(flags)) {
        aVertices[cnt + 0] = {1.f + aPaddingOffset, 0.0f};
        aVertices[cnt + 1] = {0.f - aPaddingOffset, 0.0f};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_WEST(flags)) {
        aVertices[cnt + 0] = {0.f, 0.f - aPaddingOffset};
        aVertices[cnt + 1] = {0.f, 1.f + aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_SOUTH(flags)) {
        aVertices[cnt + 0] = {0.0f - aPaddingOffset, 1.f};
        aVertices[cnt + 1] = {0.5f + aPaddingOffset, 1.f};
        cnt += 2;
    }

    return cnt;
}

template <>
std::size_t GetVisibilityVertices<Shape::TALL_SMALL_TRIANGLE_VER | Shape::HFLIP>(
    const CellModel&                   aCell,
    std::uint16_t                      aEdgesOfInterest,
    bool                               aAllEdgesOverride,
    float                              aPaddingOffset,
    std::array<hg::math::Vector2f, 8>& aVertices) //
{
    std::size_t cnt = 0;

    const auto flags = aCell.getFlags() | aEdgesOfInterest;

    if (aAllEdgesOverride || GG_EAST(flags)) {
        aVertices[cnt + 0] = {1.f, 1.f + aPaddingOffset};
        aVertices[cnt + 1] = {1.f, 0.f - aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_NORTH(flags)) {
        aVertices[cnt + 0] = {1.f + aPaddingOffset, 0.f};
        aVertices[cnt + 1] = {0.f - aPaddingOffset, 0.f};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_WEST(flags) || GG_SOUTH(flags)) {
        aVertices[cnt + 0] = {0.0f - aPaddingOffset, 0.0f - aPaddingOffset};
        aVertices[cnt + 1] = {0.5f + aPaddingOffset, 1.0f + aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_SOUTH(flags)) {
        aVertices[cnt + 0] = {0.5f - aPaddingOffset, 1.f};
        aVertices[cnt + 1] = {1.0f + aPaddingOffset, 1.f};
        cnt += 2;
    }

    return cnt;
}

template <>
std::size_t GetVisibilityVertices<Shape::TALL_SMALL_TRIANGLE_VER | Shape::VFLIP>(
    const CellModel&                   aCell,
    std::uint16_t                      aEdgesOfInterest,
    bool                               aAllEdgesOverride,
    float                              aPaddingOffset,
    std::array<hg::math::Vector2f, 8>& aVertices) //
{
    std::size_t cnt = 0;

    const auto flags = aCell.getFlags() | aEdgesOfInterest;

    if (aAllEdgesOverride || GG_EAST(flags) || GG_NORTH(flags)) {
        aVertices[cnt + 0] = {1.0f + aPaddingOffset, 1.0f + aPaddingOffset};
        aVertices[cnt + 1] = {0.5f - aPaddingOffset, 0.0f - aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_NORTH(flags)) {
        aVertices[cnt + 0] = {0.5f + aPaddingOffset, 0.f};
        aVertices[cnt + 1] = {0.0f - aPaddingOffset, 0.f};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_WEST(flags)) {
        aVertices[cnt + 0] = {0.f, 0.f - aPaddingOffset};
        aVertices[cnt + 1] = {0.f, 1.f + aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_SOUTH(flags)) {
        aVertices[cnt + 0] = {0.f - aPaddingOffset, 1.f};
        aVertices[cnt + 1] = {1.f + aPaddingOffset, 1.f};
        cnt += 2;
    }

    return cnt;
}

template <>
std::size_t GetVisibilityVertices<Shape::TALL_SMALL_TRIANGLE_VER | Shape::HVFLIP>(
    const CellModel&                   aCell,
    std::uint16_t                      aEdgesOfInterest,
    bool                               aAllEdgesOverride,
    float                              aPaddingOffset,
    std::array<hg::math::Vector2f, 8>& aVertices) //
{
    std::size_t cnt = 0;

    const auto flags = aCell.getFlags() | aEdgesOfInterest;

    if (aAllEdgesOverride || GG_EAST(flags)) {
        aVertices[cnt + 0] = {1.f, 1.f + aPaddingOffset};
        aVertices[cnt + 1] = {1.f, 0.f - aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_NORTH(flags)) {
        aVertices[cnt + 0] = {1.0f + aPaddingOffset, 0.f};
        aVertices[cnt + 1] = {0.5f - aPaddingOffset, 0.f};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_NORTH(flags) || GG_WEST(flags)) {
        aVertices[cnt + 0] = {0.5f + aPaddingOffset, 0.0f - aPaddingOffset};
        aVertices[cnt + 1] = {0.0f - aPaddingOffset, 1.0f + aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_SOUTH(flags)) {
        aVertices[cnt + 0] = {0.f - aPaddingOffset, 1.f};
        aVertices[cnt + 1] = {1.f + aPaddingOffset, 1.f};
        cnt += 2;
    }

    return cnt;
}

// MARK: HALF_SQUARE_VER

template <>
std::size_t GetVisibilityVertices<Shape::HALF_SQUARE_VER>(
    const CellModel&                   aCell,
    std::uint16_t                      aEdgesOfInterest,
    bool                               aAllEdgesOverride,
    float                              aPaddingOffset,
    std::array<hg::math::Vector2f, 8>& aVertices) //
{
    std::size_t cnt = 0;

    const auto flags = aCell.getFlags() | aEdgesOfInterest;

    // if (aAllEdgesOverride || GG_EAST(flags)) {
    aVertices[cnt + 0] = {0.5f, 1.f + aPaddingOffset};
    aVertices[cnt + 1] = {0.5f, 0.f - aPaddingOffset};
    cnt += 2;
    // }
    if (aAllEdgesOverride || GG_NORTH(flags)) {
        aVertices[cnt + 0] = {0.5f + aPaddingOffset, 0.f};
        aVertices[cnt + 1] = {0.0f - aPaddingOffset, 0.f};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_WEST(flags)) {
        aVertices[cnt + 0] = {0.f, 0.f - aPaddingOffset};
        aVertices[cnt + 1] = {0.f, 1.f + aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_SOUTH(flags)) {
        aVertices[cnt + 0] = {0.0f - aPaddingOffset, 1.f};
        aVertices[cnt + 1] = {0.5f + aPaddingOffset, 1.f};
        cnt += 2;
    }

    return cnt;
}

template <>
std::size_t GetVisibilityVertices<Shape::HALF_SQUARE_VER | Shape::HFLIP>(
    const CellModel&                   aCell,
    std::uint16_t                      aEdgesOfInterest,
    bool                               aAllEdgesOverride,
    float                              aPaddingOffset,
    std::array<hg::math::Vector2f, 8>& aVertices) //
{
    std::size_t cnt = 0;

    const auto flags = aCell.getFlags() | aEdgesOfInterest;

    if (aAllEdgesOverride || GG_EAST(flags)) {
        aVertices[cnt + 0] = {1.f, 1.f + aPaddingOffset};
        aVertices[cnt + 1] = {1.f, 0.f - aPaddingOffset};
        cnt += 2;
    }
    if (aAllEdgesOverride || GG_NORTH(flags)) {
        aVertices[cnt + 0] = {1.0f + aPaddingOffset, 0.f};
        aVertices[cnt + 1] = {0.5f - aPaddingOffset, 0.f};
        cnt += 2;
    }
    // if (aAllEdgesOverride || GG_WEST(flags)) {
    aVertices[cnt + 0] = {0.5f, 0.f - aPaddingOffset};
    aVertices[cnt + 1] = {0.5f, 1.f + aPaddingOffset};
    cnt += 2;
    // }
    if (aAllEdgesOverride || GG_SOUTH(flags)) {
        aVertices[cnt + 0] = {0.5f - aPaddingOffset, 1.f};
        aVertices[cnt + 1] = {1.0f + aPaddingOffset, 1.f};
        cnt += 2;
    }

    return cnt;
}

template <>
std::size_t GetVisibilityVertices<Shape::HALF_SQUARE_VER | Shape::VFLIP>(
    const CellModel&                   aCell,
    std::uint16_t                      aEdgesOfInterest,
    bool                               aAllEdgesOverride,
    float                              aPaddingOffset,
    std::array<hg::math::Vector2f, 8>& aVertices) //
{
    return GetVisibilityVertices<Shape::HALF_SQUARE_VER>(aCell,
                                                         aEdgesOfInterest,
                                                         aAllEdgesOverride,
                                                         aPaddingOffset,
                                                         aVertices);
}

template <>
std::size_t GetVisibilityVertices<Shape::HALF_SQUARE_VER | Shape::HVFLIP>(
    const CellModel&                   aCell,
    std::uint16_t                      aEdgesOfInterest,
    bool                               aAllEdgesOverride,
    float                              aPaddingOffset,
    std::array<hg::math::Vector2f, 8>& aVertices) //
{
    return GetVisibilityVertices < Shape::HALF_SQUARE_VER |
           Shape::HFLIP > (aCell, aEdgesOfInterest, aAllEdgesOverride, aPaddingOffset, aVertices);
}

// MASK: TABLE

namespace {
using GetVisibilityVerticesFunc = decltype(&GetVisibilityVertices<Shape::FULL_SQUARE>);

std::array<GetVisibilityVerticesFunc, MAX_SHAPE_NUMBER> MakeVisibilityVerticesFuncTable() {
    std::array<GetVisibilityVerticesFunc, MAX_SHAPE_NUMBER> result;

#define ADD_ENTRY(_shape_) result[static_cast<std::size_t>(_shape_)] = &GetVisibilityVertices<_shape_>

    ADD_ENTRY(Shape::FULL_SQUARE);
    ADD_ENTRY(Shape::FULL_SQUARE | Shape::HFLIP);
    ADD_ENTRY(Shape::FULL_SQUARE | Shape::VFLIP);
    ADD_ENTRY(Shape::FULL_SQUARE | Shape::HVFLIP);
    ADD_ENTRY(Shape::LARGE_TRIANGLE);
    ADD_ENTRY(Shape::LARGE_TRIANGLE | Shape::HFLIP);
    ADD_ENTRY(Shape::LARGE_TRIANGLE | Shape::VFLIP);
    ADD_ENTRY(Shape::LARGE_TRIANGLE | Shape::HVFLIP);
    ADD_ENTRY(Shape::SMALL_TRIANGLE_HOR);
    ADD_ENTRY(Shape::SMALL_TRIANGLE_HOR | Shape::HFLIP);
    ADD_ENTRY(Shape::SMALL_TRIANGLE_HOR | Shape::VFLIP);
    ADD_ENTRY(Shape::SMALL_TRIANGLE_HOR | Shape::HVFLIP);
    ADD_ENTRY(Shape::TALL_SMALL_TRIANGLE_HOR);
    ADD_ENTRY(Shape::TALL_SMALL_TRIANGLE_HOR | Shape::HFLIP);
    ADD_ENTRY(Shape::TALL_SMALL_TRIANGLE_HOR | Shape::VFLIP);
    ADD_ENTRY(Shape::TALL_SMALL_TRIANGLE_HOR | Shape::HVFLIP);
    ADD_ENTRY(Shape::HALF_SQUARE_HOR);
    ADD_ENTRY(Shape::HALF_SQUARE_HOR | Shape::HFLIP);
    ADD_ENTRY(Shape::HALF_SQUARE_HOR | Shape::VFLIP);
    ADD_ENTRY(Shape::HALF_SQUARE_HOR | Shape::HVFLIP);
    ADD_ENTRY(Shape::SMALL_TRIANGLE_VER);
    ADD_ENTRY(Shape::SMALL_TRIANGLE_VER | Shape::HFLIP);
    ADD_ENTRY(Shape::SMALL_TRIANGLE_VER | Shape::VFLIP);
    ADD_ENTRY(Shape::SMALL_TRIANGLE_VER | Shape::HVFLIP);
    ADD_ENTRY(Shape::TALL_SMALL_TRIANGLE_VER);
    ADD_ENTRY(Shape::TALL_SMALL_TRIANGLE_VER | Shape::HFLIP);
    ADD_ENTRY(Shape::TALL_SMALL_TRIANGLE_VER | Shape::VFLIP);
    ADD_ENTRY(Shape::TALL_SMALL_TRIANGLE_VER | Shape::HVFLIP);
    ADD_ENTRY(Shape::HALF_SQUARE_VER);
    ADD_ENTRY(Shape::HALF_SQUARE_VER | Shape::HFLIP);
    ADD_ENTRY(Shape::HALF_SQUARE_VER | Shape::VFLIP);
    ADD_ENTRY(Shape::HALF_SQUARE_VER | Shape::HVFLIP);

#undef ADD_ENTRY

    return result;
}

const auto VISIBILITY_VERTICES_FUNC_TABLE = MakeVisibilityVerticesFuncTable();
} // namespace

std::size_t GetVisibilityVertices(Shape                              aShape,
                                  const CellModel&                   aCell,
                                  std::uint16_t                      aEdgesOfInterest,
                                  bool                               aAllEdgesOverride,
                                  float                              aPaddingOffset,
                                  std::array<hg::math::Vector2f, 8>& aVertices) //
{
    const auto funcIdx = static_cast<std::size_t>(aShape);
    HG_ASSERT(funcIdx < VISIBILITY_VERTICES_FUNC_TABLE.size());
    const auto func = VISIBILITY_VERTICES_FUNC_TABLE[funcIdx];
    return func(aCell, aEdgesOfInterest, aAllEdgesOverride, aPaddingOffset, aVertices);
}

} // namespace gridgoblin
} // namespace jbatnozic
