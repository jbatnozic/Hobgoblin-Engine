// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#ifndef UHOBGOBLIN_UTIL_STREAM_INPUT_HPP
#define UHOBGOBLIN_UTIL_STREAM_INPUT_HPP

#include <Hobgoblin/Common.hpp>
#include <Hobgoblin/HGExcept.hpp>

#include <Hobgoblin/Utility/Stream_base.hpp>
#include <Hobgoblin/Utility/Stream_errors.hpp>
#include <Hobgoblin/Utility/Stream_operators.hpp>

#include <cstdint>
#include <type_traits>
#include <utility>

#include <Hobgoblin/Private/Pmacro_define.hpp>

HOBGOBLIN_NAMESPACE_BEGIN
namespace util {

/**
 * Abstract base class for a binary input stream.
 * 
 * Child classes must override*:
 * - State checking:
 *   - `hasReadError`         | Check for read error state of stream.
 *   - `clearReadError`       | Clear the read error state of stream.
 *   - `_setReadError`        | Set read error state of stream.
 *   - `isGood`               | Check if the stream is in a valid state for I/O operations.
 *   - `_setNotGood`          | Set the stream to the permanently invalid state.
 * 
 * - I/O operations:
 *   - `_seek`                | Set the read position to absolute value.
 *   - `_seekRelative`        | Move the read position relative to its current value.
 *   - `_read`                | Copy data out to a preallocated buffer and advance the read position.
 *   - `_readInPlace`         | Return ptr to the internal buffer, if possible, and advance the read pos.
 *   - `_readInPlaceNoThrow`  | Same as above, but nonthrowing in case of failure.
 * 
 * - Introspection (optional to override):
 *   - `isPredetermined`      | Check whether amount of data can be determined ahead of time of reading.
 *   - `getDataSize`          | (if is predetermined) Get total size of data.
 *   - `getRemainingDataSize` | (if is predetermined) Get size of remaining data.
 *   - `getData`              | (if is predetermined) Access the internal data buffer.
 *   - `getReadPosition`      | (if is predetermined) Access the internal read position.
 * 
 * *-Method names starting with an underscore are private implementations of public methods.
 *   They can be overriden regardless.
 */
class InputStream : public StreamBase {
public:
    //! Virtual destructor.
    virtual ~InputStream() = default;

    ///////////////////////////////////////////////////////////////////////////
    // MARK: I/O OPERATIONS                                                  //
    ///////////////////////////////////////////////////////////////////////////

    //! Change the current reading position of the stream.
    //!
    //! \param aPosition absolute byte offset to jump to (it is relative to the beginning
    //!                  of the data, not to the current position).
    //!
    //! \returns new position, or an error code (see `StreamBase`) on failure.
    HG_NODISCARD std::int64_t seek(std::int64_t aPosition);

    //! Change the current reading position of the stream.
    //!
    //! \param aPosition relative byte offset to jump to (it is relative to the current position,
    //!                  not the beginning of data).
    //!
    //! \returns new position, or an error code (see `StreamBase`) on failure.
    HG_NODISCARD std::int64_t seekRelative(std::int64_t aOffset);

    //! \brief Extracts raw data from the stream by copying it to a preallocated buffer.
    //!        Advances the read position of the stream accordingly.
    //!        Returns the number of read bytes or an error code (see `StreamBase`) on failure.
    //!
    //! \param aDestination Pointer to a preallocated buffer where the data should be copied.
    //! \param aByteCount How many bytes to extract.
    //! \param aAllowPartal Whether partial (smaller than aByteCount - zero included) reads are allowed.
    //!
    //! \returns The number of bytes read (could be 0 if there was no more data in the stream,
    //!          or a negative error code (see `StreamBase`) on failure).
    HG_NODISCARD std::int64_t read(NeverNull<void*> aDestination,
                                   PZInteger        aByteCount,
                                   bool             aAllowPartal = false);

