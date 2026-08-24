#pragma once

#include "vehicle/mcsmv2/model/McsMv22KinematicAssuranceReport.h"

#include <string>

class McsMv22KinematicAssuranceEvaluationService
{
public:
	McsMv22KinematicAssuranceReport evaluate(
		const std::string &variant_identifier,
		bool source_release_passed,
		bool static_v2_geometry_passed,
		bool hardpoint_validation_passed,
		bool tyre_topology_and_pose_parity_passed,
		bool native_tyre_sweep_validation_passed,
		bool closure_binding_and_motion_passed,
		bool native_closure_sweep_validation_passed,
		bool glass_binding_and_motion_passed,
		bool native_glass_motion_validation_passed) const;
};
