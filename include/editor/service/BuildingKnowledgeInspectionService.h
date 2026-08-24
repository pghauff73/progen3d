#pragma once

class SmallModernBuildingModel;
class SpatialObjectId;
class SpatialObjectInspection;

class BuildingKnowledgeInspectionService {
public:
	void appendKnowledge(
		const SmallModernBuildingModel &building_model,
		const SpatialObjectId &object_id,
		SpatialObjectInspection *inspection) const;
};
