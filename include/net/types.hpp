#pragma once

#include <array>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <span>
#include <string>
#include <sys/types.h>
#include <vector>

#include "util/bitset.hpp"
#include "util/endian.hpp"
#include "version.hpp"

namespace net {
namespace packet {

using packet_flag = struct { };

template <class T>
concept packet = requires(T x) { x.packet_id; };

template <class T>
concept compressed_packet = requires(T x) { packet<T> && x.compressed; };

template <class T>
concept netmodule = requires(T x) { x.module_id; };

struct nstring : std::string {
	enum : uint8_t {
		Literal,
		Formattable,
		LocalizationKey,
	} mode;

	std::string s;

	nstring(std::string&& s)
	    : mode(Literal)
	    , s(s)
	{
	}

	nstring(std::string& s)
	    : mode(Literal)
	    , s(s)
	{
	}
};

struct rgb {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

struct death_reason {
    enum class reason : int {
		Player = 0,
		NPC,
		Projectile,
		Other,
		ProjectileType,
		Item,
		ItemPrefix,
		Custom,
	};

	bitset<8> reasons;
	
	int16_t player_index;
	int16_t npc_index;
	int16_t projectile_index;
	uint8_t other_index;
	int16_t projectile_type;
	int16_t item_type;
	uint8_t item_prefix;
	std::string custom;
};

void encode_field(std::vector<uint8_t>& data, const std::string& f)
{
	auto bytes = reinterpret_cast<uint8_t const*>(f.data());
	data.insert(data.end(), uint8_t(f.size()));
	data.insert(data.end(), bytes, bytes + f.size());
}

template <std::integral T>
void encode_field(std::vector<uint8_t>& data, T f)
{
	auto fixed = to_little(f);
	auto* p = reinterpret_cast<uint8_t const*>(&fixed);
	data.insert(data.end(), p, p + sizeof(T));
}

template <std::floating_point T>
void encode_field(std::vector<uint8_t>& data, T f)
{
	auto x = reinterpret_cast<uint8_t*>(&f);
	data.insert(data.end(), x, x + sizeof(T));
}

void encode_field(std::vector<uint8_t>& data, nstring& s)
{
	data.insert(data.end(), uint8_t(s.mode));
	encode_field(data, s.s);
}

void encode_field(std::vector<uint8_t>& data, const rgb& f)
{
	encode_field(data, f.r);
	encode_field(data, f.g);
	encode_field(data, f.b);
}

template <typename T, std::size_t sz>
void encode_field(std::vector<uint8_t>& data, std::array<T, sz>& f)
{
	for (auto& e : f) {
		encode_field(data, e);
	}
}

template <typename T>
void encode_field(std::vector<uint8_t>& data, std::vector<T> &f)
{
	for (auto& e : f) {
		encode_field(data, e);
	}
}

template <typename T>
void encode_field(std::vector<uint8_t>& data, std::span<T> &f)
{
	for (auto& e : f) {
		encode_field(data, e);
	}
}

template <unsigned int bits>
void encode_field(std::vector<uint8_t>& data, bitset<bits> &f)
{
	for (auto& e : f) {
		encode_field(data, e);
	}
}

void encode_field(std::vector<uint8_t>& data, death_reason &f)
{
	encode_field(data, f.reasons);
	if (f.reasons[death_reason::reason::Player]) {
		encode_field(data, f.player_index);
	}
	if (f.reasons[death_reason::reason::NPC]) {
		encode_field(data, f.npc_index);
	}
	if (f.reasons[death_reason::reason::Projectile]) {
		encode_field(data, f.projectile_index);
	}
	if (f.reasons[death_reason::reason::Other]) {
		encode_field(data, f.other_index);
	}
	if (f.reasons[death_reason::reason::ProjectileType]) {
		encode_field(data, f.projectile_type);
	}
	if (f.reasons[death_reason::reason::Item]) {
		encode_field(data, f.item_type);
	}
	if (f.reasons[death_reason::reason::ItemPrefix]) {
		encode_field(data, f.item_prefix);
	}
	if (f.reasons[death_reason::reason::Custom]) {
		encode_field(data, f.custom);
	}
}

ssize_t decode_field(std::span<uint8_t> data, std::string& f)
{
	assert(data.size() > 0);
	uint8_t size = uint8_t(data[0]); // FIXME: varint

	assert(data.size() >= std::size_t(size + 1));

	f.resize(size);
	f.assign(reinterpret_cast<const char*>(data.data() + 1 /* FIXME: varint*/), size);
	return size + 1;
}

template <std::integral T>
ssize_t decode_field(std::span<uint8_t> data, T& f)
{
	assert(data.size() >= sizeof(T));
	std::memcpy(&f, data.data(), sizeof(T));
	f = to_little(f);
	return sizeof(T);
}


ssize_t decode_field(std::span<uint8_t> data, std::vector<uint8_t>& f)
{	
	f.insert(f.begin(), data.begin(), data.end());
	return f.size();
}


template <typename T, std::size_t sz>
ssize_t decode_field(std::span<uint8_t> data, std::array<T, sz>& f)
{
	assert(data.size() >= sizeof(T) * sz);

	ssize_t offset = 0;
	for (auto& e : f) {
		offset += decode_field(data.subspan(offset), e);
	}

	return offset;
}

ssize_t decode_field(std::span<uint8_t> data, death_reason &f)
{
	int offset = 0;
	offset += decode_field(data, f.reasons);
	if (f.reasons[0]) {
		offset += decode_field(data.subspan(offset), f.player_index);
	}
	if (f.reasons[1]) {
		offset += decode_field(data.subspan(offset), f.npc_index);
	}
	if (f.reasons[2]) {
		offset += decode_field(data.subspan(offset), f.projectile_index);
	}
	if (f.reasons[3]) {
		offset += decode_field(data.subspan(offset), f.other_index);
	}
	if (f.reasons[4]) {
		offset += decode_field(data.subspan(offset), f.projectile_type);
	}
	if (f.reasons[5]) {
		offset += decode_field(data.subspan(offset), f.item_type);
	}
	if (f.reasons[6]) {
		offset += decode_field(data.subspan(offset), f.item_prefix);
	}
	if (f.reasons[7]) {
		offset += decode_field(data.subspan(offset), f.custom);
	}

	return offset;
}
ssize_t decode_field(std::span<uint8_t> data, rgb& f)
{
	assert(data.size() >= 3);

	f.r = uint8_t(data[0]);
	f.g = uint8_t(data[1]);
	f.b = uint8_t(data[2]);
	return 3;
}

struct conn_request {
	static constexpr uint8_t packet_id = 1;

