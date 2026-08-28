#include "geometry/service/BotanicalBladeMeshGenerator.h"

#include "geometry/model/BotanicalBladeShapeSpecification.h"

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

namespace {

float normalized_beta(float parameter, float left_power, float right_power)
{
	const float value = std::pow(std::max(parameter, 1.0e-5f), left_power) *
	                    std::pow(std::max(1.0f - parameter, 1.0e-5f), right_power);
	const float peak_parameter = left_power / (left_power + right_power);
	const float peak = std::pow(peak_parameter, left_power) *
	                   std::pow(1.0f - peak_parameter, right_power);
	return peak > 0.0f ? value / peak : 0.0f;
}

float profile_width_factor(BotanicalBladeProfile profile, float parameter)
{
	const float sine = std::max(0.0f, std::sin(glm::pi<float>() * parameter));
	switch (profile) {
	case BotanicalBladeProfile::Linear:
		return std::pow(sine, 0.20f);
	case BotanicalBladeProfile::Lanceolate:
		return std::pow(sine, 1.75f);
	case BotanicalBladeProfile::Elliptic:
		return sine;
	case BotanicalBladeProfile::Ovate:
		return normalized_beta(parameter, 0.75f, 1.25f);
	case BotanicalBladeProfile::Obovate:
		return normalized_beta(parameter, 1.25f, 0.75f);
	case BotanicalBladeProfile::Cordate:
		return std::min(
			1.0f,
			normalized_beta(parameter, 0.45f, 1.15f) *
				(1.0f + 0.12f * std::cos(glm::two_pi<float>() * parameter)));
	case BotanicalBladeProfile::Palmate:
		return std::max(
			0.0f,
			sine * (0.86f + 0.14f * std::cos(6.0f * glm::pi<float>() * parameter)));
	case BotanicalBladeProfile::Needle:
		return std::pow(sine, 0.35f);
	}
	return sine;
}

glm::vec3 blade_point(
	const BotanicalBladeShapeSpecification &specification,
	float longitudinal_parameter,
	float lateral_parameter,
	float thickness_side)
{
	const float profile_factor = std::pow(
		std::max(0.0f, profile_width_factor(
			specification.profile(), longitudinal_parameter)),
		specification.widthPower());
	const float width_floor = specification.maximumWidth() * 0.015f;
	const float half_width = std::max(
		width_floor,
		0.5f * specification.maximumWidth() * profile_factor);
	const float angle = glm::radians(
		specification.twistDegrees() * longitudinal_parameter);
	const glm::vec3 lateral_axis(std::cos(angle), 0.0f, std::sin(angle));
	const glm::vec3 thickness_axis(-std::sin(angle), 0.0f, std::cos(angle));
	const float arch = std::sin(glm::pi<float>() * longitudinal_parameter);
	const glm::vec3 centre(
		0.0f,
		specification.length() * longitudinal_parameter,
		specification.longitudinalCurvature() * arch);
	const float camber_offset =
		specification.camber() * arch * (1.0f - lateral_parameter * lateral_parameter);
	return centre + lateral_axis * (lateral_parameter * half_width) +
	       thickness_axis *
		       (camber_offset + thickness_side * specification.thickness() * 0.5f);
}

class BotanicalBladeMeshConstruction
{
public:
	explicit BotanicalBladeMeshConstruction(
		const BotanicalBladeShapeSpecification &specification)
		: specification_(specification), mesh_(std::make_shared<Mesh>())
	{
	}

	GeometryBuildResult build()
	{
		add_surface(1.0f, false, MeshSurfaceRole::BotanicalBladeUpper);
		if (specification_.thickness() > 0.0f) {
			add_surface(-1.0f, true, MeshSurfaceRole::BotanicalBladeLower);
			add_longitudinal_edge(-1.0f);
			add_longitudinal_edge(1.0f);
			add_lateral_edge(0.0f);
			add_lateral_edge(1.0f);
		}
		mesh_->buildCollisionAccel();
		return GeometryBuildResult::createSuccess(
			GeneratedPrimitiveMesh(mesh_, std::move(face_surface_tags_)));
	}

private:
	int add_vertex(
		const glm::vec3 &position,
		const glm::vec3 &normal,
		const glm::vec3 &texcoord)
	{
		mesh_->vertices.push_back(position);
		mesh_->normals.push_back(normal);
		mesh_->texcoords.push_back(texcoord);
		return static_cast<int>(mesh_->vertices.size() - 1u);
	}

	void add_triangle(
		const glm::vec3 &first,
		const glm::vec3 &second,
		const glm::vec3 &third,
		const glm::vec3 &first_texcoord,
		const glm::vec3 &second_texcoord,
		const glm::vec3 &third_texcoord,
		MeshSurfaceRole role)
	{
		const glm::vec3 normal = glm::normalize(
			glm::cross(second - first, third - first));
		const int first_index = add_vertex(first, normal, first_texcoord);
		const int second_index = add_vertex(second, normal, second_texcoord);
		const int third_index = add_vertex(third, normal, third_texcoord);
		mesh_->faces.emplace_back(first_index, second_index, third_index);
		face_surface_tags_.emplace_back(role, 0u);
	}

	void add_quad(
		const glm::vec3 &first,
		const glm::vec3 &second,
		const glm::vec3 &third,
		const glm::vec3 &fourth,
		const glm::vec3 &first_texcoord,
		const glm::vec3 &second_texcoord,
		const glm::vec3 &third_texcoord,
		const glm::vec3 &fourth_texcoord,
		bool reverse,
		MeshSurfaceRole role)
	{
		if (reverse) {
			add_triangle(
				first, third, second, first_texcoord, third_texcoord,
				second_texcoord, role);
			add_triangle(
				first, fourth, third, first_texcoord, fourth_texcoord,
				third_texcoord, role);
		}
		else {
			add_triangle(
				first, second, third, first_texcoord, second_texcoord,
				third_texcoord, role);
			add_triangle(
				first, third, fourth, first_texcoord, third_texcoord,
				fourth_texcoord, role);
		}
	}

