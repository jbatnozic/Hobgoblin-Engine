// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#pragma once

#include <GridGoblin/Spatial/Position_in_world.hpp>

#include <Hobgoblin/Graphics/Canvas.hpp>
#include <Hobgoblin/Graphics/Color.hpp>
#include <Hobgoblin/Graphics/Render_texture.hpp>
#include <Hobgoblin/Graphics/Sprite_loader.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace jbatnozic {
namespace gridgoblin {

namespace hg = ::jbatnozic::hobgoblin;

// Forward-declare
class World;

class LightingRenderer {
public:
    enum class Purpose {
        NORMAL_RENDERING,
        DIMETRIC_RENDERING
    };

    LightingRenderer(const World&                aWorld,
                     const hg::gr::SpriteLoader& aSpriteLoader,
                     hg::math::Vector2pz         aTextureSize,
                     Purpose                     aPurpose = Purpose::NORMAL_RENDERING);

    ~LightingRenderer();

    void prepareToRender(PositionInWorld aViewCenter, hg::math::Vector2f aViewSize);

    void render(hg::gr::Canvas& aCanvas, const hg::gr::RenderStates& aRenderStates = {}) const;

    std::optional<hg::gr::Color> getColorAt(PositionInWorld aPosition) const;

private:
    const World& _world;
    const hg::gr::SpriteLoader& _spriteLoader;

    std::unique_ptr<hg::gr::RenderTexture> _renderTexture;

    PositionInWorld    _viewCenter;
    PositionInWorld    _viewCenterOffset; //!< Offset since last render cycle
    hg::math::Vector2f _textureDrawScale;
    hg::math::Vector2pz _textureSize;

    Purpose _purpose;

    //! Buffer which will hold the contents of _renderTexture in RAM
    //! (in RGBA format), for fast access for purposes of getColorAt().
    std::vector<std::uint8_t> _textureRamBuffer;

    //! Names of OpenGL Pixel Buffer Objects.
    std::array<unsigned int, 2> _pboNames;

    //! Counter used to know which PBO to write to and which PBO to read from.
    //! - In even-numbered steps, we start writing to pbo[0] and read from pbo[1],
    //! - In odd-numbered steps, we start writing to pbo[1] and read from pbo[0].
    unsigned int _stepCounter = -1;
};

} // namespace gridgoblin
} // namespace jbatnozic
