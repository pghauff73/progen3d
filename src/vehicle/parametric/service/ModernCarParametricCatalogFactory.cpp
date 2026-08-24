#include "vehicle/parametric/service/ModernCarParametricCatalogFactory.h"

#include "vehicle/parametric/generated/GeneratedModernCarParametricCatalog.h"

ModernCarFamilyDefinition ModernCarParametricCatalogFactory::createFamilyDefinition() const
{
	return generatedModernCarFamilyDefinition();
}

ParametricModelSourceManifest ModernCarParametricCatalogFactory::createSourceManifest() const
{
	return generatedModernCarSourceManifest();
}

ModernCarParametricObjectModel ModernCarParametricCatalogFactory::createObjectModel(
	ParametricModelGenerationPolicy generation_policy) const
{
	return ModernCarParametricObjectModel(
		"ProGen3D-ModernCarParametricObjectModel-v1",
		ModernCarCoordinateFrame(),
		createFamilyDefinition(),
		createSourceManifest(),
		std::move(generation_policy));
}
