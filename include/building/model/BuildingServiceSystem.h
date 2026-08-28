#pragma once

#include "building/model/BuildingServiceMedium.h"
#include "building/model/BuildingServiceSystemId.h"
#include "spatial/model/SpatialObjectId.h"

#include <string>
#include <utility>
#include <vector>

class BuildingServiceSystem {
public:
	BuildingServiceSystem(BuildingServiceSystemId system_id,
	                      SpatialObjectId owner_object_id,
	                      std::string purpose,
	                      std::vector<BuildingServiceMedium> supported_media)
		: system_id_(std::move(system_id)),
		  owner_object_id_(std::move(owner_object_id)),
		  purpose_(std::move(purpose)),
		  supported_media_(std::move(supported_media)) {}

	const BuildingServiceSystemId &systemId() const { return system_id_; }
	const SpatialObjectId &ownerObjectId() const { return owner_object_id_; }
	const std::string &purpose() const { return purpose_; }
	const std::vector<BuildingServiceMedium> &supportedMedia() const
	{
		return supported_media_;
	}

private:
	BuildingServiceSystemId system_id_;
	SpatialObjectId owner_object_id_;
	std::string purpose_;
	std::vector<BuildingServiceMedium> supported_media_;
};
