// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#pragma once

#include <GridGoblin/Model/Light.hpp>
#include <GridGoblin/Model/Light_id.hpp>
#include <GridGoblin/Rendering/Dimetric_transform.hpp>
#include <GridGoblin/Rendering/Visibility_calculator.hpp>

#include <Hobgoblin/Graphics/Canvas.hpp>
#include <Hobgoblin/Graphics/Draw_batcher.hpp>
#include <Hobgoblin/Graphics/Render_texture.hpp>
#include <Hobgoblin/Math/Vector.hpp>

#include <GridGoblin/Spatial/Position_conversions.hpp>

#include <Hobgoblin/Graphics/Sprite.hpp>

#include <memory>

namespace jbatnozic {
namespace gridgoblin {

// Forward-declare
class World;

namespace detail {

namespace hg = jbatnozic::hobgoblin;

class LightExt : public Light {
public:
    LightExt(PositionInWorld     aCenter,
             float               aRadius,
             hg::gr::Color       aColor,
             SpriteId            aSpriteId,
             hg::math::Vector2pz aTextureSize,
             const World&        aWorld,
             LightId             aLightId)
        : Light{aCenter, aRadius, aColor, aSpriteId}
        , mutableExtensionData{aTextureSize, aWorld, aLightId} {}

    class ExtensionData {
    public:
        ExtensionData(hg::math::Vector2pz aTextureSize, const World& aWorld, LightId aLightId);

        LightId getId() const;

        void render(hg::gr::Canvas&             aCanvas,
                    const hg::gr::SpriteLoader& aSpriteLoader,
                    const hg::gr::RenderStates& aRenderStates = {}) const;

        static const LightExt* getLightAddress(const ExtensionData* aExtensionDataAddress);

    private:
        const World& _world;

        LightId _id;

        mutable std::unique_ptr<hg::gr::RenderTexture> _renderTexture;
        mutable VisibilityCalculator                   _visCalc;
    };

    mutable ExtensionData mutableExtensionData;
};

} // namespace detail
} // namespace gridgoblin
} // namespace jbatnozic
