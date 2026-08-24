#include "editor/model/ApplicationLog.h"

#include <algorithm>
#include <utility>

ApplicationLog::ApplicationLog(std::size_t maximum_entry_count,
	                           std::size_t trim_entry_count)
	: maximum_entry_count_(std::max<std::size_t>(1, maximum_entry_count)),
	  trim_entry_count_(std::max<std::size_t>(1, trim_entry_count))
{
}

void ApplicationLog::append(std::string message)
{
	if (message.empty()) {
		return;
	}
	if (message.back() == '\n') {
		message.pop_back();
	}
	std::lock_guard<std::mutex> lock(mutex_);
	entries_.push_back(std::move(message));
	if (entries_.size() > maximum_entry_count_) {
		const std::size_t removable_count = std::min(trim_entry_count_, entries_.size());
		entries_.erase(entries_.begin(),
		               entries_.begin() + static_cast<std::ptrdiff_t>(removable_count));
	}
}

void ApplicationLog::clear()
{
	std::lock_guard<std::mutex> lock(mutex_);
	entries_.clear();
}

std::size_t ApplicationLog::size() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return entries_.size();
}

std::vector<std::string> ApplicationLog::snapshot() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return entries_;
}
