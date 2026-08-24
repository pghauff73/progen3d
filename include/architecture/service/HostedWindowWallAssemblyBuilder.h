#pragma once

#include "architecture/model/ArchitecturalReferenceBindingCatalog.h"
#include "architecture/model/HostedWindowWallAssemblyBuildResult.h"
#include "architecture/model/WindowFrameSpecification.h"
#include "architecture/model/WindowOpeningSpecification.h"
#include "geometry/model/GeometryComplexityLimits.h"
#include "geometry/model/GeometryDetailLevel.h"

#include <memory>

class HostedWindowWallAssemblyBuilder
{
public:
	explicit HostedWindowWallAssemblyBuilder(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits(),
		std::shared_ptr<const ArchitecturalReferenceBindingCatalog>
			reference_binding_catalog = {})
		: complexity_limits_(complexity_limits),
		  reference_binding_catalog_(std::move(reference_binding_catalog))
	{
	}

	HostedWindowWallAssemblyBuildResult build(
		const WindowOpeningSpecification &opening,
		const WindowFrameSpecification &frame,
		GeometryDetailLevel detail_level =
			GeometryDetailLevel::FastenersAndSeals) const;

private:
	GeometryComplexityLimits complexity_limits_;
	std::shared_ptr<const ArchitecturalReferenceBindingCatalog>
		reference_binding_catalog_;
};
