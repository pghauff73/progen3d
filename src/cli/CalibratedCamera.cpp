#include "CalibratedCamera.h"

#include "ManifestInput.h"
#include "imgui_render.h"

#include <algorithm>
#include <stdexcept>

namespace {

glm::dvec3 coordinates(const Json::Value &value, const char *label)
{
	if (!value.isArray() || value.size() != 3) throw std::runtime_error(std::string(label) + " needs three coordinates.");
	return {manifest_input::read_number(value[0], label), manifest_input::read_number(value[1], label),
	        manifest_input::read_number(value[2], label)};
}

Json::Value vector_json(const glm::vec3 &vector)
{
	Json::Value result(Json::arrayValue);
	for (int axis = 0; axis < 3; ++axis) result.append(vector[axis]);
	return result;
}

}

CalibratedCamera CalibratedCamera::parse(const Json::Value &camera, int width, int height)
{
	using manifest_input::read_number;
	manifest_input::check_fields(camera, {"center", "direction", "up", "pixels_per_unit", "distance"}, "orthographic camera");
	const glm::dvec3 center = coordinates(camera["center"], "camera.center");
	const glm::dvec3 direction = coordinates(camera["direction"], "camera.direction");
	const glm::dvec3 up_hint = camera.isMember("up") ? coordinates(camera["up"], "camera.up") : glm::dvec3(0, 1, 0);
	const double density = read_number(camera["pixels_per_unit"], "camera.pixels_per_unit");
	const double distance = camera.isMember("distance") ? read_number(camera["distance"], "camera.distance") : 10.0;
	if (density < 1.0e-6 || density > 1.0e7 || distance < 0.02 || distance > 1.0e6 ||
	    glm::length(center) > 1.0e7) {
		throw std::runtime_error("Orthographic camera scale, center, or distance is outside the supported world range.");
	}
	const double direction_length = glm::length(direction);
	const double up_length = glm::length(up_hint);
	if (!(direction_length > 1.0e-8 && up_length > 1.0e-8)) {
		throw std::runtime_error("Orthographic direction and up must be nonzero.");
	}
	const auto forward = direction / direction_length;
	const auto lateral = glm::cross(forward, up_hint / up_length);
	if (!(glm::length(lateral) > 1.0e-5)) throw std::runtime_error("Orthographic direction and up must be independent.");
	const auto right = glm::normalize(lateral);
	CalibratedCamera result;
	result.projection_ = {width, height, static_cast<float>(density), center, right, glm::cross(right, forward),
	                      forward, static_cast<float>(distance)};
	const auto preview = result.preview();
	if (glm::length(preview.target - *preview.world_position) <= preview.near_clip) {
		throw std::runtime_error("Orthographic distance is too small at the specified center.");
	}
	return result;
}

PreviewCameraState CalibratedCamera::preview() const
{
	PreviewCameraState state;
	state.width = projection_.width;
	state.height = projection_.height;
	state.target = projection_.center;
	state.world_position = projection_.center - projection_.forward * projection_.distance;
	state.world_up = projection_.up;
	state.near_clip = CalibratedOrthographicView::near_clip;
	state.far_clip = projection_.farClip();
	state.orthographic_vertical_span = static_cast<double>(projection_.height) / projection_.pixels_per_unit;
	return state;
}

Json::Value CalibratedCamera::metadata() const
{
	Json::Value result;
	result["projection"] = "orthographic";
	result["center"] = vector_json(projection_.center);
	result["direction"] = vector_json(projection_.forward);
	result["up"] = vector_json(projection_.up);
	result["distance"] = projection_.distance;
	result["pixels_per_unit"] = projection_.pixels_per_unit;
	return result;
}