    //! \brief Extracts raw data from the stream; throws on failure.
    //!
    //! \warning not all implementations of InputStream are obligated to implement
    //!          this function! Those that do not shall return `nullptr` whenever
    //!          it is called, even if the stream is in an otherwise valid state.
    //! 
    //! Returns a pointer to a buffer of size N containing the next N bytes to be
    //! extracted from the stream, where N is equal to `aByteCount`. These bytes
    //! can then be copied, read directly or otherwise used. Any subsequent
    //! operation on the stream will invalidate the returned pointer, so you should
    //! process the data immediately. In any case, this function will advance the
    //! stream's internal reading position by `aByteCount`.
    //!
    //! \param aByteCount How many bytes to extract.
    //!
    //! \returns Pointer to data to be extracted, or `nullptr` if `aByteCount` is 0, or
    //!          if the concrete stream implementation does not support this method.
    //!
    //! \throws StreamReadError if stream has less than `aByteCount` bytes left
    //!         to extract. If this exception is thrown, the stream's read error
    //!         flag will also be set.
    HG_NODISCARD const void* readInPlace(PZInteger aByteCount);

    //! \brief Extracts raw data from the packet; does not throw on failure.
    //!
    //! \warning not all implementations of InputStream are obligated to implement
    //!          this function! Those that do not shall return `nullptr` whenever
    //!          it is called, even if the stream is in an otherwise valid state.
    //! 
    //! Returns a pointer to a buffer of size N containing the next N bytes to be
    //! extracted from the stream, where N is equal to `aByteCount`. These bytes
    //! can then be copied, read directly or otherwise used. Any subsequent
    //! operation on the stream will invalidate the returned pointer, so you should
    //! process the data immediately. In any case, this function will advance the
    //! stream's internal reading position by `aByteCount`.
    //!
    //! \param aByteCount How many bytes to extract.
    //!
    //! \returns Pointer to data to be extracted. Returns `nullptr` if `aByteCount` is 0,
    //!          or if the concrete stream implementation does not support this method,
    //!          or if the Packet has less than `aByteCount` bytes left to extract
    //!          (in this case the stream's read error flag will also be set.).
    HG_NODISCARD const void* readInPlaceNoThrow(PZInteger aByteCount);

    //! Extracts an object of type `T` from the stream (assuming it has the proper `operator>>`
    //! defined.
    //! \throws StreamReadError on failure. If this exception is thrown, the read error flag
    //!                         will also be set to an invalid state.
    template <class T>
    HG_NODISCARD T extract();

    //! Extracts an object of type `T` from the stream (assuming it has the proper `operator>>`
    //! defined. This method throws no exceptions, so you will have to check `hasReadError()` or
    //! `operator StrongBool` on the stream after one or several calls to this method - if they
    //! return false, then all extracts since the last time they returned true should be
    //! considered invalid.
    template <class T>
    HG_NODISCARD T extractNoThrow();

    //! Compile-time checker for whether a type has a compatible `operator>>`
    //! for extracting from `hg::util::InputStreamExtender`.
    template <class taType>
    class supports_extracting_of {
        template <class taType2>
        static auto test(int)
            -> decltype(std::declval<InputStreamExtender&>() >> std::declval<taType2&>(),
                        std::true_type());

        template <class>
        static auto test(...) -> std::false_type;

    public:
        static const bool value = decltype(test<taType>(0))::value;
    };

    //! Extraction operator; it has the same behaviour as `extract()`.
    //! \returns reference to self.
    template <class T>
    auto operator>>(T& aData) -> std::enable_if_t<supports_extracting_of<T>::value, InputStream&>;

    //! TODO
    class NoThrowAdapter {
    public:
        // clang-format off
        template <class T,
                  T_ENABLE_IF(InputStream::supports_extracting_of<T&>::value)>
        NoThrowAdapter& operator>>(T& aRef) {
            aRef = _istream.extractNoThrow<T>();
            return SELF;
        }
        // clang-format on

        operator StrongBool() const {
            return _istream.hasReadError() ? SBOOL_FALSE : SBOOL_TRUE;
        }

