#pragma once

#include "geometry/model/GeneratedMeshReferenceShapeSpecification.h"

#include <cctype>
#include <memory>
#include <string>
#include <utility>

struct GeneratedMeshReferenceSpecificationCandidate
{
	std::string mesh_key;
	GeometryDetailLevel detail_level = GeometryDetailLevel::Assembly;
	ShapeTopology topology = ShapeTopology::Solid;
};

class GeneratedMeshReferenceSpecificationValidator
{
public:
	std::shared_ptr<const GeneratedMeshReferenceShapeSpecification> validate(
		GeneratedMeshReferenceSpecificationCandidate candidate,
		std::string *diagnostic) const
	{
		if (!isIdentifier(candidate.mesh_key)) {
			if (diagnostic != nullptr) {
				*diagnostic =
					"GeneratedMeshReference meshKey must be a non-empty identifier.";
			}
			return {};
		}
		if (candidate.topology == ShapeTopology::Shell) {
			if (diagnostic != nullptr) {
				*diagnostic =
					"GeneratedMeshReference topology must be solid or surface.";
			}
			return {};
		}
		std::string canonical_text =
			"GeneratedMeshReference(meshKey(" + candidate.mesh_key +
			") detail(" + geometryDetailLevelName(candidate.detail_level) + ")";
		std::string key =
			"GeneratedMeshReference:v1:" + candidate.mesh_key + ":" +
			std::to_string(geometryDetailLevelRank(candidate.detail_level));
		if (candidate.topology == ShapeTopology::Surface) {
			canonical_text += " topology(surface)";
			key = "GeneratedMeshReference:v2:" + candidate.mesh_key + ":" +
				std::to_string(geometryDetailLevelRank(candidate.detail_level)) +
				":surface";
		}
		canonical_text += ")";
		return std::make_shared<const GeneratedMeshReferenceShapeSpecification>(
			std::move(candidate.mesh_key),
			ShapeSpecificationKey(key),
			canonical_text,
			candidate.detail_level,
			candidate.topology);
	}

private:
	static bool isIdentifier(const std::string &value)
	{
		if (value.empty() ||
		    !(std::isalpha(static_cast<unsigned char>(value.front())) ||
		      value.front() == '_')) {
			return false;
		}
		for (char character : value) {
			if (!(std::isalnum(static_cast<unsigned char>(character)) ||
			      character == '_')) {
				return false;
			}
		}
		return true;
	}
};
