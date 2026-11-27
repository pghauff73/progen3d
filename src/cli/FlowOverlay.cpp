#include "FlowOverlay.h"

#include "Context.h"
#include "ManifestInput.h"
#include "PreviewColor.h"
#include "ShotCamera.h"
#include "imgui_render.h"
#include "spatial/model/SpatialBuildingModel.h"

#include <glm/geometric.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

enum class EffectKind { electrical, thermal, data, gas, air, inertial, force, tension };

struct AnchoredPoint {
	glm::vec3 coordinate{0.0f};
	std::optional<ShotAttachment> attachment;

	glm::vec3 resolve(const SceneGenerationContext &scene) const
	{
		if (attachment) {
			return glm::vec3(resolve_scene_attachment_frame(scene, *attachment) * glm::vec4(coordinate, 1.0f));
		}
		return coordinate;
	}
};

struct FlowEffect {
	std::string id;
	std::string name;
	EffectKind kind = EffectKind::electrical;
	std::string provenance;
	std::string units;
	std::optional<AnchoredPoint> source;
	std::optional<AnchoredPoint> target;
	std::vector<AnchoredPoint> guides;
	std::string connection;
	std::vector<ShotKeyframe<float>> signal;
	std::vector<ShotKeyframe<glm::vec3>> vectors;
	std::vector<ShotKeyframe<bool>> gate;
	double start = 0.0;
	double end = 0.0;
	double period = 1.0;
	float radius = 2.5f;
	float gain = 1.0f;
	float vector_scale = 1.0f;
	std::optional<float> mass;
	bool xray = false;
	bool body_reference = false;
	bool anchor_space = false;

	bool scheduled(double time) const { return time >= start && time < end; }
};

bool is_vector(EffectKind kind) { return kind == EffectKind::force || kind == EffectKind::inertial; }

float bounded_number(const Json::Value &value, const std::string &field, double minimum, double maximum)
{
	const double number = manifest_input::read_number(value, field);
	if (number < minimum || number > maximum) {
		throw std::runtime_error(field + " is outside the supported range.");
	}
	return static_cast<float>(number);
}

glm::vec3 read_vector(const Json::Value &value, const std::string &field)
{
	if (!value.isArray() || value.size() != 3) throw std::runtime_error(field + " must contain three numbers.");
	glm::vec3 point;
	for (int axis = 0; axis < 3; ++axis) {
		point[axis] = bounded_number(value[axis], field, -1e7, 1e7);
	}
	return point;
}

AnchoredPoint read_point(const Json::Value &value, const std::string &field)
{
	if (value.isArray()) return {read_vector(value, field), std::nullopt};
	manifest_input::check_fields(value, {"object", "attachment", "offset"}, field);
	ShotAttachment attachment{manifest_input::read_string(value["object"], field + ".object"), {}};
	if (value.isMember("attachment")) {
		attachment.attachment = manifest_input::read_string(value["attachment"], field + ".attachment");
	}
	return {value.isMember("offset") ? read_vector(value["offset"], field + ".offset") : glm::vec3(0.0f),
	        std::move(attachment)};
}

bool read_bool(const Json::Value &value, const std::string &field)
{
	if (!value.isBool()) throw std::runtime_error(field + " must be true or false.");
	return value.asBool();
}

template <typename Value, typename Read>
std::vector<ShotKeyframe<Value>> read_keys(const Json::Value &keys, const std::string &field, Read read)
{
	if (!keys.isArray() || keys.empty() || keys.size() > 8192) {
		throw std::runtime_error(field + " must be a nonempty array of keyframes.");
	}
	std::vector<ShotKeyframe<Value>> result;
	for (const auto &key : keys) {
		manifest_input::check_fields(key, {"time", "value"}, field);
		const double time = manifest_input::read_number(key["time"], field + ".time");
		if (time < 0 || (!result.empty() && time <= result.back().time_seconds)) {
			throw std::runtime_error(field + " times must be nonnegative and strictly increasing.");
		}
		result.push_back({time, read(key["value"], field + ".value")});
	}
	return result;
}

template <typename Value>
Value interpolate(const std::vector<ShotKeyframe<Value>> &keys, double time, Value fallback)
{
	if (keys.empty()) return fallback;
	const auto after = std::upper_bound(keys.begin(), keys.end(), time,
		[](double seconds, const auto &key) { return seconds < key.time_seconds; });
	if (after == keys.begin()) return after->value;
	if (after == keys.end()) return keys.back().value;
	const auto before = std::prev(after);
	const auto weight = static_cast<float>((time - before->time_seconds) /
	                                       (after->time_seconds - before->time_seconds));
	return before->value + (after->value - before->value) * weight;
}

