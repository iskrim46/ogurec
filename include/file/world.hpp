#pragma once

#include <array>
#include <cstdint>
#include <cstdio>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "file/metadata.hpp"
#include "util/bitset.hpp"
#include "util/io.hpp"
#include "version.hpp"

namespace file {

class world {
	int32_t version;
	// metadata meta;

	std::vector<uint32_t> positions;
	bitset<0> importance;

	int32_t seed;
	std::array<uint8_t, 16> guid;
	int32_t worldID;
	int32_t world_left;
	int32_t world_right;
	int32_t world_top;
	int32_t world_botton;
	int32_t height;
	int32_t width;

public:
	void load_header(std::ifstream& f)
	{
	}

	world(std::string filename)
	{
		if (!std::filesystem::exists(filename)) {
			throw std::runtime_error("file not found");
		}
		auto rw = io::serialized_io(io::file_io { filename });

		rw.read(version);
		if (version <= 88 || version > terraria_version) {
			throw std::runtime_error("invalid world file version");
		}
		std::cout << version << std::endl;
		metadata meta;

		rw.read(meta);

		if (meta.ftype != filetype::World) {
			throw std::runtime_error("expected world file");
		}
	}
};

}