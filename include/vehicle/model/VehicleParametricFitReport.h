#pragma once

#include "vehicle/model/VehicleFittingEvidence.h"

#include <glm/glm.hpp>

#include <cmath>
#include <string>
#include <utility>
#include <vector>

class VehicleSilhouetteAffineTransform
{
public:
	VehicleSilhouetteAffineTransform(
		glm::dvec2 scale = glm::dvec2(1.0),
		double longitudinal_height_shear = 0.0,
		glm::dvec2 translation = glm::dvec2(0.0))
		: scale_(scale),
		  longitudinal_height_shear_(longitudinal_height_shear),
		  translation_(translation)
	{
	}

	const glm::dvec2 &scale() const { return scale_; }
	double longitudinalHeightShear() const
	{
		return longitudinal_height_shear_;
	}
	const glm::dvec2 &translation() const { return translation_; }
	glm::dvec2 transform(const glm::dvec2 &source_point) const
	{
		return {
			source_point.x * scale_.x + translation_.x,
			(source_point.y + source_point.x * longitudinal_height_shear_) *
				scale_.y + translation_.y};
	}
	bool isFiniteAndNonDegenerate() const
	{
		return std::isfinite(scale_.x) && std::isfinite(scale_.y) &&
		       scale_.x > 0.0 && scale_.y > 0.0 &&
		       std::isfinite(longitudinal_height_shear_) &&
		       std::isfinite(translation_.x) && std::isfinite(translation_.y);
	}

private:
	glm::dvec2 scale_{1.0};
	double longitudinal_height_shear_ = 0.0;
	glm::dvec2 translation_{0.0};
};

class VehicleParametricViewFit
{
public:
	VehicleParametricViewFit(
		VehicleReferenceView view,
		double silhouette_intersection_over_union,
		double required_intersection_over_union,
		VehicleSilhouetteAffineTransform source_to_authority_transform = {})
		: view_(view),
		  silhouette_intersection_over_union_(silhouette_intersection_over_union),
		  required_intersection_over_union_(required_intersection_over_union),
		  source_to_authority_transform_(std::move(source_to_authority_transform))
	{
	}

	VehicleReferenceView view() const { return view_; }
	double silhouetteIntersectionOverUnion() const
	{
		return silhouette_intersection_over_union_;
	}
	double requiredIntersectionOverUnion() const
	{
		return required_intersection_over_union_;
	}
	const glm::dvec2 &sourceToAuthorityScale() const
	{
		return source_to_authority_transform_.scale();
	}
	double sourceToAuthorityLongitudinalHeightShear() const
	{
		return source_to_authority_transform_.longitudinalHeightShear();
	}
	const glm::dvec2 &sourceToAuthorityTranslation() const
	{
		return source_to_authority_transform_.translation();
	}
	const VehicleSilhouetteAffineTransform &sourceToAuthorityTransform() const
	{
		return source_to_authority_transform_;
	}
	bool isFiniteAndOrdered() const
	{
		return std::isfinite(silhouette_intersection_over_union_) &&
		       std::isfinite(required_intersection_over_union_) &&
		       silhouette_intersection_over_union_ >= 0.0 &&
		       silhouette_intersection_over_union_ <= 1.0 &&
		       required_intersection_over_union_ >= 0.0 &&
		       required_intersection_over_union_ <= 1.0 &&
		       source_to_authority_transform_.isFiniteAndNonDegenerate();
	}
	bool passes() const
	{
		return silhouette_intersection_over_union_ >=
		       required_intersection_over_union_;
	}

private:
	VehicleReferenceView view_ = VehicleReferenceView::Front;
	double silhouette_intersection_over_union_ = 0.0;
	double required_intersection_over_union_ = 0.80;
	VehicleSilhouetteAffineTransform source_to_authority_transform_;
};

class VehicleParametricFitReport
{
public:
	VehicleParametricFitReport(
		std::string identifier,
		std::vector<VehicleParametricViewFit> view_fits,
		double maximum_package_residual,
		bool package_authority_preserved,
		std::vector<std::string> blocking_diagnostics)
		: identifier_(std::move(identifier)),
		  view_fits_(std::move(view_fits)),
		  maximum_package_residual_(maximum_package_residual),
		  package_authority_preserved_(package_authority_preserved),
		  blocking_diagnostics_(std::move(blocking_diagnostics))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::vector<VehicleParametricViewFit> &viewFits() const
	{
		return view_fits_;
	}
	double maximumPackageResidual() const { return maximum_package_residual_; }
	bool packageAuthorityPreserved() const { return package_authority_preserved_; }
	const std::vector<std::string> &blockingDiagnostics() const
	{
		return blocking_diagnostics_;
	}
	bool isValid() const
	{
		for (const VehicleParametricViewFit &view_fit : view_fits_) {
			if (!view_fit.isFiniteAndOrdered()) return false;
		}
		return package_authority_preserved_ && blocking_diagnostics_.empty();
	}

private:
	std::string identifier_;
	std::vector<VehicleParametricViewFit> view_fits_;
	double maximum_package_residual_ = 0.0;
	bool package_authority_preserved_ = false;
	std::vector<std::string> blocking_diagnostics_;
};
