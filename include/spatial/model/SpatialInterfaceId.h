#pragma once

#include <string>
#include <utility>

class SpatialInterfaceId {
public:
	SpatialInterfaceId() = default;
	explicit SpatialInterfaceId(std::string value) : value_(std::move(value)) {}

	const std::string &value() const { return value_; }
	bool empty() const { return value_.empty(); }

	bool operator==(const SpatialInterfaceId &other) const { return value_ == other.value_; }
	bool operator!=(const SpatialInterfaceId &other) const { return !(*this == other); }
	bool operator<(const SpatialInterfaceId &other) const { return value_ < other.value_; }

private:
	std::string value_;
};
