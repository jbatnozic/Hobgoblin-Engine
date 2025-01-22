#include "Character_renderer.hpp"

#include <Hobgoblin/HGExcept.hpp>
#include <Hobgoblin/Math.hpp>

#include "Resource_manager_interface.hpp"
#include "Sprite_manifest.hpp"

#include <array>
#include <cmath>

CharacterRenderer::CharacterRenderer(spe::GameContext& aCtx, hg::gr::Color aColor)
    : _ctx{aCtx}
    , _color{aColor} {
    const auto& sprLoader = _ctx.getComponent<MResource>().getSpriteLoader();

    _legs       = sprLoader.getMultiBlueprint(SPR_LEGS).multispr();
    _openHand   = sprLoader.getMultiBlueprint(SPR_HAND_OPEN).multispr();
    _closedHand = sprLoader.getMultiBlueprint(SPR_HAND_CLOSED).multispr();

    setSize(1);
}

void CharacterRenderer::setSize(hg::PZInteger aSize) {
    if (_size == aSize) {
        return;
    }
    _size = aSize;

    const auto& sprLoader = _ctx.getComponent<MResource>().getSpriteLoader();
    switch (aSize) {
    case 1:
        {
            _desiredScale = 1.0f;

            _body    = sprLoader.getMultiBlueprint(SPR_BODY_SIZE_1).multispr();
            _biceps  = sprLoader.getMultiBlueprint(SPR_BICEPS_SIZE_1).multispr();
            _forearm = sprLoader.getMultiBlueprint(SPR_FOREARM_SIZE_1).multispr();
        }
        break;

    case 2:
        {
            _desiredScale = 1.1f;

            _body    = sprLoader.getMultiBlueprint(SPR_BODY_SIZE_2).multispr();
            _biceps  = sprLoader.getMultiBlueprint(SPR_BICEPS_SIZE_2).multispr();
            _forearm = sprLoader.getMultiBlueprint(SPR_FOREARM_SIZE_2).multispr();
        }
        break;

    case 3:
        {
            _desiredScale = 1.2f;

            _body    = sprLoader.getMultiBlueprint(SPR_BODY_SIZE_3).multispr();
            _biceps  = sprLoader.getMultiBlueprint(SPR_BICEPS_SIZE_3).multispr();
            _forearm = sprLoader.getMultiBlueprint(SPR_FOREARM_SIZE_3).multispr();
        }
        break;

    case 4:
        {
            _desiredScale = 1.3f;

            _body    = sprLoader.getMultiBlueprint(SPR_BODY_SIZE_4).multispr();
            _biceps  = sprLoader.getMultiBlueprint(SPR_BICEPS_SIZE_4).multispr();
            _forearm = sprLoader.getMultiBlueprint(SPR_FOREARM_SIZE_4).multispr();
        }
        break;

    case 5:
        {
            _desiredScale = 1.5f;

            _body    = sprLoader.getMultiBlueprint(SPR_BODY_SIZE_5).multispr();
            _biceps  = sprLoader.getMultiBlueprint(SPR_BICEPS_SIZE_5).multispr();
            _forearm = sprLoader.getMultiBlueprint(SPR_FOREARM_SIZE_5).multispr();
        }
        break;

    default:
        HG_UNREACHABLE("Invalid body size ({})", aSize);
    }
}

void CharacterRenderer::setPosition(hg::math::Vector2f aPosition) {
    _position = aPosition;
}

void CharacterRenderer::setMode(Mode aMode) {
    if (_mode != aMode) {
        _mode         = aMode;
        _frameCounter = 0.f;
    }
}

