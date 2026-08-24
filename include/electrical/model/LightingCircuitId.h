#pragma once

#include <string>
#include <utility>

class LightingCircuitId
{
public:
	LightingCircuitId() = default;
	explicit LightingCircuitId(std::string value) : value_(std::move(value)) {}
	const std::string &value() const { return value_; }
	bool empty() const { return value_.empty(); }
	bool operator==(const LightingCircuitId &other) const { return value_ == other.value_; }
	bool operator<(const LightingCircuitId &other) const { return value_ < other.value_; }

private:
	std::string value_;
};