bool gate_open(const FlowEffect &effect, double time)
{
	if (effect.gate.empty()) return true;
	const auto after = std::upper_bound(effect.gate.begin(), effect.gate.end(), time,
		[](double seconds, const auto &key) { return seconds < key.time_seconds; });
	return after == effect.gate.begin() ? after->value : std::prev(after)->value;
}

EffectKind read_kind(const std::string &name)
{
	if (name == "electrical") return EffectKind::electrical;
	if (name == "thermal") return EffectKind::thermal;
	if (name == "data") return EffectKind::data;
	if (name == "gas") return EffectKind::gas;
	if (name == "air") return EffectKind::air;
	if (name == "inertial") return EffectKind::inertial;
	if (name == "force") return EffectKind::force;
	if (name == "tension") return EffectKind::tension;
	throw std::runtime_error("Unknown overlay kind: " + name + ".");
}

void check_effect(const FlowEffect &effect, const Json::Value &value)
{
	const std::string field = "Overlay '" + effect.id + "'";
	const bool vector = is_vector(effect.kind);
	const bool tension = effect.kind == EffectKind::tension;
	if (vector) {
		if (!effect.source || effect.target || effect.vectors.empty() || !effect.connection.empty() || !effect.guides.empty()) {
			throw std::runtime_error(field + " needs a source and a vector or vector_track.");
		}
		if (effect.anchor_space && !effect.source->attachment) {
			throw std::runtime_error(field + " requires a named source for anchor-space vectors.");
		}
	} else {
		if ((!effect.connection.empty() && (effect.source || effect.target || tension)) ||
		    (effect.connection.empty() && (!effect.source || !effect.target))) {
			throw std::runtime_error(field + " needs a connection ID or explicit source and target points.");
		}
		if (!effect.vectors.empty() || value.isMember("vector_space") || value.isMember("reference_frame")) {
			throw std::runtime_error(field + " only supports vector settings for force and inertial overlays.");
		}
	}
	if (tension && effect.signal.empty()) throw std::runtime_error(field + " needs a slack-to-taut signal.");
	if (effect.provenance == "illustrative" && effect.units != "relative") {
		throw std::runtime_error(field + " uses relative units for illustrative inputs.");
	}
	if (effect.units != "relative" &&
	    !((vector || tension) && effect.units == "N") &&
	    !(effect.kind == EffectKind::inertial && effect.units == "m/s2")) {
		throw std::runtime_error(field + " has incompatible units.");
	}
	if (effect.mass.has_value() != (effect.kind == EffectKind::inertial && effect.units == "N")) {
		throw std::runtime_error(field + " needs mass_kg exactly when converting inertial acceleration to newtons.");
	}
	if (effect.body_reference && effect.kind != EffectKind::inertial) {
		throw std::runtime_error(field + " only supports apparent loads in the body frame for inertial overlays.");
	}
}

