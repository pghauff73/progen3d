#pragma once

#include <string>
#include <utility>

class SpatialObjectClass {
public:
	SpatialObjectClass() = default;
	explicit SpatialObjectClass(std::string value) : value_(std::move(value)) {}

	const std::string &value() const { return value_; }
	bool empty() const { return value_.empty(); }

private:
	std::string value_;
};
