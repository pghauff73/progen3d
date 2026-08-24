#include "vehicle/mcsmv2/service/McsMv22KinematicAssuranceEvaluationService.h"

#include <utility>
#include <vector>

McsMv22KinematicAssuranceReport
McsMv22KinematicAssuranceEvaluationService::evaluate(
	const std::string &variant_identifier,
	bool source_release_passed,
	bool static_v2_geometry_passed,
	bool hardpoint_validation_passed,
	bool tyre_topology_and_pose_parity_passed,
	bool native_tyre_sweep_validation_passed,
	bool closure_binding_and_motion_passed,
	bool native_closure_sweep_validation_passed,
	bool glass_binding_and_motion_passed,
	bool native_glass_motion_validation_passed) const
{
	std::vector<std::string> diagnostics;
	auto require_gate = [&diagnostics](bool passed, const char *message) {
		if (!passed) diagnostics.emplace_back(message);
	};
	require_gate(source_release_passed, "The immutable MCSMv2.2 source release gate did not pass.");
	require_gate(static_v2_geometry_passed, "The inherited native MCSMv2.1 static V2 geometry did not pass.");
	require_gate(hardpoint_validation_passed, "The 32-hardpoint suspension definition did not pass native validation.");
	require_gate(tyre_topology_and_pose_parity_passed, "Native tyre topology or 36-pose transform parity failed.");
	require_gate(native_tyre_sweep_validation_passed, "Independent native tyre sweep validation did not pass.");
	require_gate(closure_binding_and_motion_passed, "Native closure binding or seven-state motion parity failed.");
	require_gate(native_closure_sweep_validation_passed, "Independent native closure sweep validation did not pass.");
	require_gate(glass_binding_and_motion_passed, "Native parent-relative glass binding or motion parity failed.");
	require_gate(native_glass_motion_validation_passed, "Independent native helical glass validation did not pass.");

	const bool v0 = source_release_passed;
	const bool v1 = v0 && hardpoint_validation_passed;
	const bool v2 = v1 && static_v2_geometry_passed;
	const bool v3 =
		v2 && tyre_topology_and_pose_parity_passed &&
		native_tyre_sweep_validation_passed && closure_binding_and_motion_passed &&
		native_closure_sweep_validation_passed && glass_binding_and_motion_passed &&
		native_glass_motion_validation_passed;
	std::vector<McsMv22AssuranceLevelStatus> levels;
	levels.emplace_back("V0", v0, "Immutable source identity and schema are verified.", "No native geometry claim.");
	levels.emplace_back("V1", v1, "Concept hardpoints and explicit kinematic definitions are represented.", "No static body certification.");
	levels.emplace_back("V2", v2, "Accepted native static semantic body geometry is reused.", "Static concept geometry only.");
	levels.emplace_back("V3", v3, "Sampled concept suspension, tyre, closure, and side-glass kinematics are reproduced.", "Sampled source-accepted concept states; not continuous-interval proof.");
	levels.emplace_back("V4", false, "Production subsystem engineering.", "Unclaimed by the source release.");
	levels.emplace_back("V5", false, "Manufacturing-release engineering.", "Unclaimed by the source release.");
	return McsMv22KinematicAssuranceReport(
		variant_identifier, std::move(levels), std::move(diagnostics));
}
