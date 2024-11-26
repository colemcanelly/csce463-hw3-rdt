/*
* Cole McAnelly
* CSCE 463 - Dist. Network Systems
* Fall 2024
*/

//#pragma once

#include "pch.h"

// RAII for winsock DLL
class WinSock {
public:
	WinSock()
	{
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData)) throw WinSock::FatalError("WSAStartup");
	}

	~WinSock() { WSACleanup(); }

	struct FatalError : public std::runtime_error {
		explicit FatalError(const std::string& func = "") : std::runtime_error("failed " + func + " with " + std::to_string(h_errno)) {}
	};

	struct Error : public FatalError {
		explicit Error(const std::string& func = "") : FatalError(func) {}
	};

	static inline uint32_t dns(const std::string& hostname) {
		// Got info from reading inaddr.h
		auto start = Time::now();
		in_addr ip;
		ip.s_addr = inet_addr(hostname.c_str());
		if (ip.s_addr == INADDR_NONE) {
			hostent* host = gethostbyname(hostname.c_str());
			if (host == NULL) throw WinSock::Error("(DNS failure)");;
			ip.s_addr = *reinterpret_cast<uint32_t*>(host->h_addr);
		}
		return ip.s_addr;
	}
};
