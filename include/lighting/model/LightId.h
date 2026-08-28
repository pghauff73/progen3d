#pragma once

#include <string>
#include <utility>

class LightId
{
public:
	LightId() = default;
	explicit LightId(std::string value) : value_(std::move(value)) {}

	const std::string &value() const { return value_; }
	bool empty() const { return value_.empty(); }

	bool operator==(const LightId &other) const { return value_ == other.value_; }
	bool operator!=(const LightId &other) const { return !(*this == other); }
	bool operator<(const LightId &other) const { return value_ < other.value_; }

private:
	std::string value_;
};
