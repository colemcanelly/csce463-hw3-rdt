/*
* Cole McAnelly
* CSCE 463 - Dist. Network Systems
* Fall 2024
*/

#pragma once

#include "pch.h"


class Packet
{
	const size_t len;
	const std::byte* bytes;

	inline static uint32_t number = 0;

	Packet(size_t _len) : len(_len), bytes(new std::byte[_len]) {}
	Packet(size_t _len, void* data) : len(_len), bytes((std::byte*)data) { data = nullptr; }

public:
	~Packet() { delete[] bytes; }

	constexpr const std::byte* data() const { return bytes; }
	constexpr const size_t size() const { return len; }

	constexpr const bool is_syn() const { return ((net::SenderDataHeader*)data())->flags.SYN == 0x1; }
	constexpr const bool is_fin() const { return ((net::SenderDataHeader*)data())->flags.FIN == 0x1; }
	constexpr const uint32_t get_seq() const { return ((net::SenderDataHeader*)data())->seq; }

	static Packet syn_from(const net::LinkProperties& _link) {
		return Packet(sizeof(net::SenderSynHeader), new net::SenderSynHeader(number, _link, net::PacketType::Syn));
	};

	static Packet as_fin() {
		return Packet(sizeof(net::SenderDataHeader), new net::SenderDataHeader(number, net::PacketType::Fin));
	}

	//static Packet data_from();
};
