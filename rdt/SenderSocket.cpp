/*
* Cole McAnelly
* CSCE 463 - Dist. Network Systems
* Fall 2024
*/


#include "pch.h"
#include "SenderSocket.h"


net::ReceiverHeader SenderSocket::attempt_send(const Packet& packet, const size_t attempts) try {
	int dup_ack = 0;
	for (int i = 1; i <= attempts; i++)
	{
		//this->print_send(packet, i, attempts);

		microseconds duration = 0s;
		Time::time_point packet_sent = Time::now();
		this->udt_send(packet);
	recv:
		auto res = this->udt_recv(to_timeval(rto - duration));
		if (!res) {				// timeout
			stats.n_timeouts++;
			continue;
		}
		net::ReceiverHeader ack = res.value();

		duration = time_elapsed<microseconds>(packet_sent);
		if (!packet.ack_flags_match(ack)) goto recv;
		if (!packet.ack_seq_correct(ack.get_seq())) {
			if (packet.get_seq() == ack.get_seq()) // dup ack
				if (++dup_ack == 3) {		// fast retransmit
					stats.n_fast_retransmits++;
					continue;
				}
			goto recv;
		}

		// Calulate new timeout values
		est_rtt = Calc::est_rtt(duration, est_rtt, packet.is_syn());
		dev_rtt = Calc::dev_rtt(duration, dev_rtt, est_rtt, packet.is_syn());
		rto = Calc::new_rto(duration, dev_rtt, est_rtt, packet.is_syn());
		
		//this->print_recv(ack);
		stats.bytes_acked += packet.size();
		return ack;
	}
	throw SenderSocket::Timeout();
}
catch (const Udp::SendToError& e) {
	printf("[ %.3f] --> %s\n", to_float(time_elapsed<microseconds>(socket_opened)), e.what());
	throw SenderSocket::FailedSend();
}
catch (const Udp::SelectError& e) {
	printf("[ %.3f] <-- %s\n", to_float(time_elapsed<microseconds>(socket_opened)), e.what());
	throw SenderSocket::FailedRecv();
}
catch (const Udp::RecvFromError& e) {
	printf("[ %.3f] <-- %s\n", to_float(time_elapsed<microseconds>(socket_opened)), e.what());
	throw SenderSocket::FailedRecv();
}

void SenderSocket::print_send(const Packet& p, int i, size_t attempts) const {
	if (p.is_syn())
		printf("[ %.3f] --> SYN %u (attempt %d of %llu, RTO %.3f) to %s\n",
			to_float(time_elapsed<microseconds>(socket_opened)),
			p.get_seq(), i, attempts, to_float(rto), inet_ntoa(server.sin_addr)
		);
	else if (p.is_fin())
		printf("[ %.3f] --> FIN %u (attempt %d of %llu, RTO %.3f)\n",
			to_float(time_elapsed<microseconds>(socket_opened)),
			p.get_seq(), i, attempts, to_float(rto)
		);
	else
		printf("[ %.3f] --> data %u (attempt %d of %llu, RTO %.3f)\n",
			to_float(time_elapsed<microseconds>(socket_opened)),
			p.get_seq(), i, attempts, to_float(rto)
		);
}

void SenderSocket::print_recv(const net::ReceiverHeader& ack) const {
	if (ack.is_syn_ack())
		printf("[ %.3f] <-- SYN-ACK %u window %u; setting initial RTO to %.3f\n",
			to_float(time_elapsed<microseconds>(socket_opened)),
			ack.get_seq(), ack.get_window(), to_float(rto)
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


void SenderSocket::print_stats(std::stop_token st) const {
	uint32_t sender_base = 0;
	static constexpr uint32_t CONSUMPTION_SPEED_MULTIPLE = 8 * (MAX_PKT_SIZE - sizeof(net::SenderDataHeader));
	while (!st.stop_requested()) {
		std::this_thread::sleep_for(2s);
		if (st.stop_requested()) break;
		uint16_t seconds_elapsed = time_elapsed<seconds>(socket_opened).count();

		printf("[%2hu] B   %4u (  %3.1f MB) N   %4u T %3u F %3u W %3u S %.3f Mbps RTT %.3f\n"
			, seconds_elapsed
			, Packet::seq.load()
			, unit_cast<std::mega>((double)stats.bytes_acked.load())
			, Packet::seq.load() + 1
			, stats.n_timeouts.load()
			, stats.n_fast_retransmits.load()
			, window_size.load()
			, (double((Packet::seq.load() - sender_base) * CONSUMPTION_SPEED_MULTIPLE) / 2) / 1e6
			, to_float(this->est_rtt)
		);
		sender_base = Packet::seq.load();
	}
}