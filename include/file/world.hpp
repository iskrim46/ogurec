#pragma once

#include <cstdint>
#include <cstdio>
#include <fcntl.h>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "file/metadata.hpp"
#include "util/bitset.hpp"
#include "util/io.hpp"
#include "version.hpp"

namespace file {

class world {
	// TODO: World, Player and Map files probably share similar(or the same) header format.
	// If so, move this into file/header.hpp, and defile `struct header : file::header` here
	struct header {
		int32_t version;
		metadata meta;
		std::vector<int32_t> positions;
		bitset<0> importance;
		

		ssize_t read(io::serialized_file f)
		{
			auto bytes = f.read(version);

			if (version < 88 || version > terraria_version) {
				throw std::runtime_error("unsupported world version");
			}

			if (version >= 135) {
				bytes += f.read(meta);
				if (meta.ftype != metadata::filetype::World) {
					throw std::runtime_error("unexpected relogic file format");
				}
			} else {
				throw std::runtime_error("unimplemented");
			}

			int16_t positions_size;
			bytes += f.read(positions_size);
			positions.resize(positions_size);
			bytes += f.read(positions);
			
			int16_t importance_size;
			bytes += f.read(importance_size);
			importance.resize(bits_ceil(importance_size));
			bytes += f.read(importance);
			return bytes;
		}
	};
	
	enum file_positions {
		FileHeaderEnd,
		HeaderEnd,
		WorldTilesEnd,
		ChestsEnd,
		SignsEnd,
		NPCsEnd,
		DummiesEnd, // Terraria[116;122)
		TileEntitiesEnd = DummiesEnd,
		WeightedPressurePlatesEnd,
		TownManagerEnd,
		BestiaryEnd,
		CreativePowersEnd,
	};

	header hdr;

public:
	world(std::string filename)
	{
		if (!std::filesystem::exists(filename)) {
			throw std::runtime_error("file not found");
		}
		auto rw = io::serialized_io(io::file_io { filename });

		auto offset = rw.read(hdr);
		if (offset != hdr.positions[file_positions::FileHeaderEnd]) {
			throw std::runtime_error("currupted file!");
		}
	}
};

}