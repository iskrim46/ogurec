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
#include "util/io.hpp"

namespace net {

namespace packet {

using packet_header = std::tuple<uint8_t, uint16_t>;

template <packet T>
void decode_packet(std::span<uint8_t> payload, T& t)
{
	std::size_t len = 0;

	auto rd = io::serialized_io(io::buffered_io { payload });
	len = rd.read(t);

	if (len != payload.size()) {
		throw std::runtime_error(std::format("{}: {}", len, payload.size()));
	}
}

template <packet T>
std::vector<uint8_t> encode_packet_impl(T& t)
{
	std::vector<uint8_t> payload;
	auto wt = io::serialized_io(io::buffered_io { payload });
	wt.write(t);

	return payload;
}

template <packet T>
std::vector<uint8_t> encode_packet(T& t)
{
	return encode_packet_impl(t);
}

template <compressed_packet T>
std::vector<uint8_t> encode_packet(T& t)
{
	static constexpr auto chunk_size = 1024;
	std::array<uint8_t, chunk_size> buffer;
	std::vector<uint8_t> payload;

	int ret, flush;
	z_stream strm;

	auto raw_payload = encode_packet_impl(t);

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
		strm.next_out = reinterpret_cast<Bytef*>(buffer.data());
		ret = deflate(&strm, flush);
		assert(ret != Z_STREAM_ERROR);
		payload.insert(payload.end(), buffer.begin(), buffer.end() - strm.avail_out);
	} while (strm.avail_out == 0);

	assert(strm.avail_in == 0); /* all input will be used */

	deflateEnd(&strm);
	return payload;
}
}

}