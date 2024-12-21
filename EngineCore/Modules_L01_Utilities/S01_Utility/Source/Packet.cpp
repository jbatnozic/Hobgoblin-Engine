// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#include <Hobgoblin/Utility/Packet.hpp>

#include <Hobgoblin/HGExcept.hpp>

#include <cstring>
#include <type_traits>

#include <Hobgoblin/Private/Pmacro_define.hpp>

HOBGOBLIN_NAMESPACE_BEGIN
namespace util {

void* Packet::getMutableData() {
    return _buffer.empty() ? nullptr : _buffer.data();
}

///////////////////////////////////////////////////////////////////////////
// MARK: PRIVATE METHODS                                                 //
///////////////////////////////////////////////////////////////////////////

std::int64_t Packet::_write(NeverNull<const void*> aData, PZInteger aByteCount, bool /*aAllowPartal*/) {
    if (aData && aByteCount > 0) {
        _buffer.insert(_buffer.end(),
                       static_cast<const std::uint8_t*>(aData.get()),
                       static_cast<const std::uint8_t*>(aData.get()) + aByteCount);
    }
    return aByteCount;
}

std::int64_t Packet::_seek(std::int64_t aPosition) {
    if (_readErrorLevel < 0) {
        return _readErrorLevel;
    }

    if (_buffer.size() == 0) {
        return 0;
    }

    const std::size_t min = 0;
    const std::size_t max = _buffer.size() - 1;

    _readPos = math::Clamp(static_cast<std::size_t>(aPosition), min, max);
    return static_cast<std::int64_t>(_readPos);
}

std::int64_t Packet::_seekRelative(std::int64_t aOffset) {
    if (_readErrorLevel < 0) {
        return _readErrorLevel;
    }

    if (_buffer.size() == 0) {
        return 0;
    }

    const std::size_t min = 0;
    const std::size_t max = _buffer.size() - 1;

    const std::size_t newPos = static_cast<std::size_t>(static_cast<std::int64_t>(_readPos) + aOffset);

    _readPos = math::Clamp(newPos, min, max);
    return static_cast<std::int64_t>(_readPos);
}

std::int64_t Packet::_read(NeverNull<void*> aDestination, PZInteger aByteCount, bool aAllowPartal) {
    if (_readErrorLevel < 0) {
        return _readErrorLevel;
    }

    if (aByteCount == 0) {
        return 0;
    }

    const auto rem = _buffer.size() - _readPos;
    if (rem >= pztos(aByteCount)) {
        std::memcpy(aDestination, &(_buffer[_readPos]), pztos(aByteCount));
        _readPos += pztos(aByteCount);
        return aByteCount;
    } else if (aAllowPartal && rem > 0) {
        std::memcpy(aDestination, &(_buffer[_readPos]), rem);
        _readPos += rem;
        return (std::int64_t)rem;
    } else {
        return 0;
    }
}

const void* Packet::_readInPlace(PZInteger aByteCount) {
    const void* result = readInPlaceNoThrow(aByteCount);
    if (!SELF) {
        HG_THROW_TRACED(
            StreamReadError,
            0,
            "Failed to extract {} raw bytes from hg::util::Packet (actual # of bytes remaining: {}).",
            aByteCount,
            getRemainingDataSize());
    }
    return result;
}

std::int64_t Packet::_readInPlaceNoThrow(PZInteger aByteCount, const void** aResult) {
    if (_readErrorLevel < 0) {
        *aResult = nullptr;
        return _readErrorLevel;
    }

    if (aByteCount == 0) {
        *aResult = nullptr;
        return 0;
    }

    const auto rem = _buffer.size() - _readPos;
    if (pztos(aByteCount) > rem) {
        *aResult = nullptr;
        return E_FAILURE;
    }

    *aResult = &(_buffer[_readPos]);
    _readPos += pztos(aByteCount);
    return aByteCount;
}

///////////////////////////////////////////////////////////////////////////
// MARK: OPERATOR DEFINITIONS                                            //
///////////////////////////////////////////////////////////////////////////

OutputStream& operator<<(OutputStreamExtender& aPacketExt, const Packet& aData) {
    aPacketExt << static_cast<std::int32_t>(aData.getDataSize());
    if (const auto size = aData.getDataSize(); size > 0) {
        (void)aPacketExt->write(aData.getData(), size);
    }
    return *aPacketExt;
}

InputStream& operator>>(InputStreamExtender& aPacketExt, Packet& aData) {
    // First extract data length
    const auto length = aPacketExt->extractNoThrow<std::int32_t>();
    if (!*aPacketExt) {
        return *aPacketExt;
    }

    aData.clear();
    if (length > 0) {
        const auto  pzlen = static_cast<PZInteger>(length);
        const auto* bytes = aPacketExt->readInPlaceNoThrow(pzlen);
        if (bytes) {
            const auto writeCnt = aData.write(bytes, pzlen);
            if (writeCnt < pzlen) {}
        }
    }

    return *aPacketExt;
}

} // namespace util
HOBGOBLIN_NAMESPACE_END

#include <Hobgoblin/Private/Pmacro_undef.hpp>