	std::string ver;
};

struct disconnect {
	static constexpr uint8_t packet_id = 2;

	nstring reason;
};

struct accept {
	static constexpr uint8_t packet_id = 3;

	std::uint8_t client_id;
	uint8_t _ = 0;
};

struct player_info {
	static constexpr uint8_t packet_id = 4;

	uint8_t client_id;
	uint8_t skin_variant;
	uint8_t hair_variant;
	std::string name;
	uint8_t hair_dye;
	uint16_t hide_visuals;
	uint8_t hide_misc;
	rgb hair_color;
	rgb skin_color;
	rgb eye_color;
	rgb shirt_color;
	rgb undershirt_color;
	rgb pants_color;
	rgb shoes_color;
	uint8_t difficulty;
	uint8_t torches;
	uint8_t permanent_buffs;
};

struct player_inventory_slot {
	static constexpr uint8_t packet_id = 5;

	uint8_t client_id;
	int16_t slot_id;
	int16_t amount;
	uint8_t prefix;
	int16_t item_id;
};

struct request_world_data {
	static constexpr uint8_t packet_id = 6;
};

struct world_info {
	static constexpr uint8_t packet_id = 7;

	// FIXME: i'm not sure this struct is correct
	int32_t time;
	uint8_t day_info;
	uint8_t moon_phase;
	int16_t max_tiles_x;
	int16_t max_tiles_y;
	int16_t spawn_x;
	int16_t spawn_y;
	int16_t world_surface;
	int16_t rock_layer;
	int32_t world_id;
	std::string world_name;
	uint8_t game_mode;
	std::array<uint8_t, 16> world_unique_id;
	std::array<int32_t, 2> world_generator_version;
	uint8_t moon_type;
	std::array<uint8_t, 13> backgrounds;
	uint8_t ice_back_style;
	uint8_t jungle_back_style;
	uint8_t hell_back_style;
	float wind_speed_set;
	uint8_t cloud_number;
	std::array<int32_t, 3> trees;
	std::array<uint8_t, 4> tree_styles;
	std::array<int32_t, 3> cave_backs;
	std::array<uint8_t, 4> cave_back_styles;
	std::array<uint8_t, 13> tree_top_styles;
	float rain;
	bitset<104> flags1;
	std::array<int16_t, 7> ore_tiers_tiles;
	int8_t invasion_type;
	uint64_t lobby_id;
	float sandstorm_severity;
};

struct request_tiles_at {
	static constexpr uint8_t packet_id = 8;
	int32_t spawn_x;
	int32_t spawn_y;
};

struct statusbar_text {
	static constexpr uint8_t packet_id = 9;

