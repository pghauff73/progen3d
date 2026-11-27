#include "ShotCamera.h"

#include "ShotCameraRail.h"

#include "Context.h"
#include "editor/model/PreviewProjectionConfiguration.h"
#include "spatial/model/SpatialBuildingModel.h"
#include "spatial/service/SpatialInterfaceQueryService.h"

#include <json/json.h>

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace {

void check_fields(const Json::Value &object, std::initializer_list<std::string_view> fields,
                  const std::string &location)
{
	if (!object.isObject()) {
		throw std::runtime_error(location + " must be a JSON object.");
	}
	for (const std::string &name : object.getMemberNames()) {
		if (std::find(fields.begin(), fields.end(), name) == fields.end()) {
			throw std::runtime_error("Unknown field: " + location + "." + name);
		}
	}
}

double read_number(const Json::Value &value, const std::string &location, double minimum, double maximum)
{
	if (!value.isNumeric() || !std::isfinite(value.asDouble()) ||
	    value.asDouble() < minimum || value.asDouble() > maximum) {
		throw std::runtime_error(location + " must be a finite number in the permitted range.");
	}
	return value.asDouble();
}

double read_time(const Json::Value &value, const std::string &location)
{
	return read_number(value, location, 0.0, std::numeric_limits<double>::max());
}

glm::dvec3 read_vector(const Json::Value &value, const std::string &location)
{
	if (!value.isArray() || value.size() != 3) {
		throw std::runtime_error(location + " must have three world coordinates [x, y, z].");
	}
	constexpr double limit = std::numeric_limits<float>::max();
	return {read_number(value[0], location + "[0]", -limit, limit),
	        read_number(value[1], location + "[1]", -limit, limit),
	        read_number(value[2], location + "[2]", -limit, limit)};
}

glm::vec3 read_position(const Json::Value &value, const std::string &location)
{
	return glm::vec3(read_vector(value, location));
}

std::string read_identifier(const Json::Value &value, const std::string &location)
{
	if (!value.isString() || value.asString().empty() || value.asString().find('\0') != std::string::npos) {
		throw std::runtime_error(location + " must be a nonempty identifier.");
	}
	return value.asString();
}

ShotAttachment read_attachment(const Json::Value &value, const std::string &location)
{
	check_fields(value, {"object", "attachment"}, location);
	ShotAttachment reference;
	reference.object = read_identifier(value["object"], location + ".object");
	if (value.isMember("attachment")) {
		reference.attachment = read_identifier(value["attachment"], location + ".attachment");
	}
	return reference;
}

ShotTarget read_target(const Json::Value &value, const std::string &location)
{
	if (value.isArray()) return read_position(value, location);
	return read_attachment(value, location);
}

ShotLens read_lens(const Json::Value &value, const std::string &location)
{
	check_fields(value, {"fov_degrees", "aperture_radius", "max_blur_px"}, location);
	ShotLens lens;
	lens.fov_degrees = static_cast<float>(read_number(
		value["fov_degrees"], location + ".fov_degrees",
		PreviewProjectionConfiguration::minimum_vertical_field_of_view_degrees,
		PreviewProjectionConfiguration::maximum_vertical_field_of_view_degrees));
	lens.aperture_radius = static_cast<float>(read_number(
		value["aperture_radius"], location + ".aperture_radius", 0.0, std::numeric_limits<float>::max()));
	if (value.isMember("max_blur_px")) {
		lens.max_blur_px = static_cast<float>(read_number(
			value["max_blur_px"], location + ".max_blur_px", 0.0, 32.0));
	}
	return lens;
}

template <typename ReadValue>
auto read_track(const Json::Value &value, const std::string &location, ReadValue read_value)
{
	using Value = decltype(read_value(value, location));
	if (!value.isArray() || value.empty()) {
		throw std::runtime_error(location + " needs at least one keyframe.");
	}
	std::vector<ShotKeyframe<Value>> track;
	track.reserve(value.size());
	for (Json::ArrayIndex index = 0; index < value.size(); ++index) {
		const std::string key = location + "[" + std::to_string(index) + "]";
		const Json::Value &keyframe = value[index];
		check_fields(keyframe, {"time", "value"}, key);
		const double time = read_time(keyframe["time"], key + ".time");
		if (!track.empty() && time <= track.back().time_seconds) {
			throw std::runtime_error(location + " keyframe times must be strictly increasing.");
		}
		track.push_back({time, read_value(keyframe["value"], key + ".value")});
	}
	return track;
}

glm::mat4 resolve_attachment_frame(const ShotAttachment &reference, const SceneGenerationContext &scene)
{
	const auto *model = scene.spatialBuildingModel().get();
	const SpatialBuildingObject *object = model == nullptr
		? nullptr : model->objects().find(SpatialObjectId(reference.object));
	if (object == nullptr) {
		throw std::runtime_error("Unknown camera target object '" + reference.object + "'.");
	}
	glm::mat4 world_transform = object->frameState().resolvedWorldTransform();
	for (const auto &binding : model->bindingsFor(object->identity().objectId())) {
		const auto &body = scene.primitive_instances.at(binding.primitiveInstanceIndex());
		if (body.immovable || body.removed) {
			continue;
		}
		if (body.primary_transform != body.initial_primary_transform &&
		    std::fabs(glm::determinant(body.initial_primary_transform)) > 1.0e-8f) {
			world_transform = body.primary_transform * glm::inverse(body.initial_primary_transform) * world_transform;
		}
		break;
	}
	if (reference.attachment.empty()) {
		return world_transform;
	}
	const SpatialInterface *attachment = object->findInterface(SpatialInterfaceId(reference.attachment));
	if (attachment == nullptr) {
		throw std::runtime_error("Unknown camera attachment '" + reference.attachment +
		                         "' on object '" + reference.object + "'.");
	}
	const auto frame = SpatialInterfaceQueryService().resolveWorldFrame(*attachment, world_transform);
	return {glm::vec4(frame.tangent(), 0.0f), glm::vec4(frame.bitangent(), 0.0f),
	        glm::vec4(frame.normal(), 0.0f), glm::vec4(frame.origin(), 1.0f)};
}

glm::vec3 resolve_target(const ShotTarget &target, const SceneGenerationContext &scene)
{
	if (const auto *point = std::get_if<glm::vec3>(&target)) return *point;
	return glm::vec3(resolve_attachment_frame(std::get<ShotAttachment>(target), scene)[3]);
}

std::shared_ptr<const ShotCameraRail> read_rail(const Json::Value &value)
{
	const std::string location = "camera.rail";
	check_fields(value, {"id", "segments", "progress", "anchor", "up"}, location);
	const auto id = read_identifier(value["id"], location + ".id");
	const Json::Value &segments = value["segments"];
	if (!segments.isArray() || segments.empty()) {
		throw std::runtime_error(location + ".segments must contain at least one cubic curve.");
	}
	std::vector<ShotRailCurve> curves;
	curves.reserve(segments.size());
	for (Json::ArrayIndex index = 0; index < segments.size(); ++index) {
		const auto &segment = segments[index];
		const std::string segment_location = location + ".segments[" + std::to_string(index) + "]";
		if (!segment.isArray() || segment.size() != 4) {
			throw std::runtime_error(segment_location + " requires four control points.");
		}
		ShotRailCurve curve;
		for (Json::ArrayIndex point = 0; point < segment.size(); ++point) {
			curve[point] = read_vector(segment[point], segment_location + "[" + std::to_string(point) + "]");
		}
		curves.push_back(curve);
	}
	auto progress = read_track(value["progress"], location + ".progress",
		[](const Json::Value &fraction, const std::string &field) { return read_number(fraction, field, 0.0, 1.0); });
	std::optional<ShotAttachment> anchor;
	if (value.isMember("anchor")) anchor = read_attachment(value["anchor"], location + ".anchor");
	const glm::dvec3 up = value.isMember("up")
		? read_vector(value["up"], location + ".up")
		: glm::dvec3(0.0, 1.0, 0.0);
	return std::make_shared<ShotCameraRail>(id, std::move(curves), std::move(progress), std::move(anchor), up);
}

std::vector<ShotKeyframe<glm::vec3>> resolve_track(
	const std::vector<ShotKeyframe<ShotTarget>> &targets, const SceneGenerationContext &scene)
{
	std::vector<ShotKeyframe<glm::vec3>> track;
	track.reserve(targets.size());
	for (const auto &key : targets) {
		track.push_back({key.time_seconds, resolve_target(key.value, scene)});
	}
	return track;
}

template <typename Value, typename Blend>
Value sample_track(const std::vector<ShotKeyframe<Value>> &track, double time, const Blend &blend)
{
	const auto next = std::upper_bound(track.begin(), track.end(), time,
		[](double sample_time, const ShotKeyframe<Value> &key) { return sample_time < key.time_seconds; });
	if (next == track.begin()) {
		return track.front().value;
	}
	if (next == track.end()) {
		return track.back().value;
	}
	const auto &previous = *(next - 1);
	const double weight = (time - previous.time_seconds) / (next->time_seconds - previous.time_seconds);
	return blend(previous.value, next->value, weight);
}

double ease(double weight)
{
	return weight * weight * (3.0 - 2.0 * weight);
}

glm::vec3 blend_position(const glm::vec3 &first, const glm::vec3 &second, double weight)
{
	return glm::vec3(glm::mix(glm::dvec3(first), glm::dvec3(second), weight));
}

ShotLens blend_lens(const ShotLens &first, const ShotLens &second, double weight)
{
	return {static_cast<float>(glm::mix<double>(first.fov_degrees, second.fov_degrees, weight)),
	        static_cast<float>(glm::mix<double>(first.aperture_radius, second.aperture_radius, weight)),
	        static_cast<float>(glm::mix<double>(first.max_blur_px, second.max_blur_px, weight))};
}

}

