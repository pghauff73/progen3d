#pragma once

#include "vehicle/model/VehicleFittingEvidence.h"
#include "vehicle/parametric/model/GeneratedVehicleRealization.h"

#include <string>
#include <utility>
#include <vector>

class VehicleParametricStyleTerm
{
public:
	VehicleParametricStyleTerm(
		std::string identifier,
		double value,
		double weight,
		VehicleConstraintStrength strength)
		: identifier_(std::move(identifier)),
		  value_(value),
		  weight_(weight),
		  strength_(strength)
	{
	}

	const std::string &identifier() const { return identifier_; }
	double value() const { return value_; }
	double weight() const { return weight_; }
	VehicleConstraintStrength strength() const { return strength_; }

private:
	std::string identifier_;
	double value_ = 0.0;
	double weight_ = 0.0;
	VehicleConstraintStrength strength_ = VehicleConstraintStrength::Soft;
};

class VehicleParametricShapePrior
{
public:
	VehicleParametricShapePrior(
		std::string identifier,
		std::string source_variant_identifier,
		GeneratedCharacterCurveSet character_curves,
		GeneratedBodySectionSet body_sections,
		std::vector<VehicleParametricStyleTerm> style_terms)
		: identifier_(std::move(identifier)),
		  source_variant_identifier_(std::move(source_variant_identifier)),
		  character_curves_(std::move(character_curves)),
		  body_sections_(std::move(body_sections)),
		  style_terms_(std::move(style_terms))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &sourceVariantIdentifier() const
	{
		return source_variant_identifier_;
	}
	const GeneratedCharacterCurveSet &characterCurves() const
	{
		return character_curves_;
	}
	const GeneratedBodySectionSet &bodySections() const { return body_sections_; }
	const std::vector<VehicleParametricStyleTerm> &styleTerms() const
	{
		return style_terms_;
	}

private:
	std::string identifier_;
	std::string source_variant_identifier_;
	GeneratedCharacterCurveSet character_curves_{{}};
	GeneratedBodySectionSet body_sections_{{}};
	std::vector<VehicleParametricStyleTerm> style_terms_;
};
