#include "geometry/service/SphereMeshGenerator.h"

#include "geometry/model/SphereShapeSpecification.h"
#include "geometry/service/HalfSpaceMeshClipper.h"

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>

#include <cmath>
#include <memory>
#include <utility>
#include <vector>

namespace {

class SphereMeshConstruction
{
public:
	explicit SphereMeshConstruction(const SphereShapeSpecification &specification)
		: specification_(specification),
		  mesh_(std::make_shared<Mesh>())
	{
	}

	GeneratedPrimitiveMesh build()
	{
		addSphericalSurface(outerRadius(), MeshSurfaceRole::Outer, false);
		if (specification_.topology() == ShapeTopology::Shell) {
			addSphericalSurface(innerRadius(), MeshSurfaceRole::Inner, true);
		}
		if (specification_.topology() != ShapeTopology::Surface) {
			if (hasPolarStartBoundary() && specification_.closurePolicy().closesRims()) {
				addPolarBoundary(false);
			}
			if (hasPolarEndBoundary() && specification_.closurePolicy().closesRims()) {
				addPolarBoundary(true);
			}
			if (!specification_.angularDomain().isFullRevolution() &&
			    (specification_.closurePolicy().closesAngularBoundaries() ||
			     specification_.closurePolicy().closesRims())) {
				addAngularBoundary(false);
				addAngularBoundary(true);
			}
		}
		mesh_->buildCollisionAccel();
		return GeneratedPrimitiveMesh(mesh_, std::move(face_surface_tags_));
	}

private:
	struct SphereRow
	{
		bool pole = false;
		std::vector<int> vertices;
	};

	float outerRadius() const
	{
		return 0.5f * specification_.radialDomain().maximumRadiusFraction();
	}

	float innerRadius() const
	{
		return 0.5f * specification_.radialDomain().minimumRadiusFraction();
	}

	int azimuthSegmentCount() const
	{
		return specification_.tessellation().circumferentialSegments();
	}

	int polarSegmentCount() const
	{
		return specification_.tessellation().longitudinalSegments();
	}

	int angularVertexCount() const
	{
		return specification_.angularDomain().isFullRevolution()
			? azimuthSegmentCount() : azimuthSegmentCount() + 1;
	}

	int nextAngularIndex(int index) const
	{
		return specification_.angularDomain().isFullRevolution()
			? (index + 1) % angularVertexCount() : index + 1;
	}

	float azimuthRadians(int index) const
	{
		const float fraction = static_cast<float>(index) /
		                       static_cast<float>(azimuthSegmentCount());
		return glm::radians(specification_.angularDomain().startDegrees() +
		                    specification_.angularDomain().sweepDegrees() * fraction);
	}

	float polarRadians(int index) const
	{
		const float fraction = static_cast<float>(index) /
		                       static_cast<float>(polarSegmentCount());
		return glm::radians(specification_.polarDomain().minimumDegrees() +
		                    (specification_.polarDomain().maximumDegrees() -
		                     specification_.polarDomain().minimumDegrees()) * fraction);
	}

	bool hasPolarStartBoundary() const
	{
		return specification_.polarDomain().minimumDegrees() > 0.0f;
	}

	bool hasPolarEndBoundary() const
	{
		return specification_.polarDomain().maximumDegrees() < 180.0f;
	}

	glm::vec3 direction(float polar, float azimuth) const
	{
		return glm::vec3(std::sin(polar) * std::cos(azimuth),
		                 std::cos(polar),
		                 std::sin(polar) * std::sin(azimuth));
	}

	int addVertex(const glm::vec3 &position,
	             const glm::vec3 &normal,
	             const glm::vec3 &texcoord)
	{
		mesh_->vertices.push_back(position);
		mesh_->normals.push_back(normal);
		mesh_->texcoords.push_back(texcoord);
		return static_cast<int>(mesh_->vertices.size() - 1);
	}

	void addTriangle(int first, int second, int third, MeshSurfaceRole role)
	{
		mesh_->faces.emplace_back(first, second, third);
		face_surface_tags_.emplace_back(role);
	}

