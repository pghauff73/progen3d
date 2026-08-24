#pragma once

#include "vehicle/mcsmv2/model/VehicleFunctionalEnvelope.h"

#include <string>
#include <utility>
#include <vector>

class VehicleFunctionalPackageSystem
{
public:
	VehicleFunctionalPackageSystem(
		std::string variant_identifier,
		std::vector<VehicleFunctionalEnvelope> envelopes)
		: variant_identifier_(std::move(variant_identifier)),
		  envelopes_(std::move(envelopes))
	{
	}

	const std::string &variantIdentifier() const { return variant_identifier_; }
	const std::vector<VehicleFunctionalEnvelope> &envelopes() const
	{
		return envelopes_;
	}

private:
	std::string variant_identifier_;
	std::vector<VehicleFunctionalEnvelope> envelopes_;
};
