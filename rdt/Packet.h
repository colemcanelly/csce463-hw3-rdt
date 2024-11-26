/*
* Cole McAnelly
* CSCE 463 - Dist. Network Systems
* Fall 2024
*/

#pragma once

#include "pch.h"


class Packet
{
	size_t len;
	std::byte* bytes;

	Packet(size_t _len) : len(_len), bytes(new std::byte[_len](std::byte{ 0 })) {}
	Packet(size_t _len, void* data) : len(_len), bytes((std::byte*)data) { data = nullptr; }

public:
	Packet(Packet&) = delete;
	Packet() : len(0), bytes(nullptr) {}
	
	Packet(Packet&& other) noexcept : len(other.len), bytes(other.bytes) {
		other.len = 0;
		other.bytes = nullptr;
	}
	
	~Packet() { if (bytes) delete[] bytes; bytes = nullptr; }

	Packet& operator=(Packet&& other) noexcept {
		if (this != &other) {
			bytes = other.bytes;
			len = other.len;

			other.bytes = nullptr;
			other.len = 0;
		}
		return *this;
	}

	inline static std::atomic_uint32_t seq = 0;


	constexpr const std::byte* data() const { return bytes; }
	constexpr const size_t size() const { return len; }

	constexpr const bool is_syn() const { return ((net::SenderDataHeader*)data())->flags.SYN == 0x1; }
	constexpr const bool is_fin() const { return ((net::SenderDataHeader*)data())->flags.FIN == 0x1; }
	constexpr const uint32_t get_seq() const { return ((net::SenderDataHeader*)data())->seq; }
	inline const bool ack_flags_match(const net::ReceiverHeader ack) const {
		net::SenderDataHeader* head = (net::SenderDataHeader*)data();
		return ack.flags.ACK == 0x1
			&& ack.flags.SYN == head->flags.SYN
			&& ack.flags.FIN == head->flags.FIN;
	}

	enum class MatchAck { RandomACK, DuplicateACK, SynACK, FinACK, DataACK };

	inline const MatchAck ack_matches(const net::ReceiverHeader ack) const {
		if (ack.get_seq() == get_seq()) {
			if (is_syn() && ack.is_syn_ack()) return MatchAck::SynACK;
			if (is_fin() && ack.is_fin_ack()) return MatchAck::FinACK;
			else return MatchAck::DuplicateACK;
		}
		if (ack.get_seq() > get_seq()) return MatchAck::DataACK;
		return MatchAck::RandomACK;
	}

	static Packet syn_from(const net::LinkProperties& _link) {
		return Packet(sizeof(net::SenderSynHeader), new net::SenderSynHeader(seq.load(), _link, net::PacketType::Syn));
	};

	static Packet as_fin() {
		return Packet(sizeof(net::SenderDataHeader), new net::SenderDataHeader(seq.load(), net::PacketType::Fin));
	}

	static Packet data_from(const std::byte* ptr, size_t len) {
		auto p = Packet(sizeof(net::SenderDataHeader) + len);

		new((void*)p.data()) net::SenderDataHeader(seq++);
		memcpy(&((net::SenderDataHeader*)p.data())[1], ptr, len);

		return p;
	}

	//static Packet data_from();
};
