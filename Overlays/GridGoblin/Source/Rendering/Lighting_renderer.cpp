// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#include <GridGoblin/Rendering/Lighting_renderer.hpp>
#include <GridGoblin/World/World.hpp>

#include <Hobgoblin/Graphics.hpp>
#include <Hobgoblin/HGExcept.hpp>

#include "../Detail_access.hpp"
#include "OpenGL_helpers.hpp"

#include <algorithm>

namespace jbatnozic {
namespace gridgoblin {

namespace {
void RenderLightsToRenderTexture(World::LightIterator   aStartIter,
                                 World::LightIterator   aEndIter,
                                 hg::gr::RenderTexture& aRenderTexture,
                                 const hg::gr::SpriteLoader& aSpriteLoader) {
    for (; aStartIter != aEndIter; ++aStartIter) {
        const auto& light = *aStartIter;
        auto&       ext   = GetMutableExtensionData(light);

        ext.render(aRenderTexture, aSpriteLoader, hg::gr::BLEND_ADD);
    }
}
} // namespace

using namespace opengl;

LightingRenderer::LightingRenderer(const World&                aWorld,
                                   const hg::gr::SpriteLoader& aSpriteLoader,
                                   hg::math::Vector2pz         aTextureSize,
                                   Purpose                     aPurpose)
    : _world{aWorld}
    , _spriteLoader{aSpriteLoader}
    , _renderTexture{std::make_unique<hg::gr::RenderTexture>()}
    , _purpose{aPurpose} //
{
    if (_purpose == Purpose::DIMETRIC_RENDERING) {
        HG_VALIDATE_ARGUMENT(aTextureSize.x == aTextureSize.y,
                             "Only square-shaped textures are supported for dimetric rendering.");
    }

    _renderTexture->create(aTextureSize);
    _textureSize = aTextureSize;

    _textureRamBuffer.resize(static_cast<std::size_t>(aTextureSize.x * aTextureSize.y) * 4u);
    std::memset(_textureRamBuffer.data(), 0x00, _textureRamBuffer.size());

    DualPBO_Init(_pboNames, _textureRamBuffer.size());
}

LightingRenderer::~LightingRenderer() {
    DualPBO_Destroy(_pboNames);
}

void LightingRenderer::prepareToRender(PositionInWorld aViewCenter, hg::math::Vector2f aViewSize) {
    _viewCenterOffset = PositionInWorld{*aViewCenter - *_viewCenter};
    _viewCenter       = aViewCenter;

    const float squareSize = [this, &aViewSize]() {
        switch (_purpose) {
        case Purpose::NORMAL_RENDERING:
            return std::max(aViewSize.x, aViewSize.y);

        case Purpose::DIMETRIC_RENDERING:
            if (aViewSize.x > aViewSize.y) {
                return (aViewSize.x + 2.f * aViewSize.y) / std::sqrt(2.f);
            } else {
                return (aViewSize.y + 2.f * aViewSize.x) / std::sqrt(2.f);
            }

        default:
            HG_UNREACHABLE("Illegal value for enum Purpose ({}).", (int)_purpose);
        }
    }();

    _textureDrawScale = {squareSize / _renderTexture->getSize().x,
                         squareSize / _renderTexture->getSize().y};

    _renderTexture->clear(hg::gr::COLOR_BLACK); // TODO

    auto& view = _renderTexture->getView();
    view.setCenter(*_viewCenter);
    view.setSize({squareSize, squareSize});

    RenderLightsToRenderTexture(_world.dynamicLightsBegin(),
                                _world.dynamicLightsEnd(),
                                *_renderTexture,
                                _spriteLoader);
    // TODO: render static lights
    // TODO: render simple lights

    _renderTexture->display();

    _stepCounter += 1;

    {
        const unsigned targetPbo = (_stepCounter % 2);
        DualPBO_StartTransfer(_pboNames, targetPbo, _renderTexture->getTexture().getNativeHandle());
    }

    {
        const unsigned targetPbo = ((_stepCounter + 1) % 2);
        DualPBO_LoadIntoRam(_pboNames, targetPbo, _textureRamBuffer.data(), _textureRamBuffer.size());
    }
}

void LightingRenderer::render(hg::gr::Canvas& aCanvas, const hg::gr::RenderStates& aRenderStates) const {
    hg::gr::Sprite spr{&(_renderTexture->getTexture())};

    spr.setOrigin(_renderTexture->getSize().x / 2.f, _renderTexture->getSize().y / 2.f);
    spr.setPosition(*_viewCenter);
    spr.setScale(_textureDrawScale);

    aCanvas.draw(spr, aRenderStates);
}

std::optional<hg::gr::Color> LightingRenderer::getColorAt(PositionInWorld aPosition) const {
    const auto pixelPos = _renderTexture->mapCoordsToPixel(*aPosition + *_viewCenterOffset, 0);

    if (pixelPos.x < 0 || pixelPos.x >= _textureSize.x || pixelPos.y < 0 ||
        pixelPos.y >= _textureSize.y) //
    {
        return std::nullopt;
    }

    // Note: The calculation is as such because the pixel data we get from OpenGL is flipped vertically.
    const auto* p =
        _textureRamBuffer.data() + ((_textureSize.y - 1 - pixelPos.y) * _textureSize.x + pixelPos.x) * 4;

    return hg::gr::Color{p[0], p[1], p[2], 255};
}

} // namespace gridgoblin
} // namespace jbatnozic
