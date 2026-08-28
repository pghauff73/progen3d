#pragma once

#include "grammar/model/GeometryExpression.h"

#include <string>
#include <utility>
#include <vector>

class ShapeOptionSyntax
{
public:
	ShapeOptionSyntax(std::string option_name,
	                  std::vector<GeometryExpression> arguments)
		: option_name_(std::move(option_name)),
		  arguments_(std::move(arguments))
	{
	}

	const std::string &optionName() const
	{
		return option_name_;
	}

	const std::vector<GeometryExpression> &arguments() const
	{
		return arguments_;
	}

private:
	std::string option_name_;
	std::vector<GeometryExpression> arguments_;
};
