#include "architecture/service/WindowOpeningCollisionPositioningService.h"

#include "spatial/service/SpatialResolutionEvidenceHashService.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

OpeningBoundaryPlacementResult WindowOpeningCollisionPositioningService::position(
	const WindowFrameSpecification &frame,
	const WindowOpeningSpecification &opening,
	float tolerance) const
{
	if (!std::isfinite(frame.width()) || !std::isfinite(frame.height()) ||
	    !std::isfinite(frame.perimeterClearance()) ||
	    !std::isfinite(tolerance) || tolerance < 0.0f ||
	    frame.width() <= 0.0f || frame.height() <= 0.0f ||
	    frame.perimeterClearance() < 0.0f) {
		return OpeningBoundaryPlacementResult::failed(
			"Window opening positioning requires finite positive dimensions and nonnegative clearance.");
	}
	const float horizontal_gap =
		(opening.openingWidth() - frame.width()) * 0.5f;
	const float vertical_gap =
		(opening.openingHeight() - frame.height()) * 0.5f;
	if (horizontal_gap < frame.perimeterClearance() - tolerance ||
	    vertical_gap < frame.perimeterClearance() - tolerance) {
		return OpeningBoundaryPlacementResult::failed(
			"Window frame does not fit the opening with the requested perimeter clearance.");
	}

	const glm::mat4 final_transform = glm::translate(
		glm::mat4(1.0f),
		glm::vec3(opening.openingCenter(), opening.wallDepth() * 0.5f));
	const glm::vec4 gaps(
		horizontal_gap, horizontal_gap, vertical_gap, vertical_gap);
	const float residual = std::max(
		std::fabs(horizontal_gap - frame.perimeterClearance()),
		std::fabs(vertical_gap - frame.perimeterClearance()));
	SpatialResolutionRecord record(
		SpatialConstraintId("PositionWindowFrameInOpening"),
		SpatialObjectId(frame.objectIdentifier()),
		SpatialObjectId(opening.objectIdentifier()),
		glm::mat4(1.0f),
		final_transform,
		SpatialResolutionAlgorithm::OpeningBoundaryFit,
		4,
		0,
		4,
		tolerance,
		glm::vec3(opening.openingCenter(), opening.wallDepth() * 0.5f),
		glm::vec3(0.0f, 0.0f, 1.0f),
		std::min(horizontal_gap, vertical_gap),
		residual,
		SpatialResolutionStatus::Succeeded,
		{},
		0u);
	record = SpatialResolutionEvidenceHashService().attachCalculatedHash(record);
	return OpeningBoundaryPlacementResult::succeeded(
		final_transform, gaps, std::move(record));
}
