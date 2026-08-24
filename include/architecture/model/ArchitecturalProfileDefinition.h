#pragma once

#include "architecture/model/ArchitecturalProfileKind.h"
#include "architecture/model/ArchitecturalReferenceIdentity.h"
#include "geometry/model/Profile2D.h"

#include <optional>
#include <string>
#include <utility>

class ArchitecturalProfileDefinition
{
public:
	ArchitecturalProfileDefinition(
		ArchitecturalProfileKind kind,
		std::string catalog_identifier,
		Profile2D cross_section,
		float face_width,
		float depth,
		std::optional<ArchitecturalReferenceIdentity> reference_identity =
			std::nullopt)
		: kind_(kind),
		  catalog_identifier_(std::move(catalog_identifier)),
		  cross_section_(std::move(cross_section)),
		  face_width_(face_width),
		  depth_(depth),
		  reference_identity_(std::move(reference_identity))
	{
	}

	ArchitecturalProfileKind kind() const { return kind_; }
	const std::string &catalogIdentifier() const { return catalog_identifier_; }
	const Profile2D &crossSection() const { return cross_section_; }
	float faceWidth() const { return face_width_; }
	float depth() const { return depth_; }
	bool isReferenceBound() const { return reference_identity_.has_value(); }
	const std::optional<ArchitecturalReferenceIdentity> &referenceIdentity() const
	{
		return reference_identity_;
	}

private:
	ArchitecturalProfileKind kind_ = ArchitecturalProfileKind::AluminiumWindowFrame;
	std::string catalog_identifier_;
	Profile2D cross_section_{{{}, ProfileWindingCorrection::None}, {}};
	float face_width_ = 0.0f;
	float depth_ = 0.0f;
	std::optional<ArchitecturalReferenceIdentity> reference_identity_;
};
