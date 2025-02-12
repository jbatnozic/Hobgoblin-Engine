// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#ifndef UHOBGOBLIN_UNICODE_REGEX_HPP
#define UHOBGOBLIN_UNICODE_REGEX_HPP

#include <Hobgoblin/Common.hpp>
#include <Hobgoblin/HGExcept.hpp>
#include <Hobgoblin/Unicode/Unicode_string.hpp>

#include <unicode/regex.h>

#include <cstdint>

#include <Hobgoblin/Private/Pmacro_define.hpp>

HOBGOBLIN_NAMESPACE_BEGIN

// MARK: URegexError

//! Traced exception that's thrown in case one of the regex operations fails.
class URegexError : public TracedRuntimeError {
public:
    using TracedRuntimeError::TracedRuntimeError;
};

// MARK: UMatchResults

class URegex;

//! Match results for the unicode-aware regex.
class UMatchResults {
public:
    UMatchResults() = default;

    PZInteger getGroupCount() const;

    UnicodeString operator[](PZInteger aGroup) const;

private:
    friend bool MatchRegex(const UnicodeString& aString, URegex& aRegex, UMatchResults& aResults);

    URegex* _regex = nullptr;
};

// MARK: URegex

//! Reusable unicode-aware regex pattern.
class URegex {
public:
    enum Flags : std::uint32_t {
        /**  Enable case insensitive matching.  @stable ICU 2.4 */
        CASE_INSENSITIVE = 2,

        /**  Allow white space and comments within patterns  @stable ICU 2.4 */
        COMMENTS = 4,

        /**  If set, '.' matches line terminators,  otherwise '.' matching stops at line end.
         *  @stable ICU 2.4 */
        DOTALL = 32,

        /**  If set, treat the entire pattern as a literal string.
         *  Metacharacters or escape sequences in the input sequence will be given
         *  no special meaning.
         *
         *  The flag CASE_INSENSITIVE retains its impact
         *  on matching when used in conjunction with this flag.
         *  The other flags become superfluous.
         *
         * @stable ICU 4.0
         */
        LITERAL = 16,

        /**   Control behavior of "$" and "^"
         *    If set, recognize line terminators within string,
         *    otherwise, match only at start and end of input string.
         *   @stable ICU 2.4 */
        MULTILINE = 8,

        /**   Unix-only line endings.
         *   When this mode is enabled, only \\u000a is recognized as a line ending
         *    in the behavior of ., ^, and $.
         *   @stable ICU 4.0
         */
        UNIX_LINES = 1,

        /**  Unicode word boundaries.
         *     If set, \b uses the Unicode TR 29 definition of word boundaries.
         *     Warning: Unicode word boundaries are quite different from
         *     traditional regular expression word boundaries.  See
         *     http://unicode.org/reports/tr29/#Word_Boundaries
         *     @stable ICU 2.8
         */
        UWORD = 256,

        /**  Error on Unrecognized backslash escapes.
         *     If set, fail with an error on patterns that contain
         *     backslash-escaped ASCII letters without a known special
         *     meaning.  If this flag is not set, these
         *     escaped letters represent themselves.
         *     @stable ICU 4.0
         */
        ERROR_ON_UNKNOWN_ESCAPES = 512
    };

    URegex(const UnicodeString& aPattern, std::uint32_t aFlags = 0);

private:
    friend bool MatchRegex(const UnicodeString& aString, URegex& aRegex, UMatchResults& aResults);
    friend class UMatchResults;

    UErrorCode        _status;
    icu::RegexMatcher _matcher;
};

// MARK: Functions

//! Try to match the entire input string against a regex pattern.
bool MatchRegex(const UnicodeString& aString, URegex& aRegex, UMatchResults& aResults);

HOBGOBLIN_NAMESPACE_END

#include <Hobgoblin/Private/Pmacro_undef.hpp>
#include <Hobgoblin/Private/Short_namespace.hpp>

#endif // !UHOBGOBLIN_UNICODE_REGEX_HPP