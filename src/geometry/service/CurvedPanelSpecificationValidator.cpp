#include "geometry/service/CurvedPanelSpecificationValidator.h"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <utility>

std::shared_ptr<const CurvedPanelShapeSpecification>
CurvedPanelSpecificationValidator::validate(
	CurvedPanelShapeSpecificationCandidate candidate,
	std::string *diagnostic) const
{
	if (!std::isfinite(candidate.width) || !std::isfinite(candidate.height) ||
	    !std::isfinite(candidate.horizontal_curvature) ||
	    !std::isfinite(candidate.vertical_curvature) ||
	    !std::isfinite(candidate.thickness) || candidate.width <= 0.0f ||
	    candidate.height <= 0.0f || candidate.thickness < 0.0f) {
		if (diagnostic != nullptr) {
			*diagnostic = "CurvedPanel dimensions and curvature must be finite; width and height must be positive and thickness non-negative.";
		}
		return {};
	}
	if (candidate.horizontal_segments < 1 || candidate.vertical_segments < 1) {
		if (diagnostic != nullptr) {
			*diagnostic = "CurvedPanel requires at least one segment on each axis.";
		}
		return {};
	}
	const std::size_t grid_vertices =
		static_cast<std::size_t>(candidate.horizontal_segments + 1) *
		static_cast<std::size_t>(candidate.vertical_segments + 1);
	const std::size_t vertex_count =
		grid_vertices * (candidate.thickness > 0.0f ? 2u : 1u);
	const std::size_t triangle_count =
		static_cast<std::size_t>(candidate.horizontal_segments) *
		static_cast<std::size_t>(candidate.vertical_segments) * 2u *
		(candidate.thickness > 0.0f ? 2u : 1u) +
		(candidate.thickness > 0.0f
			? static_cast<std::size_t>(
				candidate.horizontal_segments + candidate.vertical_segments) * 4u
			: 0u);
	if (vertex_count > complexity_limits_.maximumGeneratedVertices() ||
	    triangle_count > complexity_limits_.maximumGeneratedTriangles()) {
		if (diagnostic != nullptr) {
			*diagnostic = "CurvedPanel exceeds the configured generated mesh limits.";
		}
		return {};
	}

	std::ostringstream key;
	key << std::hexfloat
	    << "CurvedPanel:v1:w=" << candidate.width
	    << ":h=" << candidate.height
	    << ":cx=" << candidate.horizontal_curvature
	    << ":cy=" << candidate.vertical_curvature
	    << ":t=" << candidate.thickness
	    << ":segments=" << candidate.horizontal_segments << ","
	    << candidate.vertical_segments;
	std::ostringstream canonical;
	canonical << "CurvedPanel(width(" << candidate.width << ") height("
	          << candidate.height << ") curvature("
	          << candidate.horizontal_curvature << " "
	          << candidate.vertical_curvature << ") thickness("
	          << candidate.thickness << ") segments("
	          << candidate.horizontal_segments << " "
	          << candidate.vertical_segments << "))";
	return std::make_shared<const CurvedPanelShapeSpecification>(
		candidate.width, candidate.height,
		candidate.horizontal_curvature, candidate.vertical_curvature,
		candidate.thickness, candidate.horizontal_segments,
		candidate.vertical_segments, ShapeSpecificationKey(key.str()),
		canonical.str(), candidate.detail_level);
}
