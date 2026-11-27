#include "geometry/service/DocumentMeshLibrary.h"

#include "Context.h"
#include "geometry/model/GeneratedMeshReferenceShapeSpecification.h"
#include "geometry/service/GeneratedMeshProviderRegistry.h"
#include "geometry/service/MeshTopologyAnalyzer.h"

#include <algorithm>
#include <cmath>
#include <locale>
#include <set>
#include <sstream>
#include <stdexcept>

namespace {

constexpr std::size_t max_vertices = 65535;
constexpr std::size_t max_triangles = 150000;

bool isValidMeshKey(const std::string &key)
{
	return !key.empty() && key.size() <= 96 &&
	       !(key.front() >= '0' && key.front() <= '9') &&
	       key.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_") ==
	           std::string::npos;
}

bool isCatalogueMesh(const std::string &key)
{
	const auto &providers =
		GeneratedMeshProviderRegistry::accessProcessRegistry().listRegisteredProviders();
	return std::any_of(providers.begin(), providers.end(),
	                   [&](const auto &provider) { return provider->recognizes(key); });
}

double metresPerUnit(const std::string &unit)
{
	if (unit == "mm") return 0.001;
	if (unit == "cm") return 0.01;
	if (unit == "m") return 1.0;
	if (unit == "in") return 0.0254;
	throw std::invalid_argument("Mesh units must be mm, cm, m or in.");
}

void readVertex(std::istream &row, double scale, const std::string &up, Mesh &mesh)
{
	glm::dvec3 source;
	if (!(row >> source.x >> source.y >> source.z) || mesh.vertices.size() >= max_vertices) {
		throw std::invalid_argument("Invalid project mesh vertex or vertex count.");
	}
	source *= scale;
	for (int axis = 0; axis < 3; ++axis) {
		if (!std::isfinite(source[axis]) || std::abs(source[axis]) > 1.0e9) {
			throw std::invalid_argument("Mesh coordinates exceed the supported range.");
		}
	}
	if (up == "Z") mesh.vertices.emplace_back(source.x, source.z, -source.y);
	else if (up == "X") mesh.vertices.emplace_back(-source.y, source.x, source.z);
	else mesh.vertices.emplace_back(source);
}

void readTriangle(std::istream &row, Mesh &mesh)
{
	glm::ivec3 face;
	if (!(row >> face.x >> face.y >> face.z) || mesh.faces.size() >= max_triangles ||
	    std::min({face.x, face.y, face.z}) < 0 ||
	    static_cast<std::size_t>(std::max({face.x, face.y, face.z})) >= mesh.vertices.size()) {
		throw std::invalid_argument("Invalid project mesh triangle or index.");
	}
	const glm::dvec3 a(mesh.vertices[face.x]);
	const glm::dvec3 b(mesh.vertices[face.y]);
	const glm::dvec3 c(mesh.vertices[face.z]);
	const glm::dvec3 normal = glm::cross(b - a, c - a);
	if (glm::dot(normal, normal) == 0.0) {
		throw std::invalid_argument("Project mesh contains a degenerate triangle.");
	}
	mesh.faces.push_back(face);
	mesh.face_normals.emplace_back(glm::normalize(normal));
}

} // namespace

std::shared_ptr<const DocumentMeshLibrary> DocumentMeshLibrary::read(
	const std::vector<std::string> &lines, int &source_line)
{
	auto library = std::make_shared<DocumentMeshLibrary>();
	std::shared_ptr<Mesh> mesh;
	std::string key, up, tag;
	double scale = 1.0;
	int declaration_line = 0;
	std::istringstream row;
	row.imbue(std::locale::classic());
	std::set<std::pair<std::string, std::size_t>> seam_vertices;
	for (source_line = 0; source_line < static_cast<int>(lines.size()); ++source_line) {
		const std::string &line = lines[source_line];
		const auto first = line.find_first_not_of(" \t\r");
		if (first == std::string::npos) continue;
		if (!mesh && line.compare(first, 7, "#@mesh-") != 0) continue;
		row.clear();
		row.str(line);
		row >> tag;
		if (!mesh) {
			if (tag == "#@mesh-seam-v1") {
				std::vector<SeamVertex> seam;
				std::string mesh_key;
				bool different_meshes = false;
				while (row >> mesh_key) {
					std::size_t index;
					const auto found = library->meshes_.find(mesh_key);
					if (!(row >> index) || found == library->meshes_.end() ||
					    index >= found->second->vertices.size()) {
						throw std::invalid_argument("Invalid vertex in project mesh seam.");
					}
					if (!seam_vertices.emplace(mesh_key, index).second) {
						throw std::invalid_argument("A project mesh vertex may belong to only one seam point.");
					}
					different_meshes |= !seam.empty() && mesh_key != seam.front().mesh_key;
					seam.push_back({mesh_key, index});
				}
				if (!different_meshes) {
					throw std::invalid_argument("A project mesh seam point must join at least two meshes.");
				}
				library->seams_.push_back(std::move(seam));
				continue;
			}
			if (tag != "#@mesh-v1") continue; // Other grammar comments remain opaque.
			std::string unit;
			if (!(row >> key >> unit >> up) || !isValidMeshKey(key)) {
				throw std::invalid_argument("Invalid project mesh declaration.");
			}
			if (library->recognizes(key) || isCatalogueMesh(key)) {
				throw std::invalid_argument("Mesh key already registered: " + key);
			}
			if (up != "X" && up != "Y" && up != "Z") {
				throw std::invalid_argument("Mesh up axis must be X, Y or Z.");
			}
			scale = metresPerUnit(unit);
			mesh = std::make_shared<Mesh>();
			declaration_line = source_line;
		} else if (tag == "#v") {
			readVertex(row, scale, up, *mesh);
		} else if (tag == "#f") {
			readTriangle(row, *mesh);
		} else if (tag == "#@endmesh") {
			if (mesh->faces.empty()) throw std::invalid_argument("Project mesh has no triangles.");
			library->meshes_.emplace(key, std::move(mesh));
		} else {
			throw std::invalid_argument("Expected #v, #f or #@endmesh in project mesh.");
		}
		std::string extra;
		if (row >> extra) throw std::invalid_argument("Unexpected field in project mesh.");
	}
	if (mesh) {
		source_line = declaration_line;
		throw std::invalid_argument("Missing #@endmesh for " + key);
	}
	return library;
}

