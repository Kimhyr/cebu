#pragma once
#include <type_traits>
#define CEBU_INCLUDED_UTILITIES_TYPE_TRAITS_H

#include <concepts>

namespace cebu
{

struct not_found_t {};

template<typename T, typename ...Ts>
struct find_type : std::false_type {};

template<typename T>
struct find_type<T> : std::false_type {};

template<typename T, typename ...Ts>
struct find_type<T, T, Ts...> : std::true_type {};

template<typename T, typename ...Ts>
static constexpr bool find_type_v = find_type<T, Ts...>::value;

}