namespace {
constexpr double SHOULDER_Y_OFFSET = -1.5f * 12.f;
constexpr double SHOULDER_X_OFFSET = +5.f * 12.f;

constexpr double LEGS_Y_OFFSET = 82.5;

constexpr double BICEPS_LENGTH  = 7.f * 12.f;
constexpr double FOREARM_LENGTH = 6.f * 12.f;

// Climb vertical
// clang-format off
const std::array<hg::math::Vector2d, 8> CLIMB_V_HAND_OFFSETS = {{
     {7.0 * 12.0, -6.0 * 12.0},
     {7.0 * 12.0, -7.0 * 12.0},
     {7.0 * 12.0, -8.0 * 12.0},
     {7.0 * 12.0, -7.0 * 12.0},
     {7.0 * 12.0, -6.0 * 12.0},
     {7.0 * 12.0, -5.0 * 12.0},
     {7.0 * 12.0, -4.0 * 12.0},
     {7.0 * 12.0, -5.0 * 12.0},
}};
// clang-format on

// Climb horizontal
// clang-format off
const std::array<hg::math::Vector2d, 8> CLIMB_H_HAND_OFFSETS = {{
     {7.0 * 12.0, -6.0 * 12.0},
     {8.0 * 12.0, -6.0 * 12.0},
     {9.0 * 12.0, -6.0 * 12.0},
     {8.0 * 12.0, -6.0 * 12.0},
     {7.0 * 12.0, -6.0 * 12.0},
     {6.0 * 12.0, -6.0 * 12.0},
     {5.0 * 12.0, -6.0 * 12.0},
     {6.0 * 12.0, -6.0 * 12.0},
}};
// clang-format on

// Fling
// clang-format off
const std::array<hg::math::Vector2d, 8> FLING_HAND_OFFSETS = {{
     {7.0 * 12.0,  -6.0 * 12.0},
     {6.0 * 12.0,  -2.0 * 12.0},
     {5.0 * 12.0,  +1.0 * 12.0},
     {9.0 * 12.0,  +8.0 * 12.0},
     {7.0 * 12.0, +10.0 * 12.0},
     {6.0 * 12.0, +11.0 * 12.0},
     {9.0 * 12.0,  +8.0 * 12.0},
     {5.0 * 12.0,  +1.0 * 12.0}
}};
// clang-format on

struct ArmDrawParams {
    hg::math::Vector2f bicepsOffset;
    hg::math::AngleF   bicepsAngle;

    hg::math::Vector2f forearmOffset;
    hg::math::AngleF   forearmAngle;

