// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#pragma once

#include <GridGoblin/Model/Sprites.hpp>
#include <GridGoblin/Spatial/Position_in_world.hpp>

#include <Hobgoblin/Graphics/Color.hpp>
#include <Hobgoblin/Math/Vector.hpp>

namespace jbatnozic {
namespace gridgoblin {

namespace hg = jbatnozic::hobgoblin;

class Light {
public:
    Light(PositionInWorld aCenter, float aRadius, hg::gr::Color aColor, SpriteId aSpriteId)
        : _center{aCenter}
        , _radius{aRadius}
        , _spriteId{aSpriteId}
        , _color{aColor} {}

    PositionInWorld getCenter() const;
    void            setCenter(PositionInWorld aNewCenter);

    float getRadius() const;
    void  setRadius(float aNewRadius);

    hg::gr::Color getColor() const;
    void          setColor(hg::gr::Color aNewColor);

    SpriteId getSpriteId() const {
        return _spriteId;
    }

private:
    PositionInWorld _center;
    float           _radius;
    SpriteId        _spriteId;
    hg::gr::Color   _color;
    bool            _isDirty = true;
};

inline PositionInWorld Light::getCenter() const {
    return _center;
}

inline void Light::setCenter(PositionInWorld aNewCenter) {
    _center  = aNewCenter;
    _isDirty = true;
}

inline float Light::getRadius() const {
    return _radius;
}

inline void Light::setRadius(float aNewRadius) {
    _radius  = aNewRadius;
    _isDirty = true;
}

inline hg::gr::Color Light::getColor() const {
    return _color;
}

inline void Light::setColor(hg::gr::Color aNewColor) {
    _color = aNewColor;
    // _isDirty  = true;
}

} // namespace gridgoblin
} // namespace jbatnozic
