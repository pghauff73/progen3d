#pragma once

#include "vehicle/mcsmv2/model/McsMv220SourceRelease.h"
#include "vehicle/mcsmv2/model/McsMv22KinematicGeometry.h"
#include "vehicle/mcsmv2/model/McsMv22KinematicVariantDefinition.h"
#include "vehicle/mcsmv2/model/McsMv22NativeKinematicEvidenceGeometry.h"
#include "vehicle/mcsmv2/service/McsMv22ClosureKinematicEvaluationService.h"
#include "vehicle/mcsmv2/service/McsMv22KinematicAssuranceEvaluationService.h"
#include "vehicle/mcsmv2/service/McsMv22SemanticKinematicBindingService.h"
#include "vehicle/mcsmv2/service/McsMv22SuspensionKinematicEvaluationService.h"
#include "vehicle/service/VehicleClosureSweepValidationService.h"
#include "vehicle/service/VehicleGlassMotionValidationService.h"
#include "vehicle/service/VehicleReferenceFrameTransformationService.h"
#include "vehicle/service/VehicleTyreMeshGenerationService.h"
#include "vehicle/service/VehicleTyreSweepValidationService.h"

#include <memory>
#include <optional>
#include <string>

class GeneratedMcsMv22KinematicGeometryResult
{
public:
	GeneratedMcsMv22KinematicGeometryResult(
		std::optional<McsMv22KinematicGeometry> generated_geometry,
		std::string diagnostic)
		: generated_geometry_(std::move(generated_geometry)),
		  diagnostic_(std::move(diagnostic))
	{
	}

	bool succeeded() const
	{
		return generated_geometry_.has_value() &&
		       generated_geometry_->assuranceReport().achievedV3();
	}
	const std::optional<McsMv22KinematicGeometry> &generatedGeometry() const
	{
		return generated_geometry_;
	}
	const std::string &diagnostic() const { return diagnostic_; }

private:
	std::optional<McsMv22KinematicGeometry> generated_geometry_;
	std::string diagnostic_;
};

class McsMv22KinematicGeometryGenerationService
{
public:
	GeneratedMcsMv22KinematicGeometryResult generate(
		std::shared_ptr<const McsMv21SemanticGeometry> semantic_geometry,
		const McsMv220SourceRelease &source_release,
		const McsMv22KinematicVariantDefinition &kinematic_definition,
		const McsMv22NativeKinematicEvidenceGeometry &evidence_geometry) const;

private:
	McsMv22SuspensionKinematicEvaluationService suspension_service_;
	McsMv22ClosureKinematicEvaluationService closure_service_;
	McsMv22SemanticKinematicBindingService semantic_binding_service_;
	VehicleTyreMeshGenerationService tyre_mesh_service_;
	VehicleTyreSweepValidationService tyre_validation_service_;
	VehicleClosureSweepValidationService closure_validation_service_;
	VehicleGlassMotionValidationService glass_validation_service_;
	VehicleReferenceFrameTransformationService frame_service_;
	McsMv22KinematicAssuranceEvaluationService assurance_service_;
};
