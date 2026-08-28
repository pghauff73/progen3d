#pragma once

#include "geometry/model/GeneratedPrimitiveMesh.h"
#include "geometry/model/ImplicitFieldBounds.h"
#include "geometry/model/MeshTopologyReport.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

class ImplicitSurfaceGenerationResult
{
public:
	static ImplicitSurfaceGenerationResult createSuccess(
		GeneratedPrimitiveMesh generated_mesh,
		ImplicitFieldBounds generated_bounds,
		MeshTopologyReport topology_report)
	{
		return ImplicitSurfaceGenerationResult(
			std::move(generated_mesh), generated_bounds, topology_report, {});
	}

	static ImplicitSurfaceGenerationResult createFailure(std::string diagnostic)
	{
		std::vector<std::string> diagnostics;
		if (!diagnostic.empty()) diagnostics.push_back(std::move(diagnostic));
		return ImplicitSurfaceGenerationResult(
			std::nullopt, ImplicitFieldBounds(), MeshTopologyReport(),
			std::move(diagnostics));
	}

	bool succeeded() const { return generated_mesh_.has_value(); }
	const std::optional<GeneratedPrimitiveMesh> &generatedMesh() const
	{
		return generated_mesh_;
	}
	const ImplicitFieldBounds &generatedBounds() const { return generated_bounds_; }
	const MeshTopologyReport &topologyReport() const { return topology_report_; }
	const std::vector<std::string> &diagnostics() const { return diagnostics_; }

	std::string firstDiagnostic() const
	{
		return diagnostics_.empty() ? std::string() : diagnostics_.front();
	}

private:
	ImplicitSurfaceGenerationResult(
		std::optional<GeneratedPrimitiveMesh> generated_mesh,
		ImplicitFieldBounds generated_bounds,
		MeshTopologyReport topology_report,
		std::vector<std::string> diagnostics)
		: generated_mesh_(std::move(generated_mesh)),
		  generated_bounds_(generated_bounds),
		  topology_report_(topology_report),
		  diagnostics_(std::move(diagnostics))
	{
	}

	std::optional<GeneratedPrimitiveMesh> generated_mesh_;
	ImplicitFieldBounds generated_bounds_;
	MeshTopologyReport topology_report_;
	std::vector<std::string> diagnostics_;
};
