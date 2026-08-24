#pragma once

#include "grammar/model/GeometryExpression.h"
#include "grammar/model/GrammarSourceRange.h"
#include "grammar/model/SpatialClearanceSyntax.h"

#include <sstream>
#include <string>
#include <utility>

class SpatialConnectionDeclarationSyntax
{
public:
	SpatialConnectionDeclarationSyntax(std::string connection_id,
	                                   std::string source_object_id,
	                                   std::string source_interface_id,
	                                   std::string target_object_id,
	                                   std::string target_interface_id,
	                                   std::string connection_type,
	                                   SpatialClearanceSyntax clearance,
	                                   GeometryExpression insertion_depth,
	                                   GrammarSourceRange source_range)
		: connection_id_(std::move(connection_id)),
		  source_object_id_(std::move(source_object_id)),
		  source_interface_id_(std::move(source_interface_id)),
		  target_object_id_(std::move(target_object_id)),
		  target_interface_id_(std::move(target_interface_id)),
		  connection_type_(std::move(connection_type)),
		  clearance_(std::move(clearance)),
		  insertion_depth_(std::move(insertion_depth)),
		  source_range_(source_range)
	{
	}

	const std::string &connectionId() const { return connection_id_; }
	const std::string &sourceObjectId() const { return source_object_id_; }
	const std::string &sourceInterfaceId() const { return source_interface_id_; }
	const std::string &targetObjectId() const { return target_object_id_; }
	const std::string &targetInterfaceId() const { return target_interface_id_; }
	const std::string &connectionType() const { return connection_type_; }
	const SpatialClearanceSyntax &clearance() const { return clearance_; }
	const GeometryExpression &insertionDepth() const { return insertion_depth_; }
	const GrammarSourceRange &sourceRange() const { return source_range_; }

	std::string canonicalText() const
	{
		std::ostringstream text;
		text << "Connect(id(" << connection_id_ << ") source(" << source_object_id_
		     << " " << source_interface_id_ << ") target(" << target_object_id_
		     << " " << target_interface_id_ << ") type(" << connection_type_ << ") "
		     << clearance_.canonicalText() << " insertionDepth("
		     << insertion_depth_.sourceText() << "))";
		return text.str();
	}

private:
	std::string connection_id_;
	std::string source_object_id_;
	std::string source_interface_id_;
	std::string target_object_id_;
	std::string target_interface_id_;
	std::string connection_type_;
	SpatialClearanceSyntax clearance_;
	GeometryExpression insertion_depth_;
	GrammarSourceRange source_range_;
};
