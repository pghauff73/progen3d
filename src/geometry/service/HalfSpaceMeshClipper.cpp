#include "geometry/service/HalfSpaceMeshClipper.h"

#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <memory>
#include <limits>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

namespace {

constexpr float kClipTolerance = 1.0e-6f;
constexpr double kLoopQuantization = 100000.0;

struct ClipVertex
{
	glm::vec3 position{0.0f};
	glm::vec3 normal{0.0f, 1.0f, 0.0f};
	glm::vec3 texcoord{0.0f};
};

struct QuantizedPosition
{
	std::int64_t x = 0;
	std::int64_t y = 0;
	std::int64_t z = 0;

	bool operator<(const QuantizedPosition &other) const
	{
		return std::tie(x, y, z) < std::tie(other.x, other.y, other.z);
	}

	bool operator==(const QuantizedPosition &other) const
	{
		return x == other.x && y == other.y && z == other.z;
	}
};

struct QuantizedEdge
{
	QuantizedPosition first;
	QuantizedPosition second;

	bool operator<(const QuantizedEdge &other) const
	{
		return std::tie(first, second) < std::tie(other.first, other.second);
	}
};

struct PlaneProjection
{
	glm::vec3 horizontal{1.0f, 0.0f, 0.0f};
	glm::vec3 vertical{0.0f, 1.0f, 0.0f};
	glm::vec3 normal{0.0f, 0.0f, 1.0f};
};

QuantizedPosition quantize(const glm::vec3 &position)
{
	return {
		static_cast<std::int64_t>(std::llround(position.x * kLoopQuantization)),
		static_cast<std::int64_t>(std::llround(position.y * kLoopQuantization)),
		static_cast<std::int64_t>(std::llround(position.z * kLoopQuantization))};
}

QuantizedEdge make_edge(QuantizedPosition first, QuantizedPosition second)
{
	if (second < first) std::swap(first, second);
	return {first, second};
}

float signed_distance(const glm::vec3 &position,
	                  const PlaneClipSpecification &clip,
	                  float outer_radius)
{
	const float distance = glm::dot(clip.normal(), position) -
	                       clip.normalizedOffset() * outer_radius;
	return clip.retainedSide() == ClipRetainedSide::Positive
		? distance : -distance;
}

ClipVertex interpolate(const ClipVertex &first,
	                   const ClipVertex &second,
	                   float first_distance,
	                   float second_distance)
{
	const float denominator = first_distance - second_distance;
	const float fraction = std::fabs(denominator) <= kClipTolerance
		? 0.5f : first_distance / denominator;
	ClipVertex result;
	result.position = first.position + (second.position - first.position) * fraction;
	const glm::vec3 interpolated_normal =
		first.normal + (second.normal - first.normal) * fraction;
	result.normal = glm::dot(interpolated_normal, interpolated_normal) > 1.0e-12f
		? glm::normalize(interpolated_normal) : first.normal;
	result.texcoord = first.texcoord + (second.texcoord - first.texcoord) * fraction;
	return result;
}

int append_vertex(Mesh *mesh, const ClipVertex &vertex)
{
	mesh->vertices.push_back(vertex.position);
	mesh->normals.push_back(vertex.normal);
	mesh->texcoords.push_back(vertex.texcoord);
	return static_cast<int>(mesh->vertices.size() - 1);
}

void append_triangle(Mesh *mesh,
	                 std::vector<MeshSurfaceTag> *tags,
	                 const ClipVertex &first,
	                 const ClipVertex &second,
	                 const ClipVertex &third,
	                 const MeshSurfaceTag &tag)
{
	const glm::vec3 cross = glm::cross(second.position - first.position,
	                                  third.position - first.position);
	if (glm::dot(cross, cross) <= 1.0e-16f) return;
	const int first_index = append_vertex(mesh, first);
	const int second_index = append_vertex(mesh, second);
	const int third_index = append_vertex(mesh, third);
	mesh->faces.emplace_back(first_index, second_index, third_index);
	tags->push_back(tag);
}

PlaneProjection projection_for(const glm::vec3 &normal)
{
	PlaneProjection projection;
	projection.normal = normal;
	const glm::vec3 reference = std::fabs(normal.y) < 0.9f
		? glm::vec3(0.0f, 1.0f, 0.0f)
		: glm::vec3(1.0f, 0.0f, 0.0f);
	projection.horizontal = glm::normalize(glm::cross(reference, normal));
	projection.vertical = glm::normalize(glm::cross(normal, projection.horizontal));
	return projection;
}

glm::vec2 project(const glm::vec3 &position, const PlaneProjection &projection)
{
	return glm::vec2(glm::dot(position, projection.horizontal),
	                 glm::dot(position, projection.vertical));
}

glm::vec3 planar_texcoord(
	const glm::vec3 &position,
	const PlaneProjection &projection)
{
	const glm::vec2 projected = project(position, projection);
	return glm::vec3(projected, 0.0f);
}

float signed_area(const std::vector<glm::vec3> &loop,
	              const PlaneProjection &projection)
{
	float area = 0.0f;
	for (std::size_t index = 0; index < loop.size(); ++index) {
		const glm::vec2 current = project(loop[index], projection);
		const glm::vec2 next = project(loop[(index + 1) % loop.size()], projection);
		area += current.x * next.y - next.x * current.y;
	}
	return area * 0.5f;
}

bool point_in_polygon(const glm::vec2 &point,
	                  const std::vector<glm::vec3> &loop,
	                  const PlaneProjection &projection)
{
	bool inside = false;
	for (std::size_t current = 0, previous = loop.size() - 1;
	     current < loop.size(); previous = current++) {
		const glm::vec2 a = project(loop[current], projection);
		const glm::vec2 b = project(loop[previous], projection);
		const bool intersects = ((a.y > point.y) != (b.y > point.y)) &&
			(point.x < (b.x - a.x) * (point.y - a.y) /
			           ((b.y - a.y) == 0.0f ? 1.0e-12f : (b.y - a.y)) + a.x);
		if (intersects) inside = !inside;
	}
	return inside;
}

bool point_in_triangle(const glm::vec2 &point,
	                   const glm::vec2 &a,
	                   const glm::vec2 &b,
	                   const glm::vec2 &c)
{
	const auto cross2 = [](const glm::vec2 &first, const glm::vec2 &second) {
		return first.x * second.y - first.y * second.x;
	};
	const float first = cross2(b - a, point - a);
	const float second = cross2(c - b, point - b);
	const float third = cross2(a - c, point - c);
	return first >= -kClipTolerance && second >= -kClipTolerance &&
	       third >= -kClipTolerance;
}

bool triangulate_simple_loop(
	std::vector<glm::vec3> loop,
	const PlaneProjection &projection,
	const glm::vec3 &normal,
	std::size_t clip_index,
	Mesh *mesh,
	std::vector<MeshSurfaceTag> *tags)
{
	if (loop.size() < 3) return false;
	if (signed_area(loop, projection) < 0.0f) {
		std::reverse(loop.begin(), loop.end());
	}
	bool convex = true;
	for (std::size_t index = 0; index < loop.size(); ++index) {
		const glm::vec2 previous = project(
			loop[(index + loop.size() - 1) % loop.size()], projection);
		const glm::vec2 current = project(loop[index], projection);
		const glm::vec2 next = project(loop[(index + 1) % loop.size()], projection);
		const float cross = (current.x - previous.x) * (next.y - current.y) -
		                    (current.y - previous.y) * (next.x - current.x);
		if (cross < -kClipTolerance) {
			convex = false;
			break;
		}
	}
	if (convex) {
		glm::vec3 center(0.0f);
		for (const glm::vec3 &position : loop) center += position;
		center /= static_cast<float>(loop.size());
		const ClipVertex center_vertex{
			center, normal, planar_texcoord(center, projection)};
		for (std::size_t index = 0; index < loop.size(); ++index) {
			const std::size_t next = (index + 1) % loop.size();
			append_triangle(
				mesh,
				tags,
				center_vertex,
				ClipVertex{loop[index], normal,
				           planar_texcoord(loop[index], projection)},
				ClipVertex{loop[next], normal,
				           planar_texcoord(loop[next], projection)},
				MeshSurfaceTag(MeshSurfaceRole::Clip, clip_index));
		}
		return true;
	}
	std::vector<std::size_t> polygon(loop.size());
	for (std::size_t index = 0; index < polygon.size(); ++index) polygon[index] = index;

	std::size_t guard = 0;
	while (polygon.size() > 3 && guard++ < loop.size() * loop.size()) {
		bool removed_ear = false;
		for (std::size_t polygon_index = 0;
		     polygon_index < polygon.size();
		     ++polygon_index) {
			const std::size_t previous = polygon[(polygon_index + polygon.size() - 1) % polygon.size()];
			const std::size_t current = polygon[polygon_index];
			const std::size_t next = polygon[(polygon_index + 1) % polygon.size()];
			const glm::vec2 a = project(loop[previous], projection);
			const glm::vec2 b = project(loop[current], projection);
			const glm::vec2 c = project(loop[next], projection);
			const float cross = (b.x - a.x) * (c.y - b.y) -
			                    (b.y - a.y) * (c.x - b.x);
			if (cross <= kClipTolerance) continue;
			bool contains_vertex = false;
			for (std::size_t candidate : polygon) {
				if (candidate == previous || candidate == current || candidate == next) continue;
				if (point_in_triangle(project(loop[candidate], projection), a, b, c)) {
					contains_vertex = true;
					break;
				}
			}
			if (contains_vertex) continue;
			const ClipVertex first{loop[previous], normal, planar_texcoord(loop[previous], projection)};
			const ClipVertex second{loop[current], normal, planar_texcoord(loop[current], projection)};
			const ClipVertex third{loop[next], normal, planar_texcoord(loop[next], projection)};
			append_triangle(mesh, tags, first, second, third,
			                MeshSurfaceTag(MeshSurfaceRole::Clip, clip_index));
			polygon.erase(polygon.begin() + static_cast<std::ptrdiff_t>(polygon_index));
			removed_ear = true;
			break;
		}
		if (!removed_ear) return false;
	}
	if (polygon.size() == 3) {
		append_triangle(
			mesh,
			tags,
			ClipVertex{loop[polygon[0]], normal, planar_texcoord(loop[polygon[0]], projection)},
			ClipVertex{loop[polygon[1]], normal, planar_texcoord(loop[polygon[1]], projection)},
			ClipVertex{loop[polygon[2]], normal, planar_texcoord(loop[polygon[2]], projection)},
			MeshSurfaceTag(MeshSurfaceRole::Clip, clip_index));
	}
	return true;
}

bool connect_equal_loops(
	std::vector<glm::vec3> outer,
	std::vector<glm::vec3> inner,
	const PlaneProjection &projection,
	const glm::vec3 &normal,
	std::size_t clip_index,
	Mesh *mesh,
	std::vector<MeshSurfaceTag> *tags)
{
	if (outer.size() != inner.size() || outer.size() < 3) return false;
	if (signed_area(outer, projection) < 0.0f) std::reverse(outer.begin(), outer.end());
	if (signed_area(inner, projection) < 0.0f) std::reverse(inner.begin(), inner.end());

	std::size_t best_offset = 0;
	float best_distance = std::numeric_limits<float>::max();
	for (std::size_t offset = 0; offset < inner.size(); ++offset) {
		const float distance = glm::length(outer[0] - inner[offset]);
		if (distance < best_distance) {
			best_distance = distance;
			best_offset = offset;
		}
	}
	std::rotate(inner.begin(), inner.begin() + static_cast<std::ptrdiff_t>(best_offset), inner.end());

	for (std::size_t index = 0; index < outer.size(); ++index) {
		const std::size_t next = (index + 1) % outer.size();
		const ClipVertex outer_current{outer[index], normal, planar_texcoord(outer[index], projection)};
		const ClipVertex outer_next{outer[next], normal, planar_texcoord(outer[next], projection)};
		const ClipVertex inner_next{inner[next], normal, planar_texcoord(inner[next], projection)};
		const ClipVertex inner_current{inner[index], normal, planar_texcoord(inner[index], projection)};
		append_triangle(mesh, tags, outer_current, outer_next, inner_next,
		                MeshSurfaceTag(MeshSurfaceRole::Clip, clip_index));
		append_triangle(mesh, tags, outer_current, inner_next, inner_current,
		                MeshSurfaceTag(MeshSurfaceRole::Clip, clip_index));
	}
	return true;
}

std::vector<std::vector<glm::vec3>> build_loops(
	const std::vector<std::pair<glm::vec3, glm::vec3>> &segments,
	std::string *diagnostic)
{
	std::map<QuantizedPosition, glm::vec3> positions;
	std::map<QuantizedPosition, std::set<QuantizedPosition>> adjacency;
	std::set<QuantizedEdge> unique_edges;
	for (const auto &segment : segments) {
		const QuantizedPosition first = quantize(segment.first);
		const QuantizedPosition second = quantize(segment.second);
		if (first == second) continue;
		const QuantizedEdge edge = make_edge(first, second);
		if (!unique_edges.insert(edge).second) continue;
		positions[first] = segment.first;
		positions[second] = segment.second;
		adjacency[first].insert(second);
		adjacency[second].insert(first);
	}
	for (const auto &entry : adjacency) {
		if (entry.second.size() != 2) {
			if (diagnostic != nullptr) {
				*diagnostic = "Clip boundary could not be reconstructed into deterministic closed loops.";
			}
			return {};
		}
	}

	std::set<QuantizedEdge> visited;
	std::vector<std::vector<glm::vec3>> loops;
	for (const QuantizedEdge &seed_edge : unique_edges) {
		if (visited.count(seed_edge) != 0) continue;
		std::vector<glm::vec3> loop;
		QuantizedPosition start = seed_edge.first;
		QuantizedPosition previous = seed_edge.first;
		QuantizedPosition current = seed_edge.second;
		loop.push_back(positions[start]);
		visited.insert(seed_edge);
		std::size_t guard = 0;
		while (!(current == start) && guard++ <= unique_edges.size() + 1) {
			loop.push_back(positions[current]);
			const auto &neighbors = adjacency[current];
			auto neighbor = neighbors.begin();
			QuantizedPosition next = *neighbor;
			if (next == previous) {
				++neighbor;
				next = *neighbor;
			}
			visited.insert(make_edge(current, next));
			previous = current;
			current = next;
		}
		if (!(current == start) || loop.size() < 3) {
			if (diagnostic != nullptr) {
				*diagnostic = "Clip boundary loop is open or degenerate.";
			}
			return {};
		}
		loops.push_back(std::move(loop));
	}
	return loops;
}

bool close_loops(const std::vector<std::vector<glm::vec3>> &input_loops,
	             const glm::vec3 &outward_normal,
	             std::size_t clip_index,
	             Mesh *mesh,
	             std::vector<MeshSurfaceTag> *tags,
	             std::string *diagnostic)
{
	if (input_loops.empty()) return true;
	const PlaneProjection projection = projection_for(outward_normal);
	std::vector<std::vector<glm::vec3>> loops = input_loops;
	std::sort(loops.begin(), loops.end(), [&](const auto &first, const auto &second) {
		return std::fabs(signed_area(first, projection)) >
		       std::fabs(signed_area(second, projection));
	});

	std::vector<bool> consumed(loops.size(), false);
	for (std::size_t outer_index = 0; outer_index < loops.size(); ++outer_index) {
		if (consumed[outer_index]) continue;
		std::vector<std::size_t> holes;
		const glm::vec2 test_point = project(loops[outer_index][0], projection);
		(void)test_point;
		for (std::size_t candidate = outer_index + 1;
		     candidate < loops.size(); ++candidate) {
			if (!consumed[candidate] &&
			    point_in_polygon(project(loops[candidate][0], projection),
			                     loops[outer_index], projection)) {
				holes.push_back(candidate);
			}
		}
		if (holes.empty()) {
			if (!triangulate_simple_loop(loops[outer_index], projection,
			                             outward_normal, clip_index,
			                             mesh, tags)) {
				if (diagnostic != nullptr) *diagnostic = "Clip closure triangulation failed.";
				return false;
			}
			consumed[outer_index] = true;
			continue;
		}
		if (holes.size() == 1 &&
		    connect_equal_loops(loops[outer_index], loops[holes[0]], projection,
		                        outward_normal, clip_index, mesh, tags)) {
			consumed[outer_index] = true;
			consumed[holes[0]] = true;
			continue;
		}
		if (diagnostic != nullptr) {
			*diagnostic = "Clip closure contains unsupported nested boundary loops.";
		}
		return false;
	}
	return true;
}

}

