#pragma once

#include "vehicle/model/VehicleClosureGeometry25.h"

#include <glm/glm.hpp>

#include <cstddef>

class AutomotiveClosureGeometryService
{
public:
	glm::mat4 calculateClosureTransform(
		const ClosureHingeStudy &hinge_study,
		float normalized_state) const;

	float calculateClosureRise(
		const ClosureHingeStudy &hinge_study,
		float normalized_state) const;

	AxisAlignedBounds calculateClosureSweptBounds(
		const ClosureHingeStudy &hinge_study,
		std::size_t sample_count) const;

	glm::mat4 calculateHelicalGlassTransform(
		const HelicalGlassDrop &glass_drop,
		float normalized_state) const;

	AxisAlignedBounds calculateHelicalGlassSweptBounds(
		const HelicalGlassDrop &glass_drop,
		std::size_t sample_count) const;

	bool glassRemainsInsideDoorCavity(
		const HelicalGlassDrop &glass_drop,
		std::size_t sample_count,
		float tolerance) const;

	GlassChannel generateChannel(
		const std::string &identifier,
		const SideGlassSurface &glass_surface,
		bool front,
		float channel_width,
		float channel_depth,
		float clearance) const;

	VariableSealSectionStation interpolateSealSection(
		const VariableSealSweep &seal,
		float parameter) const;

	glm::mat4 composeClosureChildTransform(
		const ClosureHingeStudy &hinge_study,
		float normalized_state,
		const glm::mat4 &child_local_transform) const;
};
