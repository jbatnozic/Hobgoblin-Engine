// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#include <GridGoblin/Private/Light_ext.hpp>

namespace jbatnozic {
namespace gridgoblin {
namespace detail {

namespace {
const VisibilityCalculatorConfig& GetVisCalcConfig() {
    static VisibilityCalculatorConfig config{.minRingsBeforeRaycasting =
                                                 VisibilityCalculatorConfig::NO_RAYCASTING,
                                             .rayCount = 0};
    return config;
}
} // namespace

LightExt::ExtensionData::ExtensionData(hg::math::Vector2pz aTextureSize,
                                       const World&        aWorld,
                                       LightId             aLightId)
    : _world{aWorld}
    , _visCalc{_world, GetVisCalcConfig()}
    , _id{aLightId} //
{
    _renderTexture = std::make_unique<hg::gr::RenderTexture>();
    _renderTexture->create(aTextureSize);
}

LightId LightExt::ExtensionData::getId() const {
    return _id;
}

void LightExt::ExtensionData::render(hg::gr::Canvas&             aCanvas,
                                     const hg::gr::SpriteLoader& aSpriteLoader,
                                     const hg::gr::RenderStates& aRenderStates) const {
    const auto* light = getLightAddress(this);

    const auto size = hg::math::Vector2f{light->getRadius() * 2.f, light->getRadius() * 2.f};

    _renderTexture->clear(hg::gr::COLOR_BLACK);
    _renderTexture->setView(hg::gr::View{*light->getCenter(), size});
    {
        auto lightSprite = aSpriteLoader.getBlueprint(light->getSpriteId()).spr();
        lightSprite.setColor(light->getColor());
        lightSprite.setPosition(*light->getCenter());
        lightSprite.setScale(size.x / lightSprite.getLocalBounds().w,
                             size.y / lightSprite.getLocalBounds().h);
        _renderTexture->draw(lightSprite);
    }

    _visCalc.calc(light->getCenter(), size, light->getCenter());
    _visCalc.render(*_renderTexture);
    _renderTexture->display();

    hg::gr::Sprite rtSprite{&_renderTexture->getTexture()};
    rtSprite.setOrigin({_renderTexture->getSize().x / 2.f, _renderTexture->getSize().y / 2.f});
    rtSprite.setScale({size.x / _renderTexture->getSize().x, size.y / _renderTexture->getSize().y});

    rtSprite.setPosition(*light->getCenter());
    aCanvas.draw(rtSprite, aRenderStates);
}

const LightExt* LightExt::ExtensionData::getLightAddress(const ExtensionData* aExtensionDataAddress) {
    static constexpr auto EXTENSION_OFFSET = offsetof(LightExt, mutableExtensionData);

    const char* extensionRawAddress = reinterpret_cast<const char*>(aExtensionDataAddress);
    const char* lightRawAddress     = extensionRawAddress - EXTENSION_OFFSET;

    return reinterpret_cast<const LightExt*>(lightRawAddress);
}

} // namespace detail
} // namespace gridgoblin
} // namespace jbatnozic
