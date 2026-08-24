#pragma once

#include "geometry/model/GeneratedMeshResolutionResult.h"
#include "geometry/model/GeometryDetailLevel.h"
#include "geometry/service/GeneratedMeshProviderRegistry.h"

#include <string>

class GeneratedMeshReferenceResolutionService
{
public:
	GeneratedMeshReferenceResolutionService() = default;
	~GeneratedMeshReferenceResolutionService() = default;

	GeneratedMeshResolutionResult resolve(
		const std::string &mesh_key,
		GeometryDetailLevel detail_level) const
	{
		const auto &providers = GeneratedMeshProviderRegistry::accessProcessRegistry()
			.listRegisteredProviders();
		const GeneratedMeshProvider *recognized_provider = nullptr;
		for (const std::shared_ptr<const GeneratedMeshProvider> &provider : providers) {
			if (!provider->recognizes(mesh_key)) continue;
			if (recognized_provider != nullptr) {
				return GeneratedMeshResolutionResult::createFailure(
					"Multiple generated mesh providers recognize key '" +
					mesh_key + "'.");
			}
			recognized_provider = provider.get();
		}
		if (recognized_provider != nullptr) {
			return recognized_provider->resolve(mesh_key, detail_level);
		}
		return GeneratedMeshResolutionResult::createFailure(
			"No generated mesh provider recognizes key '" + mesh_key + "'.");
	}
};