    hg::math::Vector2f handOffset;
    hg::math::AngleF   handAngle;
};

ArmDrawParams GetArmDrawParams_ClimbVertical(hg::PZInteger aAnimationFrame) {
    using namespace hg::math;

    ArmDrawParams result;

    const double C = EuclideanDist(0.0,
                                   0.0,
                                   CLIMB_V_HAND_OFFSETS[aAnimationFrame].x,
                                   CLIMB_V_HAND_OFFSETS[aAnimationFrame].y);

    const double cosAf = (Sqr(BICEPS_LENGTH) + Sqr(C) - Sqr(FOREARM_LENGTH)) / (2.0 * BICEPS_LENGTH * C);

    const auto Af = AngleD::fromRad(std::acos(cosAf));

    const auto P = AngleD::fromVector(
        {CLIMB_V_HAND_OFFSETS[aAnimationFrame].x, CLIMB_V_HAND_OFFSETS[aAnimationFrame].y});

    const auto Pp = P - Af;

    const double elbowOffsetX = +Pp.cos() * BICEPS_LENGTH;
    const double elbowOffsetY = -Pp.sin() * BICEPS_LENGTH;

    const double bicepsCenterX = elbowOffsetX / 2; // from shoulder
    const double bicepsCenterY = elbowOffsetY / 2; // from shoulder

    result.bicepsOffset = {static_cast<float>(SHOULDER_X_OFFSET + bicepsCenterX),
                           static_cast<float>(SHOULDER_Y_OFFSET + bicepsCenterY)};
    result.bicepsAngle  = PointDirection<float>(0.f,
                                               0.f,
                                               static_cast<float>(elbowOffsetX),
                                               static_cast<float>(elbowOffsetY));

    const double forearmCenterX =
        (CLIMB_V_HAND_OFFSETS[aAnimationFrame].x + elbowOffsetX) / 2.0; // from shoulder
    const double forearmCenterY =
        (CLIMB_V_HAND_OFFSETS[aAnimationFrame].y + elbowOffsetY) / 2.0; // from shoulder

    result.forearmOffset = {static_cast<float>(SHOULDER_X_OFFSET + forearmCenterX),
                            static_cast<float>(SHOULDER_Y_OFFSET + forearmCenterY)};
    result.forearmAngle =
        PointDirection<float>(static_cast<float>(elbowOffsetX),
                              static_cast<float>(elbowOffsetY),
                              static_cast<float>(CLIMB_V_HAND_OFFSETS[aAnimationFrame].x),
                              static_cast<float>(CLIMB_V_HAND_OFFSETS[aAnimationFrame].y)) -
        AngleF::fromDegrees(90.f);

    result.handOffset = {
        static_cast<float>(SHOULDER_X_OFFSET + CLIMB_V_HAND_OFFSETS[aAnimationFrame].x),
        static_cast<float>(SHOULDER_Y_OFFSET + CLIMB_V_HAND_OFFSETS[aAnimationFrame].y)};
    result.handAngle = result.forearmAngle;

    return result;
}

ArmDrawParams GetArmDrawParams_ClimbHorizontal(hg::PZInteger aAnimationFrame) {
    using namespace hg::math;

    ArmDrawParams result;

    const double C = EuclideanDist(0.0,
                                   0.0,
                                   CLIMB_H_HAND_OFFSETS[aAnimationFrame].x,
                                   CLIMB_H_HAND_OFFSETS[aAnimationFrame].y);

    const double cosAf = (Sqr(BICEPS_LENGTH) + Sqr(C) - Sqr(FOREARM_LENGTH)) / (2.0 * BICEPS_LENGTH * C);

    const auto Af = AngleD::fromRad(std::acos(cosAf));

    const auto P = AngleD::fromVector(
        {CLIMB_H_HAND_OFFSETS[aAnimationFrame].x, CLIMB_H_HAND_OFFSETS[aAnimationFrame].y});

    const auto Pp = P - Af;

    const double elbowOffsetX = +Pp.cos() * BICEPS_LENGTH;
    const double elbowOffsetY = -Pp.sin() * BICEPS_LENGTH;

    const double bicepsCenterX = elbowOffsetX / 2; // from shoulder
    const double bicepsCenterY = elbowOffsetY / 2; // from shoulder

    result.bicepsOffset = {static_cast<float>(SHOULDER_X_OFFSET + bicepsCenterX),
                           static_cast<float>(SHOULDER_Y_OFFSET + bicepsCenterY)};
    result.bicepsAngle  = PointDirection<float>(0.f,
                                               0.f,
                                               static_cast<float>(elbowOffsetX),
                                               static_cast<float>(elbowOffsetY));

    const double forearmCenterX =
        (CLIMB_H_HAND_OFFSETS[aAnimationFrame].x + elbowOffsetX) / 2.0; // from shoulder
    const double forearmCenterY =
        (CLIMB_H_HAND_OFFSETS[aAnimationFrame].y + elbowOffsetY) / 2.0; // from shoulder

    result.forearmOffset = {static_cast<float>(SHOULDER_X_OFFSET + forearmCenterX),
                            static_cast<float>(SHOULDER_Y_OFFSET + forearmCenterY)};
    result.forearmAngle =
        PointDirection<float>(static_cast<float>(elbowOffsetX),
                              static_cast<float>(elbowOffsetY),
                              static_cast<float>(CLIMB_H_HAND_OFFSETS[aAnimationFrame].x),
                              static_cast<float>(CLIMB_H_HAND_OFFSETS[aAnimationFrame].y)) -
        AngleF::fromDegrees(90.f);

    result.handOffset = {
        static_cast<float>(SHOULDER_X_OFFSET + CLIMB_H_HAND_OFFSETS[aAnimationFrame].x),
        static_cast<float>(SHOULDER_Y_OFFSET + CLIMB_H_HAND_OFFSETS[aAnimationFrame].y)};
    result.handAngle = result.forearmAngle;

    return result;
}

ArmDrawParams GetArmDrawParams_Fling(hg::PZInteger aAnimationFrame) {
    using namespace hg::math;

    ArmDrawParams result;

    const double C = EuclideanDist(0.0,
                                   0.0,
                                   FLING_HAND_OFFSETS[aAnimationFrame].x,
                                   FLING_HAND_OFFSETS[aAnimationFrame].y);

    const double cosAf = (Sqr(BICEPS_LENGTH) + Sqr(C) - Sqr(FOREARM_LENGTH)) / (2.0 * BICEPS_LENGTH * C);

    const auto Af = AngleD::fromRad(std::acos(cosAf));

    const auto P = AngleD::fromVector(
        {FLING_HAND_OFFSETS[aAnimationFrame].x, FLING_HAND_OFFSETS[aAnimationFrame].y});

    const auto Pp = P - Af;

    const double elbowOffsetX = +Pp.cos() * BICEPS_LENGTH;
    const double elbowOffsetY = -Pp.sin() * BICEPS_LENGTH;

    const double bicepsCenterX = elbowOffsetX / 2; // from shoulder
    const double bicepsCenterY = elbowOffsetY / 2; // from shoulder

    result.bicepsOffset = {static_cast<float>(SHOULDER_X_OFFSET + bicepsCenterX),
                           static_cast<float>(SHOULDER_Y_OFFSET + bicepsCenterY)};
    result.bicepsAngle  = PointDirection<float>(0.f,
                                               0.f,
                                               static_cast<float>(elbowOffsetX),
                                               static_cast<float>(elbowOffsetY));

    const double forearmCenterX =
        (FLING_HAND_OFFSETS[aAnimationFrame].x + elbowOffsetX) / 2.0; // from shoulder
    const double forearmCenterY =
        (FLING_HAND_OFFSETS[aAnimationFrame].y + elbowOffsetY) / 2.0; // from shoulder

    result.forearmOffset = {static_cast<float>(SHOULDER_X_OFFSET + forearmCenterX),
                            static_cast<float>(SHOULDER_Y_OFFSET + forearmCenterY)};
    result.forearmAngle =
        PointDirection<float>(static_cast<float>(elbowOffsetX),
                              static_cast<float>(elbowOffsetY),
                              static_cast<float>(FLING_HAND_OFFSETS[aAnimationFrame].x),
                              static_cast<float>(FLING_HAND_OFFSETS[aAnimationFrame].y)) -
        AngleF::fromDegrees(90.f);

    result.handOffset = {static_cast<float>(SHOULDER_X_OFFSET + FLING_HAND_OFFSETS[aAnimationFrame].x),
                         static_cast<float>(SHOULDER_Y_OFFSET + FLING_HAND_OFFSETS[aAnimationFrame].y)};
    result.handAngle  = result.forearmAngle;

    return result;
}

} // namespace

