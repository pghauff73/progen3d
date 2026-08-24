#pragma once

#include "grammar/model/GeometryExpression.h"
#include "grammar/model/GrammarSourceRange.h"
#include "grammar/model/SpatialClearanceSyntax.h"

#include <array>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

class SpatialInterfaceDeclarationSyntax
{
public:
	SpatialInterfaceDeclarationSyntax(
		std::string interface_id,
		std::string interface_type,
		std::array<GeometryExpression, 3> origin,
		std::array<GeometryExpression, 3> normal,
		std::array<GeometryExpression, 3> tangent,
		std::string region_kind,
		std::vector<GeometryExpression> region_extent_expressions,
		std::string boundary_face_name,
		GeometryExpression tolerance,
		SpatialClearanceSyntax clearance,
		GrammarSourceRange source_range)
		: interface_id_(std::move(interface_id)),
		  interface_type_(std::move(interface_type)),
		  origin_(std::move(origin)),
		  normal_(std::move(normal)),
		  tangent_(std::move(tangent)),
		  region_kind_(std::move(region_kind)),
		  region_extent_expressions_(std::move(region_extent_expressions)),
		  boundary_face_name_(std::move(boundary_face_name)),
		  tolerance_(std::move(tolerance)),
		  clearance_(std::move(clearance)),
		  source_range_(source_range)
	{
	}

	const std::string &interfaceId() const { return interface_id_; }
	const std::string &interfaceType() const { return interface_type_; }
	const std::array<GeometryExpression, 3> &origin() const { return origin_; }
	const std::array<GeometryExpression, 3> &normal() const { return normal_; }
	const std::array<GeometryExpression, 3> &tangent() const { return tangent_; }
	const std::string &regionKind() const { return region_kind_; }
	const std::vector<GeometryExpression> &regionExtentExpressions() const
	{
		return region_extent_expressions_;
	}
	const std::string &boundaryFaceName() const { return boundary_face_name_; }
	const GeometryExpression &tolerance() const { return tolerance_; }
	const SpatialClearanceSyntax &clearance() const { return clearance_; }
	const GrammarSourceRange &sourceRange() const { return source_range_; }

	std::string canonicalText() const;

private:
	std::string interface_id_;
	std::string interface_type_;
	std::array<GeometryExpression, 3> origin_;
	std::array<GeometryExpression, 3> normal_;
	std::array<GeometryExpression, 3> tangent_;
	std::string region_kind_;
	std::vector<GeometryExpression> region_extent_expressions_;
	std::string boundary_face_name_;
	GeometryExpression tolerance_;
	SpatialClearanceSyntax clearance_;
	GrammarSourceRange source_range_;
};

inline std::string SpatialInterfaceDeclarationSyntax::canonicalText() const
{
	const auto append_vector = [](std::ostringstream &text,
	                              const char *name,
	                              const std::array<GeometryExpression, 3> &values) {
		text << " " << name << "(";
		for (std::size_t index = 0; index < values.size(); ++index) {
			if (index > 0) text << " ";
			text << values[index].sourceText();
		}
		text << ")";
	};

	std::ostringstream text;
	text << "Interface(id(" << interface_id_ << ") type(" << interface_type_ << ")";
	append_vector(text, "origin", origin_);
	append_vector(text, "normal", normal_);
	append_vector(text, "tangent", tangent_);
	text << " region(" << region_kind_;
	if (!boundary_face_name_.empty()) text << " " << boundary_face_name_;
	for (const GeometryExpression &expression : region_extent_expressions_) {
		text << " " << expression.sourceText();
	}
	text << ") tolerance(" << tolerance_.sourceText() << ")";
	text << " " << clearance_.canonicalText() << ")";
	return text.str();
}
