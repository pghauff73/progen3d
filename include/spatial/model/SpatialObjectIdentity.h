#pragma once

#include "spatial/model/SpatialObjectClass.h"
#include "spatial/model/SpatialObjectId.h"
#include "spatial/model/SpatialObjectProvenance.h"
#include "spatial/model/SpatialTaxonomyPath.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

class SpatialObjectIdentity {
public:
	SpatialObjectIdentity(SpatialObjectId object_id,
	                      std::string object_name,
	                      SpatialObjectClass object_class,
	                      SpatialTaxonomyPath taxonomy_path,
	                      std::size_t instance_index,
	                      std::uint64_t revision,
	                      SpatialObjectProvenance provenance)
		: object_id_(std::move(object_id)),
		  object_name_(std::move(object_name)),
		  object_class_(std::move(object_class)),
		  taxonomy_path_(std::move(taxonomy_path)),
		  instance_index_(instance_index),
		  revision_(revision),
		  provenance_(std::move(provenance)) {}

	const SpatialObjectId &objectId() const { return object_id_; }
	const std::string &objectName() const { return object_name_; }
	const SpatialObjectClass &objectClass() const { return object_class_; }
	const SpatialTaxonomyPath &taxonomyPath() const { return taxonomy_path_; }
	std::size_t instanceIndex() const { return instance_index_; }
	std::uint64_t revision() const { return revision_; }
	const SpatialObjectProvenance &provenance() const { return provenance_; }

private:
	SpatialObjectId object_id_;
	std::string object_name_;
	SpatialObjectClass object_class_;
	SpatialTaxonomyPath taxonomy_path_;
	std::size_t instance_index_ = 0;
	std::uint64_t revision_ = 0;
	SpatialObjectProvenance provenance_;
};
