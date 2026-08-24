#pragma once

#include "vehicle/model/AutomotiveWireframe.h"
#include "vehicle/model/VehicleBodyInWhite.h"
#include "vehicle/model/VehicleChassisMvp25.h"
#include "vehicle/model/VehicleClassASurface.h"
#include "vehicle/model/VehicleClosureGeometry25.h"
#include "vehicle/model/VehicleFitObjective.h"
#include "vehicle/model/VehicleMvp25Reference.h"

#include <string>
#include <utility>
#include <vector>

class VehicleMvp25Architecture
{
public:
	VehicleMvp25Architecture(
		std::string identifier,
		VehicleReferenceFrame reference_frame,
		VehiclePackageEvidence package_evidence,
		std::vector<OccupantPackage> occupant_packages,
		VehicleObservationSet observations,
		AutomotiveWireframe wireframe,
		std::vector<SilhouetteConstraint> silhouette_constraints,
		ClassASurfaceGraph class_a_surfaces,
		std::vector<AutomotivePanelGap> panel_gaps,
		std::vector<RolledEdge> rolled_edges,
		std::vector<BodySideAperture> body_side_apertures,
		std::vector<DoorEgressSurface> door_egress_surfaces,
		std::vector<AutomotiveClosure> closures,
		std::vector<HelicalGlassDrop> glass_drops,
		std::vector<VariableSealSweep> seal_sweeps,
		BodyInWhite body_in_white,
		std::vector<SuspensionHardpointModel> suspension_models,
		std::vector<WheelPoseFunction> wheel_pose_functions,
		std::vector<WheelSweptEnvelope> wheel_swept_envelopes,
		std::vector<TyreGeometry> tyre_geometries,
		std::vector<AutomotiveRimGeometry> rim_geometries,
		std::vector<AutomotiveBrakeGeometry> brake_geometries,
		std::vector<WheelHouse> wheel_houses,
		AeroGeometry aero_geometry,
		VehicleFitObjective fit_objective,
		VehicleFitResidualReport residual_report,
		HighlightFlowReport highlight_flow_report)
		: identifier_(std::move(identifier)),
		  reference_frame_(std::move(reference_frame)),
		  package_evidence_(std::move(package_evidence)),
		  occupant_packages_(std::move(occupant_packages)),
		  observations_(std::move(observations)),
		  wireframe_(std::move(wireframe)),
		  silhouette_constraints_(std::move(silhouette_constraints)),
		  class_a_surfaces_(std::move(class_a_surfaces)),
		  panel_gaps_(std::move(panel_gaps)),
		  rolled_edges_(std::move(rolled_edges)),
		  body_side_apertures_(std::move(body_side_apertures)),
		  door_egress_surfaces_(std::move(door_egress_surfaces)),
		  closures_(std::move(closures)),
		  glass_drops_(std::move(glass_drops)),
		  seal_sweeps_(std::move(seal_sweeps)),
		  body_in_white_(std::move(body_in_white)),
		  suspension_models_(std::move(suspension_models)),
		  wheel_pose_functions_(std::move(wheel_pose_functions)),
		  wheel_swept_envelopes_(std::move(wheel_swept_envelopes)),
		  tyre_geometries_(std::move(tyre_geometries)),
		  rim_geometries_(std::move(rim_geometries)),
		  brake_geometries_(std::move(brake_geometries)),
		  wheel_houses_(std::move(wheel_houses)),
		  aero_geometry_(std::move(aero_geometry)),
		  fit_objective_(std::move(fit_objective)),
		  residual_report_(std::move(residual_report)),
		  highlight_flow_report_(std::move(highlight_flow_report))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const VehicleReferenceFrame &referenceFrame() const { return reference_frame_; }
	const VehiclePackageEvidence &packageEvidence() const { return package_evidence_; }
	const std::vector<OccupantPackage> &occupantPackages() const
	{
		return occupant_packages_;
	}
	const VehicleObservationSet &observations() const { return observations_; }
	const AutomotiveWireframe &wireframe() const { return wireframe_; }
	const std::vector<SilhouetteConstraint> &silhouetteConstraints() const
	{
		return silhouette_constraints_;
	}
	const ClassASurfaceGraph &classASurfaces() const { return class_a_surfaces_; }
	const std::vector<AutomotivePanelGap> &panelGaps() const { return panel_gaps_; }
	const std::vector<RolledEdge> &rolledEdges() const { return rolled_edges_; }
	const std::vector<BodySideAperture> &bodySideApertures() const
	{
		return body_side_apertures_;
	}
	const std::vector<DoorEgressSurface> &doorEgressSurfaces() const
	{
		return door_egress_surfaces_;
	}
	const std::vector<AutomotiveClosure> &closures() const { return closures_; }
	const std::vector<HelicalGlassDrop> &glassDrops() const { return glass_drops_; }
	const std::vector<VariableSealSweep> &sealSweeps() const { return seal_sweeps_; }
	const BodyInWhite &bodyInWhite() const { return body_in_white_; }
	const std::vector<SuspensionHardpointModel> &suspensionModels() const
	{
		return suspension_models_;
	}
	const std::vector<WheelPoseFunction> &wheelPoseFunctions() const
	{
		return wheel_pose_functions_;
	}
	const std::vector<WheelSweptEnvelope> &wheelSweptEnvelopes() const
	{
		return wheel_swept_envelopes_;
	}
	const std::vector<TyreGeometry> &tyreGeometries() const { return tyre_geometries_; }
	const std::vector<AutomotiveRimGeometry> &rimGeometries() const
	{
		return rim_geometries_;
	}
	const std::vector<AutomotiveBrakeGeometry> &brakeGeometries() const
	{
		return brake_geometries_;
	}
	const std::vector<WheelHouse> &wheelHouses() const { return wheel_houses_; }
	const AeroGeometry &aeroGeometry() const { return aero_geometry_; }
	const VehicleFitObjective &fitObjective() const { return fit_objective_; }
	const VehicleFitResidualReport &residualReport() const { return residual_report_; }
	const HighlightFlowReport &highlightFlowReport() const
	{
		return highlight_flow_report_;
	}

private:
	std::string identifier_;
	VehicleReferenceFrame reference_frame_;
	VehiclePackageEvidence package_evidence_;
	std::vector<OccupantPackage> occupant_packages_;
	VehicleObservationSet observations_;
	AutomotiveWireframe wireframe_;
	std::vector<SilhouetteConstraint> silhouette_constraints_;
	ClassASurfaceGraph class_a_surfaces_;
	std::vector<AutomotivePanelGap> panel_gaps_;
	std::vector<RolledEdge> rolled_edges_;
	std::vector<BodySideAperture> body_side_apertures_;
	std::vector<DoorEgressSurface> door_egress_surfaces_;
	std::vector<AutomotiveClosure> closures_;
	std::vector<HelicalGlassDrop> glass_drops_;
	std::vector<VariableSealSweep> seal_sweeps_;
	BodyInWhite body_in_white_;
	std::vector<SuspensionHardpointModel> suspension_models_;
	std::vector<WheelPoseFunction> wheel_pose_functions_;
	std::vector<WheelSweptEnvelope> wheel_swept_envelopes_;
	std::vector<TyreGeometry> tyre_geometries_;
	std::vector<AutomotiveRimGeometry> rim_geometries_;
	std::vector<AutomotiveBrakeGeometry> brake_geometries_;
	std::vector<WheelHouse> wheel_houses_;
	AeroGeometry aero_geometry_;
	VehicleFitObjective fit_objective_;
	VehicleFitResidualReport residual_report_;
	HighlightFlowReport highlight_flow_report_;
};
