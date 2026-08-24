#include "geometry/service/CylinderMeshGenerator.h"

#include "geometry/model/CylinderShapeSpecification.h"
#include "geometry/service/HalfSpaceMeshClipper.h"

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>

#include <cmath>
#include <memory>
#include <utility>

namespace {

class CylinderMeshConstruction
{
public:
	explicit CylinderMeshConstruction(const CylinderShapeSpecification &specification)
		: specification_(specification),
		  mesh_(std::make_shared<Mesh>())
	{
	}

	GeneratedPrimitiveMesh build()
	{
		addOuterSurface();
		if (specification_.topology() == ShapeTopology::Shell) {
			addInnerSurface();
		}
		if (specification_.topology() != ShapeTopology::Surface) {
			if (specification_.closurePolicy().closesAxialBoundaries() ||
			    specification_.closurePolicy().closesRims()) {
				addAxialBoundary(false);
				addAxialBoundary(true);
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
	float outerRadius() const
	{
		return 0.5f * specification_.radialDomain().maximumRadiusFraction();
	}

	float innerRadius() const
	{
		return 0.5f * specification_.radialDomain().minimumRadiusFraction();
	}

	float angleRadians(int angle_index) const
	{
		const float fraction = static_cast<float>(angle_index) /
		                       static_cast<float>(angleSegmentCount());
		return glm::radians(specification_.angularDomain().startDegrees() +
		                    specification_.angularDomain().sweepDegrees() * fraction);
	}

	int angleSegmentCount() const
	{
		return specification_.tessellation().circumferentialSegments();
	}

	int axialSegmentCount() const
	{
		return specification_.tessellation().longitudinalSegments();
	}

	int angularVertexCount() const
	{
		return specification_.angularDomain().isFullRevolution()
			? angleSegmentCount() : angleSegmentCount() + 1;
	}

	int nextAngularIndex(int angle_index) const
	{
		return specification_.angularDomain().isFullRevolution()
			? (angle_index + 1) % angularVertexCount()
			: angle_index + 1;
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

	void addTriangle(int first,
	                 int second,
	                 int third,
	                 MeshSurfaceRole role)
	{
		mesh_->faces.emplace_back(first, second, third);
		face_surface_tags_.emplace_back(role);
	}

	void addQuad(int lower_left,
	             int lower_right,
	             int upper_right,
	             int upper_left,
	             MeshSurfaceRole role,
	             bool reverse)
	{
		if (reverse) {
			addTriangle(lower_left, upper_right, lower_right, role);
			addTriangle(lower_left, upper_left, upper_right, role);
		}
		else {
			addTriangle(lower_left, lower_right, upper_right, role);
			addTriangle(lower_left, upper_right, upper_left, role);
		}
	}

	void addOuterSurface()
	{
		const int angular_count = angularVertexCount();
		std::vector<int> vertices(
			static_cast<std::size_t>((axialSegmentCount() + 1) * angular_count));
		for (int axial_index = 0; axial_index <= axialSegmentCount(); ++axial_index) {
			const float axial_fraction = static_cast<float>(axial_index) /
			                             static_cast<float>(axialSegmentCount());
			const float y = specification_.axialDomain().minimumPosition() +
			                (specification_.axialDomain().maximumPosition() -
			                 specification_.axialDomain().minimumPosition()) * axial_fraction;
			for (int angle_index = 0; angle_index < angular_count; ++angle_index) {
				const float angle = angleRadians(angle_index);
				const glm::vec3 radial(std::cos(angle), 0.0f, std::sin(angle));
				vertices[static_cast<std::size_t>(axial_index * angular_count + angle_index)] =
					addVertex(radial * outerRadius() + glm::vec3(0.0f, y, 0.0f),
					          radial,
					          glm::vec3(static_cast<float>(angle_index) /
					                        static_cast<float>(angleSegmentCount()),
					                    axial_fraction,
					                    0.0f));
			}
		}
		for (int axial_index = 0; axial_index < axialSegmentCount(); ++axial_index) {
			for (int angle_index = 0; angle_index < angleSegmentCount(); ++angle_index) {
				const int next = nextAngularIndex(angle_index);
				addQuad(
					vertices[static_cast<std::size_t>(axial_index * angular_count + angle_index)],
					vertices[static_cast<std::size_t>(axial_index * angular_count + next)],
					vertices[static_cast<std::size_t>((axial_index + 1) * angular_count + next)],
					vertices[static_cast<std::size_t>((axial_index + 1) * angular_count + angle_index)],
					MeshSurfaceRole::Outer,
					true);
			}
		}
	}

	void addInnerSurface()
	{
		const int angular_count = angularVertexCount();
		std::vector<int> vertices(
			static_cast<std::size_t>((axialSegmentCount() + 1) * angular_count));
		for (int axial_index = 0; axial_index <= axialSegmentCount(); ++axial_index) {
			const float axial_fraction = static_cast<float>(axial_index) /
			                             static_cast<float>(axialSegmentCount());
			const float y = specification_.axialDomain().minimumPosition() +
			                (specification_.axialDomain().maximumPosition() -
			                 specification_.axialDomain().minimumPosition()) * axial_fraction;
			for (int angle_index = 0; angle_index < angular_count; ++angle_index) {
				const float angle = angleRadians(angle_index);
				const glm::vec3 radial(std::cos(angle), 0.0f, std::sin(angle));
				vertices[static_cast<std::size_t>(axial_index * angular_count + angle_index)] =
					addVertex(radial * innerRadius() + glm::vec3(0.0f, y, 0.0f),
					          -radial,
					          glm::vec3(static_cast<float>(angle_index) /
					                        static_cast<float>(angleSegmentCount()),
					                    axial_fraction,
					                    0.0f));
			}
		}
		for (int axial_index = 0; axial_index < axialSegmentCount(); ++axial_index) {
			for (int angle_index = 0; angle_index < angleSegmentCount(); ++angle_index) {
				const int next = nextAngularIndex(angle_index);
				addQuad(
					vertices[static_cast<std::size_t>(axial_index * angular_count + angle_index)],
					vertices[static_cast<std::size_t>(axial_index * angular_count + next)],
					vertices[static_cast<std::size_t>((axial_index + 1) * angular_count + next)],
					vertices[static_cast<std::size_t>((axial_index + 1) * angular_count + angle_index)],
					MeshSurfaceRole::Inner,
					false);
			}
		}
	}

	void addAxialBoundary(bool top)
	{
		const float y = top ? specification_.axialDomain().maximumPosition()
		                    : specification_.axialDomain().minimumPosition();
		const glm::vec3 normal = top ? glm::vec3(0.0f, 1.0f, 0.0f)
		                             : glm::vec3(0.0f, -1.0f, 0.0f);
		const MeshSurfaceRole role = top ? MeshSurfaceRole::AxialEnd
		                                 : MeshSurfaceRole::AxialStart;
		if (specification_.topology() == ShapeTopology::Solid) {
			const int center = addVertex(glm::vec3(0.0f, y, 0.0f), normal,
			                             glm::vec3(0.5f, 0.5f, 0.0f));
			std::vector<int> ring(static_cast<std::size_t>(angularVertexCount()));
			for (int angle_index = 0; angle_index < angularVertexCount(); ++angle_index) {
				const float angle = angleRadians(angle_index);
				const glm::vec3 position(outerRadius() * std::cos(angle), y,
				                         outerRadius() * std::sin(angle));
				ring[static_cast<std::size_t>(angle_index)] = addVertex(
					position, normal,
					glm::vec3(position.x / (2.0f * outerRadius()) + 0.5f,
					          position.z / (2.0f * outerRadius()) + 0.5f,
					          0.0f));
			}
			for (int angle_index = 0; angle_index < angleSegmentCount(); ++angle_index) {
				const int next = nextAngularIndex(angle_index);
				if (top) addTriangle(center, ring[static_cast<std::size_t>(next)],
				                     ring[static_cast<std::size_t>(angle_index)], role);
				else addTriangle(center, ring[static_cast<std::size_t>(angle_index)],
				                      ring[static_cast<std::size_t>(next)], role);
			}
			return;
		}

		std::vector<int> outer(static_cast<std::size_t>(angularVertexCount()));
		std::vector<int> inner(static_cast<std::size_t>(angularVertexCount()));
		for (int angle_index = 0; angle_index < angularVertexCount(); ++angle_index) {
			const float angle = angleRadians(angle_index);
			const glm::vec3 radial(std::cos(angle), 0.0f, std::sin(angle));
			outer[static_cast<std::size_t>(angle_index)] = addVertex(
				radial * outerRadius() + glm::vec3(0.0f, y, 0.0f), normal,
				glm::vec3(1.0f, static_cast<float>(angle_index) /
				                    static_cast<float>(angleSegmentCount()), 0.0f));
			inner[static_cast<std::size_t>(angle_index)] = addVertex(
				radial * innerRadius() + glm::vec3(0.0f, y, 0.0f), normal,
				glm::vec3(0.0f, static_cast<float>(angle_index) /
				                    static_cast<float>(angleSegmentCount()), 0.0f));
		}
		for (int angle_index = 0; angle_index < angleSegmentCount(); ++angle_index) {
			const int next = nextAngularIndex(angle_index);
			if (top) {
				addQuad(inner[static_cast<std::size_t>(angle_index)],
				        inner[static_cast<std::size_t>(next)],
				        outer[static_cast<std::size_t>(next)],
				        outer[static_cast<std::size_t>(angle_index)], role, false);
			}
			else {
				addQuad(inner[static_cast<std::size_t>(angle_index)],
				        inner[static_cast<std::size_t>(next)],
				        outer[static_cast<std::size_t>(next)],
				        outer[static_cast<std::size_t>(angle_index)], role, true);
			}
		}
	}

	void addAngularBoundary(bool end)
	{
		const int angle_index = end ? angleSegmentCount() : 0;
		const float angle = angleRadians(angle_index);
		const glm::vec3 radial(std::cos(angle), 0.0f, std::sin(angle));
		const glm::vec3 tangent(-std::sin(angle), 0.0f, std::cos(angle));
		const glm::vec3 normal = end ? tangent : -tangent;
		const float radial_minimum = specification_.topology() == ShapeTopology::Shell
			? innerRadius() : 0.0f;
		const MeshSurfaceRole role = end ? MeshSurfaceRole::AngularEnd
		                                 : MeshSurfaceRole::AngularStart;
		for (int axial_index = 0; axial_index < axialSegmentCount(); ++axial_index) {
			const float first_fraction = static_cast<float>(axial_index) /
			                             static_cast<float>(axialSegmentCount());
			const float second_fraction = static_cast<float>(axial_index + 1) /
			                              static_cast<float>(axialSegmentCount());
			const float first_y = specification_.axialDomain().minimumPosition() +
				(specification_.axialDomain().maximumPosition() -
				 specification_.axialDomain().minimumPosition()) * first_fraction;
			const float second_y = specification_.axialDomain().minimumPosition() +
				(specification_.axialDomain().maximumPosition() -
				 specification_.axialDomain().minimumPosition()) * second_fraction;
			const int inner_first = addVertex(radial * radial_minimum + glm::vec3(0.0f, first_y, 0.0f),
			                                  normal, glm::vec3(0.0f, first_fraction, 0.0f));
			const int outer_first = addVertex(radial * outerRadius() + glm::vec3(0.0f, first_y, 0.0f),
			                                  normal, glm::vec3(1.0f, first_fraction, 0.0f));
			const int outer_second = addVertex(radial * outerRadius() + glm::vec3(0.0f, second_y, 0.0f),
			                                   normal, glm::vec3(1.0f, second_fraction, 0.0f));
			const int inner_second = addVertex(radial * radial_minimum + glm::vec3(0.0f, second_y, 0.0f),
			                                   normal, glm::vec3(0.0f, second_fraction, 0.0f));
			addQuad(inner_first, outer_first, outer_second, inner_second,
			        role, end);
		}
	}

	const CylinderShapeSpecification &specification_;
	std::shared_ptr<Mesh> mesh_;
	std::vector<MeshSurfaceTag> face_surface_tags_;
};

}

GeneratedPrimitiveMesh CylinderMeshGenerator::generate(
	const ShapeSpecification &specification,
	std::string *diagnostic) const
{
	const auto *cylinder = dynamic_cast<const CylinderShapeSpecification *>(&specification);
	if (cylinder == nullptr) {
		if (diagnostic != nullptr) {
			*diagnostic = "CylinderMeshGenerator requires a CylinderShapeSpecification.";
		}
		return GeneratedPrimitiveMesh(std::make_shared<Mesh>(), {});
	}

	GeneratedPrimitiveMesh generated = CylinderMeshConstruction(*cylinder).build();
	HalfSpaceMeshClipper clipper;
	for (std::size_t clip_index = 0; clip_index < cylinder->clips().size(); ++clip_index) {
		generated = clipper.clip(
			generated,
			cylinder->clips()[clip_index],
			0.5f * cylinder->radialDomain().maximumRadiusFraction(),
			clip_index,
			cylinder->closurePolicy().closesClipBoundaries(),
			diagnostic);
		if (!generated.mesh() || generated.mesh()->faces.empty()) return generated;
	}
	return generated;
}
