#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <format>
#include <stdexcept>
#include <vector>

#include <sys/socket.h>
#include <sys/types.h>

#include <boost/pfr/core.hpp>
#include <zlib.h>

#include "types.hpp"

namespace net {

namespace packet {

using packet_header = std::tuple<uint8_t, uint16_t>;

template <packet T>
void decode_packet(std::span<uint8_t> payload, T& t)
{
	std::size_t offset = 0;

	boost::pfr::for_each_field(t, [&payload, &offset](auto& field, int i) {
		if (offset >= payload.size()) {
			throw std::runtime_error("Received invalid packet");
		}
		offset += decode_field(payload.subspan(offset), field);
	});

	if (offset < payload.size()) {
		throw std::runtime_error(std::format("{}: {}", offset, payload.size()));
	}
}

template <packet T>
std::vector<uint8_t> encode_raw_packet(T& t)
{
	std::vector<uint8_t> payload;

	boost::pfr::for_each_field(t, [&payload](auto& field) {
		encode_field(payload, field);
	});

	return payload;
}

template <packet T>
std::vector<uint8_t> encode_packet(T& t)
{
	return encode_raw_packet(t);
}

template <compressed_packet T>
std::vector<uint8_t> encode_packet(T& t)
{
	static constexpr auto chunk_size = 1024; 
	std::array<uint8_t, chunk_size> buffer;
	std::vector<uint8_t> payload;

	int ret, flush;
	z_stream strm;

	auto raw_payload = encode_raw_packet(t);

	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	ret = deflateInit(&strm, 15);
	if (ret != Z_OK)
		throw std::runtime_error("deflate init error");

	strm.avail_in = raw_payload.size();
	strm.next_in = raw_payload.data();
	do {
		strm.avail_out = chunk_size;
		strm.next_out = reinterpret_cast<Bytef *>(buffer.data());
		ret = deflate(&strm, flush);
		assert(ret != Z_STREAM_ERROR);
		payload.insert(payload.end(), buffer.begin(), buffer.end()-strm.avail_out);
	} while (strm.avail_out == 0);

	assert(strm.avail_in == 0); /* all input will be used */

	deflateEnd(&strm);
	return payload;
}
}

}