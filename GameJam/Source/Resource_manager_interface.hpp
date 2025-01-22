#pragma once

#include "Engine.hpp"

#include <Hobgoblin/Graphics.hpp>

#include <string>

class ResourceManagerInterface : public spe::ContextComponent {
public:
    enum class Mode {
        UNINITIALIZED,
        HOST,
        CLIENT,
    };

    ~ResourceManagerInterface() override = default;

    virtual void setToHostMode() = 0;
    virtual void setToClientMode() = 0;
    virtual Mode getMode() const = 0;

    virtual const hg::gr::SpriteLoader& getSpriteLoader() const = 0;

    virtual hg::gr::Shader& getUnderpantsShader() = 0;

private:
    SPEMPE_CTXCOMP_TAG("ResourceManagerInterface");
};

using MResource = ResourceManagerInterface;
