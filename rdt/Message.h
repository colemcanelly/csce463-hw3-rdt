/*
* Cole McAnelly
* CSCE 463 - Dist. Network Systems
* Fall 2024
*/

#pragma once


#include "pch.h"


class Message
{
	const size_t len;
	const std::byte* bytes;

	Message(size_t _len) : len(_len << 2), bytes(new std::byte[_len << 2]) {
		for (int i = 0; i < _len; i++) ((uint32_t*)bytes)[i] = i;
	}
public:

	static Message from_exponent(size_t exp) {
		printf("Main:\tinitializing DWORD array with 2^%llu elements... ", exp);
		
		auto start = Time::now();

		Message ret(1ULL << exp);
		
		printf("done in %lld ms\n", time_elapsed<milliseconds>(start).count());
		return ret;
	}


	~Message() { delete[] bytes; }

	constexpr const std::byte* data() const { return bytes; }
	constexpr const size_t size() const { return len; }


};
