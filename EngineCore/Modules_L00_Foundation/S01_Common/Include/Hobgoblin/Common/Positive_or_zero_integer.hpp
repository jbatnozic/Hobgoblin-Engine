// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

// clang-format off

#ifndef UHOBGOBLIN_COMMON_POSITIVE_OR_ZERO_INTEGER_HPP
#define UHOBGOBLIN_COMMON_POSITIVE_OR_ZERO_INTEGER_HPP

#include <cstddef>
#include <utility>

#include <Hobgoblin/Private/Pmacro_define.hpp>

HOBGOBLIN_NAMESPACE_BEGIN

// Positive-or-Zero Integer
using PZInteger = int;

// std::size_t to PZInteger
constexpr
PZInteger stopz(std::size_t value) {
    return static_cast<PZInteger>(value);
}

// PZInteger to std::size_t
constexpr
std::size_t pztos(PZInteger value) {
    return static_cast<std::size_t>(value);
}

template <class T>
constexpr PZInteger ToPz(T&& value) {
    return static_cast<PZInteger>(std::forward<T>(value));
}

template <class T>
constexpr std::size_t ToSz(T&& value) {
    return static_cast<std::size_t>(std::forward<T>(value));
}

HOBGOBLIN_NAMESPACE_END

#include <Hobgoblin/Private/Pmacro_undef.hpp>
#include <Hobgoblin/Private/Short_namespace.hpp>

#endif // !UHOBGOBLIN_COMMON_POSITIVE_OR_ZERO_INTEGER_HPP

// clang-format on