FlowEffect read_effect(const Json::Value &value)
{
	using manifest_input::read_number;
	using manifest_input::read_string;
	manifest_input::check_fields(value, {"id", "kind", "active", "source", "target", "guides", "signal", "units",
		"vector", "vector_track", "provenance", "period_seconds", "width_px", "gain", "xray", "gate",
		"mass_kg", "vector_scale", "reference_frame", "vector_space", "connection"}, "overlay effect");
	FlowEffect result;
	result.id = read_string(value["id"], "effect.id");
	result.name = read_string(value["kind"], "effect.kind");
	result.kind = read_kind(result.name);
	const auto &range = value["active"];
	if (!range.isArray() || range.size() != 2) throw std::runtime_error("effect.active needs [start, end] seconds.");
	result.start = read_number(range[0], "effect.active start");
	result.end = read_number(range[1], "effect.active end");
	if (result.start < 0 || result.end <= result.start) throw std::runtime_error("Invalid effect time range.");
	result.provenance = value.isMember("provenance") ? read_string(value["provenance"], "effect.provenance") : "illustrative";
	if (result.provenance != "illustrative" && result.provenance != "authored" && result.provenance != "solved") {
		throw std::runtime_error("effect.provenance must be illustrative, authored or solved.");
	}
	result.units = value.isMember("units") ? read_string(value["units"], "effect.units") : "relative";
	if (value.isMember("source")) result.source = read_point(value["source"], "effect.source");
	if (value.isMember("target")) result.target = read_point(value["target"], "effect.target");
	if (value.isMember("connection")) result.connection = read_string(value["connection"], "effect.connection");
	if (value.isMember("guides")) {
		if (!value["guides"].isArray() || value["guides"].size() > 64) {
			throw std::runtime_error("effect.guides must be an array of up to 64 route points.");
		}
		for (const auto &guide : value["guides"]) result.guides.push_back(read_point(guide, "effect.guide"));
	}
	if (value.isMember("signal")) {
		result.signal = read_keys<float>(value["signal"], "effect.signal", [](const auto &key, const auto &field) {
			return bounded_number(key, field, 0.0, 1e8);
		});
	}
	if (value.isMember("vector_track")) {
		if (value.isMember("vector")) throw std::runtime_error("Choose vector or vector_track.");
		result.vectors = read_keys<glm::vec3>(value["vector_track"], "effect.vector_track", read_vector);
	} else if (value.isMember("vector")) {
		result.vectors.push_back({result.start, read_vector(value["vector"], "effect.vector")});
	}
	if (value.isMember("gate")) result.gate = read_keys<bool>(value["gate"], "effect.gate", read_bool);
	if (value.isMember("xray")) result.xray = read_bool(value["xray"], "effect.xray");
	if (value.isMember("period_seconds")) result.period = bounded_number(value["period_seconds"], "effect.period_seconds", 0.02, 60);
	if (value.isMember("width_px")) result.radius = 0.5f * bounded_number(value["width_px"], "effect.width_px", 0.5, 48);
	if (value.isMember("gain")) result.gain = bounded_number(value["gain"], "effect.gain", 0.0, 32);
	if (value.isMember("vector_scale")) result.vector_scale = bounded_number(value["vector_scale"], "effect.vector_scale", 1e-8, 1e5);
	if (value.isMember("mass_kg")) result.mass = bounded_number(value["mass_kg"], "effect.mass_kg", 1e-8, 1e8);
	if (value.isMember("reference_frame")) {
		const auto frame = read_string(value["reference_frame"], "effect.reference_frame");
		if (frame != "world" && frame != "body") throw std::runtime_error("effect.reference_frame must be world or body.");
		result.body_reference = frame == "body";
	}
	if (value.isMember("vector_space")) {
		const auto space = read_string(value["vector_space"], "effect.vector_space");
		if (space != "world" && space != "anchor") throw std::runtime_error("effect.vector_space must be world or anchor.");
		result.anchor_space = space == "anchor";
	}
	check_effect(result, value);
	return result;
}

struct ResolvedRoute {
	std::vector<glm::vec3> points;
	std::vector<float> distances;
	float length() const { return distances.back(); }
};

void check_port(const SpatialBuildingModel &model, const SpatialInterfaceReference &reference, EffectKind kind)
{
	const auto *object = model.objects().find(reference.objectId());
	const auto *port = object ? object->findInterface(reference.interfaceId()) : nullptr;
	if (!port) throw std::runtime_error("Overlay connection endpoint does not exist.");
	const auto type = port->type();
	const bool matches =
		(kind == EffectKind::electrical && type == SpatialInterfaceType::ElectricalPort) ||
		(kind == EffectKind::thermal && type == SpatialInterfaceType::ThermalInterface) ||
		(kind == EffectKind::gas && type == SpatialInterfaceType::PipePort) ||
		(kind == EffectKind::air && type == SpatialInterfaceType::DuctPort) ||
		(kind == EffectKind::data && (type == SpatialInterfaceType::DataPort || type == SpatialInterfaceType::ControlPort));
	if (!matches) throw std::runtime_error("Overlay effect kind does not match the declared connection ports.");
}

std::vector<glm::vec3> resolve_knots(const FlowEffect &effect, const SceneGenerationContext &scene)
{
	std::vector<glm::vec3> knots;
	std::optional<AnchoredPoint> source = effect.source;
	std::optional<AnchoredPoint> target = effect.target;
	if (!effect.connection.empty()) {
		const auto &model = scene.spatialBuildingModel();
		if (!model) throw std::runtime_error("A spatial scene is required for connection '" + effect.connection + "'.");
		const auto &connections = model->connectionGraph().connections();
		const auto found = std::find_if(connections.begin(), connections.end(), [&](const auto &connection) {
			return connection.connectionId().value() == effect.connection;
		});
		if (found == connections.end()) throw std::runtime_error("Unknown active overlay connection: " + effect.connection + ".");
		check_port(*model, found->sourceInterface(), effect.kind);
		check_port(*model, found->targetInterface(), effect.kind);
		const auto point = [](const SpatialInterfaceReference &reference) -> AnchoredPoint {
			return {glm::vec3(0.0f), ShotAttachment{reference.objectId().value(), reference.interfaceId().value()}};
		};
		source = point(found->sourceInterface());
		target = point(found->targetInterface());
	}
	knots.push_back(source->resolve(scene));
	for (const auto &guide : effect.guides) knots.push_back(guide.resolve(scene));
	knots.push_back(target->resolve(scene));
	return knots;
}

