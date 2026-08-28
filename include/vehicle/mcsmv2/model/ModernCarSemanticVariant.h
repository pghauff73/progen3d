#pragma once

#include "vehicle/mcsmv2/model/ModernCarClosureDefinition.h"
#include "vehicle/mcsmv2/model/ImplicitFieldCalibration.h"
#include "vehicle/mcsmv2/model/ModernCarParameterDependencyGraph.h"
#include "vehicle/mcsmv2/model/ModernCarPlatformDefinition.h"
#include "vehicle/mcsmv2/model/ModernCarPowertrainDefinition.h"
#include "vehicle/mcsmv2/model/ModernCarSemanticStyleDefinition.h"
#include "vehicle/mcsmv2/model/ModernCarSourceAssurance.h"
#include "vehicle/mcsmv2/model/VehicleBodyColor.h"
#include "vehicle/mcsmv2/model/VehicleReferenceFrameDefinition.h"
#include "vehicle/mcsmv2/model/VehicleSemanticSectionField.h"
#include "vehicle/mcsmv2/model/VehicleWheelMotionParameters.h"
#include "vehicle/parametric/model/VehiclePackageParameters.h"

#include <string>
#include <utility>

class ModernCarSemanticVariant
{
public:
	ModernCarSemanticVariant(
		std::string identifier,
		std::string display_name,
		std::string description,
		std::string package_basis,
		VehicleReferenceFrameDefinition reference_frame,
		ImplicitFieldCalibration field_calibration,
		ModernCarSourceAssurance source_assurance,
		VehiclePackageParameters package,
		ModernCarPlatformDefinition platform,
		ModernCarSemanticStyleDefinition style,
		VehicleWheelMotionParameters wheel_motion,
		ModernCarPowertrainDefinition powertrain,
		ModernCarClosureDefinition closures,
		VehicleBodyColor body_color,
		bool has_crossover_cladding,
		ModernCarParameterDependencyGraph parameter_dependency_graph,
		VehicleSemanticSectionField section_field)
		: identifier_(std::move(identifier)),
		  display_name_(std::move(display_name)),
		  description_(std::move(description)),
		  package_basis_(std::move(package_basis)),
		  reference_frame_(std::move(reference_frame)),
		  field_calibration_(std::move(field_calibration)),
		  source_assurance_(std::move(source_assurance)),
		  package_(std::move(package)),
		  platform_(std::move(platform)),
		  style_(std::move(style)),
		  wheel_motion_(std::move(wheel_motion)),
		  powertrain_(std::move(powertrain)),
		  closures_(std::move(closures)),
		  body_color_(body_color),
		  has_crossover_cladding_(has_crossover_cladding),
		  parameter_dependency_graph_(std::move(parameter_dependency_graph)),
		  section_field_(std::move(section_field))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &displayName() const { return display_name_; }
	const std::string &description() const { return description_; }
	const std::string &packageBasis() const { return package_basis_; }
	const VehicleReferenceFrameDefinition &referenceFrame() const
	{
		return reference_frame_;
	}
	const ImplicitFieldCalibration &fieldCalibration() const
	{
		return field_calibration_;
	}
	const ModernCarSourceAssurance &sourceAssurance() const
	{
		return source_assurance_;
	}
	const VehiclePackageParameters &package() const { return package_; }
	const ModernCarPlatformDefinition &platform() const { return platform_; }
	const ModernCarSemanticStyleDefinition &style() const { return style_; }
	const VehicleWheelMotionParameters &wheelMotion() const { return wheel_motion_; }
	const ModernCarPowertrainDefinition &powertrain() const { return powertrain_; }
	const ModernCarClosureDefinition &closures() const { return closures_; }
	const VehicleBodyColor &bodyColor() const { return body_color_; }
	bool hasCrossoverCladding() const { return has_crossover_cladding_; }
	const ModernCarParameterDependencyGraph &parameterDependencyGraph() const
	{
		return parameter_dependency_graph_;
	}
	const VehicleSemanticSectionField &sectionField() const { return section_field_; }

private:
	std::string identifier_;
	std::string display_name_;
	std::string description_;
	std::string package_basis_;
	VehicleReferenceFrameDefinition reference_frame_{
		"", "", glm::dvec3(0.0), glm::dvec3(0.0),
		glm::dvec3(1.0, 0.0, 0.0), glm::dvec3(0.0, 1.0, 0.0),
		glm::dvec3(0.0, 0.0, 1.0), "", "", "",
		VehicleReferenceFrameDefinition::TransformationMatrix{{}}, 0.0};
	ImplicitFieldCalibration field_calibration_{
		"", "", 0, glm::dvec3(1.0), glm::dvec3(0.0), 0.0, false, 0.0,
		glm::dvec3(0.0), glm::dvec3(0.0), glm::dvec3(0.0), glm::dvec3(0.0),
		0.0};
	ModernCarSourceAssurance source_assurance_{"", "", "", false, {}, {}};
	VehiclePackageParameters package_{"", 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
	ModernCarPlatformDefinition platform_{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
	ModernCarSemanticStyleDefinition style_{
		0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
		0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
	VehicleWheelMotionParameters wheel_motion_{
		VehicleWheelParameters(0.0, 0.0, 0.0, 0.0), 0.0, 0.0, 0.0, 0.0, 0.0};
	ModernCarPowertrainDefinition powertrain_{"", "", false, 0.0, 0.0, 0.0, 0.0, 0};
	ModernCarClosureDefinition closures_{0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
	VehicleBodyColor body_color_{0, 0, 0};
	bool has_crossover_cladding_ = false;
	ModernCarParameterDependencyGraph parameter_dependency_graph_{"", {}};
	VehicleSemanticSectionField section_field_{"", {}};
};
