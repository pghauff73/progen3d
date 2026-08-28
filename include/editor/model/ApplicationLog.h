#pragma once

#include <cstddef>
#include <mutex>
#include <string>
#include <vector>

class ApplicationLog
{
public:
	explicit ApplicationLog(std::size_t maximum_entry_count = 2000,
	                        std::size_t trim_entry_count = 500);

	void append(std::string message);
	void clear();
	std::size_t size() const;
	std::vector<std::string> snapshot() const;

private:
	std::size_t maximum_entry_count_;
	std::size_t trim_entry_count_;
	mutable std::mutex mutex_;
	std::vector<std::string> entries_;
};
