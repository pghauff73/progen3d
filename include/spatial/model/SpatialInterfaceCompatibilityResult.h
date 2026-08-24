#pragma once

#include <string>
#include <utility>

class SpatialInterfaceCompatibilityResult {
public:
	static SpatialInterfaceCompatibilityResult compatible()
	{
		return SpatialInterfaceCompatibilityResult(true, {});
	}

	static SpatialInterfaceCompatibilityResult incompatible(std::string diagnostic)
	{
		return SpatialInterfaceCompatibilityResult(false, std::move(diagnostic));
	}

	bool isCompatible() const { return compatible_; }
	const std::string &diagnostic() const { return diagnostic_; }

private:
	SpatialInterfaceCompatibilityResult(bool compatible, std::string diagnostic)
		: compatible_(compatible), diagnostic_(std::move(diagnostic)) {}

	bool compatible_ = false;
	std::string diagnostic_;
};
