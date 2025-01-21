// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#pragma once

#include <GridGoblin/Spatial/Position_in_world.hpp>

#include <Hobgoblin/Graphics/Canvas.hpp>
#include <Hobgoblin/Graphics/Color.hpp>
#include <Hobgoblin/Graphics/Render_texture.hpp>

#include <memory>
#include <optional>

namespace jbatnozic {
namespace gridgoblin {

namespace hg = ::jbatnozic::hobgoblin;

// Forward-declare
class World;

class LightingRenderer {
public:
    LightingRenderer(const World& aWorld, hg::math::Vector2pz aTextureSize);

    void start(const hg::gr::View& aView);

    void render(hg::gr::Canvas& aCanvas) const;

    std::optional<hg::gr::Color> getColorAt(PositionInWorld aPosition) const;

private:
    const World& _world;

    PositionInWorld _viewCenter;
    float _drawScaleX;
    float _drawScaleY;

    std::unique_ptr<hg::gr::RenderTexture> _renderTexture;
};

} // namespace gridgoblin
} // namespace jbatnozic
