/*
* Cole McAnelly
* CSCE 463 - Dist. Network Systems
* Fall 2024
*/


#pragma once
#pragma pack(push, 1)	 // Set packing alignment to 1 byte

#include "pch.h"


#define FORWARD_PATH  0 
#define RETURN_PATH  1 

namespace net {

#define MAGIC_PROTOCOL 0x8311AA 
	struct Flags {
		uint32_t   reserved : 5;   // must be zero  
		uint32_t   SYN : 1;
		uint32_t   ACK : 1;
		uint32_t   FIN : 1;
		uint32_t   magic : 24;

		Flags() { memset(this, 0, sizeof(*this)); magic = MAGIC_PROTOCOL; }
		std::string debug() const {
			std::string ret = "";
			if (this->SYN == 0x1) ret.append(" SYN ");
			if (this->ACK == 0x1) ret.append(" ACK ");
			if (this->FIN == 0x1) ret.append(" FIN ");
			return ret;
		}
	};

	struct LinkProperties {
		// transfer parameters 
		float   rtt;  // propagation RTT (in sec) 
		float   speed;  // bottleneck bandwidth (in bits/sec) 
		float   pLoss[2]; // probability of loss in each direction 
		uint32_t bufferSize; // buffer size of emulated routers (in packets) 

		LinkProperties() { memset(this, 0, sizeof(*this)); }
		LinkProperties(float _rtt, float _speed, float forward_loss, float reverse_loss, uint32_t buf)
			: rtt(_rtt)
			, speed(1e6 * _speed)
			, pLoss{ forward_loss, reverse_loss }
			, bufferSize(buf)
		{}

		std::string debug() const {
			return "Link: { Buffer " + std::to_string(bufferSize) +
				", RTT " + std::to_string(rtt) + " sec" \
				", loss " + std::to_string(pLoss[0]) + " / " + std::to_string(pLoss[1]) +
				", link " + std::to_string(speed / 1e6) + " Mbps" \
				" }";
		}
	};

	enum class PacketType : uint8_t { None, Syn, Fin };

	struct SenderDataHeader {
		Flags   flags;
		uint32_t   seq;  // must begin from 0 

		SenderDataHeader(uint32_t _seq, PacketType t = PacketType::None) : flags(), seq(_seq) {
			if (t == PacketType::Syn) this->flags.SYN = 0x1;
			if (t == PacketType::Fin) this->flags.FIN = 0x1;
		}
		std::string debug() const { return "Packet #" + std::to_string(seq) + " (" + flags.debug() + ")"; }
	};


	struct SenderSynHeader {
		SenderDataHeader sdh;
		LinkProperties  lp;

		SenderSynHeader(uint32_t _seq, const LinkProperties& _link, PacketType t = PacketType::None) : lp(_link), sdh(_seq, t) {}
		std::string debug() const { return sdh.debug() + "\n\t" + lp.debug(); }
	};

	struct ReceiverHeader {
		Flags   flags;
		uint32_t recvWnd;	// receiver window for flow control (in pkts) 
		uint32_t ackSeq;		// ack value = next expected sequence 

		constexpr const uint32_t get_seq() const { return this->ackSeq; }
		constexpr const uint32_t get_window() const { return this->recvWnd; }
		constexpr bool is_syn_ack() const { return (this->flags.ACK == 0x1) && (this->flags.SYN == 0x1); }
		constexpr bool is_fin_ack() const { return (this->flags.ACK == 0x1) && (this->flags.FIN == 0x1); }
	};
}

#pragma pack(pop)	// Restore default packing alignment