void CharacterRenderer::update() {
    _frameCounter += 0.25f;

    if (std::fabs(_scale - _desiredScale) > 0.01f) {
        if (_scale < _desiredScale) {
            _scale += 0.01f;
        } else {
            _scale -= 0.01f;
        }
    }

    switch (_mode) {
    case Mode::STILL:
        _legsAngleCounter += (hg::math::PI_F / 25.f) * 0.2f;
        break;

    case Mode::CRAWL_VERTICAL:
        _legsAngleCounter += (hg::math::PI_F / 25.f) * 0.6f;
        break;

    case Mode::CRAWL_HORIZONTAL_PLUS:
        if (std::abs(std::sinf(_legsAngleCounter) - (-1.f)) > 0.1f) {
            _legsAngleCounter += (hg::math::PI_F / 25.f) * 2.f;
        }
        break;

    case Mode::CRAWL_HORIZONTAL_MINUS:
        if (std::abs(std::sinf(_legsAngleCounter) - (+1.f)) > 0.1f) {
            _legsAngleCounter += (hg::math::PI_F / 25.f) * 2.f;
        }
        break;

    case Mode::FLING:
    case Mode::HUNKER:
        _legsAngleCounter = 0.f;
        break;

    default:
        HG_UNREACHABLE("Invalid draw mode.");
    }
}

void CharacterRenderer::draw(hg::gr::Canvas& aCanvas) {
    switch (_mode) {
    case Mode::STILL:
        _drawStill(aCanvas);
        break;

    case Mode::CRAWL_VERTICAL:
        _drawClimbVertical(aCanvas);
        break;

    case Mode::CRAWL_HORIZONTAL_PLUS:
        _drawClimbHorizontal(aCanvas, +1);
        break;

    case Mode::CRAWL_HORIZONTAL_MINUS:
        _drawClimbHorizontal(aCanvas, -1);
        break;

    case Mode::FLING:
        _drawFling(aCanvas);
        break;

    case Mode::HUNKER:
        break;

    default:
        HG_UNREACHABLE("Invalid draw mode.");
    }
}