ResolvedRoute curve_route(const std::vector<glm::vec3> &knots, bool straight)
{
	ResolvedRoute route{{knots.front()}, {0.0f}};
	const auto append = [&](const glm::vec3 &point) {
		const float length = glm::length(point - route.points.back());
		if (!std::isfinite(length)) throw std::runtime_error("Overlay contains a non-finite world coordinate.");
		if (length <= 1e-6f) return;
		route.points.push_back(point);
		route.distances.push_back(route.distances.back() + length);
	};
	for (std::size_t index = 1; index < knots.size(); ++index) {
		const auto &first = knots[index - 1];
		const auto &last = knots[index];
		if (straight || knots.size() == 2 || glm::length(last - first) <= 1e-6f) {
			append(last);
			continue;
		}
		const auto &before = knots[index > 1 ? index - 2 : index - 1];
		const auto &after = knots[index + 1 < knots.size() ? index + 1 : index];
		const glm::vec3 tangent1 = (last - before) * 0.5f;
		const glm::vec3 tangent2 = (after - first) * 0.5f;
		for (int sample = 1; sample <= 12; ++sample) {
			const float t = sample / 12.0f;
			const float t2 = t * t, t3 = t2 * t;
			append((2*t3 - 3*t2 + 1)*first + (t3 - 2*t2 + t)*tangent1 +
			       (-2*t3 + 3*t2)*last + (t3 - t2)*tangent2);
		}
	}
	return route;
}

class OverlayRaster {
public:
	OverlayRaster(const PreviewCameraState &camera,
	              const std::vector<float> &scene_depth,
	              FlowOverlayFrame &frame)
		: matrices_(preview_camera_matrices(camera)),
		  scale_(camera.scale_global),
		  scene_depth_(scene_depth),
		  frame_(frame),
		  local_emission_(scene_depth.size(), glm::vec3(0.0f)),
		  local_depth_(scene_depth.size(), 0.0f)
	{
	}

	glm::vec3 forward() const { return -glm::vec3(glm::transpose(matrices_.view)[2]); }

	void draw(const glm::vec3 &first, const glm::vec3 &last,
	          const glm::vec3 &color, float brightness, float radius, bool xray)
	{
		if (brightness <= 0.0f) return;
		const auto view_first = matrices_.view * glm::vec4(first * scale_, 1.0f);
		const auto view_last = matrices_.view * glm::vec4(last * scale_, 1.0f);
		const auto clip_first = matrices_.projection * view_first;
		const auto clip_last = matrices_.projection * view_last;
		const float extent = radius * 3.4f + 1.0f;
		float enter = 0.0f;
		float exit = 1.0f;
		if (!clip_segment(clip_first, clip_last, extent, enter, exit)) return;

		const auto clip_a = clip_first + enter * (clip_last - clip_first);
		const auto clip_b = clip_first + exit * (clip_last - clip_first);
		if (clip_a.w <= 0.0f || clip_b.w <= 0.0f) return;
		const float inverse_a = 1.0f / clip_a.w;
		const float inverse_b = 1.0f / clip_b.w;
		const float depth_a = -(view_first.z + enter * (view_last.z - view_first.z)) / scale_;
		const float depth_b = -(view_first.z + exit * (view_last.z - view_first.z)) / scale_;
		const glm::vec2 a = screen(clip_a, inverse_a);
		const glm::vec2 b = screen(clip_b, inverse_b);
		const glm::vec2 delta = b - a;
		const float squared_length = glm::dot(delta, delta);
		const int start_x = std::max(0, static_cast<int>(std::floor(std::min(a.x, b.x) - extent)));
		const int end_x = std::min(frame_.width, static_cast<int>(std::ceil(std::max(a.x, b.x) + extent)));
		const int start_y = std::max(0, static_cast<int>(std::floor(std::min(a.y, b.y) - extent)));
		const int end_y = std::min(frame_.height, static_cast<int>(std::ceil(std::max(a.y, b.y) + extent)));
		for (int y = start_y; y < end_y; ++y) {
			for (int x = start_x; x < end_x; ++x) {
				const glm::vec2 pixel(x + 0.5f, y + 0.5f);
				const float progress = squared_length > 1e-6f
					? std::clamp(glm::dot(pixel - a, delta) / squared_length, 0.0f, 1.0f)
					: (depth_a <= depth_b ? 0.0f : 1.0f);
				const float separation = glm::length(pixel - (a + progress * delta));
				if (separation >= extent) continue;
				const float w_a = (1.0f - progress) * inverse_a;
				const float w_b = progress * inverse_b;
				const float depth = (w_a * depth_a + w_b * depth_b) / (w_a + w_b);
				const auto index = static_cast<std::size_t>(y) * frame_.width + x;
				const float surface = scene_depth_[index];
				if (!xray && surface > 0.0f && depth > surface + std::min(0.005f, 0.0005f + surface * 0.0001f)) continue;

				const float edge = std::clamp(radius + 0.5f - separation, 0.0f, 1.0f);
				const float relative_distance = separation / (1.2f * radius + 0.5f);
				const float halo = 0.46f * std::exp(-relative_distance * relative_distance);
				const float energy = brightness * (1.55f * edge + halo);
				if (energy < 0.001f) continue;
				if (local_depth_[index] == 0.0f) touched_.push_back(index);
				const glm::vec3 incoming = color * energy;
				for (int channel = 0; channel < 3; ++channel) {
					local_emission_[index][channel] = std::max(local_emission_[index][channel], incoming[channel]);
				}
				update_depth(local_depth_[index], depth);
			}
		}
	}

