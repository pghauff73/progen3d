#include "architecture/service/ArchitecturalProfileLibrary.h"

#include "geometry/service/Profile2DFactory.h"

#include <optional>

namespace {

struct ProfileDimensions
{
	std::string identifier;
	float face_width = 0.0f;
	float depth = 0.0f;
};

ProfileDimensions dimensions_for(ArchitecturalProfileKind kind)
{
	switch (kind) {
	case ArchitecturalProfileKind::AluminiumWindowFrame:
		return {"Aluminium.WindowFrame", 0.050f, 0.080f};
	case ArchitecturalProfileKind::AluminiumWindowMullion:
		return {"Aluminium.WindowMullion", 0.035f, 0.070f};
	case ArchitecturalProfileKind::WindowGasket:
		return {"Polymer.WindowGasket", 0.008f, 0.012f};
	case ArchitecturalProfileKind::SteelHandrail:
		return {"Steel.Handrail40", 0.040f, 0.040f};
	case ArchitecturalProfileKind::TimberLouver:
		return {"Timber.Louver40x120", 0.040f, 0.120f};
	case ArchitecturalProfileKind::SheetMetalCoping:
		return {"SheetMetal.Coping", 0.050f, 0.080f};
	case ArchitecturalProfileKind::SheetMetalFlashing:
		return {"SheetMetal.Flashing", 0.025f, 0.060f};
	}
	return {};
}

}

std::shared_ptr<const ArchitecturalProfileDefinition>
ArchitecturalProfileLibrary::find(
	ArchitecturalProfileKind kind,
	std::string *diagnostic) const
{
	ProfileDimensions dimensions = dimensions_for(kind);
	std::optional<ArchitecturalReferenceIdentity> reference_identity;
	if (reference_binding_catalog_) {
		const ArchitecturalProfileReferenceBinding *binding =
			reference_binding_catalog_->findProfileBinding(kind);
		if (binding != nullptr) {
			dimensions.identifier = binding->catalogIdentifier();
			dimensions.face_width = binding->faceWidth();
			dimensions.depth = binding->depth();
			reference_identity = binding->referenceIdentity();
		}
	}
	if (dimensions.face_width <= 0.0f || dimensions.depth <= 0.0f) {
		if (diagnostic != nullptr) {
			*diagnostic = "Architectural profile kind is not registered.";
		}
		return {};
	}
	std::shared_ptr<const Profile2D> profile =
		Profile2DFactory(complexity_limits_).createRectangle(
			dimensions.face_width, dimensions.depth, diagnostic);
	if (!profile) return {};
	return std::make_shared<const ArchitecturalProfileDefinition>(
		kind,
		dimensions.identifier,
		*profile,
		dimensions.face_width,
		dimensions.depth,
		std::move(reference_identity));
}