    private:
        friend class InputStream;

        NoThrowAdapter(InputStream& aIStream)
            : _istream{aIStream} {}

        InputStream& _istream;
    };

    //! TODO
    NoThrowAdapter noThrow() {
        return NoThrowAdapter{SELF};
    }

    ///////////////////////////////////////////////////////////////////////////
    // MARK: STATE CHECKING                                                  //
    ///////////////////////////////////////////////////////////////////////////

    //! Check if the stream's read error flag is set.
    //! 
    //! \note if `true`, that means that all non-throwing reads since the last time
    //!       that this method was called and returned `false` are invalid. Furthermore,
    //!       all future reads until the read error is successfully cleared will also
    //!       be invalid.
    //! 
    //! \note if `isGood` returns `false`, then this method shall return `true`.
    virtual bool hasReadError() const = 0;

    //! Attempt to clear the read error in order to unblock future reads.
    //! \returns `true` is successful, `false` on failure.
    virtual bool clearReadError() = 0;

    //! Checks if the stream as a whole is in a valid state. If this method returns `false`,
    //! then the stream has encountered an unrecoverable error state and all further operations
    //! (other than to destroy it) are invalid.
    virtual bool isGood() const = 0;

    //! \brief Test the validity of the stream for reading (equivalent to `!hasReadError()`)
    //! 
    //! This operator allows to test the stream as a boolean
    //! variable, to check if a reading operation(s) was(were) successful.
    //!
    //! This behavior is the same as standard C++ streams.
    //!
    //! Usage example:
    //! \code
    //! float x;
    //! stream >> x;
    //! if (stream) {
    //!    // ok, x was extracted successfully
    //! }
    //!
    //! // -- or --
    //!
    //! float x, y;
    //! if (stream >> x >> y) {
    //!    // ok, x and y were extracted successfully
    //! } else {
    //!    // error, either x or y (or both) could not be extracted
    //! }
    //! \endcode
    //!
    //! \return True if last data extraction from the stream was successful
    operator StrongBool() const {
        return hasReadError() ? SBOOL_FALSE : SBOOL_TRUE;
    }

    ///////////////////////////////////////////////////////////////////////////
    // MARK: INTROSPECTION                                                   //
    ///////////////////////////////////////////////////////////////////////////

    //! Check if the data in the stream is predetermined, such as when the stream has an
    //! internal data buffer. An opposite example would be a network socket, where we don't
    //! always know how much more data to expect.
    //! If this method returns false (which is also the default if not overriden), then it is
    //! expected that the following methods will return their default values as well:
    //! - `getDataSize`          (default: `E_UNKNOWN`)
    //! - `getRemainingDataSize` (default: `E_UNKNOWN`)
    //! - `getData`              (default: `nullptr`)
    //! - `getReadPosition`      (default: `E_UNKNOWN`)
    //! - `tell`                 (default: `E_UNKNOWN`)
    //! Overall there is not much utility in these methods if the stream is not predetermined
    //! and they probably shouldn't be used.
    virtual bool isPredetermined() const {
        return false;
    }

    virtual std::int64_t getDataSize() const {
        return E_UNKNOWN;
    }

    virtual std::int64_t getRemainingDataSize() const {
        return E_UNKNOWN;
    }

    //! \brief Get a pointer to the data contained in the Packet.
    //!
    //! \warning the returned pointer may become invalid after
    //! you append data to the packet, therefore it should never
    //! be stored.
    //!
    //! \returns Pointer to the data, or `nullptr` if the Packet is empty.
    //!
    //! \see getDataSize
    virtual const void* getData() const {
        return nullptr;
    }

    bool endOfStream() const {
        return (getRemainingDataSize() == 0);
    }

    virtual std::int64_t getReadPosition() const {
        return E_UNKNOWN;
    }

