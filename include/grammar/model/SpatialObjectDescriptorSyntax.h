#pragma once

#include "grammar/model/CollisionParticipationSyntax.h"
#include "grammar/model/GrammarSourceRange.h"
#include "grammar/model/SpatialTaxonomySyntax.h"

#include <sstream>
#include <string>
#include <utility>

class SpatialObjectDescriptorSyntax
{
public:
	SpatialObjectDescriptorSyntax(std::string object_id,
	                              std::string object_name,
	                              std::string object_class,
	                              SpatialTaxonomySyntax taxonomy,
	                              std::string container_object_id,
	                              CollisionParticipationSyntax collision_participation,
	                              GrammarSourceRange source_range)
		: object_id_(std::move(object_id)),
		  object_name_(std::move(object_name)),
		  object_class_(std::move(object_class)),
		  taxonomy_(std::move(taxonomy)),
		  container_object_id_(std::move(container_object_id)),
		  collision_participation_(std::move(collision_participation)),
		  source_range_(source_range)
	{
	}

	const std::string &objectId() const { return object_id_; }
	const std::string &objectName() const { return object_name_; }
	const std::string &objectClass() const { return object_class_; }
	const SpatialTaxonomySyntax &taxonomy() const { return taxonomy_; }
	const std::string &containerObjectId() const { return container_object_id_; }
	const CollisionParticipationSyntax &collisionParticipation() const
	{
		return collision_participation_;
	}
	const GrammarSourceRange &sourceRange() const { return source_range_; }

	std::string canonicalText() const
	{
		std::ostringstream text;
		text << "Object(id(" << object_id_ << ")";
		if (object_name_ != object_id_) text << " name(" << object_name_ << ")";
		text << " class(" << object_class_ << ")";
		if (!taxonomy_.segments().empty()) text << " " << taxonomy_.canonicalText();
		if (!container_object_id_.empty()) text << " container(" << container_object_id_ << ")";
		text << " " << collision_participation_.canonicalText() << ")";
		return text.str();
	}

private:
	std::string object_id_;
	std::string object_name_;
	std::string object_class_;
	SpatialTaxonomySyntax taxonomy_;
	std::string container_object_id_;
	CollisionParticipationSyntax collision_participation_;
	GrammarSourceRange source_range_;
};
