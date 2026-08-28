#pragma once

#include <optional>
#include <string>
#include <utility>

class BuildingRequiredValue {
public:
	BuildingRequiredValue(std::string description,
	                      std::optional<double> numeric_value = std::nullopt)
		: description_(std::move(description)), numeric_value_(numeric_value) {}

	const std::string &description() const { return description_; }
	const std::optional<double> &numericValue() const { return numeric_value_; }

private:
	std::string description_;
	std::optional<double> numeric_value_;
};
