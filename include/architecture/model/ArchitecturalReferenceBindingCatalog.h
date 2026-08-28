#pragma once

#include "architecture/model/ArchitecturalProfileReferenceBinding.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

class ArchitecturalReferenceBindingCatalog
{
public:
	static std::shared_ptr<const ArchitecturalReferenceBindingCatalog> create(
		std::vector<ArchitecturalProfileReferenceBinding> profile_bindings,
		std::string *diagnostic);

	const ArchitecturalProfileReferenceBinding *findProfileBinding(
		ArchitecturalProfileKind profile_kind) const;

	const std::vector<ArchitecturalProfileReferenceBinding> &profileBindings() const
	{
		return profile_bindings_;
	}

private:
	explicit ArchitecturalReferenceBindingCatalog(
		std::vector<ArchitecturalProfileReferenceBinding> profile_bindings)
		: profile_bindings_(std::move(profile_bindings))
	{
	}

	std::vector<ArchitecturalProfileReferenceBinding> profile_bindings_;
};
