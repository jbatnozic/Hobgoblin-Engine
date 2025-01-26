#pragma once

#include "Engine.hpp"

#include <Hobgoblin/Graphics.hpp>

#include <filesystem>

enum SpriteIds {
    // Kraken
    SPR_KRAKEN_BODY,
    SPR_KRAKEN_FINS,
    // Mountain
    SPR_TERRAIN,
    // Miscellaneous
    SPR_POWER,
    SPR_BACKGROUND,
    SPR_SPONGE,
    SPR_PEARL
};

inline void LoadSprites(hg::gr::SpriteLoader& aSpriteLoader) {
    std::filesystem::path root = std::filesystem::current_path();
    for (int i = 0; i < 10; i += 1) {
        if (std::filesystem::exists(root / "Assets")) {
            break;
        }
        root = root.parent_path();
    }

    const std::filesystem::path basePath     = root / "Assets/Sprites";
    const std::filesystem::path krakenPath   = root / "Assets/Sprites/Kraken";
    const std::filesystem::path mountainPath = root / "Assets/Sprites/Terrain";

    float occupancy = 0.f;

    aSpriteLoader
        .startTexture(2048, 2048)
        // Kraken
        ->addSubsprite(SPR_KRAKEN_BODY, krakenPath / "kraken-eat-1.png")
        ->addSubsprite(SPR_KRAKEN_BODY, krakenPath / "kraken-eat-2.png")
        ->addSubsprite(SPR_KRAKEN_BODY, krakenPath / "kraken-eat-3.png")
        ->addSubsprite(SPR_KRAKEN_BODY, krakenPath / "kraken-eat-4.png")
        ->addSubsprite(SPR_KRAKEN_FINS, krakenPath / "kraken-fin-1.png")
        ->addSubsprite(SPR_KRAKEN_FINS, krakenPath / "kraken-fin-2.png")
        ->addSubsprite(SPR_KRAKEN_FINS, krakenPath / "kraken-fin-3.png")
        ->addSubsprite(SPR_KRAKEN_FINS, krakenPath / "kraken-fin-4.png")
        ->addSubsprite(SPR_KRAKEN_FINS, krakenPath / "kraken-fin-5.png")
        ->addSubsprite(SPR_KRAKEN_FINS, krakenPath / "kraken-fin-6.png")
        // Map pieces
        ->addSubsprite(SPR_TERRAIN, mountainPath / "0.png")
        ->addSubsprite(SPR_TERRAIN, mountainPath / "1.png")
        ->addSubsprite(SPR_TERRAIN, mountainPath / "2.png")
        ->addSubsprite(SPR_TERRAIN, mountainPath / "3.png")
        ->addSubsprite(SPR_TERRAIN, mountainPath / "4.png")
        ->addSubsprite(SPR_TERRAIN, mountainPath / "5.png")
        ->addSubsprite(SPR_TERRAIN, mountainPath / "6.png")
        ->addSubsprite(SPR_TERRAIN, mountainPath / "7.png")
        ->addSubsprite(SPR_TERRAIN, mountainPath / "8.png")
        ->addSubsprite(SPR_TERRAIN, mountainPath / "9.png")
        ->addSubsprite(SPR_TERRAIN, mountainPath / "10.png")
        ->addSubsprite(SPR_TERRAIN, mountainPath / "11.png")
        // Miscellaneous
        ->addSubsprite(SPR_POWER, basePath / "power.png")
        ->addSubsprite(SPR_SPONGE, basePath / "sponge.png")
        ->addSubsprite(SPR_PEARL, basePath / "pearl.png")
        // Finalize
        ->finalize(hg::gr::TexturePackingHeuristic::BestAreaFit, &occupancy);

    HG_LOG_INFO(LOG_ID, "Game sprites loaded successfully (texture occupancy {}%).", occupancy * 100.f);


    aSpriteLoader
        .startTexture(4096, 4094)
        // Miscellaneous
        ->addSubsprite(SPR_BACKGROUND, basePath / "background.png")
        // Finalize
        ->finalize(hg::gr::TexturePackingHeuristic::BestAreaFit, &occupancy);

    HG_LOG_INFO(LOG_ID,
                "Background sprites loaded successfully (texture occupancy {}%).",
                occupancy * 100.f);
}
