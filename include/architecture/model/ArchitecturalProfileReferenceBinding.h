#pragma once

#include "architecture/model/ArchitecturalProfileKind.h"
#include "architecture/model/ArchitecturalReferenceIdentity.h"

#include <cmath>
#include <string>
#include <utility>

class ArchitecturalProfileReferenceBinding
{
public:
	ArchitecturalProfileReferenceBinding(
		ArchitecturalProfileKind profile_kind,
		std::string catalog_identifier,
		float face_width,
		float depth,
		ArchitecturalReferenceIdentity reference_identity)
		: profile_kind_(profile_kind),
		  catalog_identifier_(std::move(catalog_identifier)),
		  face_width_(face_width),
		  depth_(depth),
		  reference_identity_(std::move(reference_identity))
	{
	}

	ArchitecturalProfileKind profileKind() const { return profile_kind_; }
	const std::string &catalogIdentifier() const { return catalog_identifier_; }
	float faceWidth() const { return face_width_; }
	float depth() const { return depth_; }
	const ArchitecturalReferenceIdentity &referenceIdentity() const
	{
		return reference_identity_;
	}

	bool isValid() const
	{
		return !catalog_identifier_.empty() && std::isfinite(face_width_) &&
		       std::isfinite(depth_) && face_width_ > 0.0f && depth_ > 0.0f &&
		       reference_identity_.isComplete();
	}

private:
	ArchitecturalProfileKind profile_kind_ =
		ArchitecturalProfileKind::AluminiumWindowFrame;
	std::string catalog_identifier_;
	float face_width_ = 0.0f;
	float depth_ = 0.0f;
	ArchitecturalReferenceIdentity reference_identity_{{}, {}, {}, {}};
};
