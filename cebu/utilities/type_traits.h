#pragma once
#define CEBU_INCLUDED_UTILITIES_TYPE_TRAITS_H

#include <type_traits>

namespace cebu
{

template<auto ...Values>
auto is_one_of(auto const& value) -> bool
	requires std::is_same_v<decltype(Value), >
{
}

}
