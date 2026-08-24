#pragma once

#include "electrical/model/ElectricalObjectId.h"

#include <glm/glm.hpp>

#include <string>
#include <utility>

enum class SwitchType
{
	SinglePole,
	TwoWay,
	Intermediate,
	Dimmer,
	Momentary,
	Smart,
	SceneController
};

class SwitchState
{
public:
	bool on = true;
	float dimmer = 1.0f;
};

class LightSwitch
{
public:
	LightSwitch() = default;
	LightSwitch(ElectricalObjectId id, std::string name, SwitchType type)
		: id_(std::move(id)), name_(std::move(name)), type_(type) {}

	const ElectricalObjectId &id() const { return id_; }
	const std::string &name() const { return name_; }
	void setName(std::string name) { name_ = std::move(name); }
	SwitchType type() const { return type_; }
	void setType(SwitchType type) { type_ = type; }
	const glm::mat4 &transform() const { return transform_; }
	void setTransform(const glm::mat4 &transform) { transform_ = transform; }
	SwitchState &state() { return state_; }
	const SwitchState &state() const { return state_; }
	const std::string &mountObjectId() const { return mount_object_id_; }
	void setMountObjectId(std::string object_id) { mount_object_id_ = std::move(object_id); }
	float mountHeight() const { return mount_height_; }
	void setMountHeight(float mount_height) { mount_height_ = mount_height; }

private:
	ElectricalObjectId id_;
	std::string name_;
	glm::mat4 transform_{1.0f};
	SwitchType type_ = SwitchType::SinglePole;
	SwitchState state_;
	std::string mount_object_id_;
	float mount_height_ = 1.10f;
};
