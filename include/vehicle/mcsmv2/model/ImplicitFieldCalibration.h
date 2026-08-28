#pragma once

#include <glm/glm.hpp>

#include <algorithm>
#include <string>
#include <utility>

class ImplicitFieldCalibration
{
public:
	ImplicitFieldCalibration(
		std::string schema,
		std::string method,
		int calibration_iterations,
		glm::dvec3 prewarp_scale,
		glm::dvec3 prewarp_translation,
		double maximum_prewarp_fraction,
		bool post_mesh_affine_correction_applied,
		double maximum_post_mesh_correction_fraction,
		glm::dvec3 target_bounds_minimum,
		glm::dvec3 target_bounds_maximum,
		glm::dvec3 accepted_final_bounds_minimum,
		glm::dvec3 accepted_final_bounds_maximum,
		double package_tolerance)
		: schema_(std::move(schema)),
		  method_(std::move(method)),
		  calibration_iterations_(calibration_iterations),
		  prewarp_scale_(prewarp_scale),
		  prewarp_translation_(prewarp_translation),
		  maximum_prewarp_fraction_(maximum_prewarp_fraction),
		  post_mesh_affine_correction_applied_(
			  post_mesh_affine_correction_applied),
		  maximum_post_mesh_correction_fraction_(
			  maximum_post_mesh_correction_fraction),
		  target_bounds_minimum_(target_bounds_minimum),
		  target_bounds_maximum_(target_bounds_maximum),
		  accepted_final_bounds_minimum_(accepted_final_bounds_minimum),
		  accepted_final_bounds_maximum_(accepted_final_bounds_maximum),
		  package_tolerance_(package_tolerance)
	{
	}

	const std::string &schema() const { return schema_; }
	const std::string &method() const { return method_; }
	int calibrationIterations() const { return calibration_iterations_; }
	const glm::dvec3 &prewarpScale() const { return prewarp_scale_; }
	const glm::dvec3 &prewarpTranslation() const { return prewarp_translation_; }
	double maximumPrewarpFraction() const { return maximum_prewarp_fraction_; }
	bool postMeshAffineCorrectionApplied() const
	{
		return post_mesh_affine_correction_applied_;
	}
	double maximumPostMeshCorrectionFraction() const
	{
		return maximum_post_mesh_correction_fraction_;
	}
	const glm::dvec3 &targetBoundsMinimum() const
	{
		return target_bounds_minimum_;
	}
	const glm::dvec3 &targetBoundsMaximum() const
	{
		return target_bounds_maximum_;
	}
	const glm::dvec3 &acceptedFinalBoundsMinimum() const
	{
		return accepted_final_bounds_minimum_;
	}
	const glm::dvec3 &acceptedFinalBoundsMaximum() const
	{
		return accepted_final_bounds_maximum_;
	}
	double packageTolerance() const { return package_tolerance_; }

	ImplicitFieldCalibration resolvePrewarp(
		glm::dvec3 prewarp_scale,
		glm::dvec3 prewarp_translation) const
	{
		const glm::dvec3 delta = glm::abs(prewarp_scale - glm::dvec3(1.0));
		return ImplicitFieldCalibration(
			schema_,
			method_,
			calibration_iterations_,
			prewarp_scale,
			prewarp_translation,
			std::max({delta.x, delta.y, delta.z}),
			false,
			0.0,
			target_bounds_minimum_,
			target_bounds_maximum_,
			accepted_final_bounds_minimum_,
			accepted_final_bounds_maximum_,
			package_tolerance_);
	}

private:
	std::string schema_;
	std::string method_;
	int calibration_iterations_ = 0;
	glm::dvec3 prewarp_scale_{1.0};
	glm::dvec3 prewarp_translation_{0.0};
	double maximum_prewarp_fraction_ = 0.0;
	bool post_mesh_affine_correction_applied_ = false;
	double maximum_post_mesh_correction_fraction_ = 0.0;
	glm::dvec3 target_bounds_minimum_{0.0};
	glm::dvec3 target_bounds_maximum_{0.0};
	glm::dvec3 accepted_final_bounds_minimum_{0.0};
	glm::dvec3 accepted_final_bounds_maximum_{0.0};
	double package_tolerance_ = 0.0;
};
