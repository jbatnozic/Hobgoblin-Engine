// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#pragma once

#include <Hobgoblin/Math/Vector.hpp>

namespace jbatnozic {
namespace gridgoblin {

namespace hg = ::jbatnozic::hobgoblin;

class PositionInView {
public:
    PositionInView()                                 = default;
    PositionInView(const PositionInView&)            = default;
    PositionInView& operator=(const PositionInView&) = default;
    PositionInView(PositionInView&&)                 = default;
    PositionInView& operator=(PositionInView&&)      = default;

    explicit PositionInView(float aX, float aY)
        : pos{aX, aY} {}

    explicit PositionInView(hg::math::Vector2f aPos)
        : pos{aPos} {}

    hg::math::Vector2f pos;

    hg::math::Vector2f& operator*() {
        return pos;
    }

    const hg::math::Vector2f& operator*() const {
        return pos;
    }

    hg::math::Vector2f* operator->() {
        return &pos;
    }

    const hg::math::Vector2f* operator->() const {
        return &pos;
    }
};

static_assert(sizeof(PositionInView) <= sizeof(void*));

} // namespace gridgoblin
} // namespace jbatnozic
