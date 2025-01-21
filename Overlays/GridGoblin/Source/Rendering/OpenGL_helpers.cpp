// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#include "OpenGL_helpers.hpp"

#include <Hobgoblin/Logging.hpp>

namespace jbatnozic {
namespace gridgoblin {
namespace opengl {
static constexpr auto LOG_ID = "GridGoblin";

void DualPBO_Init(DualPBONames& aPboNames, std::size_t aRamBufferSize) {
    OpenGLErrorContext ectx;
    for (auto& pboName : aPboNames) {
        pboName = 0;
        GLCALL(ectx, glGenBuffers(1, &pboName));
        GLCALL(ectx, glBindBuffer(GL_PIXEL_PACK_BUFFER, pboName));
        GLCALL(ectx, glBufferData(GL_PIXEL_PACK_BUFFER, aRamBufferSize, NULL, GL_STREAM_READ));
    }

    // Unbind PBOs for now:
    GLCALL(ectx, glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));

    // Handle errors:
    if (ectx.hasErrors()) {
        DualPBO_Destroy(aPboNames);
        THROW_ON_ERROR(ectx);
    }
}

void DualPBO_Destroy(DualPBONames& aPboNames) {
    for (auto& pboName : aPboNames) {
        // OpenGL error handling deliberately left out:
        // "glDeleteBuffers silently ignores 0's and names that
        // do not correspond to existing buffer objects."
        glDeleteBuffers(1, &pboName);
        pboName = 0;
    }
}

void DualPBO_StartTransfer(const DualPBONames& aPboNames,
                           unsigned int        aWhich,
                           unsigned int        aTextureName) {
    OpenGLErrorContext ectx;

    GLCALL(ectx, glBindBuffer(GL_PIXEL_PACK_BUFFER, aPboNames[aWhich]));
    GLCALL(ectx, glBindTexture(GL_TEXTURE_2D, aTextureName));

    // Starts the asychronous transfer of texture pixel data into the PBO.
    // This transfer will happen whenever is suitable for the GPU, as to not
    // stall the graphics pipeline.
    // clang-format off
    GLCALL(ectx, glGetTexImage(/* target */      GL_TEXTURE_2D,
                               /* level */       0,
                               /* pixelformat */ GL_RGBA,
                               /* pixeltype */   GL_UNSIGNED_BYTE,
                               /* offset */      nullptr));
    // clang-format on

    GLCALL(ectx, glBindTexture(GL_TEXTURE_2D, 0));
    GLCALL(ectx, glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));

    THROW_ON_ERROR(ectx);
}

void DualPBO_LoadIntoRam(const DualPBONames& aPboNames,
                         unsigned int        aWhich,
                         void*               aRamBufferAddress,
                         std::size_t         aRamBufferSize) {
    OpenGLErrorContext ectx;

    const void* p = nullptr;
    GLCALL(ectx, glBindBuffer(GL_PIXEL_PACK_BUFFER, aPboNames[aWhich]));
    // Maps the memory of the target PBO into memory addressable by the CPU.
    // If a transfer into this PBO previously initiated by 'glGetTexImage' is
    // not yet complete, 'glMapBuffer' will wait until it is.
    // Note: Where the PBO holds pixel data and whether 'glMapBuffer' involves
    // copying the data is up to the specific OpenGL implementation.
    GLCALL(ectx, { p = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY); });

    if (p != nullptr) {
        std::memcpy(aRamBufferAddress, p, aRamBufferSize);
    } else {
        HG_LOG_WARN(LOG_ID, "PBO mapping into RAM returned NULL.");
        std::memset(aRamBufferAddress, 0x00, aRamBufferSize);
    }

    GLCALL(ectx, glUnmapBuffer(GL_PIXEL_PACK_BUFFER));
    GLCALL(ectx, glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));

    THROW_ON_ERROR(ectx);
}

} // namespace opengl
} // namespace gridgoblin
} // namespace jbatnozic