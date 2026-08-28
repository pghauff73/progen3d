#pragma once

#include "building/model/BuildingExtentKind.h"
#include "spatial/model/BoundaryRepresentation.h"

#include <optional>
#include <string>
#include <utility>

class BuildingExtentDescriptor {
public:
	BuildingExtentDescriptor(
		BuildingExtentKind extent_kind,
		BuildingExtentApplicability applicability,
		std::optional<BoundaryRepresentationKind> representation_kind,
		BuildingExtentSourceKind source_kind,
		std::string provenance)
		: extent_kind_(extent_kind),
		  applicability_(applicability),
		  representation_kind_(representation_kind),
		  source_kind_(source_kind),
		  provenance_(std::move(provenance)) {}

	BuildingExtentKind extentKind() const { return extent_kind_; }
	BuildingExtentApplicability applicability() const { return applicability_; }
	const std::optional<BoundaryRepresentationKind> &representationKind() const
	{
		return representation_kind_;
	}
	BuildingExtentSourceKind sourceKind() const { return source_kind_; }
	const std::string &provenance() const { return provenance_; }

private:
	BuildingExtentKind extent_kind_ = BuildingExtentKind::Visual;
	BuildingExtentApplicability applicability_ =
		BuildingExtentApplicability::NotApplicable;
	std::optional<BoundaryRepresentationKind> representation_kind_;
	BuildingExtentSourceKind source_kind_ = BuildingExtentSourceKind::None;
	std::string provenance_;
};
