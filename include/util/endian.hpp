#pragma once

#include <bit>
#include <concepts>

template <std::integral T>
    requires(sizeof(T) <= 8)
T to_little(T x)
{
	if constexpr (std::endian::native == std::endian::little || sizeof(T) == 1) {
		return x;
	}
	switch (sizeof(T)) {
	case 2:
		return __builtin_bswap16(x);
	case 4:
		return __builtin_bswap32(x);
	case 8:
		return __builtin_bswap64(x);
	}
}