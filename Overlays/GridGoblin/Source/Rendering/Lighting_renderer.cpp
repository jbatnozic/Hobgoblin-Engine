// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#include <GridGoblin/Rendering/Dimetric_transform.hpp>
#include <GridGoblin/Rendering/Lighting_renderer.hpp>
#include <GridGoblin/World/World.hpp>

#include <Hobgoblin/Graphics.hpp>

#include "../Detail_access.hpp"

#include <algorithm>

namespace jbatnozic {
namespace gridgoblin {

LightingRenderer::LightingRenderer(const World& aWorld, hg::math::Vector2pz aTextureSize)
    : _world{aWorld} {
    _renderTexture = std::make_unique<hg::gr::RenderTexture>();
    _renderTexture->create(aTextureSize);
}

void LightingRenderer::start(const hg::gr::View& aView) {
    _viewCenter = dimetric::ToPositionInWorld(PositionInView{aView.getCenter()});

    float squareSide;
    {
        const auto viewSize = aView.getSize();
        if (viewSize.x > viewSize.y) {
            squareSide = (viewSize.x + 2 * viewSize.y) / std::sqrt(2.f);
        } else {
            squareSide = (viewSize.y + 2 * viewSize.x) / std::sqrt(2.f);
        }

        _drawScaleX = squareSide / _renderTexture->getSize().x;
        _drawScaleY = squareSide / _renderTexture->getSize().y;
    }

    _renderTexture->clear(hg::gr::COLOR_BLACK);
    auto& view = _renderTexture->getView();
    view.setCenter(*_viewCenter);
    view.setSize({squareSide, squareSide});

    // Render dynamic lights
    {
        auto       iter = _world.dynamicLightsBegin();
        const auto end  = _world.dynamicLightsEnd();

        for (; iter != end; ++iter) {
            const auto& light = *iter;
            auto&       ext   = GetMutableExtensionData(light);

            ext.render(*_renderTexture); // TODO: blend mode: add
        }
    }

    _renderTexture->display();
}

void LightingRenderer::render(hg::gr::Canvas& aCanvas) const {
    hg::gr::Sprite spr{&(_renderTexture->getTexture())};

    spr.setOrigin(_renderTexture->getSize().x / 2.f, _renderTexture->getSize().y / 2.f);
    spr.setPosition(*_viewCenter);
    spr.setScale({_drawScaleX, _drawScaleY});

    aCanvas.draw(spr,
                 hg::gr::RenderStates{nullptr, nullptr, DIMETRIC_TRANSFORM, hg::gr::BLEND_MULTIPLY});
}

std::optional<hg::gr::Color> LightingRenderer::getColorAt(PositionInWorld aPosition) const {}

} // namespace gridgoblin
} // namespace jbatnozic
