#pragma once

#include "Engine.hpp"

#include <Hobgoblin/ChipmunkPhysics.hpp>
#include <Hobgoblin/Alvin.hpp>
#include <Hobgoblin/Math/Vector.hpp>

#include <string>

class EnvironmentManagerInterface : public spe::ContextComponent {
public:
    enum class Mode {
        UNINITIALIZED,
        HEADLESS_HOST,
        CLIENT,
    };

    ~EnvironmentManagerInterface() override = default;

    virtual void setToHeadlessHostMode() = 0;
    virtual void setToClientMode() = 0;
    virtual Mode getMode() const = 0;

    virtual hg::alvin::Space& getSpace() = 0;

    virtual hg::math::Vector2pz getGridSize() const = 0;
    virtual hg::math::Vector2pz getScalesGridPosition() const = 0;

    virtual void generateLoot() = 0;

private:
    SPEMPE_CTXCOMP_TAG("EnvironmentManagerInterface");
};

using MEnvironment = EnvironmentManagerInterface;
