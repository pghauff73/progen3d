#pragma once

#include "grammar/model/GrammarSourceRange.h"

#include <sstream>
#include <string>
#include <utility>
#include <vector>

class CollisionParticipationSyntax
{
public:
	CollisionParticipationSyntax() = default;
	CollisionParticipationSyntax(std::string layer_name,
	                             std::vector<std::string> mask_layer_names,
	                             GrammarSourceRange source_range)
		: layer_name_(std::move(layer_name)),
		  mask_layer_names_(std::move(mask_layer_names)),
		  source_range_(source_range)
	{
	}

	const std::string &layerName() const { return layer_name_; }
	const std::vector<std::string> &maskLayerNames() const { return mask_layer_names_; }
	const GrammarSourceRange &sourceRange() const { return source_range_; }

	std::string canonicalText() const
	{
		std::ostringstream text;
		text << "layer(" << layer_name_ << ") mask(";
		for (std::size_t index = 0; index < mask_layer_names_.size(); ++index) {
			if (index > 0) text << " ";
			text << mask_layer_names_[index];
		}
		text << ")";
		return text.str();
	}

private:
	std::string layer_name_ = "Temporary";
	std::vector<std::string> mask_layer_names_;
	GrammarSourceRange source_range_;
};
