// pch.h: This is a precompiled header file.
/*
* Cole McAnelly
* CSCE 463 - Dist. Network Systems
* Fall 2024
*/

// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here
#include <cstdint>
#include <cstddef>
#include <string>
#include <iostream>
#include <memory>
#include <optional>
#include <limits>
#include <exception>
#include <chrono>
#include <set>
#include <mutex>
#include <functional>

#define NOMINMAX
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#include "NetworkPackets.h"

// Timing operations
using Time = std::chrono::high_resolution_clock;
using seconds = std::chrono::seconds;
using milliseconds = std::chrono::milliseconds;
using microseconds = std::chrono::microseconds;

using namespace std::chrono_literals;

template <typename T>
constexpr T time_elapsed(Time::time_point begin) { return std::chrono::duration_cast<T>(Time::now() - begin); }

constexpr struct timeval to_timeval(microseconds t) {
	auto secs = std::chrono::duration_cast<seconds>(t);
	t -= secs;

	return timeval{ (long)secs.count(), (long)t.count() };
}

// Bytes per bit
using bytes = std::ratio<8, 1>;

template <typename ToRatio, typename FromRatio = std::ratio<1>, typename T>
constexpr T unit_cast(T value) {
	using conversion_ratio = std::ratio_divide<FromRatio, ToRatio>;
	return value * static_cast<T>(conversion_ratio::num) / static_cast<T>(conversion_ratio::den);
}

constexpr float to_float(microseconds t) { return std::chrono::duration_cast<std::chrono::duration<float>>(t).count(); }

struct UsageError : std::runtime_error {
	explicit UsageError() : std::runtime_error("Usage: rdt <hostname> <log_2(buffersize)> <Window Size> <RTT> <Forward loss> <Reverse Loss> <Link Speed>\n") {}
};

#endif //PCH_H