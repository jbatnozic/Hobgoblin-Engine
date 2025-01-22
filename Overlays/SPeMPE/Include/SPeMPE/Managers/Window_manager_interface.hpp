// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#ifndef SPEMPE_MANAGERS_WINDOW_MANAGER_INTERFACE_HPP
#define SPEMPE_MANAGERS_WINDOW_MANAGER_INTERFACE_HPP

#include <Hobgoblin/Graphics.hpp>
#include <Hobgoblin/Math/Vector.hpp>
#include <Hobgoblin/RmlUi.hpp>
#include <Hobgoblin/Window.hpp>

#include <SPeMPE/GameContext/Context_components.hpp>
#include <SPeMPE/Utility/Timing.hpp>
#include <SPeMPE/Utility/Window_frame_input_view.hpp>
#include <SPeMPE/Utility/Window_input_tracker.hpp>

#include <optional>

namespace jbatnozic {
namespace spempe {

constexpr bool PREVENT_BUSY_WAIT_ON  = true;
constexpr bool PREVENT_BUSY_WAIT_OFF = false;

constexpr bool VSYNC_ON  = true;
constexpr bool VSYNC_OFF = false;

class WindowManagerInterface : public ContextComponent {
public:
    virtual ~WindowManagerInterface() = default;

    ///////////////////////////////////////////////////////////////////////////
    // CONFIGURATION                                                         //
    ///////////////////////////////////////////////////////////////////////////

    enum class Mode {
        //! In this mode no window is actually opened.
        //! (This mode exists to make writing headless servers and
        //!  their clients more uniform.)
        Headless,

        //! The 'usual' mode in which a single window is opened and
        //! operated by the WindowManager to display the game.
        Normal
    };

    struct WindowConfig {
        WindowConfig(
            const hg::win::VideoMode&       aVideoMode,
            const std::string&              aTitle,
            hg::win::WindowStyle            aStyle                 = hg::win::WindowStyle::Default,
            const hg::win::ContextSettings& aOpenGlContextSettings = hg::win::ContextSettings{});

        hg::win::VideoMode       videoMode;
        std::string              title;
        hg::win::WindowStyle     style;
        hg::win::ContextSettings openGlContextSettings;
    };

    struct MainRenderTextureConfig {
        MainRenderTextureConfig(
            const hg::math::Vector2pz&      aSize,
            const bool                      aSmooth                = true,
            const hg::win::ContextSettings& aOpenGlContextSettings = hg::win::ContextSettings{});

        hg::math::Vector2pz      size;
        bool                     smooth;
        hg::win::ContextSettings openGlContextSettings;
    };

    struct TimingConfig {
        //! Constructs the configuration with the 'usual' framerate limiter.
        //!
        //! \param aFramerateLimit The maximum number of times per second that the window can
        //!                        be displayed (refreshed by the game). This is decoupled from the
        //!                        tick rate of the game and won't affect simulation speed.
        //! \param aBusyWaitPreventionEnabled When set to `true` (PREVENT_BUSY_WAIT_ON), the
        //!                                   WindowManager will sleep between GameContext/QAO
        //!                                   iterations to preserve system resources.
        //! \param aVerticalSyncEnabled When set to `true` (VSYNC_ON), the window will use
        //!                             vertical synchronization to prevent screen tearing. It is
        //!                             recommended to only turn this on in fullscreen mode (real
        //!                             or windowed fullscreen).
        //!
        //! \note This is the recommended constructor to use when running on Windows.
        TimingConfig(FrameRate aFramerateLimit,
                     bool      aBusyWaitPreventionEnabled = PREVENT_BUSY_WAIT_ON,
                     bool      aVerticalSyncEnabled       = VSYNC_OFF);