	Json::Value flush()
	{
		for (const auto index : touched_) {
			frame_.emission[index] += local_emission_[index];
			update_depth(frame_.depth[index], local_depth_[index]);
			local_emission_[index] = glm::vec3(0.0f);
			local_depth_[index] = 0.0f;
		}
		const auto count = static_cast<Json::UInt64>(touched_.size());
		touched_.clear();
		return Json::Value(count);
	}

private:
	static void update_depth(float &nearest, float depth)
	{
		if (nearest == 0.0f || depth < nearest) nearest = depth;
	}

	bool clip_segment(const glm::vec4 &a, const glm::vec4 &b, float extent, float &enter, float &exit) const
	{
		const float x_margin = 1.0f + 2.0f * extent / frame_.width;
		const float y_margin = 1.0f + 2.0f * extent / frame_.height;
		const std::array<std::array<float, 2>, 6> distances{{
			{{x_margin * a.w + a.x, x_margin * b.w + b.x}},
			{{x_margin * a.w - a.x, x_margin * b.w - b.x}},
			{{y_margin * a.w + a.y, y_margin * b.w + b.y}},
			{{y_margin * a.w - a.y, y_margin * b.w - b.y}},
			{{a.w + a.z, b.w + b.z}},
			{{a.w - a.z, b.w - b.z}},
		}};
		for (const auto &plane : distances) {
			const auto [start, end] = plane;
			if (!std::isfinite(start) || !std::isfinite(end) || (start < 0.0f && end < 0.0f)) return false;
			if (start < 0.0f) enter = std::max(enter, start / (start - end));
			if (end < 0.0f) exit = std::min(exit, start / (start - end));
		}
		return enter <= exit;
	}

	glm::vec2 screen(const glm::vec4 &clip, float inverse) const
	{
		return {(0.5f + 0.5f * clip.x * inverse) * frame_.width,
		        (0.5f - 0.5f * clip.y * inverse) * frame_.height};
	}

	PreviewCameraMatrices matrices_;
	float scale_;
	const std::vector<float> &scene_depth_;
	FlowOverlayFrame &frame_;
	std::vector<glm::vec3> local_emission_;
	std::vector<float> local_depth_;
	std::vector<std::size_t> touched_;
};

Json::Value to_json(const glm::vec3 &value)
{
	Json::Value array(Json::arrayValue);
	array.append(value.x);
	array.append(value.y);
	array.append(value.z);
	return array;
}

struct Stroke {
	glm::vec3 color;
	float energy;
	float width_multiplier;
};