glm::vec3 resolve_scene_attachment(const SceneGenerationContext &scene, const ShotAttachment &attachment)
{
	return glm::vec3(resolve_attachment_frame(attachment, scene)[3]);
}

glm::mat4 resolve_scene_attachment_frame(const SceneGenerationContext &scene, const ShotAttachment &attachment)
{
	return resolve_attachment_frame(attachment, scene);
}

ShotCamera ShotCamera::parse(const Json::Value &camera)
{
	check_fields(camera, {"time_seconds", "position", "rail", "aim", "focus", "lens", "roll"}, "camera");
	ShotCamera result;
	if (camera.isMember("time_seconds")) {
		result.time_seconds_ = read_time(camera["time_seconds"], "camera.time_seconds");
	}
	if (camera.isMember("rail")) {
		if (camera.isMember("position")) {
			throw std::runtime_error("Camera must use either a rail or a position track.");
		}
		result.rail_ = read_rail(camera["rail"]);
	} else {
		result.position_ = read_track(camera["position"], "camera.position", read_position);
	}
	if (camera.isMember("roll")) {
		result.roll_ = read_track(camera["roll"], "camera.roll",
			[](const Json::Value &angle, const std::string &field) {
				constexpr double limit = std::numeric_limits<float>::max();
				return read_number(angle, field, -limit, limit);
			});
	}
	result.aim_ = read_track(camera["aim"], "camera.aim", read_target);
	result.focus_ = read_track(camera["focus"], "camera.focus", read_target);
	result.lens_ = read_track(camera["lens"], "camera.lens", read_lens);
	return result;
}

