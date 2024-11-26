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
	WSAEVENT data_in_socket;

protected:
	struct sockaddr_in server;

public:
	Udp(const std::string& host_ip, const uint16_t port);
	~Udp() {
		closesocket(sock);
		WSACloseEvent(data_in_socket);
	}


	int udt_send(const Packet& p) const;
	std::optional<net::ReceiverHeader> udt_recv(const std::optional<microseconds>& timeout, HANDLE event, bool& event_occurred) const;

	struct Error : public std::runtime_error {
		explicit Error() : std::runtime_error("") {}
		explicit Error(const std::string& msg) : std::runtime_error("failed with " + msg) {}
	};

	struct SelectError : public WinSock::Error { explicit SelectError() : WinSock::Error("select") {} };
	struct SendToError : public WinSock::Error { explicit SendToError() : WinSock::Error("sendto") {} };
	struct RecvFromError : public WinSock::Error { explicit RecvFromError() : WinSock::Error("recvfrom") {} };

private:

	friend class std::unique_ptr<Udp>;
};