	int32_t status_max;
	nstring status_text;
	uint8_t server_flags;
};

struct send_tile_data {
	static constexpr uint8_t packet_id = 10;
	static constexpr packet_flag compressed {};

	//FIXME: implement
};

struct spawn_player {
	static constexpr uint8_t packet_id = 12;

	uint8_t client_id;

	int16_t spawn_x;
	int16_t spawn_y;
	int32_t respawn_timer;
	uint16_t pve_death_count;
	uint16_t pvp_death_count;
	uint8_t spwan_context;
};

struct player_health {
	static constexpr uint8_t packet_id = 16;

	uint8_t client_id;
	uint16_t current;
	uint16_t max;
};

struct request_password {
	static constexpr uint8_t packet_id = 37;
};

struct send_password {
	static constexpr uint8_t packet_id = 38;

	std::string password;
};

struct player_mana {
	static constexpr uint8_t packet_id = 42;

	uint8_t client_id;
	uint16_t current;
	uint16_t max;
};

struct initial_spawn_player {
	static constexpr uint8_t packet_id = 49;
};

struct update_player_buffs {
	static constexpr uint8_t packet_id = 50;

	uint8_t client_id;
	std::array<uint16_t, max_buffs> buffs;
};

struct world_evil {
	static constexpr uint8_t packet_id = 57;

	uint8_t good;
	uint8_t evil;
	uint8_t blood;
};

struct player_uuid {
	static constexpr uint8_t packet_id = 68;

	std::string uuid;
};

struct npc_kill_count {
	static constexpr uint8_t packet_id = 83;

	uint16_t npc_type;
	uint32_t npc_kill_count;
};

struct tower_powers {
	static constexpr uint8_t packet_id = 101;
	uint16_t solar, nebula, vertex, stardust;
};

struct monster_types {
	static constexpr uint8_t packet_id = 136;

	std::array<std::array<uint16_t, 3>, 2> types;
};

struct connection_completed {
	static constexpr uint8_t packet_id = 129;
};

struct player_zones {
	static constexpr uint8_t packet_id = 36;

	uint8_t client_id;
	uint32_t zone_flags;
	uint8_t zone_flags2;
};

struct player_loadout {
	static constexpr uint8_t packet_id = 147;

	uint8_t client_id;
	uint8_t index;
	uint16_t hide_accessory;
};

struct damage_player {
	static constexpr uint8_t packet_id = 117;

    uint8_t client_id;
	death_reason reason;
    int16_t damage;
    uint8_t hit_direction;
    uint8_t ty;
    int8_t cooldown_counter;
};

template <uint8_t id>
struct raw {
	static constexpr uint8_t packet_id = id;
	std::vector<uint8_t> data;
};

inline nstring operator""_ns(const char* str, std::size_t)
{
	return nstring(str);
}

}
}
