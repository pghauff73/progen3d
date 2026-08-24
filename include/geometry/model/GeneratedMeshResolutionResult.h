#pragma once

#include "geometry/model/GeneratedPrimitiveMesh.h"

#include <optional>
#include <string>
#include <utility>

class GeneratedMeshResolutionResult
{
public:
	static GeneratedMeshResolutionResult createSuccess(
		GeneratedPrimitiveMesh generated_mesh)
	{
		return GeneratedMeshResolutionResult(
			std::move(generated_mesh), std::string());
	}

	static GeneratedMeshResolutionResult createFailure(std::string diagnostic)
	{
		return GeneratedMeshResolutionResult(
			std::nullopt, std::move(diagnostic));
	}

	bool succeeded() const { return generated_mesh_.has_value(); }
	const std::optional<GeneratedPrimitiveMesh> &generatedMesh() const
	{
		return generated_mesh_;
	}
	const std::string &diagnostic() const { return diagnostic_; }

private:
	GeneratedMeshResolutionResult(
		std::optional<GeneratedPrimitiveMesh> generated_mesh,
		std::string diagnostic)
		: generated_mesh_(std::move(generated_mesh)),
		  diagnostic_(std::move(diagnostic))
	{
	}

	std::optional<GeneratedPrimitiveMesh> generated_mesh_;
	std::string diagnostic_;
};
