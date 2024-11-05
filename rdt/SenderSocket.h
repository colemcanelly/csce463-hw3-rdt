/*
* Cole McAnelly
* CSCE 463 - Dist. Network Systems
* Fall 2024
*/


#pragma once

#include "Udp.h"
#include "Packet.h"


class SenderSocket: Udp
{

	microseconds rto;
	Time::time_point socket_opened;

	SenderSocket(const std::string& host_ip, const uint16_t port) : Udp(host_ip, port), rto(1s), socket_opened(Time::now()) {}

public:
	static constexpr size_t MAX_PKT_SIZE = (1500 - 28);
	static constexpr size_t MAX_ATTEMPTS = 5;
	//~SenderSocket() { this->close(); }


	static SenderSocket open(const std::string& host, const uint16_t port, const net::LinkProperties& link) try {
		static constexpr size_t MAX_SYN_ATTEMPTS = 3;


		SenderSocket ss(host, port);
		ss.attempt_send(Packet::syn_from(link), MAX_SYN_ATTEMPTS);

		return ss;
	}
	catch (const WinSock::Error& _) { throw InvalidName(); }

	void send(const std::byte* ptr, size_t len) { return; }

	void close() { this->attempt_send(Packet::as_fin(), MAX_ATTEMPTS); }



	// +-+-+-+-+-+-+-+-+-+-+-+-+-+ Helper Methods +-+-+-+-+-+-+-+-+-+-+-+-+-+
private:
	net::ReceiverHeader attempt_send(const Packet& packet, const size_t attempts);

	constexpr void print_send(const Packet& p, int i, size_t attempts) const;
	constexpr void print_recv(const Packet& p, const net::ReceiverHeader& ack) const;


	// +-+-+-+-+-+-+-+-+-+-+-+-+-+ Error classes +-+-+-+-+-+-+-+-+-+-+-+-+-+
public:
	struct Error : public std::runtime_error {
		explicit Error(uint8_t code) : std::runtime_error("connect failed with status " + std::to_string(code)) {}
	};

	// (1) second call to ss.Open() without closing connection
	struct AlreadyConnected : public SenderSocket::Error { explicit AlreadyConnected() : Error(1) {} };
	// (2) call to ss.Send()/Close() without ss.Open()
	struct NotConnected : public SenderSocket::Error { explicit NotConnected() : Error(2) {} };
	// (3) ss.Open() with targetHost that has no DNS entry 
	struct InvalidName : public SenderSocket::Error { explicit  InvalidName() : Error(3) {} };
	// (4) sendto() failed in kernel
	struct FailedSend : public SenderSocket::Error { explicit FailedSend() : Error(4) {} };
	// (5) timeout after all retx attempts are exhausted
	struct Timeout : public SenderSocket::Error { explicit Timeout() : Error(5) {} };
	// (6) recvfrom() failed in kernel
	struct FailedRecv : public SenderSocket::Error { explicit FailedRecv() : Error(6) {} };
};

