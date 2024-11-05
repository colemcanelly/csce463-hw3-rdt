/*
* Cole McAnelly
* CSCE 463 - Dist. Network Systems
* Fall 2024
*/

#include "pch.h"
#include "Udp.h"


Udp::Udp(const std::string& host_ip, const uint16_t port)
	: sock(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))
	, server{0}
{
	if (sock == INVALID_SOCKET) throw WinSock::FatalError("couldn't create socket!");
	
	struct sockaddr_in local { 0 };
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = INADDR_ANY;
	local.sin_port = htons(0);

	int err = bind(this->sock, (struct sockaddr*)&local, sizeof(struct sockaddr_in));
	if (err == SOCKET_ERROR) throw WinSock::FatalError("Failed to bind");

	// init the server to connect to
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = WinSock::dns(host_ip);
}


int Udp::udt_send(const Packet& p) const {
	int ret = sendto(this->sock, (const char*)p.data(), p.size(), NULL, (struct sockaddr*)&this->server, sizeof(struct sockaddr_in));
	if (ret == SOCKET_ERROR) throw Udp::SendToError();
	return ret;
}

std::optional<net::ReceiverHeader> Udp::udt_recv(const struct timeval& timeout) const {
	auto start = Time::now();
	fd_set fd;
	FD_ZERO(&fd);
	FD_SET(this->sock, &fd);
	// clear the set 
	// add your socket to the set 
	int err = select(0, &fd, NULL, NULL, &timeout);
	if (err < 0) throw Udp::SelectError();
	if (err == 0) return {};

	struct sockaddr_in responder;
	int reponder_addr_size = sizeof(struct sockaddr_in);

	net::ReceiverHeader res;
	int num_bytes = recvfrom(this->sock, (char*)&res, sizeof(net::ReceiverHeader), NULL, (SOCKADDR*)&responder, &reponder_addr_size);

	if (num_bytes < 0) throw Udp::RecvFromError();
	if (num_bytes == 0) throw Udp::Error("connection closed");

	if (responder.sin_addr.s_addr != server.sin_addr.s_addr
		|| responder.sin_port != server.sin_port) throw Udp::Error("bogus reply"); // return false;

	//if (bytes.size() < sizeof(Packet::FixedDNSheader)) throw MaliciousError("\n  ++ invalid reply: packet smaller than fixed DNS header");

	return res;
}