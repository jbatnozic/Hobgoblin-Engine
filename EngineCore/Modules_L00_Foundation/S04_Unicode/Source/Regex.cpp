// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#include <Hobgoblin/Unicode/Regex.hpp>

#include <Hobgoblin/Private/Pmacro_define.hpp>

HOBGOBLIN_NAMESPACE_BEGIN

// MARK: MatchResults

PZInteger UMatchResults::getGroupCount() const {
    HG_VALIDATE_PRECONDITION(_regex != nullptr);
    return _regex->_matcher.groupCount();
}

UnicodeString UMatchResults::operator[](PZInteger aGroup) const {
    HG_VALIDATE_PRECONDITION(_regex != nullptr);

    UErrorCode status = U_ZERO_ERROR;
    auto       result = _regex->_matcher.group(aGroup, status);
    if (U_FAILURE(status)) {
        HG_THROW_TRACED(URegexError, status, "Failed to get group: {}.", u_errorName(status));
    }
    return result;
}

// MARK: Regex

namespace {
std::uint32_t HgRegexFlagsToIcuRegexFlags(std::uint32_t aFlags) {
    std::uint32_t result = 0;

    if ((aFlags & URegex::CASE_INSENSITIVE) != 0) {
        result |= UREGEX_CASE_INSENSITIVE;
    }

    if ((aFlags & URegex::COMMENTS) != 0) {
        result |= UREGEX_COMMENTS;
    }

    if ((aFlags & URegex::DOTALL) != 0) {
        result |= UREGEX_DOTALL;
    }

    if ((aFlags & URegex::LITERAL) != 0) {
        result |= UREGEX_LITERAL;
    }

    if ((aFlags & URegex::MULTILINE) != 0) {
        result |= UREGEX_MULTILINE;
    }

    if ((aFlags & URegex::UNIX_LINES) != 0) {
        result |= UREGEX_UNIX_LINES;
    }

    if ((aFlags & URegex::UWORD) != 0) {
        result |= UREGEX_UWORD;
    }

    if ((aFlags & URegex::ERROR_ON_UNKNOWN_ESCAPES) != 0) {
        result |= UREGEX_ERROR_ON_UNKNOWN_ESCAPES;
    }

    return result;
}
} // namespace

URegex::URegex(const UnicodeString& aPattern, std::uint32_t aFlags)
    : _status{U_ZERO_ERROR}
    , _matcher{aPattern, HgRegexFlagsToIcuRegexFlags(aFlags), _status} //
{
    if (U_FAILURE(_status)) {
        HG_THROW_TRACED(URegexError,
                        _status,
                        "Failed to instantiate Regex object: {}.",
                        u_errorName(_status));
    }
}

// MARK: Functions

bool RegexMatch(const UnicodeString& aString, URegex& aRegex, UMatchResults& aResults) {
    aRegex._matcher.reset(aString);

    UErrorCode status = U_ZERO_ERROR;
    if (aRegex._matcher.matches(status)) {
        if (U_FAILURE(status)) {
            HG_THROW_TRACED(URegexError, status, "Failed to match pattern: {}.", u_errorName(status));
        }
        aResults._regex = &aRegex;
        return true;
    } else {
        if (U_FAILURE(status)) {
            HG_THROW_TRACED(URegexError, status, "Failed to match pattern: {}.", u_errorName(status));
        }
        aResults._regex = nullptr;
        return false;
    }
}

HOBGOBLIN_NAMESPACE_END

#include <Hobgoblin/Private/Pmacro_undef.hpp>
