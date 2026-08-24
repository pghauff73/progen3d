#include "vehicle/model/VehicleValidationReport.h"

const char *vehicleDiagnosticCodeName(VehicleDiagnosticCode code)
{
	switch (code) {
	case VehicleDiagnosticCode::InvalidWheelbase:
		return "MVG-PKG-001 INVALID_WHEELBASE";
	case VehicleDiagnosticCode::TireOutsideBody:
		return "MVG-PKG-002 TIRE_OUTSIDE_BODY";
	case VehicleDiagnosticCode::InvalidBodySection:
		return "MVG-BODY-101 SECTION_SELF_INTERSECTION";
	case VehicleDiagnosticCode::LoftFailure:
		return "MVG-BODY-102 LOFT_FAILURE";
	case VehicleDiagnosticCode::GuideConflict:
		return "MVG-BODY-103 GUIDE_CONFLICT";
	case VehicleDiagnosticCode::ShellOffsetFailure:
		return "MVG-BODY-104 SHELL_OFFSET_FAILURE";
	case VehicleDiagnosticCode::InvalidPanelCut:
		return "MVG-PANEL-201 INVALID_PANEL_CUT";
	case VehicleDiagnosticCode::PanelGapOverlap:
		return "MVG-PANEL-202 PANEL_GAP_OVERLAP";
	case VehicleDiagnosticCode::ArchClearanceFailure:
		return "MVG-WHEEL-301 ARCH_CLEARANCE_FAILURE";
	case VehicleDiagnosticCode::InvalidJoint:
		return "MVG-SUSP-401 INVALID_JOINT";
	case VehicleDiagnosticCode::TravelCollision:
		return "MVG-SUSP-402 TRAVEL_COLLISION";
	case VehicleDiagnosticCode::InterfaceMismatch:
		return "MVG-ASM-501 INTERFACE_MISMATCH";
	case VehicleDiagnosticCode::PositioningFailure:
		return "MVG-ASM-502 POSITIONING_FAILURE";
	case VehicleDiagnosticCode::DoorCollision:
		return "MVG-KIN-601 DOOR_COLLISION";
	case VehicleDiagnosticCode::InvalidViewEvidence:
		return "MVG2-FIT-701 INVALID_VIEW_EVIDENCE";
	case VehicleDiagnosticCode::InvalidSurfaceLandmark:
		return "MVG2-FIT-702 INVALID_SURFACE_LANDMARK";
	case VehicleDiagnosticCode::InvalidCharacterCurveNetwork:
		return "MVG2-FIT-703 INVALID_CHARACTER_CURVE_NETWORK";
	case VehicleDiagnosticCode::InvalidSurfacePatchGraph:
		return "MVG2-FIT-704 INVALID_SURFACE_PATCH_GRAPH";
	case VehicleDiagnosticCode::InvalidAperture:
		return "MVG2-CLOSURE-705 INVALID_APERTURE";
	case VehicleDiagnosticCode::InvalidClosureAssembly:
		return "MVG2-CLOSURE-706 INVALID_CLOSURE_ASSEMBLY";
	case VehicleDiagnosticCode::InvalidGuideRailJoint:
		return "MVG2-KIN-707 INVALID_GUIDE_RAIL_JOINT";
	case VehicleDiagnosticCode::InvalidClosureDependency:
		return "MVG2-KIN-708 INVALID_CLOSURE_DEPENDENCY";
	case VehicleDiagnosticCode::InvalidSweptVolume:
		return "MVG2-KIN-709 INVALID_SWEPT_VOLUME";
	case VehicleDiagnosticCode::InvalidEvidenceClassification:
		return "MVG2-EVIDENCE-710 INVALID_EVIDENCE_CLASSIFICATION";
	case VehicleDiagnosticCode::InvalidVehicleReferenceFrame:
		return "MVG25-REF-801 INVALID_VEHICLE_REFERENCE_FRAME";
	case VehicleDiagnosticCode::InvalidOccupantPackage:
		return "MVG25-PKG-802 INVALID_OCCUPANT_PACKAGE";
	case VehicleDiagnosticCode::InvalidAutomotiveWireframe:
		return "MVG25-WIRE-803 INVALID_AUTOMOTIVE_WIREFRAME";
	case VehicleDiagnosticCode::InvalidClassASurfaceGraph:
		return "MVG25-CLASSA-804 INVALID_CLASS_A_SURFACE_GRAPH";
	case VehicleDiagnosticCode::InvalidAutomotivePanelGap:
		return "MVG25-GAP-805 INVALID_AUTOMOTIVE_PANEL_GAP";
	case VehicleDiagnosticCode::InvalidBodySideAperture:
		return "MVG25-APERTURE-806 INVALID_BODY_SIDE_APERTURE";
	case VehicleDiagnosticCode::InvalidHelicalGlassDrop:
		return "MVG25-GLASS-807 INVALID_HELICAL_GLASS_DROP";
	case VehicleDiagnosticCode::InvalidBodyInWhite:
		return "MVG25-BIW-808 INVALID_BODY_IN_WHITE";
	case VehicleDiagnosticCode::InvalidSuspensionHardpoint:
		return "MVG25-SUSP-809 INVALID_SUSPENSION_HARDPOINT";
	case VehicleDiagnosticCode::InvalidWheelSweptEnvelope:
		return "MVG25-WHEEL-810 INVALID_WHEEL_SWEPT_ENVELOPE";
	case VehicleDiagnosticCode::InvalidTyreGeometry:
		return "MVG25-TYRE-811 INVALID_TYRE_GEOMETRY";
	case VehicleDiagnosticCode::InvalidRimGeometry:
		return "MVG25-WHEEL-812 INVALID_RIM_GEOMETRY";
	case VehicleDiagnosticCode::InvalidBrakeGeometry:
		return "MVG25-BRAKE-813 INVALID_BRAKE_GEOMETRY";
	case VehicleDiagnosticCode::InvalidAeroGeometry:
		return "MVG25-AERO-814 INVALID_AERO_GEOMETRY";
	case VehicleDiagnosticCode::InvalidVehicleFitObjective:
		return "MVG25-FIT-815 INVALID_VEHICLE_FIT_OBJECTIVE";
	case VehicleDiagnosticCode::InvalidParametricVariant:
		return "MVG26-PARAM-901 INVALID_PARAMETRIC_VARIANT";
	case VehicleDiagnosticCode::InvalidParametricSourceEvidence:
		return "MVG26-SOURCE-902 INVALID_PARAMETRIC_SOURCE_EVIDENCE";
	case VehicleDiagnosticCode::InvalidParametricFitReport:
		return "MVG26-FIT-903 INVALID_PARAMETRIC_FIT_REPORT";
	case VehicleDiagnosticCode::InvalidMvp26Architecture:
		return "MVG26-ARCH-904 INVALID_MVP26_ARCHITECTURE";
	}
	return "MVG-UNKNOWN";
}
