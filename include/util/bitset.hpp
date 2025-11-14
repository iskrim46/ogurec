#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <vector>

constexpr int bits_ceil(int bits)
{
	return bits / 8 + (bits % 8 > 0 ? 1 : 0);
}

template <unsigned int flags, auto size = bits_ceil(flags)>
struct bitset : std::array<std::uint8_t, size> {
	using array_type = std::array<std::uint8_t, size>;

private:
	struct bit_ref {
		bitset<flags>& a;
		int referenced_byte;
		uint8_t referenced_bit_mask;

		bool operator=(bool b)
		{
			if (b)
				a.at(referenced_byte) |= referenced_bit_mask;
			else
				a.at(referenced_byte) &= ~referenced_bit_mask;
			return b;
		}

		constexpr operator bool() const
		{
			return (a.at(referenced_byte) & referenced_bit_mask) > 0;
		}
	};

public:
	template <class T>
	constexpr bool operator[](T idx) const
	{
		unsigned int i = (unsigned int)(idx);
		assert(i < flags);
		return (array_type::at(i / 8) & (1 << (i % 8))) > 0;
	}

	template <class T>
	bit_ref operator[](T idx)
	{
		unsigned int i = (unsigned int)(idx);
		assert(i < flags);
		return bit_ref(*this, i / 8, (1 << (i % 8)));
	}
};

// FIXME: ??
template <>
struct bitset<0> : std::vector<std::uint8_t> {
	using container_type = std::vector<std::uint8_t>;

private:
	struct bit_ref {
		bitset<0>& a;
		int referenced_byte;
		uint8_t referenced_bit_mask;

		bool operator=(bool b)
		{
			if (b)
				a.at(referenced_byte) |= referenced_bit_mask;
			else
				a.at(referenced_byte) &= ~referenced_bit_mask;
			return b;
		}

		constexpr operator bool() const
		{
			return (a.at(referenced_byte) & referenced_bit_mask) > 0;
		}
	};

public:
	template <class T>
	constexpr bool operator[](T idx) const
	{
		unsigned int i = (unsigned int)(idx);
		assert(i < container_type::size());
		return (container_type::at(i / 8) & (1 << (i % 8))) > 0;
	}

	template <class T>
	bit_ref operator[](T idx)
	{
		unsigned int i = (unsigned int)(idx);
		assert(i < container_type::size());
		return bit_ref(*this, i / 8, (1 << (i % 8)));
	}
};