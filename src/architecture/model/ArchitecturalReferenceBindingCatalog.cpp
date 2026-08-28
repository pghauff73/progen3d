#include "architecture/model/ArchitecturalReferenceBindingCatalog.h"

#include <set>

std::shared_ptr<const ArchitecturalReferenceBindingCatalog>
ArchitecturalReferenceBindingCatalog::create(
	std::vector<ArchitecturalProfileReferenceBinding> profile_bindings,
	std::string *diagnostic)
{
	std::set<ArchitecturalProfileKind> registered_profile_kinds;
	std::set<std::string> registered_catalog_identifiers;
	for (const ArchitecturalProfileReferenceBinding &binding : profile_bindings) {
		if (!binding.isValid()) {
			if (diagnostic != nullptr) {
				*diagnostic =
					"ARBv1 profile bindings require finite positive dimensions, "
					"a catalog identifier, and complete reference provenance.";
			}
			return {};
		}
		if (!registered_profile_kinds.insert(binding.profileKind()).second) {
			if (diagnostic != nullptr) {
				*diagnostic =
					"ARBv1 contains more than one binding for an architectural profile kind.";
			}
			return {};
		}
		if (!registered_catalog_identifiers.insert(binding.catalogIdentifier()).second) {
			if (diagnostic != nullptr) {
				*diagnostic =
					"ARBv1 contains a duplicate architectural profile catalog identifier.";
			}
			return {};
		}
	}

	return std::shared_ptr<const ArchitecturalReferenceBindingCatalog>(
		new ArchitecturalReferenceBindingCatalog(std::move(profile_bindings)));
}

const ArchitecturalProfileReferenceBinding *
ArchitecturalReferenceBindingCatalog::findProfileBinding(
	ArchitecturalProfileKind profile_kind) const
{
	for (const ArchitecturalProfileReferenceBinding &binding : profile_bindings_) {
		if (binding.profileKind() == profile_kind) return &binding;
	}
	return nullptr;
}
