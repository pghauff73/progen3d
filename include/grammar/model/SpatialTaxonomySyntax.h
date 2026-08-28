#pragma once

#include "grammar/model/GrammarSourceRange.h"

#include <sstream>
#include <string>
#include <utility>
#include <vector>

class SpatialTaxonomySyntax
{
public:
	SpatialTaxonomySyntax() = default;
	SpatialTaxonomySyntax(std::vector<std::string> segments,
	                      GrammarSourceRange source_range)
		: segments_(std::move(segments)),
		  source_range_(source_range)
	{
	}

	const std::vector<std::string> &segments() const { return segments_; }
	const GrammarSourceRange &sourceRange() const { return source_range_; }

	std::string canonicalText() const
	{
		std::ostringstream text;
		text << "taxonomy(";
		for (std::size_t index = 0; index < segments_.size(); ++index) {
			if (index > 0) text << " ";
			text << segments_[index];
		}
		text << ")";
		return text.str();
	}

private:
	std::vector<std::string> segments_;
	GrammarSourceRange source_range_;
};
