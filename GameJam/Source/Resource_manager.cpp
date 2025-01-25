#include "Resource_manager.hpp"

#include "Sprite_manifest.hpp"

#include <Hobgoblin/HGExcept.hpp>

ResourceManager::ResourceManager(QAO_RuntimeRef aRuntimeRef, int aExecutionPriority)
    : NonstateObject{aRuntimeRef, SPEMPE_TYPEID_SELF, aExecutionPriority, "ResourceManager"} {}

ResourceManager::~ResourceManager() = default;

void ResourceManager::setToHostMode() {
    HG_VALIDATE_PRECONDITION(_mode == Mode::UNINITIALIZED);
    _mode = Mode::HOST;
}

void ResourceManager::setToClientMode() {
    HG_VALIDATE_PRECONDITION(_mode == Mode::UNINITIALIZED);
    _mode = Mode::CLIENT;

    LoadSprites(_spriteLoader);

    std::filesystem::path root = std::filesystem::current_path();
    for (int i = 0; i < 10; i += 1) {
        if (std::filesystem::exists(root / "Assets")) {
            break;
        }
        root = root.parent_path();
    }

    _underpantsShader.loadFromFile(root / "Assets/underpants_shader_vertex.txt",
                                   root / "Assets/underpants_shader_fragment.txt");
}

ResourceManager::Mode ResourceManager::getMode() const {
    return _mode;
}

const hg::gr::SpriteLoader& ResourceManager::getSpriteLoader() const {
    HG_HARD_ASSERT(_mode == Mode::CLIENT);
    return _spriteLoader;
}

hg::gr::Shader& ResourceManager::getUnderpantsShader() {
    HG_HARD_ASSERT(_mode == Mode::CLIENT);
    return _underpantsShader;
}
