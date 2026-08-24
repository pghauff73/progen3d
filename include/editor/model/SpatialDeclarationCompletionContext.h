#pragma once

#include <string>
#include <utility>
#include <vector>

class SpatialDeclarationCompletionContext
{
public:
	SpatialDeclarationCompletionContext() = default;
	SpatialDeclarationCompletionContext(std::string context_name,
	                                    std::vector<std::string> completions)
		: context_name_(std::move(context_name)),
		  completions_(std::move(completions))
	{
	}

	bool isActive() const { return !completions_.empty(); }
	const std::string &contextName() const { return context_name_; }
	const std::vector<std::string> &completions() const { return completions_; }

private:
	std::string context_name_;
	std::vector<std::string> completions_;
};
