#pragma once

#include "vehicle/mcsmv2/model/McsMv22ClosureHingeDefinition.h"
#include "vehicle/mcsmv2/model/McsMv22HelicalGlassDefinition.h"
#include "vehicle/mcsmv2/model/McsMv22SurfaceOwnerDefinition.h"
#include "vehicle/mcsmv2/model/McsMv22SuspensionHardpointDefinition.h"
#include "vehicle/mcsmv2/model/McsMv22WheelPoseEvidence.h"

#include <string>
#include <utility>
#include <vector>

class McsMv22KinematicVariantDefinition
{
public:
	McsMv22KinematicVariantDefinition(
		std::string identifier,
		std::string display_name,
		std::string model_schema,
		std::vector<McsMv22SuspensionHardpointDefinition> hardpoints,
		std::vector<McsMv22WheelPoseEvidence> accepted_wheel_poses,
		std::vector<McsMv22ClosureHingeDefinition> hinges,
		std::vector<McsMv22HelicalGlassDefinition> glass_systems,
		std::vector<McsMv22SurfaceOwnerDefinition> panel_owners,
		std::vector<McsMv22SurfaceOwnerDefinition> aperture_owners,
		double wheel_radius_metres,
		double wheel_width_metres,
		double minimum_steer_degrees,
		double maximum_steer_degrees,
		double minimum_travel_metres,
		double maximum_travel_metres,
		double declared_tyre_clearance_metres,
		double accepted_minimum_tyre_clearance_metres,
		double tyre_tessellation_tolerance_metres,
		bool source_release_gate_pass,
		std::string assurance_level)
		: identifier_(std::move(identifier)),
		  display_name_(std::move(display_name)),
		  model_schema_(std::move(model_schema)),
		  hardpoints_(std::move(hardpoints)),
		  accepted_wheel_poses_(std::move(accepted_wheel_poses)),
		  hinges_(std::move(hinges)),
		  glass_systems_(std::move(glass_systems)),
		  panel_owners_(std::move(panel_owners)),
		  aperture_owners_(std::move(aperture_owners)),
		  wheel_radius_metres_(wheel_radius_metres),
		  wheel_width_metres_(wheel_width_metres),
		  minimum_steer_degrees_(minimum_steer_degrees),
		  maximum_steer_degrees_(maximum_steer_degrees),
		  minimum_travel_metres_(minimum_travel_metres),
		  maximum_travel_metres_(maximum_travel_metres),
		  declared_tyre_clearance_metres_(declared_tyre_clearance_metres),
		  accepted_minimum_tyre_clearance_metres_(accepted_minimum_tyre_clearance_metres),
		  tyre_tessellation_tolerance_metres_(tyre_tessellation_tolerance_metres),
		  source_release_gate_pass_(source_release_gate_pass),
		  assurance_level_(std::move(assurance_level))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &displayName() const { return display_name_; }
	const std::string &modelSchema() const { return model_schema_; }
	const std::vector<McsMv22SuspensionHardpointDefinition> &hardpoints() const
	{
		return hardpoints_;
	}
	const std::vector<McsMv22WheelPoseEvidence> &acceptedWheelPoses() const
	{
		return accepted_wheel_poses_;
	}
	const std::vector<McsMv22ClosureHingeDefinition> &hinges() const
	{
		return hinges_;
	}
	const std::vector<McsMv22HelicalGlassDefinition> &glassSystems() const
	{
		return glass_systems_;
	}
	const std::vector<McsMv22SurfaceOwnerDefinition> &panelOwners() const
	{
		return panel_owners_;
	}
	const std::vector<McsMv22SurfaceOwnerDefinition> &apertureOwners() const
	{
		return aperture_owners_;
	}
	double wheelRadiusMetres() const { return wheel_radius_metres_; }
	double wheelWidthMetres() const { return wheel_width_metres_; }
	double minimumSteerDegrees() const { return minimum_steer_degrees_; }
	double maximumSteerDegrees() const { return maximum_steer_degrees_; }
	double minimumTravelMetres() const { return minimum_travel_metres_; }
	double maximumTravelMetres() const { return maximum_travel_metres_; }
	double declaredTyreClearanceMetres() const
	{
		return declared_tyre_clearance_metres_;
	}
	double acceptedMinimumTyreClearanceMetres() const
	{
		return accepted_minimum_tyre_clearance_metres_;
	}
	double tyreTessellationToleranceMetres() const
	{
		return tyre_tessellation_tolerance_metres_;
	}
	bool sourceReleaseGatePass() const { return source_release_gate_pass_; }
	const std::string &assuranceLevel() const { return assurance_level_; }

private:
	std::string identifier_;
	std::string display_name_;
	std::string model_schema_;
	std::vector<McsMv22SuspensionHardpointDefinition> hardpoints_;
	std::vector<McsMv22WheelPoseEvidence> accepted_wheel_poses_;
	std::vector<McsMv22ClosureHingeDefinition> hinges_;
	std::vector<McsMv22HelicalGlassDefinition> glass_systems_;
	std::vector<McsMv22SurfaceOwnerDefinition> panel_owners_;
	std::vector<McsMv22SurfaceOwnerDefinition> aperture_owners_;
	double wheel_radius_metres_ = 0.0;
	double wheel_width_metres_ = 0.0;
	double minimum_steer_degrees_ = 0.0;
	double maximum_steer_degrees_ = 0.0;
	double minimum_travel_metres_ = 0.0;
	double maximum_travel_metres_ = 0.0;
	double declared_tyre_clearance_metres_ = 0.0;
	double accepted_minimum_tyre_clearance_metres_ = 0.0;
	double tyre_tessellation_tolerance_metres_ = 0.0;
	bool source_release_gate_pass_ = false;
	std::string assurance_level_;
};
