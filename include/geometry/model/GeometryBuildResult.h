#pragma once

#include "geometry/model/GeneratedPrimitiveMesh.h"
#include "geometry/model/GeometryBuildStatus.h"

#include <string>
#include <utility>
#include <vector>

class GeometryBuildResult
{
public:
	static GeometryBuildResult createSuccess(GeneratedPrimitiveMesh generated_mesh)
	{
		return GeometryBuildResult(
			GeometryBuildStatus::Success, std::move(generated_mesh), {});
	}

	static GeometryBuildResult createFailure(
		GeometryBuildStatus status,
		std::string diagnostic)
	{
		std::vector<std::string> diagnostics;
		if (!diagnostic.empty()) diagnostics.push_back(std::move(diagnostic));
		return GeometryBuildResult(
			status,
			GeneratedPrimitiveMesh(std::make_shared<Mesh>(), {}),
			std::move(diagnostics));
	}

	bool succeeded() const { return status_ == GeometryBuildStatus::Success; }
	GeometryBuildStatus status() const { return status_; }
	const GeneratedPrimitiveMesh &generatedMesh() const { return generated_mesh_; }
	const std::vector<std::string> &diagnostics() const { return diagnostics_; }

	std::string firstDiagnostic() const
	{
		return diagnostics_.empty() ? std::string() : diagnostics_.front();
	}

private:
	GeometryBuildResult(GeometryBuildStatus status,
	                    GeneratedPrimitiveMesh generated_mesh,
	                    std::vector<std::string> diagnostics)
		: status_(status),
		  generated_mesh_(std::move(generated_mesh)),
		  diagnostics_(std::move(diagnostics))
	{
	}

	GeometryBuildStatus status_ = GeometryBuildStatus::UnsupportedTopology;
	GeneratedPrimitiveMesh generated_mesh_{std::make_shared<Mesh>(), {}};
	std::vector<std::string> diagnostics_;
};
