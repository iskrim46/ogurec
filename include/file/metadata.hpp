#pragma once

#include "util/bitset.hpp"
#include "util/io.hpp"
#include <cstdint>
#include <cstdio>
#include <stdexcept>

namespace file {


struct metadata {
	static constexpr auto magic = 27981915666277746uL;
	static constexpr auto magic_mask = 0xFFFFFFFFFFFFFFuL;
	static constexpr auto size = 20;

	enum class filetype : uint8_t {
		Map = 1,
		World,
		Player,
		TOTAL,
	};
	
	enum class flags {
		Favorite,
	};

	filetype ftype;
	uint32_t revision;
	bitset<64> flags;

	template <class T>
	ssize_t read(io::serialized_io<T>& io)
	{
		uint64_t filemagic;
		auto bytes = io.read(filemagic);
		if ((filemagic & magic_mask) != magic) {
			throw std::runtime_error("not a relogic file");
		}

		uint8_t type = ((filemagic >> 56) & 0xFF);
		if (type == 0 || type >= int(filetype::TOTAL)) {
			throw std::runtime_error("unknown file type");
		}

		ftype = filetype(type);

		bytes += io.read(revision);
		bytes += io.read(flags);

		return bytes;
	}
};

}