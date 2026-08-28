#pragma once

#include "spatial/model/AxisAlignedBoundingBoundary.h"

#include <memory>
#include <utility>
#include <vector>

class SpatialBoundaryModel {
public:
	SpatialBoundaryModel() = default;
	explicit SpatialBoundaryModel(
		std::vector<std::shared_ptr<const BoundaryRepresentation>> representations)
		: representations_(std::move(representations)) {}

	const std::vector<std::shared_ptr<const BoundaryRepresentation>> &representations() const
	{
		return representations_;
	}

	const AxisAlignedBoundingBoundary *axisAlignedBoundary() const
	{
		for (const auto &representation : representations_) {
			if (representation &&
			    representation->kind() == BoundaryRepresentationKind::AxisAlignedBounding) {
				return static_cast<const AxisAlignedBoundingBoundary *>(representation.get());
			}
		}
		return nullptr;
	}

private:
	std::vector<std::shared_ptr<const BoundaryRepresentation>> representations_;
};