	void add_surface(float thickness_side, bool reverse, MeshSurfaceRole role)
	{
		for (int longitudinal_index = 0;
		     longitudinal_index < specification_.longitudinalSegments();
		     ++longitudinal_index) {
			const float t0 = static_cast<float>(longitudinal_index) /
			                 static_cast<float>(specification_.longitudinalSegments());
			const float t1 = static_cast<float>(longitudinal_index + 1) /
			                 static_cast<float>(specification_.longitudinalSegments());
			for (int lateral_index = 0;
			     lateral_index < specification_.lateralSegments();
			     ++lateral_index) {
				const float u0 = -1.0f + 2.0f *
					static_cast<float>(lateral_index) /
					static_cast<float>(specification_.lateralSegments());
				const float u1 = -1.0f + 2.0f *
					static_cast<float>(lateral_index + 1) /
					static_cast<float>(specification_.lateralSegments());
				add_quad(
					blade_point(specification_, t0, u0, thickness_side),
					blade_point(specification_, t0, u1, thickness_side),
					blade_point(specification_, t1, u1, thickness_side),
					blade_point(specification_, t1, u0, thickness_side),
					glm::vec3((u0 + 1.0f) * 0.5f, t0, 0.0f),
					glm::vec3((u1 + 1.0f) * 0.5f, t0, 0.0f),
					glm::vec3((u1 + 1.0f) * 0.5f, t1, 0.0f),
					glm::vec3((u0 + 1.0f) * 0.5f, t1, 0.0f),
					reverse, role);
			}
		}
	}

	void add_longitudinal_edge(float lateral_parameter)
	{
		for (int longitudinal_index = 0;
		     longitudinal_index < specification_.longitudinalSegments();
		     ++longitudinal_index) {
			const float t0 = static_cast<float>(longitudinal_index) /
			                 static_cast<float>(specification_.longitudinalSegments());
			const float t1 = static_cast<float>(longitudinal_index + 1) /
			                 static_cast<float>(specification_.longitudinalSegments());
			const bool reverse = lateral_parameter < 0.0f;
			add_quad(
				blade_point(specification_, t0, lateral_parameter, 1.0f),
				blade_point(specification_, t0, lateral_parameter, -1.0f),
				blade_point(specification_, t1, lateral_parameter, -1.0f),
				blade_point(specification_, t1, lateral_parameter, 1.0f),
				glm::vec3(0.0f, t0, 0.0f), glm::vec3(1.0f, t0, 0.0f),
				glm::vec3(1.0f, t1, 0.0f), glm::vec3(0.0f, t1, 0.0f),
				reverse, MeshSurfaceRole::BotanicalBladeEdge);
		}
	}

	void add_lateral_edge(float longitudinal_parameter)
	{
		for (int lateral_index = 0;
		     lateral_index < specification_.lateralSegments();
		     ++lateral_index) {
			const float u0 = -1.0f + 2.0f *
				static_cast<float>(lateral_index) /
				static_cast<float>(specification_.lateralSegments());
			const float u1 = -1.0f + 2.0f *
				static_cast<float>(lateral_index + 1) /
				static_cast<float>(specification_.lateralSegments());
			const bool reverse = longitudinal_parameter > 0.5f;
			add_quad(
				blade_point(specification_, longitudinal_parameter, u0, 1.0f),
				blade_point(specification_, longitudinal_parameter, u1, 1.0f),
				blade_point(specification_, longitudinal_parameter, u1, -1.0f),
				blade_point(specification_, longitudinal_parameter, u0, -1.0f),
				glm::vec3((u0 + 1.0f) * 0.5f, 0.0f, 0.0f),
				glm::vec3((u1 + 1.0f) * 0.5f, 0.0f, 0.0f),
				glm::vec3((u1 + 1.0f) * 0.5f, 1.0f, 0.0f),
				glm::vec3((u0 + 1.0f) * 0.5f, 1.0f, 0.0f),
				reverse, MeshSurfaceRole::BotanicalBladeEdge);
		}
	}

	const BotanicalBladeShapeSpecification &specification_;
	std::shared_ptr<Mesh> mesh_;
	std::vector<MeshSurfaceTag> face_surface_tags_;
};

} // namespace

GeometryBuildResult BotanicalBladeMeshGenerator::build(
	const ShapeSpecification &specification) const
{
	const auto *blade =
		dynamic_cast<const BotanicalBladeShapeSpecification *>(&specification);
	if (blade == nullptr ||
	    (specification.family() != ShapeFamily::LeafBlade &&
	     specification.family() != ShapeFamily::PetalBlade)) {
		return GeometryBuildResult::createFailure(
			GeometryBuildStatus::UnsupportedTopology,
			"BotanicalBladeMeshGenerator requires a LeafBlade or PetalBlade specification.");
	}
	return BotanicalBladeMeshConstruction(*blade).build();
}

GeneratedPrimitiveMesh BotanicalBladeMeshGenerator::generate(
	const ShapeSpecification &specification,
	std::string *diagnostic) const
{
	GeometryBuildResult result = build(specification);
	if (!result.succeeded() && diagnostic != nullptr) {
		*diagnostic = result.firstDiagnostic();
	}
	return result.generatedMesh();
}

