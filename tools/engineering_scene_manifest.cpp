#include "AppPaths.h"
#include "Context.h"
#include "editor/service/GrammarCompilationService.h"
#include "geometry/model/ResolvedPrimitiveGeometry.h"
#include "geometry/service/AffineCubeFaceQuery.h"
#include "spatial/model/SpatialBuildingModel.h"
#include "spatial/model/SpatialBuildingObject.h"

#include <jsoncpp/json/json.h>
#include <openssl/sha.h>

#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <set>
#include <random>
#include <stdexcept>
#include <unordered_map>

extern std::mt19937_64 grammar_rng;

namespace {
Json::Value coordinates(const glm::vec3 &vector)
{
	Json::Value components(Json::arrayValue);
	for (int axis = 0; axis < 3; ++axis) {
		components.append(vector[axis]);
	}
	return components;
}

std::string sceneRevision(const std::string &source, const Json::Value &objects)
{
	Json::StreamWriterBuilder writer;
	writer["indentation"] = "";
	const std::string snapshot = source + "\n" + Json::writeString(writer, objects);
	unsigned char digest[SHA256_DIGEST_LENGTH];
	SHA256(reinterpret_cast<const unsigned char *>(snapshot.data()), snapshot.size(), digest);
	std::ostringstream revision;
	for (const auto byte : digest) {
		revision << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(byte);
	}
	return revision.str();
}
}

int main(int argument_count, char **arguments)
{
	if (argument_count < 3 || argument_count > 7 || (argument_count - 3) % 2 != 0) {
		std::cerr << "usage: progen3d-scene-manifest <scene.p3d> <manifest.json> [--time seconds] [--step seconds]\n";
		return 2;
	}
	try {
		progen3d_initialize_paths(arguments[0]);
		std::ifstream source(arguments[1]);
		if (!source) {
			std::cerr << "cannot read native grammar\n";
			return 2;
		}
		SceneGenerationRequest request;
		request.source_text.assign(std::istreambuf_iterator<char>(source), {});
		request.time_sample = true;
		float step_seconds = 0.0f;
		bool has_time = false;
		for (int index = 3; index < argument_count; index += 2) {
			const std::string flag(arguments[index]);
			std::size_t consumed = 0;
			const double seconds = std::stod(arguments[index + 1], &consumed);
			if (!std::isfinite(seconds) || arguments[index + 1][consumed] != '\0') {
				throw std::runtime_error("time and step must be finite durations");
			}
			if (flag == "--time" && !has_time) {
				request.evaluation_time = seconds;
				has_time = true;
			} else if (flag == "--step" && step_seconds == 0.0f && seconds > 0.0) {
				step_seconds = static_cast<float>(seconds);
				if (!std::isfinite(step_seconds) || step_seconds <= 0.0f) {
					throw std::runtime_error("preview step is out of range");
				}
			} else {
				throw std::runtime_error("unknown, duplicate, or invalid option " + flag);
			}
		}
		GrammarCompilationService compiler(
			[](const std::string &, std::uint64_t) { grammar_rng.seed(0); }, {});

		auto generated = compiler.compile(request);
		if (!generated->succeeded()) {
			std::cerr << generated->error << '\n';
			for (const auto &diagnostic : generated->diagnostics) {
				std::cerr << diagnostic.message << '\n';
			}
			return 1;
		}
		auto &context = *generated->scene_snapshot->generationContext();
		if (!context.executionErrors().empty()) {
			return 1;
		}
		Json::Value objects(Json::objectValue);
		std::size_t index = 0;
		for (const auto &primitive : context.primitive_instances) {
			Json::Value item(Json::objectValue);
			item["type"] = primitive.type;
			item["source_line"] = primitive.source_start_line;
			item["mass"] = primitive.mass;
			item["initial_velocity"] = coordinates(primitive.initial_velocity);
			item["initial_rotational_velocity"] = coordinates(primitive.initial_rotational_velocity);
			item["initial_position"] = coordinates(primitive.initial_position);
			item["initial_rotation_degrees"] = coordinates(primitive.rotation_degrees);
			item["extent_scale"] = coordinates(primitive.size);
			Json::Value transform(Json::arrayValue);
			for (int column = 0; column < 4; ++column) {
				for (int row = 0; row < 4; ++row) {
					transform.append(primitive.primary_transform[column][row]);
				}
			}
			item["transform_column_major"] = std::move(transform);
			item["faces"] = Json::Value(Json::arrayValue);
			if (const auto &geometry = primitive.resolved_geometry) {
				item["topology_hash"] = std::to_string(geometry->topologyHash());
				std::set<std::string> face_ids;
				for (const auto &tag : geometry->faceSurfaceTags()) {
					face_ids.insert(std::to_string(static_cast<int>(tag.role())) + ":" +
					                std::to_string(tag.objectPartIndex()) + ":" +
					                std::to_string(tag.boundaryIndex()));
				}
				for (const auto &face : face_ids) {
					item["faces"].append(face);
				}
			}
			objects["primitive:" + std::to_string(index++)] = item;
		}
		if (const auto &model = context.spatialBuildingModel()) {
			std::unordered_map<std::string, std::vector<std::size_t>> indices_by_object;
			for (const auto &binding : model->geometryBindings()) {
				indices_by_object[binding.objectId().value()].push_back(binding.primitiveInstanceIndex());
			}
			for (const auto &object : model->objects().objects()) {
				const auto &identifier = object.identity().objectId().value();
				if (objects.isMember(identifier)) {
					throw std::runtime_error("duplicate native scene object ID " + identifier);
				}
				Json::Value entry(Json::objectValue);
				const auto &indices = indices_by_object[identifier];
				if (indices.size() == 1) {
					const std::size_t primitive_index = indices.front();
					const auto &primitive = context.primitive_instances.at(primitive_index);
					entry = objects["primitive:" + std::to_string(primitive_index)];
					auto &span = entry["source_span"];
					span["start_line"] = primitive.source_start_line;
					span["start_column"] = primitive.source_start_column;
					span["end_line"] = primitive.source_end_line;
					span["end_column"] = primitive.source_end_column;
					// Mesh tag groups are snapshot-local. A named part only exposes
					// face identifiers with a stable geometric interpretation.
					entry["faces"] = Json::Value(Json::arrayValue);
					if (hasAffineCubeFaces(primitive)) {
						for (const CubeFace face : cube_faces) {
							entry["faces"].append(cubeFaceName(face));
						}
					}
				}
				entry["geometry_primitive_count"] = Json::UInt64(indices.size());
				entry["name"] = object.identity().objectName();
				entry["interfaces"] = Json::Value(Json::arrayValue);
				for (const auto &interface : object.interfaces()) {
					entry["interfaces"].append(interface.interfaceId().value());
				}
				objects[identifier] = entry;
			}
		}
		Json::Value manifest(Json::objectValue);
		manifest["revision"] = sceneRevision(request.source_text, objects);
		manifest["objects"] = objects;
		manifest["evaluation_time_s"] = request.evaluation_time;
		if (step_seconds > 0.0f) {
			context.stepSimulation(step_seconds);
			Json::Value poses(Json::arrayValue);
			for (const auto &primitive : context.primitive_instances) {
				Json::Value pose(Json::objectValue);
				pose["position"] = coordinates(primitive.position);
				pose["velocity"] = coordinates(primitive.velocity);
				poses.append(pose);
			}
			manifest["preview_step"] = poses;
		}
		std::ofstream output(arguments[2]);
		output << manifest << '\n';
		return output ? 0 : 3;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 2;
	}
}
