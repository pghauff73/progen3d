#include "vehicle/mcsmv2/model/DifferenceField.h"
#include "vehicle/mcsmv2/model/InverseAffinePrewarpedField.h"
#include "vehicle/mcsmv2/model/PackageBoundaryField.h"
#include "vehicle/mcsmv2/model/SemanticSectionBodyField.h"
#include "vehicle/mcsmv2/model/SmoothMaximumField.h"
#include "vehicle/mcsmv2/model/SuperellipsoidField.h"

#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdexcept>

namespace {

void hashBytes(std::uint64_t *hash, const void *data, std::size_t size)
{
	const auto *bytes = static_cast<const unsigned char *>(data);
	for (std::size_t index = 0u; index < size; ++index) {
		*hash ^= bytes[index];
		*hash *= 1099511628211ull;
	}
}

void hashDouble(std::uint64_t *hash, double value)
{
	hashBytes(hash, &value, sizeof(value));
}

double stableSmoothMaximum(double first, double second, double sharpness)
{
	const double scaled_first = sharpness * first;
	const double scaled_second = sharpness * second;
	const double maximum = std::max(scaled_first, scaled_second);
	return (maximum + std::log(
		std::exp(scaled_first - maximum) +
		std::exp(scaled_second - maximum))) / sharpness;
}

} // namespace

SemanticSectionBodyField::SemanticSectionBodyField(
	const ModernCarSemanticVariant &variant)
	: variant_identifier_(variant.identifier()),
	  package_width_(variant.package().width()),
	  package_height_(variant.package().height()),
	  section_evaluation_(variant.sectionField())
{
}

double SemanticSectionBodyField::evaluateAt(const glm::dvec3 &position) const
{
	double value = std::fabs(position.y) -
		section_evaluation_.halfWidthAt(position.x, position.z);
	value = stableSmoothMaximum(
		value,
		section_evaluation_.evaluate(
			VehicleSectionScalarField::UnderbodyHeight, position.x) - position.z,
		28.0);
	value = stableSmoothMaximum(
		value,
		position.z - section_evaluation_.evaluate(
			VehicleSectionScalarField::RoofCrownHeight, position.x),
		28.0);
	value = stableSmoothMaximum(
		value, section_evaluation_.rearSourceX() - position.x, 28.0);
	return stableSmoothMaximum(
		value, position.x - section_evaluation_.frontSourceX(), 28.0);
}

ImplicitFieldBounds SemanticSectionBodyField::evaluationBounds() const
{
	return ImplicitFieldBounds(
		glm::dvec3(
			section_evaluation_.rearSourceX() - 0.04,
			-package_width_ * 0.57,
			0.0),
		glm::dvec3(
			section_evaluation_.frontSourceX() + 0.04,
			package_width_ * 0.57,
			package_height_ + 0.025));
}

std::uint64_t SemanticSectionBodyField::deterministicHash() const
{
	std::uint64_t hash = 1469598103934665603ull;
	hashBytes(&hash, variant_identifier_.data(), variant_identifier_.size());
	hashDouble(&hash, package_width_);
	hashDouble(&hash, package_height_);
	return hash;
}

SuperellipsoidField::SuperellipsoidField(
	glm::dvec3 center,
	glm::dvec3 radii,
	double exponent)
	: center_(center),
	  radii_(radii),
	  exponent_(exponent)
{
	if (!(radii_.x > 0.0) || !(radii_.y > 0.0) || !(radii_.z > 0.0) ||
	    !(exponent_ >= 1.0)) {
		throw std::invalid_argument(
			"Superellipsoid field requires positive radii and exponent.");
	}
}

double SuperellipsoidField::evaluateAt(const glm::dvec3 &position) const
{
	const glm::dvec3 normalized = glm::abs((position - center_) / radii_);
	const double normalized_field =
		std::pow(normalized.x, exponent_) +
		std::pow(normalized.y, exponent_) +
		std::pow(normalized.z, exponent_) - 1.0;
	return normalized_field * std::min({radii_.x, radii_.y, radii_.z});
}

ImplicitFieldBounds SuperellipsoidField::evaluationBounds() const
{
	return ImplicitFieldBounds(center_ - radii_, center_ + radii_);
}

std::uint64_t SuperellipsoidField::deterministicHash() const
{
	std::uint64_t hash = 1469598103934665603ull;
	for (double value : {center_.x, center_.y, center_.z,
	                     radii_.x, radii_.y, radii_.z, exponent_}) {
		hashDouble(&hash, value);
	}
	return hash;
}

PackageBoundaryField::PackageBoundaryField(
	glm::dvec3 outward_normal,
	double offset,
	ImplicitFieldBounds bounds)
	: outward_normal_(glm::normalize(outward_normal)),
	  offset_(offset),
	  bounds_(bounds)
{
	if (!bounds_.isValid() || glm::length(outward_normal) <= 1.0e-12) {
		throw std::invalid_argument("Package boundary field definition is invalid.");
	}
}