void CharacterRenderer::_drawClimbVertical(hg::gr::Canvas& aCanvas) {
    using namespace hg::math;

    const float scale = _scale;

    _body.setPosition(_position);
    _body.setScale({scale, scale});
    aCanvas.draw(_body);

    unsigned animationFrame = static_cast<unsigned>(_frameCounter) % 8;

    // Right arm
    {
        const auto params = GetArmDrawParams_ClimbVertical(animationFrame);

        _biceps.setPosition(
            {_position.x + scale * params.bicepsOffset.x, _position.y + scale * params.bicepsOffset.y});
        _biceps.setRotation(params.bicepsAngle);
        _biceps.setScale({scale * 1.f, scale * 1.f});

        _forearm.setPosition({_position.x + scale * params.forearmOffset.x,
                              _position.y + scale * params.forearmOffset.y});
        _forearm.setRotation(params.forearmAngle);
        _forearm.setScale({scale * 1.f, scale * 1.f});

        hg::gr::Multisprite* handSprite =
            (animationFrame >= 3 && animationFrame <= 6) ? &_closedHand : &_openHand;
        handSprite->setPosition(
            {_position.x + scale * params.handOffset.x, _position.y + scale * params.handOffset.y});
        handSprite->setRotation(params.handAngle);
        handSprite->setScale({scale * 1.f, scale * 1.f});

        aCanvas.draw(_forearm);
        aCanvas.draw(*handSprite);
        aCanvas.draw(_biceps);
    }

    // Left arm
    {
        const auto params = GetArmDrawParams_ClimbVertical((animationFrame + 4) % 8);

        _biceps.setPosition(
            {_position.x - scale * params.bicepsOffset.x, _position.y + scale * params.bicepsOffset.y});
        _biceps.setRotation(-params.bicepsAngle);
        _biceps.setScale({scale * -1.f, scale * 1.f});

        _forearm.setPosition({_position.x - scale * params.forearmOffset.x,
                              _position.y + scale * params.forearmOffset.y});
        _forearm.setRotation(-params.forearmAngle + AngleF::fromDegrees(180.0));
        _forearm.setScale({scale * 1.f, scale * -1.f});

        hg::gr::Multisprite* handSprite =
            (((animationFrame + 4) % 8) >= 3 && ((animationFrame + 4) % 8) <= 6) ? &_closedHand
                                                                                 : &_openHand;
        handSprite->setPosition(
            {_position.x - scale * params.handOffset.x, _position.y + scale * params.handOffset.y});
        handSprite->setRotation(-params.handAngle);
        handSprite->setScale({scale * -1.f, scale * 1.f});

        aCanvas.draw(_forearm);
        aCanvas.draw(*handSprite);
        aCanvas.draw(_biceps);
    }

    // Legs
    {
        const auto legsAngle = AngleF::fromDegrees(15.f) * std::sinf(_legsAngleCounter);

        auto&              shader = _ctx.getComponent<MResource>().getUnderpantsShader();
        hg::gr::glsl::Vec4 vec4{_color.r / 255.f, _color.g / 255.f, _color.b / 255.f, 1.f};
        shader.setUniform("playerColor", vec4);

        _legs.setPosition({_position.x, _position.y + scale * static_cast<float>(LEGS_Y_OFFSET)});
        _legs.setRotation(legsAngle);
        _legs.setScale({scale, scale});
        aCanvas.draw(_legs, &shader);
    }
}