Stroke flow_stroke(EffectKind kind, float fraction, float elapsed_cycles)
{
	float phase = fraction * 3.0f - elapsed_cycles;
	phase -= std::floor(phase + 0.5f);
	const float packets = std::exp(-phase * phase / 0.009f);
	constexpr float tau = 6.2831853f;
	const float wave = 0.5f + 0.5f * std::cos(tau * (fraction * 1.2f - elapsed_cycles));
	switch (kind) {
	case EffectKind::electrical:
		return {{0.14f + 0.28f * packets, 0.9f, 1.0f}, 0.08f + 1.65f * packets, 1.0f};
	case EffectKind::thermal:
		return {{1.0f, 0.09f + 0.42f * wave + 0.16f * fraction, 0.015f}, 0.38f + 0.72f * wave, 1.45f};
	case EffectKind::data:
		return {{0.065f, 1.0f, 0.70f}, 0.06f + 1.25f * packets, 0.8f};
	case EffectKind::gas:
		return {{0.55f, 1.0f, 0.055f}, 0.12f + 0.85f * packets, 1.15f};
	case EffectKind::air:
		return {{0.05f, 0.58f, 1.0f}, 0.15f + 0.75f * wave, 1.18f};
	default:
		throw std::runtime_error("An overlay flow needs a supported flow kind.");
	}
}

void draw_flow(OverlayRaster &raster, const FlowEffect &effect, const ResolvedRoute &route, float elapsed_cycles, float signal)
{
	const float length = route.length();
	for (std::size_t index = 1; index < route.points.size(); ++index) {
		const float start = route.distances[index - 1] / length;
		const float end = route.distances[index] / length;
		const int steps = std::max(1, static_cast<int>(std::ceil(160.0f * (end - start))));
		const auto a = route.points[index - 1];
		const auto delta = route.points[index] - a;
		for (int step = 0; step < steps; ++step) {
			const float from = static_cast<float>(step) / steps;
			const float to = static_cast<float>(step + 1) / steps;
			const auto stroke = flow_stroke(effect.kind, start + 0.5f * (from + to) * (end - start), elapsed_cycles);
			raster.draw(a + from * delta, a + to * delta, stroke.color,
			            effect.gain * signal * stroke.energy, effect.radius * stroke.width_multiplier, effect.xray);
		}
	}
}

void draw_arrow(OverlayRaster &raster, const FlowEffect &effect, const glm::vec3 &origin, const glm::vec3 &tip,
                const glm::vec3 &color, float intensity)
{
	const glm::vec3 vector = tip - origin;
	const float length = glm::length(vector);
	if (length <= 1e-6f) return;
	const glm::vec3 direction = vector / length;
	glm::vec3 side = glm::cross(direction, raster.forward());
	if (glm::length(side) <= 1e-5f) {
		const auto axis = std::abs(direction.x) < 0.8f ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
		side = glm::cross(direction, axis);
	}
	side = glm::normalize(side) * (length * 0.12f);
	const glm::vec3 head_base = tip - direction * (length * 0.23f);
	raster.draw(origin, tip, color, intensity, effect.radius, effect.xray);
	raster.draw(tip, head_base + side, color, intensity, effect.radius, effect.xray);
	raster.draw(tip, head_base - side, color, intensity, effect.radius, effect.xray);
}

glm::vec3 transform_direction(const glm::mat4 &frame, const glm::vec3 &direction)
{
	glm::vec3 world(0.0f);
	for (int axis = 0; axis < 3; ++axis) {
		const glm::vec3 basis(frame[axis]);
		const float length = glm::length(basis);
		if (length <= 1e-6f || !std::isfinite(length)) {
			throw std::runtime_error("Overlay source anchor has a degenerate basis.");
		}
		world += (direction[axis] / length) * basis;
	}
	return world;
}

struct ResolvedVector {
	glm::vec3 origin;
	glm::vec3 input;
	glm::vec3 world_input;
	glm::vec3 world_vector;
};

ResolvedVector resolve_vector(const FlowEffect &effect, const SceneGenerationContext &scene, double time, float signal)
{
	const glm::vec3 input = interpolate(effect.vectors, time, glm::vec3(0.0f));
	const auto world_input = effect.anchor_space
		? transform_direction(resolve_scene_attachment_frame(scene, *effect.source->attachment), input)
		: input;
	const float multiplier = signal * effect.mass.value_or(1.0f) * (effect.body_reference ? -1.0f : 1.0f);
	const glm::vec3 origin = effect.source->resolve(scene);
	const glm::vec3 world_vector = multiplier * world_input;
	for (int axis = 0; axis < 3; ++axis) {
		if (!std::isfinite(origin[axis]) || !std::isfinite(world_vector[axis] * effect.vector_scale)) {
			throw std::runtime_error("Overlay vector contains a non-finite world coordinate.");
		}
	}
	return {origin, input, world_input, world_vector};
}