GeneratedPrimitiveMesh HalfSpaceMeshClipper::clip(
	const GeneratedPrimitiveMesh &source,
	const PlaneClipSpecification &clip_specification,
	float outer_radius,
	std::size_t clip_index,
	bool close_boundary,
	std::string *diagnostic) const
{
	auto output = std::make_shared<Mesh>();
	std::vector<MeshSurfaceTag> output_tags;
	std::vector<std::pair<glm::vec3, glm::vec3>> intersection_segments;
	std::map<QuantizedPosition, glm::vec3> canonical_intersection_positions;
	if (!source.mesh()) return GeneratedPrimitiveMesh(output, {});
	const Mesh &input = *source.mesh();
	const auto canonicalize_intersection = [&](const glm::vec3 &position) {
		const QuantizedPosition key = quantize(position);
		const auto existing = canonical_intersection_positions.find(key);
		if (existing != canonical_intersection_positions.end()) {
			return existing->second;
		}
		canonical_intersection_positions.emplace(key, position);
		return position;
	};

	for (std::size_t face_index = 0; face_index < input.faces.size(); ++face_index) {
		const glm::ivec3 &face = input.faces[face_index];
		std::vector<ClipVertex> polygon;
		const int indices[3] = {face.x, face.y, face.z};
		for (int index : indices) {
			const std::size_t vertex_index = static_cast<std::size_t>(index);
			ClipVertex vertex;
			vertex.position = input.vertices[vertex_index];
			const float raw_plane_distance =
				glm::dot(clip_specification.normal(), vertex.position) -
				clip_specification.normalizedOffset() * outer_radius;
			if (std::fabs(raw_plane_distance) <= kClipTolerance) {
				vertex.position -= raw_plane_distance * clip_specification.normal();
				vertex.position = canonicalize_intersection(vertex.position);
			}
			vertex.normal = vertex_index < input.normals.size()
				? input.normals[vertex_index] : glm::vec3(0.0f, 1.0f, 0.0f);
			vertex.texcoord = vertex_index < input.texcoords.size()
				? input.texcoords[vertex_index] : glm::vec3(0.0f);
			polygon.push_back(vertex);
		}

		std::vector<ClipVertex> clipped;
		std::vector<glm::vec3> intersections;
		for (std::size_t current_index = 0; current_index < polygon.size(); ++current_index) {
			const ClipVertex &current = polygon[current_index];
			const ClipVertex &next = polygon[(current_index + 1) % polygon.size()];
			const float current_distance = signed_distance(
				current.position, clip_specification, outer_radius);
			const float next_distance = signed_distance(
				next.position, clip_specification, outer_radius);
			const bool current_inside = current_distance >= -kClipTolerance;
			const bool next_inside = next_distance >= -kClipTolerance;
			if (current_inside) clipped.push_back(current);
			if (current_inside != next_inside) {
					ClipVertex crossing = interpolate(
						current, next, current_distance, next_distance);
					crossing.position = canonicalize_intersection(crossing.position);
				clipped.push_back(crossing);
				intersections.push_back(crossing.position);
			}
		}

		if (intersections.size() == 2 &&
		    glm::length(intersections[0] - intersections[1]) > kClipTolerance) {
			intersection_segments.emplace_back(intersections[0], intersections[1]);
		}
		if (clipped.size() < 3) continue;
		const MeshSurfaceTag tag = face_index < source.faceSurfaceTags().size()
			? source.faceSurfaceTags()[face_index]
			: MeshSurfaceTag(MeshSurfaceRole::Outer);
		for (std::size_t triangle_index = 1;
		     triangle_index + 1 < clipped.size(); ++triangle_index) {
			append_triangle(output.get(), &output_tags,
			                clipped[0], clipped[triangle_index],
			                clipped[triangle_index + 1], tag);
		}
	}

	if (output->faces.empty()) {
		if (diagnostic != nullptr) {
			*diagnostic = "Clip removes the complete shape or leaves only tangent geometry.";
		}
		return GeneratedPrimitiveMesh(output, std::move(output_tags));
	}
	if (close_boundary && !intersection_segments.empty()) {
		std::vector<std::vector<glm::vec3>> loops =
			build_loops(intersection_segments, diagnostic);
		if (loops.empty() ||
		    !close_loops(
				loops,
				clip_specification.retainedSide() == ClipRetainedSide::Positive
					? -clip_specification.normal() : clip_specification.normal(),
				clip_index,
				output.get(),
				&output_tags,
				diagnostic)) {
			return GeneratedPrimitiveMesh(std::make_shared<Mesh>(), {});
		}
	}
	output->buildCollisionAccel();
	return GeneratedPrimitiveMesh(output, std::move(output_tags));
}
