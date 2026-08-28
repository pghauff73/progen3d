#pragma once

#include <string>
#include <utility>

class SpatialObjectId {
public:
	SpatialObjectId() = default;
	explicit SpatialObjectId(std::string value) : value_(std::move(value)) {}

	const std::string &value() const { return value_; }
	bool empty() const { return value_.empty(); }

	bool operator==(const SpatialObjectId &other) const { return value_ == other.value_; }
	bool operator!=(const SpatialObjectId &other) const { return !(*this == other); }
	bool operator<(const SpatialObjectId &other) const { return value_ < other.value_; }
	bool operator>(const SpatialObjectId &other) const { return other < *this; }

private:
	std::string value_;
};
