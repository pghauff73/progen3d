#pragma once

#include "building/model/BuildingExtentDescriptor.h"

#include <utility>
#include <vector>

class BuildingObjectExtentSet {
public:
	BuildingObjectExtentSet() = default;
	explicit BuildingObjectExtentSet(std::vector<BuildingExtentDescriptor> extents)
		: extents_(std::move(extents)) {}

	const std::vector<BuildingExtentDescriptor> &extents() const { return extents_; }

	const BuildingExtentDescriptor *find(BuildingExtentKind extent_kind) const
	{
		for (const BuildingExtentDescriptor &extent : extents_) {
			if (extent.extentKind() == extent_kind) return &extent;
		}
		return nullptr;
	}

private:
	std::vector<BuildingExtentDescriptor> extents_;
};
