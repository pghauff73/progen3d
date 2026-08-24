#pragma once

#include "grammar/model/GeometryExpression.h"
#include "grammar/model/GrammarSourceRange.h"

#include <sstream>
#include <utility>

class SpatialClearanceSyntax
{
public:
	SpatialClearanceSyntax()
		: nominal_("0"), minimum_("0"), maximum_("0")
	{
	}

	SpatialClearanceSyntax(GeometryExpression nominal,
	                       GeometryExpression minimum,
	                       GeometryExpression maximum,
	                       GrammarSourceRange source_range)
		: nominal_(std::move(nominal)),
		  minimum_(std::move(minimum)),
		  maximum_(std::move(maximum)),
		  source_range_(source_range)
	{
	}

	const GeometryExpression &nominal() const { return nominal_; }
	const GeometryExpression &minimum() const { return minimum_; }
	const GeometryExpression &maximum() const { return maximum_; }
	const GrammarSourceRange &sourceRange() const { return source_range_; }

	std::string canonicalText() const
	{
		std::ostringstream text;
		text << "clearance(" << nominal_.sourceText();
		if (minimum_.sourceText() != nominal_.sourceText() ||
		    maximum_.sourceText() != nominal_.sourceText()) {
			text << " " << minimum_.sourceText() << " " << maximum_.sourceText();
		}
		text << ")";
		return text.str();
	}

private:
	GeometryExpression nominal_;
	GeometryExpression minimum_;
	GeometryExpression maximum_;
	GrammarSourceRange source_range_;
};
