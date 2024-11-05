/*
* Cole McAnelly
* CSCE 463 - Dist. Network Systems
* Fall 2024
*/


#include "pch.h"
#include "SenderSocket.h"


net::ReceiverHeader SenderSocket::attempt_send(const Packet& packet, const size_t attempts) try {
	for (int i = 1; i <= attempts; i++)
	{
		this->print_send(packet, i, attempts);

		Time::time_point packet_sent = Time::now();
		this->udt_send(packet);
	recv:
		auto res = this->udt_recv(to_timeval(rto));
		if (!res) continue;						// timeout
		auto duration = time_elapsed<microseconds>(packet_sent);
		rto -= duration;
		net::ReceiverHeader ack = res.value();
		if (
			(packet.is_syn() && !(ack.is_syn_ack()))
			|| (packet.is_fin() && !(ack.is_fin_ack()))
			|| packet.get_seq() != ack.get_seq()
			) goto recv;
		rto = duration * 3;
		this->print_recv(packet, ack);
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

constexpr void SenderSocket::print_send(const Packet& p, int i, size_t attempts) const {
	if (p.is_syn())
		printf("[ %.3f] --> SYN %u (attempt %d of %llu, RTO %.3f) to %s\n",
			to_float(time_elapsed<microseconds>(socket_opened)),
			p.get_seq(), i, attempts, to_float(rto), inet_ntoa(server.sin_addr)
		);
	if (p.is_fin())
		printf("[ %.3f] --> FIN %u (attempt %d of %llu, RTO %.3f)\n",
			to_float(time_elapsed<microseconds>(socket_opened)),
			p.get_seq(), i, attempts, to_float(rto)
		);
}

constexpr void SenderSocket::print_recv(const Packet& p, const net::ReceiverHeader& ack) const {
	if (p.is_syn())
		printf("[ %.3f] <-- SYN-ACK %u window %u; setting initial RTO to %.3f\n",
			to_float(time_elapsed<microseconds>(socket_opened)),
			ack.get_seq(), ack.get_window(), to_float(rto)
		);
	if (p.is_fin())
		printf("[ %.3f] <-- FIN-ACK %u window %u\n",
			to_float(time_elapsed<microseconds>(socket_opened)),
			ack.get_seq(), ack.get_window()
		);
}