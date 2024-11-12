/*
* Cole McAnelly
* CSCE 463 - Dist. Network Systems
* Fall 2024
*/


#pragma once

#include "Udp.h"
#include "Packet.h"


class SenderSocket : Udp
{
	struct Stats {
		std::atomic_uint64_t bytes_acked = { 0 };
		std::atomic_uint32_t n_timeouts = {0};
		std::atomic_uint32_t n_fast_retransmits = {0};
	};

	// timing
	microseconds rto;
	microseconds dev_rtt;
	microseconds est_rtt;

	const Time::time_point socket_opened;
	std::atomic_uint32_t window_size;

	Stats stats;
	std::jthread stats_thread;

	SenderSocket(const std::string& host_ip, const uint16_t port)
		: Udp(host_ip, port)
		, rto(1s)
		, dev_rtt(0s)
		, est_rtt(0s)
		, socket_opened(Time::now())
		, stats_thread([this](std::stop_token st) { this->print_stats(st); })
	{}



public:
	static constexpr size_t MAX_PKT_SIZE = (1500 - 28);
	static constexpr size_t MAX_ATTEMPTS = 5;
	//~SenderSocket() { this->close(); }

	static std::unique_ptr<SenderSocket> open(const std::string& host, const uint16_t port, const net::LinkProperties& link, uint32_t w) try {
		static constexpr size_t MAX_SYN_ATTEMPTS = 3;

		auto ss = std::unique_ptr<SenderSocket>(new SenderSocket(host, port));
		ss->rto = std::chrono::duration_cast<microseconds>(std::chrono::duration<float>(__max(1, 2 * link.rtt)));
		ss->window_size.store(w);
		ss->attempt_send(Packet::syn_from(link), MAX_SYN_ATTEMPTS);

		return ss;
	}
	catch (const WinSock::Error& _) { throw InvalidName(); }

	void send(const std::byte* ptr, size_t len) { this->attempt_send(Packet::data_from(ptr, len)); }

	uint32_t close() {
		stats_thread.request_stop();
		auto ack = this->attempt_send(Packet::as_fin());
		this->print_recv(ack);
		return ack.get_window();
	}

	void print_stats(std::stop_token st) const;
	constexpr float estimated_rtt() const { return to_float(this->est_rtt); }

	// +-+-+-+-+-+-+-+-+-+-+-+-+-+ Helper Methods +-+-+-+-+-+-+-+-+-+-+-+-+-+
private:
	net::ReceiverHeader attempt_send(const Packet& packet, const size_t attempts = MAX_ATTEMPTS);

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


	friend class std::unique_ptr<SenderSocket>;
};

namespace Calc {
	inline microseconds est_rtt(microseconds sample, microseconds curr_est, bool is_syn) {
		if (is_syn) return sample;
		constexpr float a = 0.125;
		float est = (1 - a) * to_float(curr_est) + a * to_float(sample);
		return std::chrono::duration_cast<microseconds>(std::chrono::duration<float>(est));
	}

	inline microseconds dev_rtt(microseconds sample, microseconds curr_dev, microseconds curr_est, bool is_syn) {
		if (is_syn) return 0us;
		constexpr float b = 0.125;
		float dev = (1 - b) * to_float(curr_dev) + b * abs(to_float(sample) - to_float(curr_est));
		return std::chrono::duration_cast<microseconds>(std::chrono::duration<float>(dev));
	}

	constexpr microseconds new_rto(microseconds sample, microseconds new_dev, microseconds new_est, bool is_syn) {
		if (is_syn) return sample * 3;
		return new_est + 4 * __max(10ms, new_dev);
	}
};

