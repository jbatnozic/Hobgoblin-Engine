#pragma once

#include <Hobgoblin/HGExcept.hpp>

#include <ZTCpp.hpp>

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

class SimpleZeroTierError : public jbatnozic::hobgoblin::TracedRuntimeError {
public:
    using TracedRuntimeError::TracedRuntimeError;
};

void SimpleZeroTier_Init(const std::string&        aNodeIdentityPath,
                         std::uint16_t             aServicePort,
                         std::uint64_t             aNetworkId,
                         std::chrono::milliseconds aMaxTimeToWait);

enum class SimpleZeroTier_Status {
    STOPPED,
    INITIALIZING,
    FAILURE,
    ACTIVE
};

SimpleZeroTier_Status SimpleZeroTier_GetStatus();

jbatnozic::ztcpp::IpAddress SimpleZeroTier_GetLocalIpAddress();

void SimpleZeroTier_Stop();
