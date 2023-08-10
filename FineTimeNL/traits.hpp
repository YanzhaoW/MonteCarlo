#pragma once

#include <concepts>
template <int size1, std::size_t... sizes>
concept Equal = ((size1 == sizes) && ...);

template <int size1, std::size_t... sizes>
concept LessThan = ((size1 < sizes) && ...);

template <int size1, std::size_t... sizes>
concept NoLessThan = ((size1 >= sizes) && ...);

template <typename Type, typename Par>
concept Constructible_from = requires(Par par) { std::remove_cvref_t<Type>{ par }; };
