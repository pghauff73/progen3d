#include "geometry/service/CurveNetworkSurfaceMeshGenerator.h"

#include "geometry/model/CurveNetworkSurfaceShapeSpecification.h"

#include <cmath>

GeometryBuildResult CurveNetworkSurfaceMeshGenerator::build(
	const ShapeSpecification &specification) const
{
	const auto *network = dynamic_cast<const CurveNetworkSurfaceShapeSpecification *>(
		&specification);
	if (network == nullptr || specification.family() != ShapeFamily::CurveNetworkSurface) {
		return GeometryBuildResult::createFailure(
			GeometryBuildStatus::UnsupportedTopology,
			"CurveNetworkSurfaceMeshGenerator requires a CurveNetworkSurface specification.");
	}

	auto mesh = std::make_shared<Mesh>();
	const int samples_u = network->samplesU();
	const int samples_v = network->samplesV();
	mesh->vertices.reserve(static_cast<std::size_t>(samples_u * samples_v));
	mesh->normals.reserve(mesh->vertices.capacity());
	mesh->texcoords.reserve(mesh->vertices.capacity());
	for (int v_index = 0; v_index < samples_v; ++v_index) {
		const double v = static_cast<double>(v_index) /
			static_cast<double>(samples_v - 1);
		for (int u_index = 0; u_index < samples_u; ++u_index) {
			const double u = static_cast<double>(u_index) /
				static_cast<double>(samples_u - 1);
			const glm::dvec3 position = network->surface().evaluatePosition(u, v);
			const glm::dvec3 normal = network->surface().evaluateNormal(u, v);
			if (!std::isfinite(position.x) || !std::isfinite(position.y) ||
			    !std::isfinite(position.z) || glm::dot(normal, normal) <= 1.0e-20) {
				return GeometryBuildResult::createFailure(
					GeometryBuildStatus::DegenerateFace,
					"CurveNetworkSurface produced a non-finite position or collapsed normal.");
			}
			mesh->vertices.emplace_back(position);
			mesh->normals.emplace_back(normal);
			mesh->texcoords.emplace_back(
				static_cast<float>(u), static_cast<float>(v), 0.0f);
		}
	}

	std::vector<MeshSurfaceTag> tags;
	tags.reserve(static_cast<std::size_t>((samples_u - 1) * (samples_v - 1) * 2));
	for (int v_index = 0; v_index + 1 < samples_v; ++v_index) {
		for (int u_index = 0; u_index + 1 < samples_u; ++u_index) {
			const int first = v_index * samples_u + u_index;
			const int second = first + 1;
			const int fourth = first + samples_u;
			const int third = fourth + 1;
			mesh->faces.emplace_back(first, second, third);
			mesh->faces.emplace_back(first, third, fourth);
			tags.emplace_back(MeshSurfaceRole::Outer, 0u);
			tags.emplace_back(MeshSurfaceRole::Outer, 0u);
		}
	}
	mesh->buildCollisionAccel();
	return GeometryBuildResult::createSuccess(
		GeneratedPrimitiveMesh(std::move(mesh), std::move(tags)));
}

GeneratedPrimitiveMesh CurveNetworkSurfaceMeshGenerator::generate(
	const ShapeSpecification &specification,
	std::string *diagnostic) const
{
	const GeometryBuildResult result = build(specification);
	if (!result.succeeded()) {
		if (diagnostic != nullptr) *diagnostic = result.firstDiagnostic();
		return GeneratedPrimitiveMesh(std::make_shared<Mesh>(), {});
	}
	return result.generatedMesh();
}
