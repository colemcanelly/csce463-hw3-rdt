#pragma once

/*
* Cole McAnelly
* CSCE 463 - Dist. Network Systems
* Fall 2024
*/

#include <semaphore>
#include <mutex>
#include <functional>

namespace {
	struct SyncTools {
		std::atomic_int n_unread_items;
		std::counting_semaphore<> n_taken_slots;
		std::counting_semaphore<> n_empty_slots;
		std::mutex buffer_manipulation;
		HANDLE not_empty_smph;

		SyncTools(size_t init_slots) : n_unread_items(0), n_taken_slots(0), n_empty_slots(init_slots), not_empty_smph(CreateSemaphore(NULL, 0, 1, NULL)) {}
		~SyncTools() { CloseHandle(not_empty_smph); }
	};
}

template<typename T>
class CircularBoundedBuffer {
//private:
public:
	T* buf;
	std::atomic_size_t _head;
	std::atomic_size_t _tail;
	std::atomic_size_t _count;
	const size_t _cap;
	std::unique_ptr<SyncTools> sync;

public:
	CircularBoundedBuffer(size_t init_capacity)
		: buf(new T[init_capacity]())
		, _cap(init_capacity)
		, _head(0)
		, _tail(0)
		, _count(0)
		//, pushing()
		, sync(std::make_unique<SyncTools>(1))
	{}

	~CircularBoundedBuffer() { delete[] buf; }
	
	class ProxyMutex;
	class Iter;

	constexpr size_t size() const noexcept { return _count.load(); }
	constexpr size_t capacity() const noexcept { return _cap; }
	constexpr bool empty() const noexcept { return _count.load() == 0; }
	constexpr HANDLE not_empty_smph() const noexcept { return this->sync->not_empty_smph; }

	constexpr Iter iter() noexcept { return Iter(buf, sync, this->_cap, this->_head.load()); }
	
	constexpr const T& at(size_t pos = 0) const noexcept {
		_count.wait(0);
		return buf[(_head.load() + pos) % _cap];
	}

	constexpr const T& peek() const noexcept {
		return this->at(0);
	}
	constexpr ProxyMutex peek_mut() const noexcept {
		_count.wait(0);
		return ProxyMutex(sync->buffer_manipulation, buf[_head.load()]);
	}

	void push(T& item) { this->push(std::move(item)); }
	void push(T&& item) {
		sync->n_empty_slots.acquire();  // Wait for an empty slot
		{
			std::lock_guard<std::mutex> lock(sync->buffer_manipulation);
			buf[_tail] = std::move(item);
			_tail.store((_tail.load() + 1) % _cap);  // Move to the next position in circular fashion
			if (this->empty()) ReleaseSemaphore(sync->not_empty_smph, 1, NULL);
			++_count;
			_count.notify_all();
		}
		sync->n_unread_items++;
		sync->n_taken_slots.release(); // Increment unread items
	}

	template<typename R>
	auto fold_remove_n(size_t num = 1, std::function<R(const T&, R)> foldr = [](const T& cur, R acc) { return acc; }, R init = R{}) -> R {
		if (num == 0) return init;
		size_t n_pop = __min(num, this->_count.load());
		for (int _ = 0; _ < n_pop; _++) {
			init = foldr(this->pop(), init);
		}
		return init;
	}

	T&& pop() {
		std::lock_guard<std::mutex> lock(sync->buffer_manipulation);
		size_t prev_head = _head.load();
		_head.store((_head.load() + 1) % _cap);	// Move to the next position in circular fashion
		--_count;
		_count.notify_all();
		return std::move(buf[prev_head]);
	}

	void release(size_t n = 1) {
		sync->n_empty_slots.release(__min(n, this->_cap));  // Increment empty slots
	}

	void clear() noexcept {
		for (int _ = 0; _ < _count.load(); _++) this->pop();
	}
};

// Non-owning proxy object to allow mutable operations on mutex protected data
template<typename T>
class CircularBoundedBuffer<T>::ProxyMutex
{
	std::lock_guard<std::mutex> lock;
	T& item;

public:
	ProxyMutex(std::mutex& mtx, T& _item) : lock(mtx), item(_item) {}
	constexpr T& operator*() { return item; }
	constexpr T* operator->() { return &item; }
};

template<typename T>
class CircularBoundedBuffer<T>::Iter {
	T* const buf;
	std::unique_ptr<SyncTools>& sync;
	const size_t _cap;
	size_t pos;

public:
	Iter(T* const _buf, std::unique_ptr<SyncTools>& _s, const size_t _cap, const size_t _pos) noexcept
		: buf(_buf), sync(_s), _cap(_cap), pos(_pos)
	{}

	constexpr bool has_next() const { return sync->n_unread_items.load() > 0; }

	const T& next() {
		sync->n_taken_slots.acquire(); // Wait for an unread item
		sync->n_unread_items--;
		size_t prev_pos = pos;
		pos = (pos + 1) % _cap;
		return buf[prev_pos];
	}

	ProxyMutex next_mut() {
		sync->n_taken_slots.acquire(); // Wait for an unread item
		sync->n_unread_items--;
		size_t prev_pos = pos;
		pos = (pos + 1) % _cap;
		return ProxyMutex(sync->buffer_manipulation, buf[prev_pos]);
	}
};