// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#ifndef SPEMPE_MANAGERS_WINDOW_MANAGER_DEFAULT_HPP
#define SPEMPE_MANAGERS_WINDOW_MANAGER_DEFAULT_HPP

#include <Hobgoblin/Graphics.hpp>
#include <Hobgoblin/RmlUi.hpp>
#include <Hobgoblin/Utility/Time_utils.hpp>

#include <Hobgoblin/Graphics.hpp>
#include <Hobgoblin/Window.hpp>

#include <SPeMPE/GameObjectFramework/Game_object_bases.hpp>
#include <SPeMPE/Managers/Window_manager_interface.hpp>
#include <SPeMPE/Utility/Timing.hpp>
#include <SPeMPE/Utility/Window_frame_input_view.hpp>
#include <SPeMPE/Utility/Window_input_tracker.hpp>

#include <chrono>
#include <optional>

namespace jbatnozic {
namespace spempe {

class DefaultWindowManager
    : public WindowManagerInterface
    , public NonstateObject {
public:
    DefaultWindowManager(hg::QAO_RuntimeRef aRuntimeRef, int aExecutionPriority);

    ///////////////////////////////////////////////////////////////////////////
    // CONFIGURATION                                                         //
    ///////////////////////////////////////////////////////////////////////////

    void setToHeadlessMode(const TimingConfig& aTimingConfig) override;

    void setToNormalMode(const WindowConfig&            aWindowConfig,
                         const MainRenderTextureConfig& aMainRenderTextureConfig,
                         const TimingConfig&            aTimingConfig) override;

    ///////////////////////////////////////////////////////////////////////////
    // WINDOW MANAGEMENT                                                     //
    ///////////////////////////////////////////////////////////////////////////

    hg::math::Vector2pz getWindowSize() const override;

    void setStopIfCloseClicked(bool aStop) override;

    ///////////////////////////////////////////////////////////////////////////
    // GRAPHICS & DRAWING                                                    //
    ///////////////////////////////////////////////////////////////////////////

    // sf::RenderTexture& getMainRenderTexture();

    hg::gr::Canvas& getCanvas() override;

    void setMainRenderTextureDrawPosition(DrawPosition aDrawPosition) override;

    ///////////////////////////////////////////////////////////////////////////
    // VIEWS                                                                 //
    ///////////////////////////////////////////////////////////////////////////

    hg::gr::ViewController& getViewController() override;

    const hg::gr::ViewController& getViewController() const override;

    void setViewCount(hg::PZInteger aViewCount) override;

    hg::PZInteger getViewCount() const override;

    hg::gr::View& getView(hg::PZInteger aViewIndex = 0) override;

    const hg::gr::View& getView(hg::PZInteger aViewIndex = 0) const override;

    hg::math::Vector2f mapPixelToCoords(const hg::math::Vector2i& aPoint,
                                        const hg::gr::View&       aView) const override;

    hg::math::Vector2f mapPixelToCoords(const hg::math::Vector2i& aPoint,
                                        hg::PZInteger             aViewIdx = 0) const override;

    hg::math::Vector2i mapCoordsToPixel(const hg::math::Vector2f& aPoint,
                                        const hg::gr::View&       aView) const override;

    hg::math::Vector2i mapCoordsToPixel(const hg::math::Vector2f& aPoint,
                                        hg::PZInteger             aViewIdx = 0) const override;

    ///////////////////////////////////////////////////////////////////////////
    // GUI                                                                   //
    ///////////////////////////////////////////////////////////////////////////

    Rml::Context& getGUIContext() override;

    void resetGUIContext() override;

    ///////////////////////////////////////////////////////////////////////////
    // INPUT                                                                 //
    ///////////////////////////////////////////////////////////////////////////

    WindowFrameInputView getInput() const override;

private:
    struct MainRenderTexturePositioningData {
        sf::Vector2f position;
        sf::Vector2f origin;
        sf::Vector2f scale;
    };

    // Configuration:
    bool         _headless;
    TimingConfig _timingConfig;

    // Window management:
    std::optional<hg::gr::RenderWindow> _window;
    std::optional<hg::gr::DrawBatcher>  _windowDrawBatcher;
    hg::util::Stopwatch                 _timeSinceLastDisplay;
    bool                                _stopIfCloseClicked = false;

    std::vector<hg::win::Event> _events;

    // Main render texture:
    std::optional<hg::gr::RenderTexture> _mainRenderTexture;
    std::optional<hg::gr::DrawBatcher>   _mainRenderTextureDrawBatcher;

    DrawPosition _mainRenderTextureDrawPos = DrawPosition::Fit;

    // GUI:
    std::unique_ptr<hg::rml::HobgoblinBackend::BackendLifecycleGuard> _rmlUiBackendLifecycleGuard;
    std::optional<hg::rml::ContextDriver>                             _rmlUiContextDriver;

    // Input:
    detail::WindowInputTracker _inputTracker;

    bool _frameReady = false;

    void _eventPreUpdate() override;
    void _eventPreDraw() override;
    void _eventDraw2() override;
    void _eventDrawGUI() override;
    void _eventDisplay() override;

    MainRenderTexturePositioningData _getMainRenderTexturePositioningData() const;

    void _drawMainRenderTexture();
    void _displayWindowAndPollEvents();
    void _sleepUntilNextStep();

    FloatSeconds _getTickDeltaTime() const;
    FloatSeconds _getFrameDeltaTime() const;

    sf::Vector2f _getViewRelativeMousePos(hobgoblin::PZInteger aViewIndex) const;
    sf::Vector2i _getWindowRelativeMousePos() const;
};

} // namespace spempe
} // namespace jbatnozic

#endif // !SPEMPE_MANAGERS_WINDOW_MANAGER_DEFAULT_HPP
