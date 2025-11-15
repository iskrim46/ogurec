#pragma once

#include "util/endian.hpp"
#include <boost/pfr/core.hpp>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <string>
#include <unistd.h>

namespace io {

template <class T, class K>
concept custom_write = requires(T t) {
	{ t.write(K()) } -> std::same_as<ssize_t>;
};
template <class T, class K>
concept custom_read = requires(T& t, K& k) {
	{ t.read(k) } -> std::same_as<ssize_t>;
};

template <class T>
concept container = requires(T& t) {
	{ t.data() };
	{ t.size() };
	{ t.begin() };
	{ t.end() };
};

class file_io {
	int fd;

public:
	file_io(int fd)
	    : fd(fd)
	{
	}

	struct eof {};

	file_io(std::string filename)
	    : fd(open(filename.c_str(), O_RDWR))
	{
		if (fd < 0)
			throw std::runtime_error(strerror(errno));
	}

	//~file_io()
	//{
	//	::close(fd);
	//}

	ssize_t read_data(char* buf, size_t nbytes)
	{
		auto bytes = ::read(fd, buf, nbytes);
		if (bytes == EOF) {
			throw eof{};
		}

		assert(bytes >= 0);
		return bytes;
	}

	ssize_t write_data(const char* buf, size_t nbytes)
	{
		auto bytes = ::write(fd, buf, nbytes);		
		if (bytes == EOF) {
			throw eof{};
		}
		
		assert(bytes >= 0);
		return bytes;
	}
};

template <container T>
class buffered_io {
	size_t cursor = 0;
	T& buffer;

public:
	buffered_io(T& buffer)
	    : buffer(buffer)
	{
	}

	ssize_t read_data(char* buf, size_t nbytes)
	{
		auto bytes = nbytes > (buffer.size() - cursor) ? (buffer.size() - cursor) : nbytes;

		memcpy(buf, buffer.data() + cursor, bytes);
		cursor += bytes;
		return bytes;
	}

	ssize_t write_data(const char* buf, size_t nbytes)
	{
		buffer.insert(buffer.end(), buf, buf + nbytes);
		return nbytes;
	}
};

template <class I>
class serialized_io {
	I io;

public:
	serialized_io(I io)
	    : io(io)
	{
	}

	template <std::integral T>
	ssize_t read(T& f)
	{
		auto bytes = io.read_data(reinterpret_cast<char*>(&f), sizeof(T));
		f = to_little(f);
		return bytes;
	}

	template <std::floating_point T>
	ssize_t read(T& f)
	{
		auto bytes = io.read_data(reinterpret_cast<char*>(&f), sizeof(T));
		//f = to_little(f);
		return bytes;
	}

	ssize_t read(std::string& f)
	{
		// FIXME: varint
		uint8_t size;
		auto bytes = read(size);

		f.resize(size);
		bytes += io.read_data(f.data(), size);
		return bytes;
	}

	template <custom_read<serialized_io<I>> T>
	ssize_t read(T& f)
	{
		return f.read(*this);
	}

	template <container T>
	ssize_t read(T& f)
	{
		return io.read_data(reinterpret_cast<char*>(f.data()), f.size() * sizeof(typename T::value_type));
	}

	template <class T>
	ssize_t read(T& f)
	{
		ssize_t bytes = 0;
		boost::pfr::for_each_field(f, [this, &bytes](auto& field) {
			bytes += read(field);
		});
		return bytes;
	}

	template <std::integral T>
	ssize_t write(const T f)
	{
		auto x = to_little(f);
		return io.write_data(reinterpret_cast<const char*>(&x), sizeof(T));
	}

	template <std::floating_point T>
	ssize_t write(const T f)
	{
		// FIXME: endianness
		return io.write_data(reinterpret_cast<const char*>(&f), sizeof(T));
	}

	ssize_t write(const std::string f)
	{
		// FIXME: varint
		auto bytes = write(uint8_t(f.size()));
		bytes += io.write_data(f.c_str(), f.size());
		return bytes;
	}

	template <container T>
	ssize_t write(const T f)
	{
		return io.write_data(reinterpret_cast<const char*>(f.data()), f.size() * sizeof(typename T::value_type));
	}

	template <custom_write<serialized_io<I>> T>
	ssize_t write(T& f)
	{
		return f.write(*this);
	}

	template <class T>
	ssize_t write(const T f)
	{
		ssize_t bytes = 0;
		boost::pfr::for_each_field(f, [this, &bytes](auto field) {
			bytes += write(field);
		});
		return bytes;
	}
};

}