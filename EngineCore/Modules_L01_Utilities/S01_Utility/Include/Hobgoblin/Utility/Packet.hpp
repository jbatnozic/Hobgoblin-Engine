// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#ifndef UHOBGOBLIN_UTIL_PACKET_HPP
#define UHOBGOBLIN_UTIL_PACKET_HPP

#include <Hobgoblin/Utility/Stream.hpp>

#include <Hobgoblin/Math/Core.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

#include <Hobgoblin/Private/Pmacro_define.hpp>

HOBGOBLIN_NAMESPACE_BEGIN
namespace util {

using PacketReadError = StreamReadError;

class Packet final
    : public OutputStream
    , public InputStream {
public:
    //! \brief Default constructor
    //!
    //! Creates an empty packet.
    Packet() = default;

    //! \brief Copy constructor.
    Packet(const Packet& aOther) = default;

    //! \brief Move constructor.
    Packet(Packet&& aOther) = default;

    //! \brief Copy assignment operator.
    Packet& operator=(const Packet& aOther) = default;

    //! \brief Move assignment operator.
    Packet& operator=(Packet&& aOther) = default;

    //! \brief Destructor.
    ~Packet() override = default;

    //! \brief Clear the packet (reset its whole state).
    //!
    //! After calling clear(), the packet is empty.
    //!
    //! \see append
    void clear() {
        _buffer.clear();
        _readPos        = 0;
        _readErrorLevel = 0;
    }

    //! Same as `endOfStream`.
    HG_NODISCARD bool endOfPacket() const {
        return endOfStream();
    }

    //! \brief Get a non-const pointer to the data contained in the Packet.
    //!
    //! \warning the returned pointer may become invalid after
    //! you append data to the packet, therefore it should never
    //! be stored.
    //!
    //! \warning the returned pointer will allow you to edit the data stored
    //! in the packet. DO NOT do this unless you're sure you know what you're
    //! doing.
    //!
    //! \returns Pointer to the data, or `nullptr` if the Packet is empty.
    //!
    //! \see getData, getDataSize
    HG_NODISCARD void* getMutableData();

    ///////////////////////////////////////////////////////////////////////////
    // MARK: INTROSPECTION                                                   //
    ///////////////////////////////////////////////////////////////////////////

    HG_NODISCARD bool isPredetermined() const override {
        return true;
    }

    HG_NODISCARD std::int64_t getDataSize() const override {
        return static_cast<std::int64_t>(_buffer.size());
    }

    HG_NODISCARD std::int64_t getRemainingDataSize() const override {
        return static_cast<std::int64_t>(_buffer.size() - _readPos);
    }

    HG_NODISCARD const void* getData() const override {
        return _buffer.empty() ? nullptr : _buffer.data();
    }

    HG_NODISCARD std::int64_t getReadPosition() const override {
        return static_cast<std::int64_t>(_readPos);
    }

    ///////////////////////////////////////////////////////////////////////////
    // MARK: STATE CHECKING                                                  //
    ///////////////////////////////////////////////////////////////////////////

    HG_NODISCARD bool hasWriteError() const override {
        return (_readErrorLevel == E_BADSTATE);
    }

    HG_NODISCARD bool hasReadError() const override {
        return (_readErrorLevel < 0);
    }

    HG_NODISCARD bool clearWriteError() override {
        if (_readErrorLevel == E_BADSTATE) {
            return false;
        }
        return true;
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

    ///////////////////////////////////////////////////////////////////////////
    // MARK: NOTHROW ADAPTER                                                 //
    ///////////////////////////////////////////////////////////////////////////

    HG_DISAMBIGUATE_NOTHROW(Packet)

private:
    std::vector<std::uint8_t> _buffer;             //!< Buffer of data stored in the packet
    std::size_t               _readPos        = 0; //!< Current reading position in the packet
    std::int64_t              _readErrorLevel = 0; //!< Reading state of the packet

    // ===== I/O Operations =====

    std::int64_t _write(NeverNull<const void*> aData, PZInteger aByteCount, bool aAllowPartal) override;

    std::int64_t _seek(std::int64_t aPosition) override;

    std::int64_t _seekRelative(std::int64_t aOffset) override;

    std::int64_t _read(NeverNull<void*> aDestination, PZInteger aByteCount, bool aAllowPartal) override;

    const void* _readInPlace(PZInteger aByteCount) override;

    std::int64_t _readInPlaceNoThrow(PZInteger aByteCount, const void** aResult) override;

    // ===== State Checking =====

    void _setReadError() override {
        _readErrorLevel = std::min(_readErrorLevel, E_FAILURE);
    }

    void _setWriteError() override {}

    void _setNotGood() override {
        _readErrorLevel = E_BADSTATE;
    }
};

OutputStream& operator<<(OutputStreamExtender& aPacketExt, const Packet& aData);

InputStream& operator>>(InputStreamExtender& aPacketExt, Packet& aData);

//! Helper for PackArgs (see below).
// clang-format off
template <class ...taNoArgs,
          T_ENABLE_IF(sizeof...(taNoArgs) == 0)>
// clang-format on
void PackArgs(Packet&) {}

//! Pack all arguments into the given Packet, in order of appearance (left to right).
template <class taArgsHead, class... taArgsRest>
void PackArgs(Packet& aPacket, taArgsHead&& aArgsHead, taArgsRest&&... aArgsRest) {
    aPacket.append(std::forward<taArgsHead>(aArgsHead));
    PackArgs<taArgsRest...>(aPacket, std::forward<taArgsRest>(aArgsRest)...);
}

} // namespace util
HOBGOBLIN_NAMESPACE_END

#include <Hobgoblin/Private/Pmacro_undef.hpp>
#include <Hobgoblin/Private/Short_namespace.hpp>

#endif // !UHOBGOBLIN_UTIL_PACKET_HPP