void CharacterRenderer::_drawClimbHorizontal(hg::gr::Canvas& aCanvas, int aDirection) {
    using namespace hg::math;

    const float scale = _scale;

    _body.setPosition(_position);
    _body.setScale({scale, scale});
    aCanvas.draw(_body);

    unsigned animationFrame = static_cast<unsigned>(_frameCounter) % 8;

    // Right arm
    {
        const auto params = GetArmDrawParams_ClimbHorizontal(animationFrame);

        _biceps.setPosition(
            {_position.x + scale * params.bicepsOffset.x, _position.y + scale * params.bicepsOffset.y});
        _biceps.setRotation(params.bicepsAngle);
        _biceps.setScale({scale * 1.f, scale * 1.f});

        _forearm.setPosition({_position.x + scale * params.forearmOffset.x,
                              _position.y + scale * params.forearmOffset.y});
        _forearm.setRotation(params.forearmAngle);
        _forearm.setScale({scale * 1.f, scale * 1.f});

        hg::gr::Multisprite* handSprite =
            (animationFrame >= 3 && animationFrame <= 6) ? &_closedHand : &_openHand;
        handSprite->setPosition(
            {_position.x + scale * params.handOffset.x, _position.y + scale * params.handOffset.y});
        handSprite->setRotation(params.handAngle);
        handSprite->setScale({scale * 1.f, scale * 1.f});

        aCanvas.draw(_forearm);
        aCanvas.draw(*handSprite);
        aCanvas.draw(_biceps);
    }

    // Left arm
    {
        const auto params = GetArmDrawParams_ClimbHorizontal((animationFrame + 0) % 8);

        _biceps.setPosition(
            {_position.x - scale * params.bicepsOffset.x, _position.y + scale * params.bicepsOffset.y});
        _biceps.setRotation(-params.bicepsAngle);
        _biceps.setScale({scale * -1.f, scale * 1.f});

        _forearm.setPosition({_position.x - scale * params.forearmOffset.x,
                              _position.y + scale * params.forearmOffset.y});
        _forearm.setRotation(-params.forearmAngle + AngleF::fromDegrees(180.0));
        _forearm.setScale({scale * 1.f, scale * -1.f});

        hg::gr::Multisprite* handSprite =
            (((animationFrame + 0) % 8) >= 3 && ((animationFrame + 0) % 8) <= 6) ? &_closedHand
                                                                                 : &_openHand;
        handSprite->setPosition(
            {_position.x - scale * params.handOffset.x, _position.y + scale * params.handOffset.y});
        handSprite->setRotation(-params.handAngle);
        handSprite->setScale({scale * -1.f, scale * 1.f});

        aCanvas.draw(_forearm);
        aCanvas.draw(*handSprite);
        aCanvas.draw(_biceps);
    }

    // Legs
    {
        const auto legsAngle = AngleF::fromDegrees(15.f) * std::sinf(_legsAngleCounter);

        auto&              shader = _ctx.getComponent<MResource>().getUnderpantsShader();
        hg::gr::glsl::Vec4 vec4{_color.r / 255.f, _color.g / 255.f, _color.b / 255.f, 1.f};
        shader.setUniform("playerColor", vec4);

        _legs.setPosition({_position.x, _position.y + scale * static_cast<float>(LEGS_Y_OFFSET)});
        _legs.setRotation(legsAngle);
        _legs.setScale({scale, scale});
        aCanvas.draw(_legs, &shader);
    }
}

void CharacterRenderer::_drawStill(hg::gr::Canvas& aCanvas) {
    using namespace hg::math;

    const float scale = _scale;

    _body.setPosition(_position);
    _body.setScale({scale, scale});
    aCanvas.draw(_body);

    const unsigned animationFrame = 0;

    // Right arm
    {
        const auto params = GetArmDrawParams_ClimbVertical(animationFrame);

        _biceps.setPosition(
            {_position.x + scale * params.bicepsOffset.x, _position.y + scale * params.bicepsOffset.y});
        _biceps.setRotation(params.bicepsAngle);
        _biceps.setScale({scale * 1.f, scale * 1.f});

        _forearm.setPosition({_position.x + scale * params.forearmOffset.x,
                              _position.y + scale * params.forearmOffset.y});
        _forearm.setRotation(params.forearmAngle);
        _forearm.setScale({scale * 1.f, scale * 1.f});

        hg::gr::Multisprite* handSprite = &_closedHand;
        handSprite->setPosition(
            {_position.x + scale * params.handOffset.x, _position.y + scale * params.handOffset.y});
        handSprite->setRotation(params.handAngle);
        handSprite->setScale({scale * 1.f, scale * 1.f});

        aCanvas.draw(_forearm);
        aCanvas.draw(*handSprite);
        aCanvas.draw(_biceps);
    }

    // Left arm
    {
        const auto params = GetArmDrawParams_ClimbVertical(animationFrame);

        _biceps.setPosition(
            {_position.x - scale * params.bicepsOffset.x, _position.y + scale * params.bicepsOffset.y});
        _biceps.setRotation(-params.bicepsAngle);
        _biceps.setScale({scale * -1.f, scale * 1.f});

        _forearm.setPosition({_position.x - scale * params.forearmOffset.x,
                              _position.y + scale * params.forearmOffset.y});
        _forearm.setRotation(params.forearmAngle + AngleF::fromDegrees(180.0));
        _forearm.setScale({scale * 1.f, scale * -1.f});

        hg::gr::Multisprite* handSprite = &_closedHand;
        handSprite->setPosition(
            {_position.x - scale * params.handOffset.x, _position.y + scale * params.handOffset.y});
        handSprite->setRotation(params.handAngle);
        handSprite->setScale({scale * -1.f, scale * 1.f});

        aCanvas.draw(_forearm);
        aCanvas.draw(*handSprite);
        aCanvas.draw(_biceps);
    }

    // Legs
    {
        const auto legsAngle = AngleF::fromDegrees(15.f) * std::sinf(_legsAngleCounter);

        auto&              shader = _ctx.getComponent<MResource>().getUnderpantsShader();
        hg::gr::glsl::Vec4 vec4{_color.r / 255.f, _color.g / 255.f, _color.b / 255.f, 1.f};
        shader.setUniform("playerColor", vec4);

        _legs.setPosition({_position.x, _position.y + scale * static_cast<float>(LEGS_Y_OFFSET)});
        _legs.setRotation(legsAngle);
        _legs.setScale({scale, scale});
        aCanvas.draw(_legs, &shader);
    }
}

