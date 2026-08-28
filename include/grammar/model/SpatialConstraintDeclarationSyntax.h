#pragma once

#include "grammar/model/GeometryExpression.h"
#include "grammar/model/GrammarSourceRange.h"
#include "grammar/model/SpatialClearanceSyntax.h"
#include "grammar/model/SpatialDirectionSyntax.h"

#include <sstream>
#include <string>
#include <utility>
#include <vector>

class SpatialConstraintDeclarationSyntax
{
public:
	SpatialConstraintDeclarationSyntax(
		std::string constraint_id,
		std::string moving_interface_id,
		std::string target_object_id,
		std::string target_interface_id,
		std::string mode,
		SpatialDirectionSyntax direction,
		SpatialClearanceSyntax clearance,
		GeometryExpression seating_depth,
		GeometryExpression maximum_distance,
		GeometryExpression tolerance,
		GeometryExpression priority,
		std::vector<std::string> collision_mask_layer_names,
		GrammarSourceRange source_range)
		: constraint_id_(std::move(constraint_id)),
		  moving_interface_id_(std::move(moving_interface_id)),
		  target_object_id_(std::move(target_object_id)),
		  target_interface_id_(std::move(target_interface_id)),
		  mode_(std::move(mode)),
		  direction_(std::move(direction)),
		  clearance_(std::move(clearance)),
		  seating_depth_(std::move(seating_depth)),
		  maximum_distance_(std::move(maximum_distance)),
		  tolerance_(std::move(tolerance)),
		  priority_(std::move(priority)),
		  collision_mask_layer_names_(std::move(collision_mask_layer_names)),
		  source_range_(source_range)
	{
	}

	const std::string &constraintId() const { return constraint_id_; }
	const std::string &movingInterfaceId() const { return moving_interface_id_; }
	const std::string &targetObjectId() const { return target_object_id_; }
	const std::string &targetInterfaceId() const { return target_interface_id_; }
	const std::string &mode() const { return mode_; }
	const SpatialDirectionSyntax &direction() const { return direction_; }
	const SpatialClearanceSyntax &clearance() const { return clearance_; }
	const GeometryExpression &seatingDepth() const { return seating_depth_; }
	const GeometryExpression &maximumDistance() const { return maximum_distance_; }
	const GeometryExpression &tolerance() const { return tolerance_; }
	const GeometryExpression &priority() const { return priority_; }
	const std::vector<std::string> &collisionMaskLayerNames() const
	{
		return collision_mask_layer_names_;
	}
	const GrammarSourceRange &sourceRange() const { return source_range_; }

	std::string canonicalText() const
	{
		std::ostringstream text;
		text << "Position(id(" << constraint_id_ << ") moving(" << moving_interface_id_
		     << ") target(" << target_object_id_ << " " << target_interface_id_
		     << ") mode(" << mode_ << ") " << direction_.canonicalText() << " "
		     << clearance_.canonicalText() << " seatingDepth(" << seating_depth_.sourceText()
		     << ") maxDistance(" << maximum_distance_.sourceText() << ") tolerance("
		     << tolerance_.sourceText() << ") priority(" << priority_.sourceText() << ")";
		if (!collision_mask_layer_names_.empty()) {
			text << " mask(";
			for (std::size_t index = 0; index < collision_mask_layer_names_.size(); ++index) {
				if (index > 0) text << " ";
				text << collision_mask_layer_names_[index];
			}
			text << ")";
		}
		text << ")";
		return text.str();
	}

private:
	std::string constraint_id_;
	std::string moving_interface_id_;
	std::string target_object_id_;
	std::string target_interface_id_;
	std::string mode_;
	SpatialDirectionSyntax direction_;
	SpatialClearanceSyntax clearance_;
	GeometryExpression seating_depth_;
	GeometryExpression maximum_distance_;
	GeometryExpression tolerance_;
	GeometryExpression priority_;
	std::vector<std::string> collision_mask_layer_names_;
	GrammarSourceRange source_range_;
};
