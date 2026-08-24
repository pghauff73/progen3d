#pragma once

#include "vehicle/mcsmv2/model/McsMv21SemanticGeometry.h"
#include "vehicle/mcsmv2/model/McsMv22KinematicAssuranceReport.h"
#include "vehicle/model/VehicleClosureMotion.h"
#include "vehicle/model/VehicleSuspensionCornerKinematicModel.h"
#include "vehicle/model/VehicleTyrePoseSweep.h"

#include <memory>
#include <utility>
#include <vector>

class McsMv22KinematicGeometry
{
public:
	McsMv22KinematicGeometry(
		std::shared_ptr<const McsMv21SemanticGeometry> semantic_geometry,
		std::vector<VehicleSuspensionCornerKinematicModel> suspension_corners,
		VehicleTyreSweepSet tyre_sweeps,
		VehicleClosureSweepSet closure_sweeps,
		VehicleGlassMotionSet glass_motions,
		McsMv22KinematicAssuranceReport assurance_report)
		: semantic_geometry_(std::move(semantic_geometry)),
		  suspension_corners_(std::move(suspension_corners)),
		  tyre_sweeps_(std::move(tyre_sweeps)),
		  closure_sweeps_(std::move(closure_sweeps)),
		  glass_motions_(std::move(glass_motions)),
		  assurance_report_(std::move(assurance_report))
	{
	}

	const std::shared_ptr<const McsMv21SemanticGeometry> &semanticGeometry() const
	{
		return semantic_geometry_;
	}
	const std::vector<VehicleSuspensionCornerKinematicModel> &suspensionCorners() const
	{
		return suspension_corners_;
	}
	const VehicleTyreSweepSet &tyreSweeps() const { return tyre_sweeps_; }
	const VehicleClosureSweepSet &closureSweeps() const { return closure_sweeps_; }
	const VehicleGlassMotionSet &glassMotions() const { return glass_motions_; }
	const McsMv22KinematicAssuranceReport &assuranceReport() const
	{
		return assurance_report_;
	}

private:
	std::shared_ptr<const McsMv21SemanticGeometry> semantic_geometry_;
	std::vector<VehicleSuspensionCornerKinematicModel> suspension_corners_;
	VehicleTyreSweepSet tyre_sweeps_{{}};
	VehicleClosureSweepSet closure_sweeps_{{}};
	VehicleGlassMotionSet glass_motions_{{}};
	McsMv22KinematicAssuranceReport assurance_report_{"", {}, {}};
};
