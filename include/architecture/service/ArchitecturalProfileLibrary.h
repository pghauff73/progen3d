#pragma once

#include "architecture/model/ArchitecturalProfileDefinition.h"
#include "architecture/model/ArchitecturalReferenceBindingCatalog.h"
#include "geometry/model/GeometryComplexityLimits.h"

#include <memory>
#include <string>

class ArchitecturalProfileLibrary
{
public:
	explicit ArchitecturalProfileLibrary(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits(),
		std::shared_ptr<const ArchitecturalReferenceBindingCatalog>
			reference_binding_catalog = {})
		: complexity_limits_(complexity_limits),
		  reference_binding_catalog_(std::move(reference_binding_catalog))
	{
	}

	std::shared_ptr<const ArchitecturalProfileDefinition> find(
		ArchitecturalProfileKind kind,
		std::string *diagnostic) const;

private:
	GeometryComplexityLimits complexity_limits_;
	std::shared_ptr<const ArchitecturalReferenceBindingCatalog>
		reference_binding_catalog_;
};
