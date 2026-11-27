#include "geometry/service/DocumentMeshLibrary.h"

#include "geometry/service/GeneratedMeshProviderRegistry.h"

#include <algorithm>
#include <cmath>
#include <locale>
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
	for (source_line = 0; source_line < static_cast<int>(lines.size()); ++source_line) {
		const std::string &line = lines[source_line];
		const auto first = line.find_first_not_of(" \t\r");
		if (first == std::string::npos) continue;
		if (!mesh && line.compare(first, 9, "#@mesh-v1") != 0) continue;
		row.clear();
		row.str(line);
		row >> tag;
		if (!mesh) {
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
