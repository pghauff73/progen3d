#pragma once

#include "geometry/model/ExtrudeProfileShapeSpecification.h"

class HostedOpeningShapeSpecification : public ExtrudeProfileShapeSpecification
{
public:
	HostedOpeningShapeSpecification(
		Profile2D host_profile_with_openings,
		float depth,
		ExtrudeProfileCapPolicy cap_policy,
		std::size_t opening_count,
		ShapeSpecificationKey key,
		std::string canonical_text,
		GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals)
		: ExtrudeProfileShapeSpecification(
			std::move(host_profile_with_openings),
			depth,
			cap_policy,
			std::move(key),
			std::move(canonical_text),
			detail_level,
			ShapeFamily::HostedOpening),
		  opening_count_(opening_count)
	{
	}

	std::size_t openingCount() const { return opening_count_; }

private:
	std::size_t opening_count_ = 0;
};
