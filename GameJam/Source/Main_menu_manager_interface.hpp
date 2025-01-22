#pragma once

#include "Engine.hpp"

class MainMenuManagerInterface : public spe::ContextComponent {
public:
    virtual void setVisible(bool aVisible) = 0;

private:
    SPEMPE_CTXCOMP_TAG("MainMenuManagerInterface");
};