    //! Same as `getReadPosition`.
    std::int64_t tell() const {
        return getReadPosition();
    }

private:
    //! Implementation for `seek`.
    virtual std::int64_t _seek(std::int64_t aPosition) = 0;
    //! Implementation for `seekRelative`.
    virtual std::int64_t _seekRelative(std::int64_t aOffset) = 0;
    //! Implementation for `read`.
    virtual std::int64_t _read(NeverNull<void*> aDestination, PZInteger aByteCount, bool aAllowPartal) = 0;
    //! Implementation for `readInPlace`.
    virtual const void* _readInPlace(PZInteger aByteCount) = 0;
    //! Implementation for `readInPlaceNoThrow`.
    virtual std::int64_t _readInPlaceNoThrow(PZInteger aByteCount, const void** aResult) = 0;
    //! Implementation for `setReadError`.
    virtual void _setReadError() = 0;
    //! Implementation for `_setNotGood`.
    virtual void _setNotGood() = 0;

    void _setErrorIfNeeded(std::int64_t aErrorCode) {
        if (aErrorCode >= 0) {
            return;
        } else if (aErrorCode == E_FAILURE) {
            _setReadError();
        } else if (aErrorCode == E_BADSTATE) {
            _setNotGood();
        }
    }

    static void _logExtractionError(const char* aErrorMessage);
};

///////////////////////////////////////////////////////////////////////////
// MARK: INLINE FUNCTION IMPLEMENTATIONS                                 //
///////////////////////////////////////////////////////////////////////////

inline std::int64_t InputStream::seek(std::int64_t aPosition) {
    const auto result = _seek(aPosition);
    _setErrorIfNeeded(result);
    return result;
}

inline std::int64_t InputStream::seekRelative(std::int64_t aOffset) {
    const auto result = _seekRelative(aOffset);
    _setErrorIfNeeded(result);
    return result;
}

inline std::int64_t InputStream::read(NeverNull<void*> aDestination, PZInteger aByteCount, bool aAllowPartal) {
    const auto result = _read(aDestination, aByteCount, aAllowPartal);
    if (result > 0) {
        // Do nothing
    } else if (result == E_FAILURE || (!aAllowPartal &&result == 0)) {
        _setReadError();
    } else if (result == E_BADSTATE) {
        _setNotGood();
    }
    return result;
}

inline const void* InputStream::readInPlace(PZInteger aByteCount) {
    try {
        return _readInPlace(aByteCount);
    }
    catch (const std::exception& aEx) {
        _setNotGood();
        throw;
    }
}

inline const void* InputStream::readInPlaceNoThrow(PZInteger aByteCount) {
    const void* result;
    const auto code = _readInPlaceNoThrow(aByteCount, &result);
    _setErrorIfNeeded(code);
    return result;
}

///////////////////////////////////////////////////////////////////////////
// MARK: TEMPLATE FUNCTION IMPLEMENTATIONS                               //
///////////////////////////////////////////////////////////////////////////

template <class T>
auto InputStream::operator>>(T& aData)
    -> std::enable_if_t<supports_extracting_of<T>::value, InputStream&>
//
{
    InputStreamExtender extender{SELF};
    extender >> aData;
    if (hasReadError()) {
        HG_THROW_TRACED(StreamReadError,
                        0,
                        "Failed to extract data from stream (# of bytes remaining: {}).",
                        getRemainingDataSize());
    }
    return SELF;
}

template <class T>
T InputStream::extract() {
    T result;
    SELF >> result;
    return result;
}

template <class T>
T InputStream::extractNoThrow() {
    try {
        T                   result;
        InputStreamExtender extender{SELF};
        extender >> result;
        return result;
    } catch (const std::exception& aEx) {
        _logExtractionError(aEx.what());
        _setNotGood();
        return {};
    }
}

} // namespace util
HOBGOBLIN_NAMESPACE_END

#include <Hobgoblin/Private/Pmacro_undef.hpp>
#include <Hobgoblin/Private/Short_namespace.hpp>

#endif // !UHOBGOBLIN_UTIL_STREAM_INPUT_HPP
