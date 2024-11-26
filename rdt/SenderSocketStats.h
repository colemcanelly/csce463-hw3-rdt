#pragma once

/*
* Cole McAnelly
* CSCE 463 - Dist. Network Systems
* Fall 2024
*/

#include "pch.h"

namespace socket_helpers {

	struct Stats {
		std::atomic_uint32_t sender_base;
		std::atomic_uint32_t effective_window;
		std::atomic_uint64_t bytes_acked;
		std::atomic_uint32_t n_timeouts;
		std::atomic_uint32_t n_fast_retransmits;
		std::atomic_uint32_t final_checksum;

		Stats() : sender_base(0), effective_window(0), bytes_acked(0), n_timeouts(0), n_fast_retransmits(0), final_checksum(0) {}

		friend class std::unique_ptr<Stats>;
	};

	struct Timeouts {
		std::atomic<microseconds> rto;
		std::atomic<microseconds> dev_rtt;
		std::atomic<microseconds> est_rtt;

		Timeouts(microseconds init_rto = 1s) : rto(init_rto), dev_rtt(0s), est_rtt(0s) {}
		Timeouts(std::chrono::duration<float> init_rto) : Timeouts(std::chrono::duration_cast<microseconds>(init_rto)){}
		Timeouts(float init_rto) : Timeouts(std::chrono::duration<float>(init_rto)) {}

		void recalculate(microseconds measured) {
			est_rtt.store(calc_est_rtt(measured, est_rtt.load(), false));
			dev_rtt.store(calc_dev_rtt(measured, dev_rtt.load(), est_rtt.load(), false));
			rto.store(calc_new_rto(measured, dev_rtt.load(), est_rtt.load(), false));
		}

		void recalculate_syn(microseconds measured) {
			est_rtt.store(calc_est_rtt(measured, est_rtt.load(), true));
			dev_rtt.store(calc_dev_rtt(measured, dev_rtt.load(), est_rtt.load(), true));
			rto.store(calc_new_rto(measured, dev_rtt.load(), est_rtt.load(), true));
		}

		constexpr const timeval get() const noexcept { return to_timeval(rto.load()); }
		constexpr const float get(float) const noexcept { return to_float(rto.load()); }
		constexpr const microseconds get_remaining(microseconds elapsed) const noexcept { return rto.load() - elapsed; }

	private:
		inline microseconds calc_est_rtt(microseconds sample, microseconds curr_est, bool is_syn) {
			if (is_syn) return sample;
			constexpr float a = 0.125;
			float est = (1 - a) * to_float(curr_est) + a * to_float(sample);
			return std::chrono::duration_cast<microseconds>(std::chrono::duration<float>(est));
		}

		inline microseconds calc_dev_rtt(microseconds sample, microseconds curr_dev, microseconds curr_est, bool is_syn) {
			if (is_syn) return 0us;
			constexpr float b = 0.125;
			float dev = (1 - b) * to_float(curr_dev) + b * abs(to_float(sample) - to_float(curr_est));
			return std::chrono::duration_cast<microseconds>(std::chrono::duration<float>(dev));
		}

		constexpr microseconds calc_new_rto(microseconds sample, microseconds new_dev, microseconds new_est, bool is_syn) {
			if (is_syn) return sample * 3;
			return new_est + 4 * __max(10ms, new_dev);
		}

		friend class std::unique_ptr<Timeouts>;
	};

}