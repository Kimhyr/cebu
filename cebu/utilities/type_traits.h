#pragma once
#include <type_traits>
#define CEBU_INCLUDED_UTILITIES_TYPE_TRAITS_H

#include <concepts>

namespace cebu
{

struct not_found_t {};

template<typename T, typename ...Ts>
struct has_type : std::false_type {};

template<typename T>
struct has_type<T> : std::false_type {};

template<typename T, typename ...Ts>
struct has_type<T, T, Ts...> : std::true_type {};

template<typename T, typename ...Ts>
static constexpr bool has_type_v = has_type<T, Ts...>::value;

}