ShotCameraSample ShotCamera::sample(const SceneGenerationContext &scene) const
{
	return sample(scene, time_seconds_);
}

ShotCameraSample ShotCamera::sample(const SceneGenerationContext &scene, double time_seconds) const
{
	ShotCameraSample result;
	result.time_seconds = time_seconds;
	std::optional<glm::dvec3> rail_up;
	if (rail_) {
		auto rail_pose = rail_->sample(scene, time_seconds);
		result.position = glm::vec3(rail_pose.position);
		rail_up = rail_pose.up;
		result.rail = std::move(rail_pose.motion);
	} else {
		result.position = sample_track(position_, time_seconds, blend_position);
	}
	result.aim = sample_track(resolve_track(aim_, scene), time_seconds,
		[&](const glm::vec3 &first, const glm::vec3 &second, double weight) {
			return blend_position(first, second, rail_ ? ease(weight) : weight);
		});
	result.focus = sample_track(resolve_track(focus_, scene), time_seconds,
		[](const glm::vec3 &first, const glm::vec3 &second, double weight) {
			return blend_position(first, second, ease(weight));
		});
	result.lens = sample_track(lens_, time_seconds,
		[&](const ShotLens &first, const ShotLens &second, double weight) {
			return blend_lens(first, second, rail_ ? ease(weight) : weight);
		});
	if (!roll_.empty()) {
		result.roll_degrees = static_cast<float>(sample_track(roll_, time_seconds,
			[](double first, double last, double weight) { return glm::mix(first, last, ease(weight)); }));
	}
	const glm::dvec3 aim_direction = glm::dvec3(result.aim) - glm::dvec3(result.position);
	const double aim_distance = glm::length(aim_direction);
	if (!std::isfinite(aim_distance) || aim_distance <= 1.0e-5 ||
	    aim_distance > std::sqrt(std::numeric_limits<float>::max()) * 0.5) {
		throw std::runtime_error("Camera position and aim must define a finite, nonzero viewing direction.");
	}
	const glm::dvec3 forward = aim_direction / aim_distance;
	if (rail_up) {
		const glm::dvec3 projected_up = *rail_up - forward * glm::dot(*rail_up, forward);
		const double up_length = glm::length(projected_up);
		if (!std::isfinite(up_length) || up_length < 1.0e-4) {
			throw std::runtime_error("Camera rail up is parallel to aim. Adjust camera.rail.up or the aim track.");
		}
		result.world_up = glm::vec3(projected_up / up_length);
	}
	const double focus_depth = glm::dot(glm::dvec3(result.focus) - glm::dvec3(result.position), forward);
	if (!std::isfinite(focus_depth) || focus_depth <= 0.01 ||
	    focus_depth > std::numeric_limits<float>::max()) {
		throw std::runtime_error("The camera focus target must be in front of the near clipping plane.");
	}
	result.focus_depth = static_cast<float>(focus_depth);
	return result;
}
