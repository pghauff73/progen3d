#pragma once

#include <string>
#include <utility>
#include <vector>

class ShapeCompletionContext
{
public:
	ShapeCompletionContext() = default;
	ShapeCompletionContext(std::string shape_name,
	                       std::vector<std::string> completions)
		: shape_name_(std::move(shape_name)),
		  completions_(std::move(completions))
	{
	}

	bool isActive() const { return !shape_name_.empty() && !completions_.empty(); }
	const std::string &shapeName() const { return shape_name_; }
	const std::vector<std::string> &completions() const { return completions_; }

private:
	std::string shape_name_;
	std::vector<std::string> completions_;
};
