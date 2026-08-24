#pragma once

#include <string>
#include <utility>

class ElectricalObjectId
{
public:
	ElectricalObjectId() = default;
	explicit ElectricalObjectId(std::string value) : value_(std::move(value)) {}
	const std::string &value() const { return value_; }
	bool empty() const { return value_.empty(); }
	bool operator==(const ElectricalObjectId &other) const { return value_ == other.value_; }
	bool operator<(const ElectricalObjectId &other) const { return value_ < other.value_; }

private:
	std::string value_;
};