        //! Constructs the configuration with the low level framerate limiter, which will block
        //! the application if the window is being displayed too quickly.
        //!
        //! \param aLowLevelFramerateLimiter The maximum number of times per second that the window can
        //!                                  be displayed (refreshed by the game). If this is set lower
        //!                                  than the tick rate of the game, it could slow down the
        //!                                  simulation. A value of 0 will leave the framerate
        //!                                  unlimited (which is not recommended).
        //! \param aBusyWaitPreventionEnabled When set to `true` (PREVENT_BUSY_WAIT_ON), the
        //!                                   WindowManager will sleep between GameContext/QAO
        //!                                   iterations to preserve system resources.
        //! \param aVerticalSyncEnabled When set to `true` (VSYNC_ON), the window will use
        //!                             vertical synchronization to prevent screen tearing. It is
        //!                             recommended to only turn this on in fullscreen mode (real
        //!                             or windowed fullscreen).
        //!
        //! \note When using this blocking low-level framerate limiter, it's recommended to leave
        //!       both busy wait prevention and vsync off, as they can interfere with each other.
        //!
        //! \note This is the recommended constructor to use when running on Mac.
        TimingConfig(hg::PZInteger aLowLevelFramerateLimiter  = 60,
                     bool          aBusyWaitPreventionEnabled = PREVENT_BUSY_WAIT_OFF,
                     bool          aVerticalSyncEnabled       = VSYNC_OFF);

        std::optional<FrameRate> framerateLimit;
        hg::PZInteger            lowLevelFramerateLimiter;
        bool                     busyWaitPreventionEnabled;
        bool                     verticalSyncEnabled;
    };

    virtual void setToHeadlessMode(const TimingConfig& aTimingConfig) = 0;

    virtual void setToNormalMode(const WindowConfig&            aWindowConfig,
                                 const MainRenderTextureConfig& aMainRenderTextureConfig,
                                 const TimingConfig&            aTimingConfig) = 0;

    ///////////////////////////////////////////////////////////////////////////
    // WINDOW MANAGEMENT                                                     //
    ///////////////////////////////////////////////////////////////////////////

    //_window.getSettings();
    //_window.getPosition();
    virtual hg::math::Vector2pz getWindowSize() const = 0;
    //_window.hasFocus();
    //_window.mapCoordsToPixel();
    //_window.mapPixelToCoords();
    //_window.setFramerateLimit();
    //_window.setIcon();
    //_window.setPosition();
    //_window.setVerticalSyncEnabled();
    //_window.setTitle();
    //_window.setSize();
    //_window.setMouseCursor();
    //_window.setMouseCursorGrabbed();
    //_window.setMouseCursorVisible();
    // framerate limiter

    virtual void setStopIfCloseClicked(bool aStop) = 0;

    ///////////////////////////////////////////////////////////////////////////
    // GRAPHICS & DRAWING                                                    //
    ///////////////////////////////////////////////////////////////////////////

    // sf::RenderTexture& getMainRenderTexture();

    virtual hg::gr::Canvas& getCanvas() = 0;

    enum class DrawPosition {
        None,    //! Not drawn at all.
        Fill,    //! Scaled (keeping aspect ratio) and centred so that the whole window is filled.
        Fit,     //! Scaled (keeping aspect ratio) and centred so that the whole texture is visible.
        Centre,  //! Kept at original size and just centred in the window.
        Stretch, //! Stretched (or compressed) to the window's exact size.
        TopLeft, //! Kept at original size and top left corner aligned with window's top left corner.
    };

    //! Default draw position is Fit.
    virtual void setMainRenderTextureDrawPosition(DrawPosition aDrawPosition) = 0;

    ///////////////////////////////////////////////////////////////////////////
    // VIEWS                                                                 //
    ///////////////////////////////////////////////////////////////////////////

    //! Returns the view controller for the main render texture.
    //!
    //! \warning unlike `getCanvas()`, which can return a different canvas depending on whether
    //!          it's called during a DRAW_GUI event or not, this method always returns the view
    //!          controller of the main render texture, so you can only ever set/get views into
    //!          the game world (GUI doesn't support views anyway).
    virtual hg::gr::ViewController& getViewController() = 0;

    //! Returns the view controller for the main render texture.
    //!
    //! \warning unlike `getCanvas()`, which can return a different canvas depending on whether
    //!          it's called during a DRAW_GUI event or not, this method always returns the view
    //!          controller of the main render texture, so you can only ever set/get views into
    //!          the game world (GUI doesn't support views anyway).
    virtual const hg::gr::ViewController& getViewController() const = 0;

