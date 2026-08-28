#pragma once

#include "vehicle/parametric/model/GeneratedVehicleRealization.h"
#include "vehicle/parametric/model/ModernCarCoordinateFrame.h"
#include "vehicle/parametric/model/ModernCarFamilyDefinition.h"
#include "vehicle/parametric/model/ParametricModelGenerationPolicy.h"
#include "vehicle/parametric/model/ParametricModelSourceManifest.h"
#include "vehicle/parametric/model/ParametricVehicleValidationReport.h"

#include <string>
#include <utility>
#include <vector>

class ModernCarParametricObjectModel
{
public:
	ModernCarParametricObjectModel(
		std::string schema,
		ModernCarCoordinateFrame coordinate_frame,
		ModernCarFamilyDefinition family,
		ParametricModelSourceManifest source_manifest,
		ParametricModelGenerationPolicy generation_policy,
		std::vector<GeneratedVehicleRealization> realizations = {},
		std::vector<ParametricVehicleValidationReport> validation_reports = {})
		: schema_(std::move(schema)),
		  coordinate_frame_(std::move(coordinate_frame)),
		  family_(std::move(family)),
		  source_manifest_(std::move(source_manifest)),
		  generation_policy_(std::move(generation_policy)),
		  realizations_(std::move(realizations)),
		  validation_reports_(std::move(validation_reports))
	{
	}

	const std::string &schema() const { return schema_; }
	const ModernCarCoordinateFrame &coordinateFrame() const
	{
		return coordinate_frame_;
	}
	const ModernCarFamilyDefinition &family() const { return family_; }
	const ParametricModelSourceManifest &sourceManifest() const
	{
		return source_manifest_;
	}
	const ParametricModelGenerationPolicy &generationPolicy() const
	{
		return generation_policy_;
	}
	const std::vector<GeneratedVehicleRealization> &realizations() const
	{
		return realizations_;
	}
	const std::vector<ParametricVehicleValidationReport> &validationReports() const
	{
		return validation_reports_;
	}

	ModernCarParametricObjectModel withRealizations(
		std::vector<GeneratedVehicleRealization> realizations,
		std::vector<ParametricVehicleValidationReport> validation_reports) const
	{
		return ModernCarParametricObjectModel(
			schema_, coordinate_frame_, family_, source_manifest_, generation_policy_,
			std::move(realizations), std::move(validation_reports));
	}

private:
	std::string schema_;
	ModernCarCoordinateFrame coordinate_frame_;
	ModernCarFamilyDefinition family_{"", {}};
	ParametricModelSourceManifest source_manifest_{"", "", "", {}, ""};
	ParametricModelGenerationPolicy generation_policy_{
		"", ParametricGenerationResolution::Low, 0, 0, 0, 0, false};
	std::vector<GeneratedVehicleRealization> realizations_;
	std::vector<ParametricVehicleValidationReport> validation_reports_;
};
