#pragma once

#include "editor/model/PreviewImage.h"

#include <glm/glm.hpp>
#include <json/json.h>

#include <memory>
#include <vector>

class PreviewCameraState;
class SceneGenerationContext;

struct FlowOverlayFrame {
	int width = 0;
	int height = 0;
	std::vector<glm::vec3> emission;
	std::vector<float> depth;
	Json::Value effects{Json::arrayValue};

	PreviewImage composite(const PreviewImage &beauty) const;
};

class FlowOverlay {
public:
	static FlowOverlay parse(const Json::Value &manifest);

	bool active(double seconds) const;
	void validate(const SceneGenerationContext &scene, double seconds) const;
	FlowOverlayFrame render(const SceneGenerationContext &scene, const PreviewCameraState &camera,
	                        double seconds, const std::vector<float> &scene_depth) const;

private:
	struct Program;
	std::shared_ptr<const Program> program_;
};
