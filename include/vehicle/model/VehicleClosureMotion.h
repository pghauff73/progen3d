#pragma once

#include "vehicle/model/VehicleClosureMotionValidation.h"

#include <glm/glm.hpp>

#include <string>
#include <utility>
#include <vector>

class VehicleRisingRevoluteJoint
{
public:
	VehicleRisingRevoluteJoint(
		std::string identifier,
		glm::dvec3 source_pivot,
		glm::dvec3 source_axis,
		double maximum_angle_degrees,
		double direction,
		double maximum_rise_metres,
		glm::dvec3 source_translation_axis,
		std::string joint_type)
		: identifier_(std::move(identifier)),
		  source_pivot_(source_pivot),
		  source_axis_(source_axis),
		  maximum_angle_degrees_(maximum_angle_degrees),
		  direction_(direction),
		  maximum_rise_metres_(maximum_rise_metres),
		  source_translation_axis_(source_translation_axis),
		  joint_type_(std::move(joint_type))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const glm::dvec3 &sourcePivot() const { return source_pivot_; }
	const glm::dvec3 &sourceAxis() const { return source_axis_; }
	double maximumAngleDegrees() const { return maximum_angle_degrees_; }
	double direction() const { return direction_; }
	double maximumRiseMetres() const { return maximum_rise_metres_; }
	const glm::dvec3 &sourceTranslationAxis() const
	{
		return source_translation_axis_;
	}
	const std::string &jointType() const { return joint_type_; }

private:
	std::string identifier_;
	glm::dvec3 source_pivot_{0.0};
	glm::dvec3 source_axis_{0.0, 0.0, 1.0};
	double maximum_angle_degrees_ = 0.0;
	double direction_ = 1.0;
	double maximum_rise_metres_ = 0.0;
	glm::dvec3 source_translation_axis_{0.0, 0.0, 1.0};
	std::string joint_type_;
};

class VehicleClosureSurfaceBinding
{
public:
	VehicleClosureSurfaceBinding(
		std::string surface_domain_identifier,
		std::string closure_identifier,
		std::size_t source_face_count,
		VehicleRisingRevoluteJoint joint)
		: surface_domain_identifier_(std::move(surface_domain_identifier)),
		  closure_identifier_(std::move(closure_identifier)),
		  source_face_count_(source_face_count),
		  joint_(std::move(joint))
	{
	}

	const std::string &surfaceDomainIdentifier() const
	{
		return surface_domain_identifier_;
	}
	const std::string &closureIdentifier() const { return closure_identifier_; }
	std::size_t sourceFaceCount() const { return source_face_count_; }
	const VehicleRisingRevoluteJoint &joint() const { return joint_; }

private:
	std::string surface_domain_identifier_;
	std::string closure_identifier_;
	std::size_t source_face_count_ = 0u;
	VehicleRisingRevoluteJoint joint_{"", {}, {}, 0.0, 1.0, 0.0, {}, ""};
};

class VehicleClosurePoseSample
{
public:
	VehicleClosurePoseSample(
		double normalized_state,
		double angle_degrees,
		double rise_metres,
		glm::dmat4 source_transform)
		: normalized_state_(normalized_state),
		  angle_degrees_(angle_degrees),
		  rise_metres_(rise_metres),
		  source_transform_(source_transform)
	{
	}

	VehicleClosurePoseSample(
		double normalized_state,
		double angle_degrees,
		double rise_metres,
		glm::dmat4 source_transform,
		VehicleClosurePoseValidation validation)
		: normalized_state_(normalized_state),
		  angle_degrees_(angle_degrees),
		  rise_metres_(rise_metres),
		  source_transform_(source_transform),
		  validation_(std::move(validation))
	{
	}

	double normalizedState() const { return normalized_state_; }
	double angleDegrees() const { return angle_degrees_; }
	double riseMetres() const { return rise_metres_; }
	const glm::dmat4 &sourceTransform() const { return source_transform_; }
	const VehicleClosurePoseValidation &validation() const { return validation_; }
	bool passed() const { return validation_.passed(); }

private:
	double normalized_state_ = 0.0;
	double angle_degrees_ = 0.0;
	double rise_metres_ = 0.0;
	glm::dmat4 source_transform_{1.0};
	VehicleClosurePoseValidation validation_;
};

class VehicleClosureSweep
{
public:
	VehicleClosureSweep(
		VehicleClosureSurfaceBinding surface_binding,
		std::vector<VehicleClosurePoseSample> pose_samples)
		: surface_binding_(std::move(surface_binding)),
		  pose_samples_(std::move(pose_samples))
	{
	}

	const VehicleClosureSurfaceBinding &surfaceBinding() const
	{
		return surface_binding_;
	}
	const std::vector<VehicleClosurePoseSample> &poseSamples() const
	{
		return pose_samples_;
	}
	bool passed() const
	{
		if (pose_samples_.empty()) return false;
		for (const VehicleClosurePoseSample &sample : pose_samples_) {
			if (!sample.passed()) return false;
		}
		return true;
	}

private:
	VehicleClosureSurfaceBinding surface_binding_{{}, {}, 0u, {"", {}, {}, 0.0, 1.0, 0.0, {}, ""}};
	std::vector<VehicleClosurePoseSample> pose_samples_;
};

class VehicleClosureSweepSet
{
public:
	explicit VehicleClosureSweepSet(std::vector<VehicleClosureSweep> sweeps)
		: sweeps_(std::move(sweeps))
	{
	}

