#pragma once

#include "architecture/model/ArchitecturalAssemblyBuildResult.h"
#include "architecture/model/ArchitecturalReferenceBindingCatalog.h"
#include "architecture/model/WindowFrameSpecification.h"
#include "geometry/model/GeometryComplexityLimits.h"
#include "geometry/model/GeometryDetailLevel.h"

#include <memory>

class WindowFrameAssemblyBuilder
{
public:
	explicit WindowFrameAssemblyBuilder(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits(),
		std::shared_ptr<const ArchitecturalReferenceBindingCatalog>
			reference_binding_catalog = {})
		: complexity_limits_(complexity_limits),
		  reference_binding_catalog_(std::move(reference_binding_catalog))
	{
	}

	ArchitecturalAssemblyBuildResult build(
		const WindowFrameSpecification &specification,
		GeometryDetailLevel detail_level =
			GeometryDetailLevel::FastenersAndSeals) const;

private:
	GeometryComplexityLimits complexity_limits_;
	std::shared_ptr<const ArchitecturalReferenceBindingCatalog>
		reference_binding_catalog_;
};
