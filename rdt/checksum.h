#pragma once

/*
* Cole McAnelly
* CSCE 463 - Dist. Network Systems
* Fall 2024
*/

#include "pch.h"
#include <array>
#include <algorithm>

#include "Message.h"


namespace CRC32 {
	namespace {
		static inline constexpr uint32_t crc_part(uint32_t c) {
			for (int j = 0; j < 8; j++)
				c = (c & 1) ? (0xEDB88320 ^ (c >> 1)) : (c >> 1);
			return c;
		}
	};

	static constexpr auto crc_table = []{
		std::array<uint32_t, 256> arr;

		std::generate(arr.begin(), arr.end(),
			[i = 0u]() mutable { return crc_part(i++); }
		);

		return arr;
	}();

	constexpr uint32_t checksum(uint8_t* buf, size_t len) {
		uint32_t c = 0xFFFFFFFF;
		for (size_t i = 0; i < len; i++)
			c = crc_table[(c ^ buf[i]) & 0xFF] ^ (c >> 8);
		return c ^ 0xFFFFFFFF;
	}

	constexpr uint32_t checksum(const Message& m) {
		return checksum((uint8_t*)m.data(), m.size());
	}
};