void render_vector(OverlayRaster &raster, const FlowEffect &effect, const ResolvedVector &resolved, Json::Value &info)
{
	const auto tip = resolved.origin + effect.vector_scale * resolved.world_vector;
	info["origin_world"] = to_json(resolved.origin);
	info["tip_world"] = to_json(tip);
	info["input_vector"] = to_json(resolved.input);
	info["world_vector"] = to_json(resolved.world_vector);
	info["magnitude"] = glm::length(resolved.world_vector);
	info["reference_frame"] = effect.body_reference ? "body" : "world";
	info["vector_space"] = effect.anchor_space ? "anchor" : "world";
	info["vector_scale"] = effect.vector_scale;
	if (effect.mass) info["mass_kg"] = *effect.mass;
	if (effect.kind == EffectKind::inertial && effect.units != "relative") {
		info["acceleration_m_s2"] = to_json(resolved.world_input);
	}
	draw_arrow(raster, effect, resolved.origin, tip, glm::vec3(0.54f, 0.12f, 1.0f), 1.15f * effect.gain);
}

void render_tension(OverlayRaster &raster, const FlowEffect &effect, const ResolvedRoute &route, float signal,
                    Json::Value &info)
{
	const auto source = route.points.front();
	const auto target = route.points.back();
	const auto source_direction = glm::normalize(source - route.points[1]);
	const auto target_direction = glm::normalize(target - route.points[route.points.size() - 2]);
	const auto source_vector = signal * source_direction;
	const auto target_vector = signal * target_direction;
	const auto source_tip = source + effect.vector_scale * source_vector;
	const auto target_tip = target + effect.vector_scale * target_vector;
	info["magnitude"] = signal;
	info["slack"] = signal == 0.0f;
	info["source_pull_world"] = to_json(source_vector);
	info["target_pull_world"] = to_json(target_vector);
	info["source_tip_world"] = to_json(source_tip);
	info["target_tip_world"] = to_json(target_tip);
	info["vector_scale"] = effect.vector_scale;
	info["arrows_act_on"] = "member";
	if (signal > 0.0f) {
		const glm::vec3 color(1.0f, 0.06f, 0.52f);
		for (std::size_t index = 1; index < route.points.size(); ++index) {
			raster.draw(route.points[index - 1], route.points[index], color, effect.gain * 0.15f,
			            effect.radius * 0.65f, effect.xray);
		}
		draw_arrow(raster, effect, source, source_tip, color, effect.gain * 1.25f);
		draw_arrow(raster, effect, target, target_tip, color, effect.gain * 1.25f);
	}
}

ResolvedRoute resolve_route(const FlowEffect &effect, const SceneGenerationContext &scene, float signal)
{
	if (effect.kind == EffectKind::tension && signal > 0.0f) {
		// A taut member pulls along the span. Authored guide points describe its slack shape.
		auto knots = resolve_knots(effect, scene);
		knots = {knots.front(), knots.back()};
		return curve_route(knots, true);
	}
	return curve_route(resolve_knots(effect, scene), false);
}

} // namespace


struct FlowOverlay::Program {
	std::vector<FlowEffect> effects;
	std::optional<double> phase_offset_seconds;
};

FlowOverlay FlowOverlay::parse(const Json::Value &manifest)
{
	manifest_input::check_fields(manifest, {"schema_version", "effects", "phase_offset_seconds"}, "overlays");
	if (!manifest_input::is_unsigned_integer(manifest["schema_version"]) || manifest["schema_version"].asUInt64() != 1) {
		throw std::runtime_error("overlays.schema_version must be 1.");
	}
	const auto &effects = manifest["effects"];
	if (!effects.isArray() || effects.size() > 128) {
		throw std::runtime_error("overlays.effects must be an array of up to 128 effects.");
	}
	auto program = std::make_shared<Program>();
	if (manifest.isMember("phase_offset_seconds")) {
		const double offset = manifest_input::read_number(manifest["phase_offset_seconds"],
		                                                  "overlays.phase_offset_seconds");
		if (offset < 0.0) throw std::runtime_error("overlays.phase_offset_seconds must be nonnegative.");
		program->phase_offset_seconds = offset;
	}
	std::set<std::string> ids;
	for (const auto &effect : effects) {
		auto parsed = read_effect(effect);
		if (!ids.insert(parsed.id).second) throw std::runtime_error("Duplicate overlay ID: " + parsed.id + ".");
		program->effects.push_back(std::move(parsed));
	}
	FlowOverlay overlay;
	overlay.program_ = std::move(program);
	return overlay;
}

