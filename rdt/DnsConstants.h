/*
* Cole McAnelly
* CSCE 463 - Dist. Network Systems
* Fall 2024
*/

#pragma once

#include "pch.h"


static constexpr size_t MAX_DNS_SIZE = 512;

enum class DnsType : uint16_t {
	DNS_A = 1,
	DNS_NS = 2,
	DNS_CNAME = 5,
	DNS_PTR = 12,
	DNS_HINFO = 13,
	DNS_MX = 15,
	DNS_AXFR = 252,
	DNS_ANY = 255
};

enum class DnsClass : uint16_t { DNS_INET = 1 };


/* flags */
#define DNS_QUERY		(0 << 15)	/* 0 = query; 1 = response */
#define DNS_RESPONSE	(1 << 15).

#define DNS_STDQUERY	(0 << 11)	/* opcode - 4 bits */

#define DNS_AA			(1 << 10)	/* authoritative answer */
#define DNS_TC			(1 << 9)	/* truncated */
#define DNS_RD			(1 << 8)	/* recursion desired */
#define DNS_RA			(1 << 7)	/* recursion available */