	void addSphericalSurface(float radius,
	                         MeshSurfaceRole role,
	                         bool reverse)
	{
		std::vector<SphereRow> rows;
		rows.reserve(static_cast<std::size_t>(polarSegmentCount() + 1));
		for (int polar_index = 0; polar_index <= polarSegmentCount(); ++polar_index) {
			const float polar = polarRadians(polar_index);
			const bool pole = std::fabs(std::sin(polar)) <= 1.0e-6f;
			SphereRow row;
			row.pole = pole;
			const int count = pole ? 1 : angularVertexCount();
			for (int angle_index = 0; angle_index < count; ++angle_index) {
				const float azimuth = pole ? glm::radians(specification_.angularDomain().startDegrees())
				                           : azimuthRadians(angle_index);
				const glm::vec3 radial = direction(polar, azimuth);
				row.vertices.push_back(addVertex(
					radial * radius,
					reverse ? -radial : radial,
					glm::vec3(pole ? 0.5f : static_cast<float>(angle_index) /
					                           static_cast<float>(azimuthSegmentCount()),
					          static_cast<float>(polar_index) /
					                           static_cast<float>(polarSegmentCount()),
					          0.0f)));
			}
			rows.push_back(std::move(row));
		}

		for (int polar_index = 0; polar_index < polarSegmentCount(); ++polar_index) {
			const SphereRow &current = rows[static_cast<std::size_t>(polar_index)];
			const SphereRow &next_row = rows[static_cast<std::size_t>(polar_index + 1)];
			for (int angle_index = 0; angle_index < azimuthSegmentCount(); ++angle_index) {
				const int next_angle = nextAngularIndex(angle_index);
				if (current.pole) {
					const int pole = current.vertices[0];
					const int first = next_row.vertices[static_cast<std::size_t>(angle_index)];
					const int second = next_row.vertices[static_cast<std::size_t>(next_angle)];
					if (reverse) addTriangle(pole, first, second, role);
					else addTriangle(pole, second, first, role);
				}
				else if (next_row.pole) {
					const int first = current.vertices[static_cast<std::size_t>(angle_index)];
					const int second = current.vertices[static_cast<std::size_t>(next_angle)];
					const int pole = next_row.vertices[0];
					if (reverse) addTriangle(first, pole, second, role);
					else addTriangle(first, second, pole, role);
				}
				else {
					const int lower_left = current.vertices[static_cast<std::size_t>(angle_index)];
					const int lower_right = current.vertices[static_cast<std::size_t>(next_angle)];
					const int upper_right = next_row.vertices[static_cast<std::size_t>(next_angle)];
					const int upper_left = next_row.vertices[static_cast<std::size_t>(angle_index)];
					if (reverse) {
						addTriangle(lower_left, upper_right, lower_right, role);
						addTriangle(lower_left, upper_left, upper_right, role);
					}
					else {
						addTriangle(lower_left, lower_right, upper_right, role);
						addTriangle(lower_left, upper_right, upper_left, role);
					}
				}
			}
		}
	}

	void addPolarBoundary(bool end)
	{
		const float polar = glm::radians(end
			? specification_.polarDomain().maximumDegrees()
			: specification_.polarDomain().minimumDegrees());
		const float minimum_radius = specification_.topology() == ShapeTopology::Shell
			? innerRadius() : 0.0f;
		const MeshSurfaceRole role = end ? MeshSurfaceRole::PolarEnd
		                                 : MeshSurfaceRole::PolarStart;
		for (int angle_index = 0; angle_index < azimuthSegmentCount(); ++angle_index) {
			const int next = nextAngularIndex(angle_index);
			const float azimuth = azimuthRadians(angle_index);
			const float next_azimuth = azimuthRadians(next);
			const glm::vec3 outer_first_direction = direction(polar, azimuth);
			const glm::vec3 outer_second_direction = direction(polar, next_azimuth);
			const glm::vec3 polar_normal_first(
				std::cos(polar) * std::cos(azimuth),
				-std::sin(polar),
				std::cos(polar) * std::sin(azimuth));
			const glm::vec3 polar_normal_second(
				std::cos(polar) * std::cos(next_azimuth),
				-std::sin(polar),
				std::cos(polar) * std::sin(next_azimuth));
			const glm::vec3 first_normal = end ? polar_normal_first : -polar_normal_first;
			const glm::vec3 second_normal = end ? polar_normal_second : -polar_normal_second;
			const int inner_first = addVertex(outer_first_direction * minimum_radius,
			                                  first_normal, glm::vec3(0.0f, 0.0f, 0.0f));
			const int outer_first = addVertex(outer_first_direction * outerRadius(),
			                                  first_normal, glm::vec3(1.0f, 0.0f, 0.0f));
			const int outer_second = addVertex(outer_second_direction * outerRadius(),
			                                   second_normal, glm::vec3(1.0f, 1.0f, 0.0f));
			if (specification_.topology() == ShapeTopology::Solid) {
				if (end) {
					addTriangle(inner_first, outer_second, outer_first, role);
				}
				else {
					addTriangle(inner_first, outer_first, outer_second, role);
				}
				continue;
			}
			const int inner_second = addVertex(outer_second_direction * minimum_radius,
			                                   second_normal, glm::vec3(0.0f, 1.0f, 0.0f));
			if (end) {
				addTriangle(inner_first, outer_second, outer_first, role);
				addTriangle(inner_first, inner_second, outer_second, role);
			}
			else {
				addTriangle(inner_first, outer_first, outer_second, role);
				addTriangle(inner_first, outer_second, inner_second, role);
			}
		}
	}

