/*
* Cole McAnelly
* CSCE 463 - Dist. Network Systems
* Fall 2024
*/

#include "pch.h"
#include "Udp.h"


Udp::Udp(const std::string& host_ip, const uint16_t port)
	: sock(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))
	, data_in_socket(CreateEvent(NULL, false, false, NULL))
	, server{0}
{
	if (sock == INVALID_SOCKET) throw WinSock::FatalError("couldn't create socket!");
	if (data_in_socket == WSA_INVALID_EVENT) throw WinSock::FatalError("couldn't create data read event!");

	int kernelBuffer = 20e6;	// 20 meg 
	if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)&kernelBuffer, sizeof(int)) == SOCKET_ERROR)
		throw WinSock::FatalError("couldn't change kernel receive buffer size");

	kernelBuffer = 20e6;		// 20 meg 
	if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char*)&kernelBuffer, sizeof(int)) == SOCKET_ERROR)
		throw WinSock::FatalError("couldn't change kernel sender buffer size");
	
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


	if (WSAEventSelect(this->sock, data_in_socket, FD_READ) == SOCKET_ERROR) {
		WSACloseEvent(data_in_socket);
		throw WinSock::FatalError("couldn't associate data event with socket");
	}
}


int Udp::udt_send(const Packet& p) const {
	int ret = sendto(this->sock, (const char*)p.data(), p.size(), NULL, (struct sockaddr*)&this->server, sizeof(struct sockaddr_in));
	if (ret == SOCKET_ERROR) throw Udp::SendToError();
	return ret;
}


std::optional<net::ReceiverHeader> Udp::udt_recv(const std::optional<microseconds>& timeout, HANDLE event, bool& event_occurred) const
{
	HANDLE events[] = { this->data_in_socket, event };
	constexpr DWORD n_events = sizeof(events) / sizeof(events[1]);

	DWORD result = WaitForMultipleObjects(n_events, events, false, timeout ? std::chrono::duration_cast<milliseconds>(timeout.value()).count() : INFINITE);

	switch (result) {
	case WAIT_TIMEOUT: return {};
	case WAIT_OBJECT_0: break;
	case WAIT_OBJECT_0 + 1:
		event_occurred = true;
		return {};
	default: throw Udp::SelectError();
	}

	struct sockaddr_in responder;
	int reponder_addr_size = sizeof(struct sockaddr_in);

	net::ReceiverHeader res;
	int num_bytes = recvfrom(this->sock, (char*)&res, sizeof(net::ReceiverHeader), NULL, (SOCKADDR*)&responder, &reponder_addr_size);

	if (num_bytes < 0) throw Udp::RecvFromError();
	if (num_bytes == 0) throw Udp::Error("connection closed");

	if (responder.sin_addr.s_addr != server.sin_addr.s_addr
		|| responder.sin_port != server.sin_port) throw Udp::Error("bogus reply"); // return false;

	return res;
}