    //! Equivalent to `getViewController().setViewCount()`.
    virtual void setViewCount(hg::PZInteger aViewCount) = 0;

    //! Equivalent to `getViewController().getViewCount()`.
    virtual hg::PZInteger getViewCount() const = 0;

    //! Equivalent to `getViewController().getView()`.
    virtual hg::gr::View& getView(hg::PZInteger aViewIndex = 0) = 0;

    //! Equivalent to `getViewController().getView()`.
    virtual const hg::gr::View& getView(hg::PZInteger aViewIndex = 0) const = 0;

    //! Equivalent to `getViewController().mapPixelToCoords()`.
    virtual hg::math::Vector2f mapPixelToCoords(const hg::math::Vector2i& aPoint,
                                                const hg::gr::View&       aView) const = 0;

    //! Equivalent to `getViewController().mapPixelToCoords()`.
    virtual hg::math::Vector2f mapPixelToCoords(const hg::math::Vector2i& aPoint,
                                                hg::PZInteger             aViewIdx = 0) const = 0;

    //! Equivalent to `getViewController().mapCoordsToPixel()`.
    virtual hg::math::Vector2i mapCoordsToPixel(const hg::math::Vector2f& aPoint,
                                                const hg::gr::View&       aView) const = 0;

    //! Equivalent to `getViewController().mapCoordsToPixel()`.
    virtual hg::math::Vector2i mapCoordsToPixel(const hg::math::Vector2f& aPoint,
                                                hg::PZInteger             aViewIdx = 0) const = 0;

    ///////////////////////////////////////////////////////////////////////////
    // GUI                                                                   //
    ///////////////////////////////////////////////////////////////////////////

    virtual Rml::Context& getGUIContext() = 0;

    virtual void resetGUIContext() = 0;

    ///////////////////////////////////////////////////////////////////////////
    // KEYBOARD & MOUSE INPUT                                                //
    ///////////////////////////////////////////////////////////////////////////

    virtual WindowFrameInputView getInput() const = 0;

private:
    SPEMPE_CTXCOMP_TAG("jbatnozic::spempe::WindowManagerInterface");
};

inline WindowManagerInterface::WindowConfig::WindowConfig(
    const hg::win::VideoMode&       aVideoMode,
    const std::string&              aTitle,
    hg::win::WindowStyle            aStyle,
    const hg::win::ContextSettings& aOpenGlContextSettings)
    : videoMode{aVideoMode}
    , title{aTitle}
    , style{aStyle}
    , openGlContextSettings{aOpenGlContextSettings} {}

inline WindowManagerInterface::MainRenderTextureConfig::MainRenderTextureConfig(
    const hg::math::Vector2pz&      aSize,
    const bool                      aSmooth,
    const hg::win::ContextSettings& aOpenGlContextSettings)
    : size{aSize}
    , smooth{aSmooth}
    , openGlContextSettings{aOpenGlContextSettings} {}

inline WindowManagerInterface::TimingConfig::TimingConfig(FrameRate aFrameRateLimit,
                                                          bool      aBusyWaitPreventionEnabled,
                                                          bool      aVerticalSyncEnabled)
    : framerateLimit{aFrameRateLimit}
    , lowLevelFramerateLimiter{0}
    , busyWaitPreventionEnabled{aBusyWaitPreventionEnabled}
    , verticalSyncEnabled{aVerticalSyncEnabled} {}

inline WindowManagerInterface::TimingConfig::TimingConfig(hg::PZInteger aLowLevelFramerateLimiter,
                                                          bool          aBusyWaitPreventionEnabled,
                                                          bool          aVerticalSyncEnabled)
    : framerateLimit{std::nullopt}
    , lowLevelFramerateLimiter{aLowLevelFramerateLimiter}
    , busyWaitPreventionEnabled{aBusyWaitPreventionEnabled}
    , verticalSyncEnabled{aVerticalSyncEnabled} {}

} // namespace spempe
} // namespace jbatnozic

#endif // !SPEMPE_MANAGERS_WINDOW_MANAGER_INTERFACE_HPP
