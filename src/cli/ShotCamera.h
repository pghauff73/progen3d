#pragma once

#include <glm/glm.hpp>

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

class SceneGenerationContext;
class ShotCameraRail;

namespace Json {
class Value;
}

struct ShotLens {
	float fov_degrees = 43.0f;
	float aperture_radius = 0.0f;
	float max_blur_px = 24.0f;
};

struct ShotRailSample {
	std::string id;
	double distance = 0.0;
	double speed = 0.0;
};

struct ShotCameraSample {
	double time_seconds = 0.0;
	glm::vec3 position{0.0f};
	glm::vec3 aim{0.0f};
	glm::vec3 focus{0.0f};
	float focus_depth = 0.0f;
	ShotLens lens;
	std::optional<glm::vec3> world_up;
	float roll_degrees = 0.0f;
	std::optional<ShotRailSample> rail;
};

template <typename Value>
struct ShotKeyframe {
	double time_seconds = 0.0;
	Value value;
};

struct ShotAttachment {
	std::string object;
	std::string attachment;
};

using ShotTarget = std::variant<glm::vec3, ShotAttachment>;

glm::vec3 resolve_scene_attachment(const SceneGenerationContext &scene, const ShotAttachment &attachment);
glm::mat4 resolve_scene_attachment_frame(const SceneGenerationContext &scene, const ShotAttachment &attachment);

class ShotCamera
{
public:
	static ShotCamera parse(const Json::Value &camera);
	ShotCameraSample sample(const SceneGenerationContext &scene) const;
	ShotCameraSample sample(const SceneGenerationContext &scene, double time_seconds) const;

private:
	double time_seconds_ = 0.0;
	std::vector<ShotKeyframe<glm::vec3>> position_;
	std::shared_ptr<const ShotCameraRail> rail_;
	std::vector<ShotKeyframe<double>> roll_;
	std::vector<ShotKeyframe<ShotTarget>> aim_;
	std::vector<ShotKeyframe<ShotTarget>> focus_;
	std::vector<ShotKeyframe<ShotLens>> lens_;
};
