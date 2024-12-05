/*
* Cole McAnelly
* CSCE 463 - Dist. Network Systems
* Fall 2024
*/


#pragma once

#include <future>

#include "Udp.h"
#include "SenderSocketStats.h"
#include "Packet.h"

#include "RingBuffer.h"


class SenderSocket : Udp
{

	struct PacketMetadata {
		Time::time_point time_sent;
		Packet pkt;

		PacketMetadata() = default;
		PacketMetadata(Packet&& p) : pkt(std::move(p)), time_sent(Time::now()) {}
	};

	using Stats = socket_helpers::Stats;
	using Timeouts = socket_helpers::Timeouts;


	const Time::time_point socket_opened;					// immutable (thread safe)
	const uint32_t window_size;								// immutable (thread safe)

	std::unique_ptr<Stats> stats;							// atomic stats		 (thread safe)
	std::unique_ptr<Timeouts> timeouts;						// atomic timeouts	 (thread safe)

	std::unique_ptr<RingBuffer<Packet>> send_queue;			// RingBuffer  (thread safe)
	std::unique_ptr<RingBuffer<PacketMetadata>> recv_queue;	// RingBuffer  (thread safe)

	HANDLE sent_data_event;

	std::jthread stats_thread;
	std::jthread sender_thread;
	std::jthread receiver_thread;

	SenderSocket(const std::string& host_ip, const uint16_t port, uint32_t window, float rto)
		: Udp(host_ip, port)
		, socket_opened(Time::now())
		, window_size(window)
		, stats(std::make_unique<Stats>())
		, timeouts(std::make_unique<Timeouts>(rto))
		, sent_data_event(CreateSemaphore(NULL, 0, 1, NULL))
		, send_queue(std::make_unique<RingBuffer<Packet>>(window, 1))
		, recv_queue(std::make_unique<RingBuffer<PacketMetadata>>(window))
		, sender_thread([this](std::stop_token st) { this->sender(st); })
		, receiver_thread([this](std::stop_token st) { this->receiver(st); })
		, stats_thread([this](std::stop_token st) { this->print_stats(st); })
	{}

public:
	static constexpr size_t MAX_PKT_SIZE = (1500 - 28);
	//static constexpr size_t MAX_PKT_SIZE = (9000 - 28);
	static constexpr size_t MAX_ATTEMPTS = 50;
	static constexpr microseconds STATS_FREQUENCY = 2s;

	~SenderSocket() {
		stats_thread.request_stop();
		sender_thread.request_stop();
		receiver_thread.request_stop();
		
		send_queue->clear();
		send_queue->release_space(); // unblock send

		CloseHandle(sent_data_event);
	}

	static std::unique_ptr<SenderSocket> open(const std::string& host, const uint16_t port, const net::LinkProperties& link, uint32_t w) try {
		static constexpr size_t MAX_SYN_ATTEMPTS = 3;

		auto ss = std::unique_ptr<SenderSocket>(new SenderSocket(host, port, w, __max(1, 2 * link.rtt)));
		ss->send_queue->push(Packet::syn_from(link));

		return ss;
	}
	catch (const WinSock::Error& _) { throw InvalidName(); }

	void send(const std::byte* ptr, size_t len) { this->send_queue->push(Packet::data_from(ptr, len)); }

	uint32_t close() {
		stats_thread.request_stop();
		send_queue->push(Packet::as_fin());

		sender_thread.request_stop();
		receiver_thread.request_stop();

		sender_thread.join();
		receiver_thread.join();

		return this->stats->final_checksum.load();
	}

	constexpr float estimated_rtt() const { return to_float(this->timeouts->est_rtt.load()); }

	// +-+-+-+-+-+-+-+-+-+-+-+-+-+ Helper Methods +-+-+-+-+-+-+-+-+-+-+-+-+-+
private:
	void print_stats(std::stop_token st) const;
	void sender(std::stop_token st);
	void receiver(std::stop_token st);

	//net::ReceiverHeader attempt_send(const Packet& packet, const size_t attempts = MAX_ATTEMPTS);

	void print_send(const Packet& p, int i, size_t attempts) const;
	void print_recv(const net::ReceiverHeader& ack) const;

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