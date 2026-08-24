#pragma once

#include "electrical/model/ElectricalObjectId.h"
#include "electrical/model/LightEmitterDefinition.h"

#include <glm/glm.hpp>

#include <string>
#include <utility>
#include <vector>

enum class LightFixtureType
{
	RecessedDownlight,
	Pendant,
	LedStrip,
	ExteriorWallLight,
	SurfaceDownlight,
	LinearPendant,
	WallSconce,
	Bollard,
	StepLight,
	ExteriorSpot,
	AreaPanel
};

class LightFixtureObject
{
public:
	LightFixtureObject() = default;
	LightFixtureObject(ElectricalObjectId object_id,
	                   std::string name,
	                   LightFixtureType fixture_type)
		: object_id_(std::move(object_id)),
		  name_(std::move(name)),
		  fixture_type_(fixture_type) {}

	const ElectricalObjectId &objectId() const { return object_id_; }
	const std::string &name() const { return name_; }
	void setName(std::string name) { name_ = std::move(name); }
	LightFixtureType fixtureType() const { return fixture_type_; }
	const glm::mat4 &transform() const { return transform_; }
	void setTransform(const glm::mat4 &transform) { transform_ = transform; }
	std::vector<LightEmitterDefinition> &emitters() { return emitters_; }
	const std::vector<LightEmitterDefinition> &emitters() const { return emitters_; }
	const std::string &mountInterface() const { return mount_interface_; }
	void setMountInterface(std::string mount_interface)
	{
		mount_interface_ = std::move(mount_interface);
	}
	const glm::vec3 &clearanceEnvelope() const { return clearance_envelope_; }
	void setClearanceEnvelope(const glm::vec3 &clearance_envelope)
	{
		clearance_envelope_ = clearance_envelope;
	}

private:
	ElectricalObjectId object_id_;
	std::string name_;
	LightFixtureType fixture_type_ = LightFixtureType::RecessedDownlight;
	glm::mat4 transform_{1.0f};
	std::vector<LightEmitterDefinition> emitters_;
	std::string mount_interface_ = "ceilingMount";
	glm::vec3 clearance_envelope_{0.1f};
};
