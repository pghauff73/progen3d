#pragma once

#include "lighting/model/LightId.h"
#include "lighting/model/LightProvenance.h"
#include "lighting/model/PhotometricEmission.h"
#include "lighting/model/SceneLightReferenceFrame.h"
#include "lighting/model/SceneLightType.h"
#include "lighting/model/ShadowCastingPolicy.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <string>
#include <utility>
#include <variant>

class DirectionalLightData
{
public:
	glm::vec3 direction{0.0f, -1.0f, 0.0f};
};

class PointLightData
{
public:
	float range = 10.0f;
};

class SpotLightData
{
public:
	float range = 10.0f;
	float inner_cone_radians = 0.35f;
	float outer_cone_radians = 0.55f;
};

class RectangularAreaLightData
{
public:
	glm::vec2 dimensions{1.0f, 1.0f};
	float range = 10.0f;
	bool two_sided = false;
};

using SceneLightData = std::variant<
	DirectionalLightData,
	PointLightData,
	SpotLightData,
	RectangularAreaLightData>;

class SceneLight
{
public:
	SceneLight() = default;
	SceneLight(LightId id,
	           std::string name,
	           SceneLightType type,
	           SceneLightData data)
		: id_(std::move(id)),
		  name_(std::move(name)),
		  type_(type),
		  data_(std::move(data))
	{
	}

	const LightId &id() const { return id_; }
	void setId(LightId id) { id_ = std::move(id); }

	const std::string &name() const { return name_; }
	void setName(std::string name) { name_ = std::move(name); }

	SceneLightType type() const { return type_; }
	void setType(SceneLightType type);

	const glm::vec3 &position() const { return position_; }
	void setPosition(const glm::vec3 &position) { position_ = position; }

	const glm::quat &orientation() const { return orientation_; }
	void setOrientation(const glm::quat &orientation) { orientation_ = orientation; }

	bool enabled() const { return enabled_; }
	void setEnabled(bool enabled) { enabled_ = enabled; }

	bool visible() const { return visible_; }
	void setVisible(bool visible) { visible_ = visible; }

	PhotometricEmission &emission() { return emission_; }
	const PhotometricEmission &emission() const { return emission_; }

	ShadowCastingPolicy &shadow() { return shadow_; }
	const ShadowCastingPolicy &shadow() const { return shadow_; }

	float exposureCompensation() const { return exposure_compensation_; }
	void setExposureCompensation(float exposure_compensation)
	{
		exposure_compensation_ = exposure_compensation;
	}
	float controlExposureCompensation() const { return control_exposure_compensation_; }
	void setControlExposureCompensation(float exposure_compensation)
	{
		control_exposure_compensation_ = exposure_compensation;
	}
	float effectiveExposureCompensation() const
	{
		return exposure_compensation_ + control_exposure_compensation_;
	}

	LightProvenance provenance() const { return provenance_; }
	void setProvenance(LightProvenance provenance) { provenance_ = provenance; }

	SceneLightReferenceFrame referenceFrame() const { return reference_frame_; }
	void setReferenceFrame(SceneLightReferenceFrame reference_frame)
	{
		reference_frame_ = reference_frame;
	}

	SceneLightData &data() { return data_; }
	const SceneLightData &data() const { return data_; }

	glm::vec3 emissionDirection() const;
	void setEmissionDirection(const glm::vec3 &direction);
	float range() const;

private:
	LightId id_;
	std::string name_;
	SceneLightType type_ = SceneLightType::Point;
	glm::vec3 position_{0.0f};
	glm::quat orientation_{1.0f, 0.0f, 0.0f, 0.0f};
	bool enabled_ = true;
	bool visible_ = true;
	PhotometricEmission emission_;
	ShadowCastingPolicy shadow_;
	float exposure_compensation_ = 0.0f;
	float control_exposure_compensation_ = 0.0f;
	LightProvenance provenance_ = LightProvenance::Editor;
	SceneLightReferenceFrame reference_frame_ = SceneLightReferenceFrame::World;
	SceneLightData data_{PointLightData{}};
};
