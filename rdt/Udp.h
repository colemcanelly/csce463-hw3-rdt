/*
* Cole McAnelly
* CSCE 463 - Dist. Network Systems
* Fall 2024
*/

#pragma once


#include "pch.h"

#include "WinSock.h"
#include "Packet.h"

class Udp : WinSock
{
	SOCKET sock;

protected:
	struct sockaddr_in server;

public:
	Udp(const std::string& host_ip, const uint16_t port);
	~Udp() { closesocket(sock); }


	int udt_send(const Packet& p) const;
	std::optional<net::ReceiverHeader> udt_recv(const struct timeval& timeout) const;

	struct Error : public std::runtime_error {
		explicit Error() : std::runtime_error("") {}
		explicit Error(const std::string& msg) : std::runtime_error("failed with " + msg) {}
	};

	struct SelectError : public WinSock::Error { explicit SelectError() : WinSock::Error("select") {} };
	struct SendToError : public WinSock::Error { explicit SendToError() : WinSock::Error("sendto") {} };
	struct RecvFromError : public WinSock::Error { explicit RecvFromError() : WinSock::Error("recvfrom") {} };

private:

	static constexpr size_t MAX_ATTEMPTS = 3;
};