	const std::vector<VehicleClosureSweep> &sweeps() const { return sweeps_; }

private:
	std::vector<VehicleClosureSweep> sweeps_;
};

class VehicleHelicalGlassMotion
{
public:
	VehicleHelicalGlassMotion(
		std::string identifier,
		std::string side,
		double travel_metres,
		double inward_metres,
		double longitudinal_metres,
		double rotation_degrees,
		glm::dvec3 source_pivot)
		: identifier_(std::move(identifier)),
		  side_(std::move(side)),
		  travel_metres_(travel_metres),
		  inward_metres_(inward_metres),
		  longitudinal_metres_(longitudinal_metres),
		  rotation_degrees_(rotation_degrees),
		  source_pivot_(source_pivot)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &side() const { return side_; }
	double travelMetres() const { return travel_metres_; }
	double inwardMetres() const { return inward_metres_; }
	double longitudinalMetres() const { return longitudinal_metres_; }
	double rotationDegrees() const { return rotation_degrees_; }
	const glm::dvec3 &sourcePivot() const { return source_pivot_; }

private:
	std::string identifier_;
	std::string side_;
	double travel_metres_ = 0.0;
	double inward_metres_ = 0.0;
	double longitudinal_metres_ = 0.0;
	double rotation_degrees_ = 0.0;
	glm::dvec3 source_pivot_{0.0};
};

class VehicleGlassSurfaceBinding
{
public:
	VehicleGlassSurfaceBinding(
		std::string aperture_domain_identifier,
		std::string parent_closure_identifier,
		std::size_t source_face_count,
		VehicleHelicalGlassMotion motion)
		: aperture_domain_identifier_(std::move(aperture_domain_identifier)),
		  parent_closure_identifier_(std::move(parent_closure_identifier)),
		  source_face_count_(source_face_count),
		  motion_(std::move(motion))
	{
	}

	const std::string &apertureDomainIdentifier() const
	{
		return aperture_domain_identifier_;
	}
	const std::string &parentClosureIdentifier() const
	{
		return parent_closure_identifier_;
	}
	std::size_t sourceFaceCount() const { return source_face_count_; }
	const VehicleHelicalGlassMotion &motion() const { return motion_; }

private:
	std::string aperture_domain_identifier_;
	std::string parent_closure_identifier_;
	std::size_t source_face_count_ = 0u;
	VehicleHelicalGlassMotion motion_{"", "", 0.0, 0.0, 0.0, 0.0, {}};
};

class VehicleGlassPoseSample
{
public:
	VehicleGlassPoseSample(
		double normalized_state,
		double vertical_drop_metres,
		double rotation_degrees,
		glm::dmat4 local_source_transform,
		glm::dmat4 world_source_transform)
		: normalized_state_(normalized_state),
		  vertical_drop_metres_(vertical_drop_metres),
		  rotation_degrees_(rotation_degrees),
		  local_source_transform_(local_source_transform),
		  world_source_transform_(world_source_transform)
	{
	}

	VehicleGlassPoseSample(
		double normalized_state,
		double vertical_drop_metres,
		double rotation_degrees,
		glm::dmat4 local_source_transform,
		glm::dmat4 world_source_transform,
		VehicleGlassPoseValidation validation)
		: normalized_state_(normalized_state),
		  vertical_drop_metres_(vertical_drop_metres),
		  rotation_degrees_(rotation_degrees),
		  local_source_transform_(local_source_transform),
		  world_source_transform_(world_source_transform),
		  validation_(std::move(validation))
	{
	}

	double normalizedState() const { return normalized_state_; }
	double verticalDropMetres() const { return vertical_drop_metres_; }
	double rotationDegrees() const { return rotation_degrees_; }
	const glm::dmat4 &localSourceTransform() const
	{
		return local_source_transform_;
	}
	const glm::dmat4 &worldSourceTransform() const
	{
		return world_source_transform_;
	}
	const VehicleGlassPoseValidation &validation() const { return validation_; }
	bool passed() const { return validation_.passed(); }

private:
	double normalized_state_ = 0.0;
	double vertical_drop_metres_ = 0.0;
	double rotation_degrees_ = 0.0;
	glm::dmat4 local_source_transform_{1.0};
	glm::dmat4 world_source_transform_{1.0};
	VehicleGlassPoseValidation validation_;
};

class VehicleGlassMotion
{
public:
	VehicleGlassMotion(
		VehicleGlassSurfaceBinding surface_binding,
		std::vector<VehicleGlassPoseSample> pose_samples)
		: surface_binding_(std::move(surface_binding)),
		  pose_samples_(std::move(pose_samples))
	{
	}

	const VehicleGlassSurfaceBinding &surfaceBinding() const
	{
		return surface_binding_;
	}
	const std::vector<VehicleGlassPoseSample> &poseSamples() const
	{
		return pose_samples_;
	}
	bool passed() const
	{
		if (pose_samples_.empty()) return false;
		for (const VehicleGlassPoseSample &sample : pose_samples_) {
			if (!sample.passed()) return false;
		}
		return true;
	}

private:
	VehicleGlassSurfaceBinding surface_binding_{{}, {}, 0u, {"", "", 0.0, 0.0, 0.0, 0.0, {}}};
	std::vector<VehicleGlassPoseSample> pose_samples_;
};

class VehicleGlassMotionSet
{
public:
	explicit VehicleGlassMotionSet(std::vector<VehicleGlassMotion> motions)
		: motions_(std::move(motions))
	{
	}

	const std::vector<VehicleGlassMotion> &motions() const { return motions_; }

private:
	std::vector<VehicleGlassMotion> motions_;
};
