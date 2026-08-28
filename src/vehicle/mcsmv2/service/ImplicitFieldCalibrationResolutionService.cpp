#include "vehicle/mcsmv2/service/ImplicitFieldCalibrationResolutionService.h"

#include "geometry/service/ImplicitSurfaceMeshingService.h"
#include "vehicle/mcsmv2/model/InverseAffinePrewarpedField.h"

#include <glm/common.hpp>

#include <cmath>
#include <memory>

ImplicitFieldCalibrationResolutionResult
ImplicitFieldCalibrationResolutionService::resolve(
	std::shared_ptr<const ImplicitScalarField> source_field,
	const ImplicitFieldCalibration &accepted_calibration,
	const ImplicitSurfaceGenerationRequest &request) const
{
	if (!source_field) {
		return ImplicitFieldCalibrationResolutionResult(
			std::nullopt, "Field calibration requires a source scalar field.");
	}
	glm::dvec3 prewarp_scale = accepted_calibration.prewarpScale();
	glm::dvec3 prewarp_translation = accepted_calibration.prewarpTranslation();
	const glm::dvec3 target_minimum = accepted_calibration.targetBoundsMinimum();
	const glm::dvec3 target_maximum = accepted_calibration.targetBoundsMaximum();
	const glm::dvec3 target_extent = target_maximum - target_minimum;
	const glm::dvec3 target_center = 0.5 * (target_minimum + target_maximum);

	for (int iteration = 0;
	     iteration < accepted_calibration.calibrationIterations(); ++iteration) {
		const ImplicitFieldCalibration candidate_calibration =
			accepted_calibration.resolvePrewarp(
				prewarp_scale, prewarp_translation);
		const InverseAffinePrewarpedField candidate_field(
			source_field, candidate_calibration);
		const ImplicitSurfaceGenerationResult candidate =
			ImplicitSurfaceMeshingService().generate(candidate_field, request);
		if (!candidate.succeeded() || !candidate.generatedMesh() ||
		    !candidate.generatedMesh()->mesh()) {
			return ImplicitFieldCalibrationResolutionResult(
				std::nullopt,
				candidate.firstDiagnostic().empty()
					? "Field calibration did not produce a candidate mesh."
					: candidate.firstDiagnostic());
		}
		const ImplicitFieldBounds measured_bounds = candidate.generatedBounds();
		const glm::dvec3 measured_extent = measured_bounds.dimensions();
		if (!(measured_extent.x > 1.0e-12) ||
		    !(measured_extent.y > 1.0e-12) ||
		    !(measured_extent.z > 1.0e-12)) {
			return ImplicitFieldCalibrationResolutionResult(
				std::nullopt, "Field calibration produced an invalid mesh extent.");
		}
		const glm::dvec3 measured_center =
			0.5 * (measured_bounds.minimum() + measured_bounds.maximum());
		const glm::dvec3 update_ratio = target_extent / measured_extent;
		prewarp_translation = target_center +
			update_ratio * (prewarp_translation - measured_center);
		prewarp_scale = update_ratio * prewarp_scale;
	}

	return ImplicitFieldCalibrationResolutionResult(
		accepted_calibration.resolvePrewarp(
			prewarp_scale, prewarp_translation),
		"");
}
