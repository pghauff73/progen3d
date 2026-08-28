#pragma once

#include "geometry/service/GeneratedMeshProvider.h"

#include <memory>
#include <utility>
#include <vector>

class GeneratedMeshProviderRegistry
{
public:
	static GeneratedMeshProviderRegistry &accessProcessRegistry()
	{
		static GeneratedMeshProviderRegistry registry;
		return registry;
	}

	void registerProvider(std::shared_ptr<const GeneratedMeshProvider> provider)
	{
		if (!provider) return;
		providers_.push_back(std::move(provider));
	}

	const std::vector<std::shared_ptr<const GeneratedMeshProvider>> &
	listRegisteredProviders() const
	{
		return providers_;
	}

private:
	GeneratedMeshProviderRegistry() = default;

	std::vector<std::shared_ptr<const GeneratedMeshProvider>> providers_;
};