bool FlowOverlay::active(double seconds) const
{
	return program_ && std::any_of(program_->effects.begin(), program_->effects.end(),
	                              [seconds](const auto &effect) { return effect.scheduled(seconds); });
}

void FlowOverlay::validate(const SceneGenerationContext &scene, double seconds) const
{
	if (!program_) return;
	for (const auto &effect : program_->effects) {
		if (!effect.scheduled(seconds) || !gate_open(effect, seconds)) continue;
		const float signal = interpolate(effect.signal, seconds, 1.0f);
		if (is_vector(effect.kind)) {
			resolve_vector(effect, scene, seconds, signal);
		} else if (resolve_route(effect, scene, signal).points.size() < 2) {
			throw std::runtime_error("Overlay '" + effect.id + "' needs a route with nonzero length.");
		}
	}
}

FlowOverlayFrame FlowOverlay::render(const SceneGenerationContext &scene, const PreviewCameraState &camera,
                                     double seconds, const std::vector<float> &scene_depth) const
{
	FlowOverlayFrame frame;
	frame.width = camera.width;
	frame.height = camera.height;
	if (!active(seconds)) return frame;
	if (frame.width <= 0 || frame.height <= 0 || !std::isfinite(camera.scale_global) || camera.scale_global <= 0 ||
	    scene_depth.size() != static_cast<std::size_t>(frame.width) * frame.height) {
		throw std::runtime_error("Overlay rendering requires a matching linear scene depth plate.");
	}
	frame.emission.assign(scene_depth.size(), glm::vec3(0.0f));
	frame.depth.assign(scene_depth.size(), 0.0f);
	OverlayRaster raster(camera, scene_depth, frame);
	const auto offset = program_->phase_offset_seconds;
	const double master_time = seconds + offset.value_or(0.0);
	for (const auto &effect : program_->effects) {
		if (!effect.scheduled(seconds)) continue;
		Json::Value info(Json::objectValue);
		const bool enabled = gate_open(effect, seconds);
		info["id"] = effect.id;
		info["kind"] = effect.name;
		info["units"] = effect.units;
		info["provenance"] = effect.provenance;
		info["gate_open"] = enabled;
		info["xray"] = effect.xray;
		if (offset) info["master_time_seconds"] = master_time;
		if (!effect.connection.empty()) info["connection"] = effect.connection;
		if (enabled) {
			const float signal = interpolate(effect.signal, seconds, 1.0f);
			info["signal"] = signal;
			if (is_vector(effect.kind)) {
				render_vector(raster, effect, resolve_vector(effect, scene, seconds, signal), info);
			} else {
				const auto route = resolve_route(effect, scene, signal);
				if (route.points.size() < 2) {
					throw std::runtime_error("Overlay '" + effect.id + "' needs a route with nonzero length.");
				}
				info["source_world"] = to_json(route.points.front());
				info["target_world"] = to_json(route.points.back());
				info["route_length"] = route.length();
				if (effect.kind == EffectKind::tension) {
					render_tension(raster, effect, route, signal, info);
				} else if (signal > 0.0f) {
					const double phase_time = offset ? master_time : seconds - effect.start;
					const double cycle = std::fmod(phase_time, effect.period) / effect.period;
					info["phase"] = cycle;
					draw_flow(raster, effect, route, static_cast<float>(cycle), signal);
				}
			}
		}
		info["visible_pixels"] = raster.flush();
		frame.effects.append(std::move(info));
	}
	return frame;
}

PreviewImage FlowOverlayFrame::composite(const PreviewImage &beauty) const
{
	if (emission.empty()) return beauty;
	if (beauty.width() != width || beauty.height() != height ||
	    emission.size() != static_cast<std::size_t>(width) * height) {
		throw std::runtime_error("Overlay emission plate does not match the beauty image.");
	}
	auto pixels = beauty.rgbPixels();
	for (std::size_t index = 0; index < emission.size(); ++index) {
		if (emission[index] == glm::vec3(0.0f)) continue;
		const auto original = preview_color::decode_pixel(pixels, index);
		const auto glow = glm::vec3(1.0f) - glm::exp(-emission[index]);
		preview_color::encode_pixel(original + (glm::vec3(1.0f) - original) * glow, index, pixels);
	}
	return PreviewImage(width, height, std::move(pixels));
}
