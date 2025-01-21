// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#pragma once

#include <Hobgoblin/Format.hpp>
#include <Hobgoblin/HGExcept.hpp>
#include <Hobgoblin/Logging.hpp>

#include <GL/glew.h>

#include <array>
#include <string>
#include <type_traits>

namespace jbatnozic {
namespace gridgoblin {
namespace opengl {

///////////////////////////////////////////////////////////////////////////
// MARK: ERROR HANDLING                                                  //
///////////////////////////////////////////////////////////////////////////

struct OpenGLErrorContext {
    void addError(int aErrorCode, const char* aCodeString) {
        messageCount += 1;
        errorMessages +=
            fmt::format("{}. Code '{}' resulted in {:#X}; ", messageCount, aCodeString, aErrorCode);
    }

    bool hasErrors() const {
        return messageCount > 0;
    }

    std::string errorMessages;
    int         messageCount = 0;
};

#define THROW_ON_ERROR(_error_context_)                                 \
    do {                                                                \
        if ((_error_context_).hasErrors()) {                            \
            HG_THROW_TRACED(::jbatnozic::hobgoblin::TracedRuntimeError, \
                            0,                                          \
                            "There were {} OpenGL errors: {}",          \
                            (_error_context_).messageCount,             \
                            (_error_context_).errorMessages);           \
        }                                                               \
    } while (false)

inline void ClearOpenGLErrors() {
    while (glGetError() != GL_NO_ERROR) {}
}

inline void StoreOpenGLErrors(OpenGLErrorContext& aErrorCtx, const char* aCodeString) {
    while (auto error = glGetError()) {
        aErrorCtx.addError(error, aCodeString);
    }
}

#define GLCALL(_error_context_, _code_)              \
    do {                                             \
        ClearOpenGLErrors();                         \
        { _code_; }                                  \
        StoreOpenGLErrors(_error_context_, #_code_); \
    } while (false)

///////////////////////////////////////////////////////////////////////////
// MARK: PBOS                                                            //
///////////////////////////////////////////////////////////////////////////

using DualPBONames = std::array<unsigned int, 2>;

static_assert(std::is_same_v<DualPBONames::value_type, GLuint>,
              "DualPBONames::value_type must be the same as GLuint!");

void DualPBO_Init(DualPBONames& aPboNames, std::size_t aRamBufferSize);

void DualPBO_Destroy(DualPBONames& aPboNames);

void DualPBO_StartTransfer(const DualPBONames& aPboNames,
                           unsigned int        aWhich,
                           unsigned int        aTextureName);

void DualPBO_LoadIntoRam(const DualPBONames& aPboNames,
                         unsigned int        aWhich,
                         void*               aRamBufferAddress,
                         std::size_t         aRamBufferSize);

} // namespace opengl
} // namespace gridgoblin
} // namespace jbatnozic
