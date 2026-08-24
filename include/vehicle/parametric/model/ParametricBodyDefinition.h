#pragma once

#include "vehicle/parametric/model/FenderFieldDefinition.h"
#include "vehicle/parametric/model/GreenhouseFieldDefinition.h"
#include "vehicle/parametric/model/ImplicitFieldBlendDefinition.h"
#include "vehicle/parametric/model/LowerBodyFieldDefinition.h"
#include "vehicle/parametric/model/ParametricBodySectionNetwork.h"
#include "vehicle/parametric/model/ParametricCharacterCurveNetwork.h"
#include "vehicle/parametric/model/WheelhouseDifferenceDefinition.h"

class VehicleLongitudinalDomain
{
public:
	VehicleLongitudinalDomain(
		double rear,
		double middle,
		double front,
		double cabin_rear,
		double cabin_middle,
		double cabin_front)
		: rear_(rear),
		  middle_(middle),
		  front_(front),
		  cabin_rear_(cabin_rear),
		  cabin_middle_(cabin_middle),
		  cabin_front_(cabin_front)
	{
	}

	double rear() const { return rear_; }
	double middle() const { return middle_; }
	double front() const { return front_; }
	double cabinRear() const { return cabin_rear_; }
	double cabinMiddle() const { return cabin_middle_; }
	double cabinFront() const { return cabin_front_; }

private:
	double rear_ = 0.0;
	double middle_ = 0.0;
	double front_ = 0.0;
	double cabin_rear_ = 0.0;
	double cabin_middle_ = 0.0;
	double cabin_front_ = 0.0;
};

class ParametricBodyDefinition
{
public:
	ParametricBodyDefinition(
		VehicleLongitudinalDomain longitudinal_domain,
		LowerBodyFieldDefinition lower_body,
		GreenhouseFieldDefinition greenhouse,
		FenderFieldDefinition fenders,
		WheelhouseDifferenceDefinition wheelhouse_difference,
		ImplicitFieldBlendDefinition blend,
		ParametricCharacterCurveNetwork character_curve_network,
		ParametricBodySectionNetwork body_section_network)
		: longitudinal_domain_(std::move(longitudinal_domain)),
		  lower_body_(std::move(lower_body)),
		  greenhouse_(std::move(greenhouse)),
		  fenders_(std::move(fenders)),
		  wheelhouse_difference_(std::move(wheelhouse_difference)),
		  blend_(std::move(blend)),
		  character_curve_network_(std::move(character_curve_network)),
		  body_section_network_(std::move(body_section_network))
	{
	}

	const VehicleLongitudinalDomain &longitudinalDomain() const
	{
		return longitudinal_domain_;
	}
	const LowerBodyFieldDefinition &lowerBody() const { return lower_body_; }
	const GreenhouseFieldDefinition &greenhouse() const { return greenhouse_; }
	const FenderFieldDefinition &fenders() const { return fenders_; }
	const WheelhouseDifferenceDefinition &wheelhouseDifference() const
	{
		return wheelhouse_difference_;
	}
	const ImplicitFieldBlendDefinition &blend() const { return blend_; }
	const ParametricCharacterCurveNetwork &characterCurveNetwork() const
	{
		return character_curve_network_;
	}
	const ParametricBodySectionNetwork &bodySectionNetwork() const
	{
		return body_section_network_;
	}

private:
	VehicleLongitudinalDomain longitudinal_domain_{0, 0, 0, 0, 0, 0};
	LowerBodyFieldDefinition lower_body_{0, 0, 0, 0,
		{"", StationInterpolationKind::Pchip, {}},
		{"", StationInterpolationKind::Pchip, {}},
		{"", StationInterpolationKind::Pchip, {}}};
	GreenhouseFieldDefinition greenhouse_{0, 0, 0, 0, 0, 0,
		{"", StationInterpolationKind::Pchip, {}},
		{"", StationInterpolationKind::Pchip, {}},
		{"", StationInterpolationKind::Pchip, {}}};
	FenderFieldDefinition fenders_{0, 0, 0, 0, 0, 0, 0, 0, 0};
	WheelhouseDifferenceDefinition wheelhouse_difference_{0};
	ImplicitFieldBlendDefinition blend_{0, 0};
	ParametricCharacterCurveNetwork character_curve_network_{{}, 0};
	ParametricBodySectionNetwork body_section_network_{0};
};