bool DocumentMeshLibrary::recognizes(const std::string &key) const
{
	return meshes_.find(key) != meshes_.end();
}

GeneratedMeshResolutionResult DocumentMeshLibrary::resolve(
	const std::string &key, GeometryDetailLevel) const
{
	const auto found = meshes_.find(key);
	if (found == meshes_.end()) {
		return GeneratedMeshResolutionResult::createFailure("Unknown project mesh: " + key);
	}
	const auto &mesh = found->second;
	return GeneratedMeshResolutionResult::createSuccess(GeneratedPrimitiveMesh(
		mesh, std::vector<MeshSurfaceTag>(mesh->faces.size(), MeshSurfaceTag(MeshSurfaceRole::Outer))));
}

bool DocumentMeshLibrary::alignSeams(std::vector<ScenePrimitiveInstance> &instances) const
{
	if (seams_.empty()) return false;

	struct SeamMeshPose {
		ScenePrimitiveInstance *instance = nullptr;
		glm::dmat4 local_from_world{1.0};
		std::shared_ptr<Mesh> deformed_mesh;
	};
	std::unordered_map<std::string, SeamMeshPose> poses;
	for (const auto &seam : seams_) {
		for (const SeamVertex &vertex : seam) poses.try_emplace(vertex.mesh_key);
	}

	std::set<std::string> seen_meshes;
	for (auto &instance : instances) {
		const auto *reference = dynamic_cast<const GeneratedMeshReferenceShapeSpecification *>(
			instance.shape_specification.get());
		if (!reference) continue;
		const auto found = poses.find(reference->meshKey());
		if (found == poses.end()) continue;
		auto &pose = found->second;
		// Seam declarations address individual surface instances by mesh key.
		if (!seen_meshes.insert(reference->meshKey()).second) {
			pose.instance = nullptr;
			continue;
		}
		if (reference->requestsClosedGeometry() || !instance.resolved_geometry ||
		    !instance.resolved_geometry->hasTriangleMesh()) continue;
		const glm::dmat4 world_from_local(instance.primary_transform);
		const double determinant = glm::determinant(world_from_local);
		if (determinant == 0.0 || !std::isfinite(determinant)) continue;
		pose.instance = &instance;
		pose.local_from_world = glm::inverse(world_from_local);
	}

	for (const auto &seam : seams_) {
		if (std::any_of(seam.begin(), seam.end(), [&](const SeamVertex &vertex) {
			return poses.at(vertex.mesh_key).instance == nullptr;
		})) continue;

		glm::dvec3 world_point(0.0);
		for (const SeamVertex &vertex : seam) {
			const auto &instance = *poses.at(vertex.mesh_key).instance;
			const auto &mesh = instance.resolved_geometry->triangleMesh();
			world_point += glm::dvec3(glm::dmat4(instance.primary_transform) *
			                          glm::dvec4(mesh.vertices[vertex.index], 1.0));
		}
		world_point /= static_cast<double>(seam.size());
		for (const SeamVertex &vertex : seam) {
			auto &pose = poses.at(vertex.mesh_key);
			if (!pose.deformed_mesh) {
				pose.deformed_mesh = std::make_shared<Mesh>(pose.instance->resolved_geometry->triangleMesh());
			}
			pose.deformed_mesh->vertices[vertex.index] = glm::vec3(
				pose.local_from_world * glm::dvec4(world_point, 1.0));
		}
	}

	bool deformed = false;
	for (auto &entry : poses) {
		auto &pose = entry.second;
		if (!pose.deformed_mesh) continue;
		auto &mesh = *pose.deformed_mesh;
		mesh.clearCollisionAcceleration();
		mesh.calc_normals();
		for (auto &normal : mesh.face_normals) {
			const float length = glm::length(normal);
			if (length > 0.0f) normal /= length;
		}
		auto &instance = *pose.instance;
		const auto &original = *instance.resolved_geometry;
		const auto &surface_tags = original.faceSurfaceTags();
		const auto topology = MeshTopologyAnalyzer().analyze(mesh, surface_tags);
		instance.resolved_geometry = ResolvedPrimitiveGeometry::createTriangleMesh(
			std::move(pose.deformed_mesh), surface_tags, topology.isWatertight(),
			original.volumeEvidence(), original.collisionPolicy(), topology.topology_hash);
		instance.collision_geometry_dirty = true;
		deformed = true;
	}
	return deformed;
}
