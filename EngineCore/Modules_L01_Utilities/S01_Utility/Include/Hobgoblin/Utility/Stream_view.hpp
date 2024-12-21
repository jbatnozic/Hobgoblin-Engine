// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#ifndef UHOBGOBLIN_UTIL_STREAM_VIEW_HPP
#define UHOBGOBLIN_UTIL_STREAM_VIEW_HPP

#include <Hobgoblin/Utility/Stream_errors.hpp>
#include <Hobgoblin/Utility/Stream_input.hpp>

#include <Hobgoblin/Math/Core.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

#include <Hobgoblin/Private/Pmacro_define.hpp>

HOBGOBLIN_NAMESPACE_BEGIN
namespace util {

class ViewStream final : public InputStream {
public:
    //! \brief Default constructor
    //!
    //! Creates an empty packet.
    ViewStream() = default;

    ViewStream(const void* aData, std::int64_t aDataByteCount) {
        reset(aData, aDataByteCount);
    }

    //! \brief Copy constructor.
    ViewStream(const ViewStream& aOther) = default;

    //! \brief Move constructor.
    ViewStream(ViewStream&& aOther) = default;

    //! \brief Copy assignment operator.
    ViewStream& operator=(const ViewStream& aOther) = default;

    //! \brief Move assignment operator.
    ViewStream& operator=(ViewStream&& aOther) = default;

    //! \brief Destructor.
    ~ViewStream() override = default;

    void reset() {
        reset(nullptr, 0);
    }

    void reset(const void* aData, std::int64_t aDataByteCount) {
        if (aData != nullptr && aDataByteCount > 0) {
            _data           = aData;
            _dataLength     = aDataByteCount;
            _readPos        = 0;
            _readErrorLevel = 0;
        } else {
            _data           = nullptr;
            _dataLength     = 0;
            _readPos        = 0;
            _readErrorLevel = E_BADSTATE;
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // MARK: INTROSPECTION                                                   //
    ///////////////////////////////////////////////////////////////////////////

    HG_NODISCARD bool isPredetermined() const override {
        return true;
    }

    HG_NODISCARD std::int64_t getDataSize() const override {
        return static_cast<std::int64_t>(_dataLength);
    }

    HG_NODISCARD std::int64_t getRemainingDataSize() const override {
        return static_cast<std::int64_t>(_dataLength - _readPos);
    }

    HG_NODISCARD const void* getData() const override {
        return _data;
    }

    HG_NODISCARD std::int64_t getReadPosition() const override {
        return static_cast<std::int64_t>(_readPos);
    }

    ///////////////////////////////////////////////////////////////////////////
    // MARK: STATE CHECKING                                                  //
    ///////////////////////////////////////////////////////////////////////////

    HG_NODISCARD bool hasReadError() const override {
        return (_readErrorLevel < 0);
    }

    HG_NODISCARD bool clearReadError() override {
        if (_readErrorLevel == E_BADSTATE) {
            return false;
        }
        _readErrorLevel = 0;
        return true;
    }

    HG_NODISCARD bool isGood() const override {
        return (_readErrorLevel != E_BADSTATE);
    }

private:
    const void*  _data           = nullptr;
    std::size_t  _dataLength     = 0;
    std::size_t  _readPos        = 0;
    std::int64_t _readErrorLevel = E_BADSTATE;

    // ===== I/O Operations =====

    std::int64_t _seek(std::int64_t aPosition) override {
        if (_readErrorLevel < 0) {
            return _readErrorLevel;
        }

        if (_dataLength == 0) {
            return 0;
        }

        const std::size_t min = 0;
        const std::size_t max = _dataLength - 1;

        _readPos = math::Clamp(static_cast<std::size_t>(aPosition), min, max);
        return static_cast<std::int64_t>(_readPos);
    }

    std::int64_t _seekRelative(std::int64_t aOffset) override {
        if (_readErrorLevel < 0) {
            return _readErrorLevel;
        }

        if (_dataLength == 0) {
            return 0;
        }

        const std::size_t min = 0;
        const std::size_t max = _dataLength - 1;

        const std::size_t newPos = static_cast<std::size_t>(static_cast<std::int64_t>(_readPos) + aOffset);

        _readPos = math::Clamp(newPos, min, max);
        return static_cast<std::int64_t>(_readPos);
    }

    std::int64_t _read(NeverNull<void*> aDestination, PZInteger aByteCount, bool aAllowPartal) override {
        if (_readErrorLevel < 0) {
            return _readErrorLevel;
        }

        if (aByteCount == 0) {
            return 0;
        }

        const auto rem = _dataLength - _readPos;
        if (rem >= pztos(aByteCount)) {
            std::memcpy(aDestination, static_cast<const char*>(_data) + _readPos, pztos(aByteCount));
            _readPos += pztos(aByteCount);
            return aByteCount;
        } else if (aAllowPartal && rem > 0) {
            std::memcpy(aDestination, static_cast<const char*>(_data) + _readPos, rem);
            _readPos += rem;
            return (std::int64_t)rem;
        } else {
            return 0;
        }
    }

    const void* _readInPlace(PZInteger aByteCount) override {
        const void* result = readInPlaceNoThrow(aByteCount);
        if (!SELF) {
            HG_THROW_TRACED(
                StreamReadError,
                0,
                "Failed to extract {} raw bytes from hg::util::ViewStream (actual # of bytes remaining: {}).",
                aByteCount,
                getRemainingDataSize());
        }
        return result;
    }

    std::int64_t _readInPlaceNoThrow(PZInteger aByteCount, const void** aResult) override {
        if (_readErrorLevel < 0) {
            *aResult = nullptr;
            return _readErrorLevel;
        }

        if (aByteCount == 0) {
            *aResult = nullptr;
            return 0;
        }

        const auto rem = _dataLength - _readPos;
        if (pztos(aByteCount) > rem) {
            *aResult = nullptr;
            return E_FAILURE;
        }

        *aResult = static_cast<const char*>(_data) + _readPos;
        _readPos += pztos(aByteCount);
        return aByteCount;
    }

    // ===== State Checking =====

    void _setReadError() override {
        _readErrorLevel = std::min(_readErrorLevel, E_FAILURE);
    }

    void _setNotGood() override {
        _readErrorLevel = E_BADSTATE;
    }
};

} // namespace util
HOBGOBLIN_NAMESPACE_END

#include <Hobgoblin/Private/Pmacro_undef.hpp>
#include <Hobgoblin/Private/Short_namespace.hpp>

#endif // !UHOBGOBLIN_UTIL_STREAM_VIEW_HPP
