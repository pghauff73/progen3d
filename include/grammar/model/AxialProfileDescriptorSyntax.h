#pragma once

#include "geometry/model/AxialTransitionKind.h"
#include "grammar/model/GeometryExpression.h"
#include "grammar/model/ShapeDescriptorSyntax.h"

#include <sstream>
#include <string>
#include <utility>
#include <vector>

class AxialProfileTransformSyntax
{
public:
	AxialProfileTransformSyntax(
		GeometryExpression center_x = GeometryExpression("0"),
		GeometryExpression center_z = GeometryExpression("0"),
		GeometryExpression scale_x = GeometryExpression("1"),
		GeometryExpression scale_z = GeometryExpression("1"),
		GeometryExpression rotation = GeometryExpression("0"))
		: center_x_(std::move(center_x)),
		  center_z_(std::move(center_z)),
		  scale_x_(std::move(scale_x)),
		  scale_z_(std::move(scale_z)),
		  rotation_(std::move(rotation))
	{
	}

	const GeometryExpression &centerX() const { return center_x_; }
	const GeometryExpression &centerZ() const { return center_z_; }
	const GeometryExpression &scaleX() const { return scale_x_; }
	const GeometryExpression &scaleZ() const { return scale_z_; }
	const GeometryExpression &rotation() const { return rotation_; }

private:
	GeometryExpression center_x_;
	GeometryExpression center_z_;
	GeometryExpression scale_x_;
	GeometryExpression scale_z_;
	GeometryExpression rotation_;
};

class AxialProfilePolygonSyntax
{
public:
	AxialProfilePolygonSyntax(std::string name,
	                         std::vector<GeometryExpression> coordinates)
		: name_(std::move(name)), coordinates_(std::move(coordinates))
	{
	}

	const std::string &name() const { return name_; }
	const std::vector<GeometryExpression> &coordinates() const { return coordinates_; }

private:
	std::string name_;
	std::vector<GeometryExpression> coordinates_;
};

class AxialProfileLevelSyntax
{
public:
	AxialProfileLevelSyntax(AxialTransitionKind transition,
	                       GeometryExpression axial_position,
	                       std::string profile_name,
	                       AxialProfileTransformSyntax transform)
		: transition_(transition),
		  axial_position_(std::move(axial_position)),
		  profile_name_(std::move(profile_name)),
		  transform_(std::move(transform))
	{
	}

	AxialTransitionKind transition() const { return transition_; }
	const GeometryExpression &axialPosition() const { return axial_position_; }
	const std::string &profileName() const { return profile_name_; }
	const AxialProfileTransformSyntax &transform() const { return transform_; }

private:
	AxialTransitionKind transition_ = AxialTransitionKind::Initial;
	GeometryExpression axial_position_;
	std::string profile_name_;
	AxialProfileTransformSyntax transform_;
};

class AxialProfileDescriptorSyntax : public ShapeDescriptorSyntax
{
public:
	AxialProfileDescriptorSyntax(
		std::string axis_name,
		std::vector<AxialProfilePolygonSyntax> profiles,
		std::vector<AxialProfileLevelSyntax> levels,
		std::string cap_name)
		: axis_name_(std::move(axis_name)),
		  profiles_(std::move(profiles)),
		  levels_(std::move(levels)),
		  cap_name_(std::move(cap_name))
	{
	}

	std::string shapeName() const override { return "AxialProfile"; }

	const std::vector<ShapeOptionSyntax> &options() const override
	{
		static const std::vector<ShapeOptionSyntax> no_options;
		return no_options;
	}

	const std::string &axisName() const { return axis_name_; }
	const std::vector<AxialProfilePolygonSyntax> &profiles() const { return profiles_; }
	const std::vector<AxialProfileLevelSyntax> &levels() const { return levels_; }
	const std::string &capName() const { return cap_name_; }

	std::string canonicalText() const override
	{
		std::ostringstream text;
		text << "AxialProfile(axis(" << axis_name_ << ")";
		for (const AxialProfilePolygonSyntax &profile : profiles_) {
			text << " profile(" << profile.name() << " polygon(";
			for (std::size_t index = 0; index < profile.coordinates().size(); ++index) {
				if (index > 0) text << " ";
				text << profile.coordinates()[index].sourceText();
			}
			text << "))";
		}
		for (const AxialProfileLevelSyntax &level : levels_) {
			switch (level.transition()) {
			case AxialTransitionKind::Initial: text << " at("; break;
			case AxialTransitionKind::Hold: text << " hold("; break;
			case AxialTransitionKind::Linear: text << " linear("; break;
			case AxialTransitionKind::Step: text << " step("; break;
			}
			if (level.transition() != AxialTransitionKind::Step) {
				text << level.axialPosition().sourceText();
				if (level.transition() == AxialTransitionKind::Hold) {
					text << ")";
					continue;
				}
				text << " ";
			}
			text << level.profileName()
			     << " center(" << level.transform().centerX().sourceText()
			     << " " << level.transform().centerZ().sourceText() << ")"
			     << " scale(" << level.transform().scaleX().sourceText()
			     << " " << level.transform().scaleZ().sourceText() << ")"
			     << " rotate(" << level.transform().rotation().sourceText() << "))";
		}
		text << " cap(" << cap_name_ << "))";
		return text.str();
	}

private:
	std::string axis_name_;
	std::vector<AxialProfilePolygonSyntax> profiles_;
	std::vector<AxialProfileLevelSyntax> levels_;
	std::string cap_name_;
};
