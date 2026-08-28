#pragma once

#include <string>
#include <utility>

class SpatialConstraintId {
public:
	SpatialConstraintId() = default;
	explicit SpatialConstraintId(std::string value) : value_(std::move(value)) {}

	const std::string &value() const { return value_; }
	bool empty() const { return value_.empty(); }

	bool operator==(const SpatialConstraintId &other) const { return value_ == other.value_; }
	bool operator!=(const SpatialConstraintId &other) const { return !(*this == other); }
	bool operator<(const SpatialConstraintId &other) const { return value_ < other.value_; }

private:
	std::string value_;
};
