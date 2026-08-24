#pragma once

#include "vehicle/model/VehicleCharacterCurveNetwork.h"
#include "vehicle/model/VehicleClosureAssembly.h"
#include "vehicle/model/VehicleCrossSection.h"
#include "vehicle/model/VehicleFittingEvidence.h"
#include "vehicle/model/VehicleSurfacePatchGraph.h"

#include <string>
#include <utility>
#include <vector>

class VehicleFittingArchitecture
{
public:
	VehicleFittingArchitecture(
		std::string identifier,
		MultiViewEnvelope multi_view_envelope,
		std::vector<VehicleCrossSection> cross_sections,
		std::vector<SurfaceLandmark> surface_landmarks,
		CharacterCurveNetwork character_curve_network,
		SurfacePatchGraph surface_patch_graph,
		std::vector<PanelSeamLoop> panel_seam_loops,
		std::vector<ExtractedSurfacePanel> extracted_panels,
		std::vector<Aperture> apertures,
		std::vector<ClosureAssembly> closure_assemblies,
		std::vector<DropGlassAssembly> drop_glass_assemblies,
		VehicleClosureState closure_state)
		: identifier_(std::move(identifier)),
		  multi_view_envelope_(std::move(multi_view_envelope)),
		  cross_sections_(std::move(cross_sections)),
		  surface_landmarks_(std::move(surface_landmarks)),
		  character_curve_network_(std::move(character_curve_network)),
		  surface_patch_graph_(std::move(surface_patch_graph)),
		  panel_seam_loops_(std::move(panel_seam_loops)),
		  extracted_panels_(std::move(extracted_panels)),
		  apertures_(std::move(apertures)),
		  closure_assemblies_(std::move(closure_assemblies)),
		  drop_glass_assemblies_(std::move(drop_glass_assemblies)),
		  closure_state_(closure_state)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const MultiViewEnvelope &multiViewEnvelope() const { return multi_view_envelope_; }
	const std::vector<VehicleCrossSection> &crossSections() const
	{
		return cross_sections_;
	}
	const std::vector<SurfaceLandmark> &surfaceLandmarks() const
	{
		return surface_landmarks_;
	}
	const CharacterCurveNetwork &characterCurveNetwork() const
	{
		return character_curve_network_;
	}
	const SurfacePatchGraph &surfacePatchGraph() const { return surface_patch_graph_; }
	const std::vector<PanelSeamLoop> &panelSeamLoops() const
	{
		return panel_seam_loops_;
	}
	const std::vector<ExtractedSurfacePanel> &extractedPanels() const
	{
		return extracted_panels_;
	}
	const std::vector<Aperture> &apertures() const { return apertures_; }
	const std::vector<ClosureAssembly> &closureAssemblies() const
	{
		return closure_assemblies_;
	}
	const std::vector<DropGlassAssembly> &dropGlassAssemblies() const
	{
		return drop_glass_assemblies_;
	}
	const VehicleClosureState &closureState() const { return closure_state_; }

	const ClosureAssembly *findClosure(const std::string &identifier) const
	{
		for (const ClosureAssembly &closure : closure_assemblies_) {
			if (closure.identifier() == identifier) return &closure;
		}
		return nullptr;
	}

private:
	std::string identifier_;
	MultiViewEnvelope multi_view_envelope_;
	std::vector<VehicleCrossSection> cross_sections_;
	std::vector<SurfaceLandmark> surface_landmarks_;
	CharacterCurveNetwork character_curve_network_;
	SurfacePatchGraph surface_patch_graph_;
	std::vector<PanelSeamLoop> panel_seam_loops_;
	std::vector<ExtractedSurfacePanel> extracted_panels_;
	std::vector<Aperture> apertures_;
	std::vector<ClosureAssembly> closure_assemblies_;
	std::vector<DropGlassAssembly> drop_glass_assemblies_;
	VehicleClosureState closure_state_;
};
