#pragma once

#include "grammar/model/GeometryExpression.h"
#include "grammar/model/GrammarSourceRange.h"

#include <array>
#include <sstream>
#include <string>
#include <utility>

class VehicleJointDeclarationSyntax
{
public:
	VehicleJointDeclarationSyntax(
		std::string joint_identifier,
		std::string joint_type,
		std::string source_object_identifier,
		std::string source_interface_identifier,
		std::string target_object_identifier,
		std::string target_interface_identifier,
		std::array<GeometryExpression, 3> axis,
		GeometryExpression minimum_state,
		GeometryExpression maximum_state,
		GeometryExpression current_state,
		GrammarSourceRange source_range)
		: joint_identifier_(std::move(joint_identifier)),
		  joint_type_(std::move(joint_type)),
		  source_object_identifier_(std::move(source_object_identifier)),
		  source_interface_identifier_(std::move(source_interface_identifier)),
		  target_object_identifier_(std::move(target_object_identifier)),
		  target_interface_identifier_(std::move(target_interface_identifier)),
		  axis_(std::move(axis)),
		  minimum_state_(std::move(minimum_state)),
		  maximum_state_(std::move(maximum_state)),
		  current_state_(std::move(current_state)),
		  source_range_(source_range)
	{
	}

	const std::string &jointIdentifier() const { return joint_identifier_; }
	const std::string &jointType() const { return joint_type_; }
	const std::string &sourceObjectIdentifier() const
	{
		return source_object_identifier_;
	}
	const std::string &sourceInterfaceIdentifier() const
	{
		return source_interface_identifier_;
	}
	const std::string &targetObjectIdentifier() const
	{
		return target_object_identifier_;
	}
	const std::string &targetInterfaceIdentifier() const
	{
		return target_interface_identifier_;
	}
	const std::array<GeometryExpression, 3> &axis() const { return axis_; }
	const GeometryExpression &minimumState() const { return minimum_state_; }
	const GeometryExpression &maximumState() const { return maximum_state_; }
	const GeometryExpression &currentState() const { return current_state_; }
	const GrammarSourceRange &sourceRange() const { return source_range_; }

	std::string canonicalText() const
	{
		std::ostringstream text;
		text << "Joint(id(" << joint_identifier_ << ") type(" << joint_type_
		     << ") source(" << source_object_identifier_ << " "
		     << source_interface_identifier_ << ") target("
		     << target_object_identifier_ << " " << target_interface_identifier_
		     << ") axis(" << axis_[0].sourceText() << " "
		     << axis_[1].sourceText() << " " << axis_[2].sourceText()
		     << ") limits(" << minimum_state_.sourceText() << " "
		     << maximum_state_.sourceText() << ") state("
		     << current_state_.sourceText() << "))";
		return text.str();
	}

private:
	std::string joint_identifier_;
	std::string joint_type_;
	std::string source_object_identifier_;
	std::string source_interface_identifier_;
	std::string target_object_identifier_;
	std::string target_interface_identifier_;
	std::array<GeometryExpression, 3> axis_{
		GeometryExpression("0"), GeometryExpression("1"), GeometryExpression("0")};
	GeometryExpression minimum_state_{"0"};
	GeometryExpression maximum_state_{"0"};
	GeometryExpression current_state_{"0"};
	GrammarSourceRange source_range_;
};
