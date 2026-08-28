#pragma once

#include "grammar/model/GeometryExpression.h"
#include "grammar/model/GrammarSourceRange.h"

#include <array>
#include <string>
#include <utility>
#include <vector>

class SceneLightDeclarationSyntax
{
public:
	std::string light_id;
	std::string type = "Point";
	std::array<GeometryExpression, 3> position{
		GeometryExpression("0"), GeometryExpression("0"), GeometryExpression("0")};
	std::array<GeometryExpression, 3> direction{
		GeometryExpression("0"), GeometryExpression("-1"), GeometryExpression("0")};
	std::array<GeometryExpression, 3> color{
		GeometryExpression("1"), GeometryExpression("1"), GeometryExpression("1")};
	GeometryExpression temperature{"6500"};
	bool uses_temperature = false;
	GeometryExpression intensity{"1000"};
	std::string intensity_unit = "Lumens";
	GeometryExpression range{"10"};
	std::array<GeometryExpression, 2> cone{
		GeometryExpression("24"), GeometryExpression("38")};
	std::array<GeometryExpression, 2> area{
		GeometryExpression("1"), GeometryExpression("1")};
	GeometryExpression exposure{"0"};
	std::string shadow = "off";
	std::string reference_frame = "World";
	std::string two_sided = "off";
	GrammarSourceRange source_range;

	std::string canonicalText() const { return "Light(" + light_id + " ...)"; }
};

class LightFixtureDeclarationSyntax
{
public:
	std::string fixture_id;
	std::string fixture_type = "RecessedDownlight";
	std::string emitter_id;
	std::array<GeometryExpression, 3> position{
		GeometryExpression("0"), GeometryExpression("0"), GeometryExpression("0")};
	std::array<GeometryExpression, 3> direction{
		GeometryExpression("0"), GeometryExpression("-1"), GeometryExpression("0")};
	GeometryExpression temperature{"3000"};
	GeometryExpression lumens{"700"};
	GeometryExpression range{"8"};
	std::array<GeometryExpression, 2> cone{
		GeometryExpression("24"), GeometryExpression("36")};
	std::string mount_interface = "ceilingMount";
	std::array<GeometryExpression, 3> clearance{
		GeometryExpression("0.1"), GeometryExpression("0.1"), GeometryExpression("0.1")};
	std::string shadow = "off";
	GrammarSourceRange source_range;

	std::string canonicalText() const { return "LightFixture(" + fixture_id + " ...)"; }
};

class LightSwitchDeclarationSyntax
{
public:
	std::string switch_id;
	std::string switch_type = "SinglePole";
	std::string mount_object_id;
	GeometryExpression height{"1.10"};
	std::array<GeometryExpression, 3> position{
		GeometryExpression("0"), GeometryExpression("0"), GeometryExpression("0")};
	std::string state = "on";
	GeometryExpression dimmer{"1"};
	GrammarSourceRange source_range;

	std::string canonicalText() const { return "LightSwitch(" + switch_id + " ...)"; }
};

class LightingArrayDeclarationSyntax
{
public:
	std::string array_id;
	std::string fixture_type = "RecessedDownlight";
	std::array<GeometryExpression, 3> origin{
		GeometryExpression("0"), GeometryExpression("0"), GeometryExpression("0")};
	std::array<GeometryExpression, 2> grid{
		GeometryExpression("1"), GeometryExpression("1")};
	std::array<GeometryExpression, 2> spacing{
		GeometryExpression("1.8"), GeometryExpression("1.6")};
	std::array<GeometryExpression, 3> direction{
		GeometryExpression("0"), GeometryExpression("-1"), GeometryExpression("0")};
	GeometryExpression temperature{"3000"};
	GeometryExpression lumens{"700"};
	GeometryExpression range{"8"};
	std::array<GeometryExpression, 2> cone{
		GeometryExpression("24"), GeometryExpression("36")};
	std::string mount_interface = "ceilingMount";
	std::array<GeometryExpression, 3> clearance{
		GeometryExpression("0.1"), GeometryExpression("0.1"), GeometryExpression("0.1")};
	std::string shadow = "off";
	std::string circuit_id;
	std::vector<std::string> control_ids;
	GrammarSourceRange source_range;

	std::string canonicalText() const { return "LightingArray(" + array_id + " ...)"; }
};

class LightingCircuitDeclarationSyntax
{
public:
	std::string circuit_id;
	std::vector<std::string> control_ids;
	std::vector<std::string> fixture_ids;
	std::string always_on = "off";
	GrammarSourceRange source_range;

	std::string canonicalText() const { return "LightingCircuit(" + circuit_id + " ...)"; }
};

class ControlsDeclarationSyntax
{
public:
	std::string control_id;
	std::vector<std::string> light_ids;
	GrammarSourceRange source_range;

	std::string canonicalText() const { return "Controls(" + control_id + " ...)"; }
};
