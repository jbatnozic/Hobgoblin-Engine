
#include <Hobgoblin/Logging.hpp>
#include <Hobgoblin/HGExcept.hpp>

#include <Hobgoblin/Utility/Randomization.hpp>

#include "Context_factory.hpp"
#include "Engine.hpp"
#include "Simple_zerotier.hpp"

int InitializeAndRunPlayer() {
    // SimpleZeroTierInit(
    //     "ztNodeIdentity",
    //     8989,
    //     0xd3ecf5726d81ccb3,
    //     std::chrono::seconds{20}
    // );

    auto ctx = CreateBasicClientContext();
    const int status = ctx->runFor(-1);
    HG_LOG_INFO(LOG_ID, "Main context stopped with exit code {}.", status);
    // SimpleZeroTierStop();
    return status;
}

int InitializeAndRunServer(int argc, char* argv[]) {
    // SimpleZeroTierInit(
    //     "ztNodeIdentity",
    //     8989,
    //     0xd3ecf5726d81ccb3,
    //     std::chrono::seconds{20}
    // );

    // clang-format off
    const ServerGameParams params{
        .playerCount = 6,
        .portNumber  = 8888
    };
    auto ctx = CreateServerContext(params);
    const int status = ctx->runFor(-1);
    HG_LOG_INFO(LOG_ID, "Main context stopped with exit code {}.", status);
    // SimpleZeroTierStop();
    return status;
    // clang-format on
}

int main(int argc, char* argv[]) try {
    // Set up logging
    hg::log::SetMinimalLogSeverity(hg::log::Severity::Info);

    // Seed RNGs
    hg::util::DoWith32bitRNG([](std::mt19937& aRng) {
        const auto seed = hg::util::Generate32bitSeed();
        aRng.seed(seed);
        HG_LOG_INFO(LOG_ID, "32-bit RNG seeded with: {}", seed);
    });
    hg::util::DoWith64bitRNG([](std::mt19937_64& aRng) {
        const auto seed = hg::util::Generate64bitSeed();
        aRng.seed(seed);
        HG_LOG_INFO(LOG_ID, "64-bit RNG seeded with: {}", seed);
    });

    // Set up RigelNet
    RN_IndexHandlers();

    // Init & run context
    if (argc <= 1) {
        return InitializeAndRunPlayer();
    } else {
        const std::string mode = argv[1];
        if (mode == "server") {
            return InitializeAndRunServer(argc, argv);
        } else {
           HG_LOG_ERROR(LOG_ID, "Unknown mode provided ({}), exiting.", mode);
           return EXIT_FAILURE;
        }
    }
} catch (const hg::TracedException& ex) {
    HG_LOG_FATAL(LOG_ID, "{}", ex.getFormattedDescription());
    return EXIT_FAILURE;
} catch (const std::exception& ex) {
    HG_LOG_FATAL(LOG_ID, "Exception caught: {}", ex.what());
    return EXIT_FAILURE;
}
