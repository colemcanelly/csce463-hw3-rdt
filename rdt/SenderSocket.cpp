/*
* Cole McAnelly
* CSCE 463 - Dist. Network Systems
* Fall 2024
*/


#include "pch.h"
#include "SenderSocket.h"


//#define DEBUG
#ifdef DEBUG
#define LOG(print_fn) print_fn
#else
#define LOG(print_fn)
#endif


// Blast packets to the server as quickly as possible loll
void SenderSocket::sender(std::stop_token st) {
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

	auto it = pending_packets->iter();
	while (!st.stop_requested() || it.has_next())
	{
		auto next_unsent = it.next_mut();
		this->udt_send(next_unsent->pkt);
		next_unsent->time_sent = Time::now();
		next_unsent->timer_expire = Time::now() + timeouts->rto.load();
		LOG(print_send(next_unsent->pkt, 1, MAX_ATTEMPTS));
	}
	LOG(printf("Sender Done\n"));
}

void SenderSocket::receiver(std::stop_token st) try {
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

	int dup_ack = 0;
	int n_retx = 0;

	Time::time_point base_timer_expire = Time::now() + timeouts->rto.load();


	auto retx = [this, &n_retx, &dup_ack, &base_timer_expire]() {
		if (++n_retx >= MAX_ATTEMPTS) throw SenderSocket::Timeout();

		auto base = pending_packets->peek_mut();
		this->udt_send(base->pkt);
		base->time_sent = Time::now();
		base_timer_expire = Time::now() + timeouts->rto.load();
		LOG(print_send(base->pkt, n_retx + 1, MAX_ATTEMPTS));
	};



	size_t send_base = 0;
	size_t last_released = 0;
	size_t eff_window = 0;

	while (!st.stop_requested() || !pending_packets->empty())
	{
		std::optional<microseconds> timeout = {};
		if (!pending_packets->empty()) {
			timeout = std::chrono::duration_cast<microseconds>(base_timer_expire - Time::now());
			LOG(printf("New Timeout: %.3f Sec\n", to_float(timeout.value())));
		}
		else LOG(printf("Waiting Infinitely\n"));

		bool not_timeout = false;
		auto res = this->udt_recv(timeout, pending_packets->not_empty_smph(), not_timeout);
		if (!res && not_timeout) continue;	// not_empty_smph changed
		if (!res) {							// timeout
			stats->n_timeouts++;
			retx();
			continue;
		}

		net::ReceiverHeader ack = res.value();
		uint64_t bytes_acked = 0;

		switch (pending_packets->peek().pkt.ack_matches(ack))
		{
		case Packet::MatchAck::SynACK:
			LOG(print_recv(ack));
			timeouts->recalculate_syn(time_elapsed<microseconds>(pending_packets->peek().time_sent));
			bytes_acked += pending_packets->pop().pkt.size();
			last_released = __min(this->window_size, ack.get_window());
			pending_packets->release(last_released);
			break;
		case Packet::MatchAck::DataACK: {
			LOG(print_recv(ack));
			size_t advance = ack.get_seq() - send_base;

			if (n_retx == 0)
				timeouts->recalculate(time_elapsed<microseconds>(pending_packets->at(advance - 1).time_sent));
			
			bytes_acked += pending_packets->fold_remove_n<uint64_t>(
				advance,
				[](const auto& p, uint64_t byts) { return byts + p.pkt.size(); }
			);

			send_base = ack.get_seq();

			eff_window = __min(this->window_size, ack.get_window());
			size_t release = send_base + eff_window - last_released;
			pending_packets->release(release);
			stats->effective_window.store(eff_window);
			last_released += release;
			break;
		}
		case Packet::MatchAck::FinACK:
			print_recv(ack);
			stats->final_checksum.store(ack.get_window());
			bytes_acked += pending_packets->fold_remove_n<uint64_t>(
				pending_packets->size(),
				[](const auto& p, uint64_t byts) { return byts + p.pkt.size(); }
			);
			break;
		case Packet::MatchAck::DuplicateACK:
			if (++dup_ack == 3) {						// fast retransmit
				stats->n_fast_retransmits++;
				retx();
			}
			[[fallthrough]];
		case Packet::MatchAck::RandomACK:
		default: continue;
		}

		base_timer_expire = Time::now() + timeouts->rto.load();
		stats->sender_base.store(send_base);
		stats->bytes_acked += bytes_acked;
		dup_ack = 0;
		n_retx = 0;
	}

	LOG(printf("Receiver Done\n"));
}
catch (const std::runtime_error& e) {
	printf("[ %.3f] <-- %s\n", to_float(time_elapsed<microseconds>(socket_opened)), e.what());
	exit(0);
}


void SenderSocket::print_stats(std::stop_token st) const {
	uint32_t sender_base = stats->sender_base.load();
	static constexpr uint32_t CONSUMPTION_SPEED_MULTIPLE = 8 * (MAX_PKT_SIZE - sizeof(net::SenderDataHeader));
	while (!st.stop_requested()) {
		std::this_thread::sleep_for(STATS_FREQUENCY);
		if (st.stop_requested()) break;
		uint16_t seconds_elapsed = time_elapsed<seconds>(socket_opened).count();

		printf("[%3hu] B  %6u (%6.1f MB) N  %6u T %3u F %3u W %5u S %7.3f Mbps RTT %.3f\n"
			, seconds_elapsed
			, stats->sender_base.load()
			, unit_cast<std::mega>((double)stats->bytes_acked.load())
			, stats->sender_base.load() + stats->effective_window.load()
			, stats->n_timeouts.load()
			, stats->n_fast_retransmits.load()
			, stats->effective_window.load()
			, (double((stats->sender_base.load() - sender_base) * CONSUMPTION_SPEED_MULTIPLE) / 2) / 1e6
			, this->estimated_rtt()
		);
		sender_base = stats->sender_base.load();
	}
	LOG(printf("Stats Done\n"));
}

void SenderSocket::print_send(const Packet& p, int i, size_t attempts) const {
	if (p.is_syn())
		printf("[ %.3f] --> SYN %u (attempt %d of %llu, RTO %.3f) to %s\n",
			to_float(time_elapsed<microseconds>(socket_opened)),
			p.get_seq(), i, attempts, timeouts->get(float()), inet_ntoa(server.sin_addr)
		);
	else
		printf("[ %.3f] --> %s %u (attempt %d of %llu, RTO %.3f)\n",
			to_float(time_elapsed<microseconds>(socket_opened)),
			p.is_fin() ? "FIN" : "data",
			p.get_seq(), i, attempts, timeouts->get(float())
		);
}

void SenderSocket::print_recv(const net::ReceiverHeader& ack) const {
	if (ack.is_syn_ack())
		printf("[ %.3f] <-- SYN-ACK %u window %u; setting initial RTO to %.3f\n",
			to_float(time_elapsed<microseconds>(socket_opened)),
			ack.get_seq(), ack.get_window(), timeouts->get(float())
		);
	else if (ack.is_fin_ack())
		printf("[ %.3f] <-- FIN-ACK %u window %X\n",
			to_float(time_elapsed<microseconds>(socket_opened)),
			ack.get_seq(), ack.get_window()
		);
	else
		printf("[ %.3f] <-- ACK %u window %u\n",
			to_float(time_elapsed<microseconds>(socket_opened)),
			ack.get_seq(), ack.get_window()
		);
}