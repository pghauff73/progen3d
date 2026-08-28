#pragma once

#include <string>
#include <utility>

class GeometryExpression
{
public:
	explicit GeometryExpression(std::string source_text)
		: source_text_(std::move(source_text))
	{
	}

	const std::string &sourceText() const
	{
		return source_text_;
	}

private:
	std::string source_text_;
};
