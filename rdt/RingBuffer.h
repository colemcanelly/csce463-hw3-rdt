#pragma once

/*
* Cole McAnelly
* CSCE 463 - Dist. Network Systems
* Fall 2024
*/

#include <semaphore>

#include "pch.h"


template<typename T>
class RingBuffer {
private:
	T* buf;
	const size_t cap;

	std::atomic_size_t n_elems;

	std::atomic_size_t produce_idx;
	std::atomic_size_t consume_idx;

	std::counting_semaphore<> data;
	std::counting_semaphore<> space;

public:
	RingBuffer(const size_t capacity, size_t init_window)
		: buf(new T[capacity]())
		, cap(capacity)
		, produce_idx(0)
		, consume_idx(0)
		, data(0)
		, space(init_window)
	{}
	RingBuffer(const size_t capacity) : RingBuffer(capacity, capacity) {}

	~RingBuffer() { delete[] buf; }

	constexpr size_t capacity() const noexcept { return cap; }
	size_t size() const noexcept { return n_elems.load(); }
	constexpr bool empty() const noexcept { return n_elems.load() == 0; }
	constexpr bool full() const noexcept { return n_elems.load() == cap; }

	void push(T&& item) {
		space.acquire();
		buf[produce_idx.load()] = std::move(item);
		produce_idx = (produce_idx.load() + 1) % cap;
		n_elems++;
		data.release();
	}

	const T& peek() const {
		if (empty()) throw std::runtime_error("tried to peek on empty buffer");
		return buf[consume_idx.load()];
	}

	T& peek_mut() const {
		if (empty()) throw std::runtime_error("tried to peek on empty buffer");
		return buf[consume_idx.load()];
	}

	T&& pop() {
		data.acquire();
		size_t prev_consume_idx = consume_idx.load();
		consume_idx = (prev_consume_idx + 1) % cap;
		n_elems--;
		return std::move(buf[prev_consume_idx]);
	}

	T&& pop_release() {
		data.acquire();
		size_t prev_consume_idx = consume_idx.load();
		consume_idx = (prev_consume_idx + 1) % cap;
		n_elems--;
		space.release();
		return std::move(buf[prev_consume_idx]);
	}

	template<typename R>
	std::tuple<T&&, R> fold_pop_n(size_t num, std::function<R(const T&, R)> foldr, R init = R{}) {
		if (num == 0) return std::make_tuple(T(), init);;
		size_t n_pop = std::min(num, size());
		T popped;
		for (int _ = 0; _ < n_pop && !empty(); _++) {
			popped = std::move(this->pop_release());
			init = foldr(popped, init);
		}
		return std::make_tuple(std::move(popped), init);
	}


	void release_space(size_t n = 1) { space.release(std::min(n, cap)); }



	void clear() noexcept { while (!empty()) this->pop(); }
};