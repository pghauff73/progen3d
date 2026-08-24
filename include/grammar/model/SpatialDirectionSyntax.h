#pragma once

#include "grammar/model/GeometryExpression.h"
#include "grammar/model/GrammarSourceRange.h"

#include <array>
#include <sstream>
#include <string>
#include <utility>

class SpatialDirectionSyntax
{
public:
	SpatialDirectionSyntax()
		: vector_expressions_{GeometryExpression("0"),
		                      GeometryExpression("0"),
		                      GeometryExpression("0")}
	{
	}

	SpatialDirectionSyntax(std::string frame_name,
	                       std::array<GeometryExpression, 3> vector_expressions,
	                       GrammarSourceRange source_range)
		: frame_name_(std::move(frame_name)),
		  vector_expressions_(std::move(vector_expressions)),
		  source_range_(source_range)
	{
	}

	const std::string &frameName() const { return frame_name_; }
	const std::array<GeometryExpression, 3> &vectorExpressions() const
	{
		return vector_expressions_;
	}
	const GrammarSourceRange &sourceRange() const { return source_range_; }

	std::string canonicalText() const
	{
		std::ostringstream text;
		text << "direction(" << frame_name_;
		for (const GeometryExpression &expression : vector_expressions_) {
			text << " " << expression.sourceText();
		}
		text << ")";
		return text.str();
	}

private:
	std::string frame_name_ = "World";
	std::array<GeometryExpression, 3> vector_expressions_;
	GrammarSourceRange source_range_;
};
