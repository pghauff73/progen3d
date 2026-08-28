#pragma once

#include "spatial/model/SpatialObjectId.h"

#include <utility>

class SpatialFrameReference {
public:
	static SpatialFrameReference root()
	{
		return SpatialFrameReference(true, SpatialObjectId());
	}

	static SpatialFrameReference object(SpatialObjectId object_id)
	{
		return SpatialFrameReference(false, std::move(object_id));
	}

	bool isRoot() const { return root_; }
	const SpatialObjectId &objectId() const { return object_id_; }

private:
	SpatialFrameReference(bool root, SpatialObjectId object_id)
		: root_(root), object_id_(std::move(object_id)) {}

	bool root_ = true;
	SpatialObjectId object_id_;
};
