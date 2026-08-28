#pragma once

#include "vehicle/mcsmv2/model/VehicleWheelMotionEnvelope.h"

#include <array>
#include <utility>

class VehicleWheelEnvelopeSet
{
public:
	explicit VehicleWheelEnvelopeSet(
		std::array<VehicleWheelMotionEnvelope, 4> envelopes)
		: envelopes_(std::move(envelopes))
	{
	}

	const std::array<VehicleWheelMotionEnvelope, 4> &envelopes() const
	{
		return envelopes_;
	}

private:
	std::array<VehicleWheelMotionEnvelope, 4> envelopes_;
};
