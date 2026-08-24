#pragma once

#include "vehicle/mcsmv2/model/VehicleOccupantEnvelope.h"

#include <string>
#include <utility>
#include <vector>

class VehicleOccupantEnvelopeSystem
{
public:
	VehicleOccupantEnvelopeSystem(
		std::string variant_identifier,
		std::vector<VehicleOccupantEnvelope> envelopes)
		: variant_identifier_(std::move(variant_identifier)),
		  envelopes_(std::move(envelopes))
	{
	}

	const std::string &variantIdentifier() const { return variant_identifier_; }
	const std::vector<VehicleOccupantEnvelope> &envelopes() const
	{
		return envelopes_;
	}

private:
	std::string variant_identifier_;
	std::vector<VehicleOccupantEnvelope> envelopes_;
};
