#pragma once

#include "Engine.hpp"

#include <Hobgoblin/Graphics.hpp>
#include <Hobgoblin/Math/Vector.hpp>

class CharacterRenderer {
public:
    enum class Mode : std::int8_t {
        STILL,
        CRAWL_VERTICAL,
        CRAWL_HORIZONTAL_PLUS,
        CRAWL_HORIZONTAL_MINUS,
        FLING,
        HUNKER
    };

    CharacterRenderer(spe::GameContext& aCtx, hg::gr::Color aColor);

    void setColor(hg::gr::Color aColor) {
        _color = aColor;
    }

    void setSize(hg::PZInteger aSize);
    void setPosition(hg::math::Vector2f aPosition);
    void setMode(Mode aMode);

    void update();
    void draw(hg::gr::Canvas& aCanvas);

private:
    spe::GameContext& _ctx;

    Mode _mode = Mode::CRAWL_VERTICAL;

    hg::PZInteger _size         = 0;
    float         _scale        = 1.0;
    float         _desiredScale = 1.0;

    float _frameCounter     = 0.f;
    float _legsAngleCounter = 0.f;

    hg::math::Vector2f _position;
    hg::gr::Color      _color;

    hg::gr::Multisprite _body;
    hg::gr::Multisprite _legs;
    hg::gr::Multisprite _biceps;
    hg::gr::Multisprite _forearm;
    hg::gr::Multisprite _openHand;
    hg::gr::Multisprite _closedHand;

    void _drawStill(hg::gr::Canvas& aCanvas);
    void _drawClimbVertical(hg::gr::Canvas& aCanvas);
    void _drawClimbHorizontal(hg::gr::Canvas& aCanvas, int aDirection);
    void _drawFling(hg::gr::Canvas& aCanvas);
};
