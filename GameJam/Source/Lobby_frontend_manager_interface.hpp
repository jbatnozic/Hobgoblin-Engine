#pragma once

#include "Engine.hpp"

#include <string>

class LobbyFrontendManagerInterface : public spe::ContextComponent {
public:
    enum class Mode {
        UNINITIALIZED,
        HEADLESS_HOST,
        CLIENT,
    };

    ~LobbyFrontendManagerInterface() override = default;

    virtual void setToHeadlessHostMode() = 0;
    virtual void setToClientMode(const std::string& aName, const std::string& aUniqueId) = 0;
    virtual Mode getMode() const = 0;

    virtual void setVisible(bool aVisible) = 0;

private:
    SPEMPE_CTXCOMP_TAG("LobbyFrontendManagerInterface");
};