double PackageBoundaryField::evaluateAt(const glm::dvec3 &position) const
{
	return glm::dot(outward_normal_, position) - offset_;
}

std::uint64_t PackageBoundaryField::deterministicHash() const
{
	std::uint64_t hash = 1469598103934665603ull;
	for (double value : {outward_normal_.x, outward_normal_.y,
	                     outward_normal_.z, offset_}) {
		hashDouble(&hash, value);
	}
	return hash;
}

SmoothMaximumField::SmoothMaximumField(
	std::shared_ptr<const ImplicitScalarField> first,
	std::shared_ptr<const ImplicitScalarField> second,
	double sharpness)
	: first_(std::move(first)),
	  second_(std::move(second)),
	  sharpness_(sharpness)
{
	if (!first_ || !second_ || !(sharpness_ > 0.0)) {
		throw std::invalid_argument("Smooth maximum field definition is invalid.");
	}
}

double SmoothMaximumField::evaluateAt(const glm::dvec3 &position) const
{
	return stableSmoothMaximum(
		first_->evaluateAt(position), second_->evaluateAt(position), sharpness_);
}

ImplicitFieldBounds SmoothMaximumField::evaluationBounds() const
{
	const ImplicitFieldBounds first_bounds = first_->evaluationBounds();
	const ImplicitFieldBounds second_bounds = second_->evaluationBounds();
	const ImplicitFieldBounds intersection(
		glm::max(first_bounds.minimum(), second_bounds.minimum()),
		glm::min(first_bounds.maximum(), second_bounds.maximum()));
	return intersection.isValid() ? intersection : first_bounds;
}

std::uint64_t SmoothMaximumField::deterministicHash() const
{
	std::uint64_t hash = first_->deterministicHash();
	const std::uint64_t second_hash = second_->deterministicHash();
	hashBytes(&hash, &second_hash, sizeof(second_hash));
	hashDouble(&hash, sharpness_);
	return hash;
}

DifferenceField::DifferenceField(
	std::shared_ptr<const ImplicitScalarField> source,
	std::shared_ptr<const ImplicitScalarField> subtract)
	: source_(std::move(source)),
	  subtract_(std::move(subtract))
{
	if (!source_ || !subtract_) {
		throw std::invalid_argument("Difference field requires source and subtract fields.");
	}
}

InverseAffinePrewarpedField::InverseAffinePrewarpedField(
	std::shared_ptr<const ImplicitScalarField> source_field,
	ImplicitFieldCalibration calibration)
	: source_field_(std::move(source_field)),
	  calibration_(std::move(calibration))
{
	const glm::dvec3 scale = calibration_.prewarpScale();
	if (!source_field_ || !(scale.x > 0.0) || !(scale.y > 0.0) ||
	    !(scale.z > 0.0)) {
		throw std::invalid_argument(
			"Inverse affine field prewarp requires a source field and positive scale.");
	}
}

double InverseAffinePrewarpedField::evaluateAt(const glm::dvec3 &position) const
{
	return source_field_->evaluateAt(
		(position - calibration_.prewarpTranslation()) /
		calibration_.prewarpScale());
}

ImplicitFieldBounds InverseAffinePrewarpedField::evaluationBounds() const
{
	const ImplicitFieldBounds source_bounds = source_field_->evaluationBounds();
	return ImplicitFieldBounds(
		source_bounds.minimum() * calibration_.prewarpScale() +
			calibration_.prewarpTranslation(),
		source_bounds.maximum() * calibration_.prewarpScale() +
			calibration_.prewarpTranslation());
}

std::uint64_t InverseAffinePrewarpedField::deterministicHash() const
{
	std::uint64_t hash = source_field_->deterministicHash();
	for (double value : {
		calibration_.prewarpScale().x,
		calibration_.prewarpScale().y,
		calibration_.prewarpScale().z,
		calibration_.prewarpTranslation().x,
		calibration_.prewarpTranslation().y,
		calibration_.prewarpTranslation().z}) {
		hashDouble(&hash, value);
	}
	return hash;
}

double DifferenceField::evaluateAt(const glm::dvec3 &position) const
{
	return std::max(source_->evaluateAt(position), -subtract_->evaluateAt(position));
}

ImplicitFieldBounds DifferenceField::evaluationBounds() const
{
	return source_->evaluationBounds();
}

std::uint64_t DifferenceField::deterministicHash() const
{
	std::uint64_t hash = source_->deterministicHash();
	const std::uint64_t subtract_hash = subtract_->deterministicHash();
	hashBytes(&hash, &subtract_hash, sizeof(subtract_hash));
	return hash;
}
