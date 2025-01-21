// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#pragma once

#include <GridGoblin/Spatial/Position_in_world.hpp>

#include <Hobgoblin/Graphics/Color.hpp>
#include <Hobgoblin/Math/Vector.hpp>

namespace jbatnozic {
namespace gridgoblin {

namespace hg = jbatnozic::hobgoblin;

class LightModel {
public:
    LightModel(PositionInWorld aCenter, float aRadius, hg::gr::Color aColor)
        : _center{aCenter}, _radius{aRadius}, _color{aColor} {}

    PositionInWorld getCenter() const;
    void            setCenter(PositionInWorld aNewCenter);

    float getRadius() const;
    void  setRadius(float aNewRadius);

    hg::gr::Color getColor() const;
    void          setColor(hg::gr::Color aNewColor);

private:
    PositionInWorld _center;
    float           _radius;
    hg::gr::Color   _color;
    bool            _isDirty = true;
};

inline PositionInWorld LightModel::getCenter() const {
    return _center;
}

inline void LightModel::setCenter(PositionInWorld aNewCenter) {
    _center  = aNewCenter;
    _isDirty = true;
}

inline float LightModel::getRadius() const {
    return _radius;
}

inline void LightModel::setRadius(float aNewRadius) {
    _radius  = aNewRadius;
    _isDirty = true;
}

inline hg::gr::Color LightModel::getColor() const {
    return _color;
}

inline void LightModel::setColor(hg::gr::Color aNewColor) {
    _color = aNewColor;
    // _isDirty  = true;
}

} // namespace gridgoblin
} // namespace jbatnozic