void CharacterRenderer::_drawFling(hg::gr::Canvas& aCanvas) {
    using namespace hg::math;

    const float scale = _scale;

    _body.setPosition(_position);
    _body.setScale({scale, scale});
    aCanvas.draw(_body);

    unsigned animationFrame = std::min(static_cast<unsigned>(_frameCounter), 7u);

    // Right arm
    {
        const auto params = GetArmDrawParams_Fling(animationFrame);

        _biceps.setPosition(
            {_position.x + scale * params.bicepsOffset.x, _position.y + scale * params.bicepsOffset.y});
        _biceps.setRotation(params.bicepsAngle);
        _biceps.setScale({scale * 1.f, scale * 1.f});

        _forearm.setPosition({_position.x + scale * params.forearmOffset.x,
                              _position.y + scale * params.forearmOffset.y});
        _forearm.setRotation(params.forearmAngle);
        _forearm.setScale({scale * 1.f, scale * 1.f});

        hg::gr::Multisprite* handSprite = (animationFrame <= 4) ? &_closedHand : &_openHand;
        handSprite->setPosition(
            {_position.x + scale * params.handOffset.x, _position.y + scale * params.handOffset.y});
        handSprite->setRotation(params.handAngle);
        handSprite->setScale({scale * 1.f, scale * 1.f});

        aCanvas.draw(_forearm);
        aCanvas.draw(*handSprite);
        aCanvas.draw(_biceps);
    }

    // Left arm
    {
        const auto params = GetArmDrawParams_Fling(animationFrame);

        _biceps.setPosition(
            {_position.x - scale * params.bicepsOffset.x, _position.y + scale * params.bicepsOffset.y});
        _biceps.setRotation(-params.bicepsAngle);
        _biceps.setScale({scale * -1.f, scale * 1.f});

        _forearm.setPosition({_position.x - scale * params.forearmOffset.x,
                              _position.y + scale * params.forearmOffset.y});
        _forearm.setRotation(-params.forearmAngle + AngleF::fromDegrees(180.0));
        _forearm.setScale({scale * 1.f, scale * -1.f});

        hg::gr::Multisprite* handSprite = (animationFrame <= 4) ? &_closedHand : &_openHand;
        handSprite->setPosition(
            {_position.x - scale * params.handOffset.x, _position.y + scale * params.handOffset.y});
        handSprite->setRotation(-params.handAngle);
        handSprite->setScale({scale * -1.f, scale * 1.f});

        aCanvas.draw(_forearm);
        aCanvas.draw(*handSprite);
        aCanvas.draw(_biceps);
    }

    // Legs
    {
        const auto legsAngle = AngleF::fromDegrees(15.f) * std::sinf(_legsAngleCounter);

        auto&              shader = _ctx.getComponent<MResource>().getUnderpantsShader();
        hg::gr::glsl::Vec4 vec4{_color.r / 255.f, _color.g / 255.f, _color.b / 255.f, 1.f};
        shader.setUniform("playerColor", vec4);

        _legs.setPosition({_position.x, _position.y + scale * static_cast<float>(LEGS_Y_OFFSET)});
        _legs.setRotation(legsAngle);
        _legs.setScale({scale, scale});
        aCanvas.draw(_legs, &shader);
    }
}