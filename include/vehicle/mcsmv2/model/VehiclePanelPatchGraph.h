#pragma once

#include "vehicle/mcsmv2/model/VehiclePanelAdjacencyRelationship.h"
#include "vehicle/mcsmv2/model/VehiclePanelPatch.h"

#include <string>
#include <utility>
#include <vector>

class VehiclePanelPatchGraph
{
public:
	VehiclePanelPatchGraph(
		std::string variant_identifier,
		std::vector<VehiclePanelPatch> patches,
		std::vector<VehiclePanelAdjacencyRelationship> relationships)
		: variant_identifier_(std::move(variant_identifier)),
		  patches_(std::move(patches)),
		  relationships_(std::move(relationships))
	{
	}

	const std::string &variantIdentifier() const { return variant_identifier_; }
	const std::vector<VehiclePanelPatch> &patches() const { return patches_; }
	const std::vector<VehiclePanelAdjacencyRelationship> &relationships() const
	{
		return relationships_;
	}

private:
	std::string variant_identifier_;
	std::vector<VehiclePanelPatch> patches_;
	std::vector<VehiclePanelAdjacencyRelationship> relationships_;
};