	void addAngularBoundary(bool end)
	{
		const float azimuth = glm::radians(
			specification_.angularDomain().startDegrees() +
			(end ? specification_.angularDomain().sweepDegrees() : 0.0f));
		const glm::vec3 tangent(-std::sin(azimuth), 0.0f, std::cos(azimuth));
		const glm::vec3 normal = end ? tangent : -tangent;
		const float minimum_radius = specification_.topology() == ShapeTopology::Shell
			? innerRadius() : 0.0f;
		const MeshSurfaceRole role = end ? MeshSurfaceRole::AngularEnd
		                                 : MeshSurfaceRole::AngularStart;
		for (int polar_index = 0; polar_index < polarSegmentCount(); ++polar_index) {
			const float first_polar = polarRadians(polar_index);
			const float second_polar = polarRadians(polar_index + 1);
			const glm::vec3 first_direction = direction(first_polar, azimuth);
			const glm::vec3 second_direction = direction(second_polar, azimuth);
			const int inner_first = addVertex(first_direction * minimum_radius, normal,
			                                  glm::vec3(0.0f,
				static_cast<float>(polar_index) / polarSegmentCount(), 0.0f));
			const int outer_first = addVertex(first_direction * outerRadius(), normal,
			                                  glm::vec3(1.0f,
				static_cast<float>(polar_index) / polarSegmentCount(), 0.0f));
			const int outer_second = addVertex(second_direction * outerRadius(), normal,
			                                   glm::vec3(1.0f,
									   static_cast<float>(polar_index + 1) / polarSegmentCount(), 0.0f));
			if (specification_.topology() == ShapeTopology::Solid) {
				if (end) {
					addTriangle(inner_first, outer_first, outer_second, role);
				}
				else {
					addTriangle(inner_first, outer_second, outer_first, role);
				}
				continue;
			}
			const int inner_second = addVertex(second_direction * minimum_radius, normal,
			                                   glm::vec3(0.0f,
				static_cast<float>(polar_index + 1) / polarSegmentCount(), 0.0f));
			if (end) {
				addTriangle(inner_first, outer_first, outer_second, role);
				addTriangle(inner_first, outer_second, inner_second, role);
			}
			else {
				addTriangle(inner_first, outer_second, outer_first, role);
				addTriangle(inner_first, inner_second, outer_second, role);
			}
		}
	}

	const SphereShapeSpecification &specification_;
	std::shared_ptr<Mesh> mesh_;
	std::vector<MeshSurfaceTag> face_surface_tags_;
};

}

GeneratedPrimitiveMesh SphereMeshGenerator::generate(
	const ShapeSpecification &specification,
	std::string *diagnostic) const
{
	const auto *sphere = dynamic_cast<const SphereShapeSpecification *>(&specification);
	if (sphere == nullptr) {
		if (diagnostic != nullptr) {
			*diagnostic = "SphereMeshGenerator requires a SphereShapeSpecification.";
		}
		return GeneratedPrimitiveMesh(std::make_shared<Mesh>(), {});
	}

	GeneratedPrimitiveMesh generated = SphereMeshConstruction(*sphere).build();
	HalfSpaceMeshClipper clipper;
	for (std::size_t clip_index = 0; clip_index < sphere->clips().size(); ++clip_index) {
		generated = clipper.clip(
			generated,
			sphere->clips()[clip_index],
			0.5f * sphere->radialDomain().maximumRadiusFraction(),
			clip_index,
			sphere->closurePolicy().closesClipBoundaries(),
			diagnostic);
		if (!generated.mesh() || generated.mesh()->faces.empty()) return generated;
	}
	return generated;
}
