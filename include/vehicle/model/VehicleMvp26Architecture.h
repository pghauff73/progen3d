#pragma once

#include "vehicle/model/VehicleMvp25Architecture.h"
#include "vehicle/model/VehicleParametricFitReport.h"
#include "vehicle/model/VehicleParametricShapePrior.h"
#include "vehicle/model/VehicleParametricSourceEvidence.h"
#include "vehicle/model/VehicleVariantDefinition.h"

#include <cstdint>
#include <string>
#include <utility>

class VehicleMvp26Architecture
{
public:
	VehicleMvp26Architecture(
		std::string identifier,
		VehicleMvp25Architecture accepted_mvp25_architecture,
		std::uint64_t accepted_mvp25_hash,
		VehicleVariantDefinition variant,
		VehicleParametricShapePrior parametric_shape_prior,
		VehicleParametricSourceEvidence source_evidence,
		VehicleParametricFitReport fit_report)
		: identifier_(std::move(identifier)),
		  accepted_mvp25_architecture_(std::move(accepted_mvp25_architecture)),
		  accepted_mvp25_hash_(accepted_mvp25_hash),
		  variant_(std::move(variant)),
		  parametric_shape_prior_(std::move(parametric_shape_prior)),
		  source_evidence_(std::move(source_evidence)),
		  fit_report_(std::move(fit_report))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const VehicleMvp25Architecture &acceptedMvp25Architecture() const
	{
		return accepted_mvp25_architecture_;
	}
	std::uint64_t acceptedMvp25Hash() const { return accepted_mvp25_hash_; }
	const VehicleVariantDefinition &variant() const { return variant_; }
	const VehicleParametricShapePrior &parametricShapePrior() const
	{
		return parametric_shape_prior_;
	}
	const VehicleParametricSourceEvidence &sourceEvidence() const
	{
		return source_evidence_;
	}
	const VehicleParametricFitReport &fitReport() const { return fit_report_; }

private:
	std::string identifier_;
	VehicleMvp25Architecture accepted_mvp25_architecture_;
	std::uint64_t accepted_mvp25_hash_ = 0u;
	VehicleVariantDefinition variant_;
	VehicleParametricShapePrior parametric_shape_prior_;
	VehicleParametricSourceEvidence source_evidence_;
	VehicleParametricFitReport fit_report_;
};
