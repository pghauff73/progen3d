#pragma once

#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <exception>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

// One render thread submits work on disjoint row bands and also works on the first band.
class RenderRowExecutor
{
public:
	using RowTask = std::function<void(int first_row, int end_row)>;

	RenderRowExecutor()
	{
		const unsigned bands = std::clamp(std::thread::hardware_concurrency(), 1u, 8u);
		threads_.reserve(bands - 1);
		for (unsigned band = 1; band < bands; ++band) {
			try {
				threads_.emplace_back(&RenderRowExecutor::work, this, band);
			} catch (...) {
				// A smaller pool can still process every row.
				break;
			}
		}
	}

	RenderRowExecutor(const RenderRowExecutor &) = delete;
	RenderRowExecutor &operator=(const RenderRowExecutor &) = delete;

	~RenderRowExecutor()
	{
		{
			std::lock_guard<std::mutex> lock(mutex_);
			stopping_ = true;
		}
		work_ready_.notify_all();
		for (auto &thread : threads_) thread.join();
	}

	void run(int width, int height, const RowTask &task, unsigned max_bands = 8)
	{
		const unsigned bands = std::min({max_bands, static_cast<unsigned>(threads_.size()) + 1,
		                                 static_cast<unsigned>(std::max(1, height / 16))});
		if (bands <= 1 || static_cast<std::size_t>(width) * height < 65536) {
			task(0, height);
			return;
		}
		{
			std::lock_guard<std::mutex> lock(mutex_);
			task_ = &task;
			row_count_ = height;
			band_count_ = bands;
			pending_bands_ = bands - 1;
			failure_ = nullptr;
			++generation_;
		}
		work_ready_.notify_all();
		const auto caller_failure = execute(task, 0, height / static_cast<int>(bands));
		std::unique_lock<std::mutex> lock(mutex_);
		work_finished_.wait(lock, [this] { return pending_bands_ == 0; });
		task_ = nullptr;
		if (caller_failure) std::rethrow_exception(caller_failure);
		if (failure_) std::rethrow_exception(failure_);
	}

private:
	static std::exception_ptr execute(const RowTask &task, int first_row, int end_row)
	{
		try {
			task(first_row, end_row);
			return nullptr;
		} catch (...) {
			return std::current_exception();
		}
	}

	void work(unsigned band)
	{
		std::size_t last_generation = 0;
		std::unique_lock<std::mutex> lock(mutex_);
		for (;;) {
			work_ready_.wait(lock, [&] { return stopping_ || generation_ != last_generation; });
			if (stopping_) return;
			last_generation = generation_;
			if (band >= band_count_) continue;
			const auto *task = task_;
			const int first = row_count_ * static_cast<int>(band) / static_cast<int>(band_count_);
			const int end = row_count_ * static_cast<int>(band + 1) / static_cast<int>(band_count_);
			lock.unlock();
			const auto failure = execute(*task, first, end);
			lock.lock();
			if (failure && !failure_) failure_ = failure;
			if (--pending_bands_ == 0) work_finished_.notify_one();
		}
	}

	std::mutex mutex_;
	std::condition_variable work_ready_;
	std::condition_variable work_finished_;
	const RowTask *task_ = nullptr;
	std::exception_ptr failure_;
	std::size_t generation_ = 0;
	int row_count_ = 0;
	unsigned band_count_ = 0;
	unsigned pending_bands_ = 0;
	bool stopping_ = false;
	std::vector<std::thread> threads_;
};
