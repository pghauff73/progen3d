#pragma once

#include "editor/model/SpatialObjectInspection.h"

#include <cstddef>
#include <vector>

class SpatialBuildingModel;
class SpatialObjectId;
class SmallModernBuildingModel;

class SpatialObjectInspectionService
{
public:
	SpatialObjectInspection inspectObject(
		const SpatialBuildingModel &model,
		const SpatialObjectId &object_id) const;
	SpatialObjectInspection inspectObject(
		const SpatialBuildingModel &model,
		const SmallModernBuildingModel *building_model,
		const SpatialObjectId &object_id) const;

	SpatialObjectInspection inspectPrimitive(
		const SpatialBuildingModel &model,
		std::size_t primitive_instance_index) const;
	SpatialObjectInspection inspectPrimitive(
		const SpatialBuildingModel &model,
		const SmallModernBuildingModel *building_model,
		std::size_t primitive_instance_index) const;

	std::vector<SpatialObjectSelectionEntry> buildContainmentSelection(
		const SpatialBuildingModel &model) const;
};
