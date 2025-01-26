#pragma once

#include "Engine.hpp"
#include "Game_stage.hpp"

#include <Hobgoblin/Alvin.hpp>

//! TODO(add description)
class MainGameplayManagerInterface : public spe::ContextComponent {
public:
    enum class Mode {
        UNINITIALIZED,
        HOST,
        CLIENT,
    };

    ~MainGameplayManagerInterface() override = default;

    //! \param aPlayerCount number of players in the game, including player 0 (the host)
    virtual void setToHostMode(hg::PZInteger aPlayerCount) = 0;
    virtual void setToClientMode()                         = 0;
    virtual Mode getMode() const                           = 0;

    virtual void startGame() = 0;

    virtual void addAnnouncement(const std::string& aString, hg::gr::Color aColor) = 0;

    virtual int getCurrentGameStage() const = 0;

    virtual std::optional<cpVect> getPositionOfClient(int aClientIndex) const             = 0;
    virtual void                  setPositionOfClient(int aClientIndex, cpVect aPosition) = 0;

    virtual void depositPearl() = 0;

private:
    SPEMPE_CTXCOMP_TAG("MainGameplayManagerInterface");
};

using MMainGameplay = MainGameplayManagerInterface;
