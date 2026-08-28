#include "vehicle/service/VehicleValidationService.h"

#include "geometry/service/MeshTopologyAnalyzer.h"
#include "vehicle/service/VehicleKinematicValidationService.h"
#include "vehicle/service/VehiclePackageValidationService.h"

#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>

namespace {

bool finite(const glm::vec3 &value)
{
	return std::isfinite(value.x) && std::isfinite(value.y) &&
	       std::isfinite(value.z);
}

std::string quantized_vertex_key(const glm::vec3 &vertex)
{
	const auto quantize = [](float value) {
		return static_cast<long long>(std::llround(value * 100000.0f));
	};
	return std::to_string(quantize(vertex.x)) + ":" +
	       std::to_string(quantize(vertex.y)) + ":" +
	       std::to_string(quantize(vertex.z));
}

const VehiclePlacedAssembly *find_assembly(
	const std::vector<VehiclePlacedAssembly> &assemblies,
	const std::string &identifier)
{
	for (const VehiclePlacedAssembly &assembly : assemblies) {
		if (assembly.objectIdentifier() == identifier) return &assembly;
	}
	return nullptr;
}

} // namespace

VehicleValidationReport VehicleValidationService::validate(
	const VehicleDefinition &definition,
	const VehicleBodySpecification &body_specification,
	const WheelAssemblySpecification &wheel_specification,
	const std::vector<VehiclePlacedAssembly> &assemblies,
	const std::vector<VehicleJoint> &joints,
	const GeneratedPrimitiveMesh &combined_mesh) const
{
	VehicleValidationReport report;
	report.append(VehiclePackageValidationService().validate(
		definition.intent(), definition.package()));
	if (!combined_mesh.mesh() || combined_mesh.mesh()->faces.empty() ||
	    combined_mesh.faceSurfaceTags().size() != combined_mesh.mesh()->faces.size()) {
		report.addIssue(VehicleValidationIssue(
			VehicleDiagnosticCode::LoftFailure,
			"Whole-vehicle geometry is empty or has inconsistent surface tags."));
		return report;
	}
	const Mesh &mesh = *combined_mesh.mesh();
	for (const glm::vec3 &vertex : mesh.vertices) {
		if (!finite(vertex)) {
			report.addIssue(VehicleValidationIssue(
				VehicleDiagnosticCode::LoftFailure,
				"Whole-vehicle geometry contains a non-finite vertex."));
			break;
		}
	}
	for (const glm::vec3 &normal : mesh.normals) {
		if (!finite(normal) || glm::length(normal) <= 1.0e-6f) {
			report.addIssue(VehicleValidationIssue(
				VehicleDiagnosticCode::LoftFailure,
				"Whole-vehicle geometry contains an invalid normal."));
			break;
		}
	}
	const MeshTopologyReport topology = MeshTopologyAnalyzer().analyze(
		mesh, combined_mesh.faceSurfaceTags());
	if (topology.degenerate_triangle_count != 0u ||
	    topology.nonmanifold_edge_count != 0u) {
		report.addIssue(VehicleValidationIssue(
			VehicleDiagnosticCode::LoftFailure,
			"Whole-vehicle geometry contains degenerate or non-manifold triangles."));
	}

	const VehiclePlacedAssembly *body = find_assembly(assemblies, "BodyShell");
	if (body == nullptr || !body->geometry().combinedPreviewMesh().mesh()) {
		report.addIssue(VehicleValidationIssue(
			VehicleDiagnosticCode::LoftFailure,
			"BodyShell assembly is missing."));
	}
	else {
		const Mesh *symmetric_body_mesh = nullptr;
		for (const VehicleAssemblyPart &part : body->geometry().parts()) {
			if (part.semanticRole() == "BodyShell" && part.generatedMesh().mesh()) {
				symmetric_body_mesh = part.generatedMesh().mesh().get();
				break;
			}
		}
		if (symmetric_body_mesh == nullptr) {
			report.addIssue(VehicleValidationIssue(
				VehicleDiagnosticCode::LoftFailure,
				"BodyShell assembly does not contain a semantic body shell part."));
			return report;
		}
		std::set<std::string> body_vertices;
		for (const glm::vec3 &vertex : symmetric_body_mesh->vertices) {
			body_vertices.insert(quantized_vertex_key(vertex));
		}
		for (const glm::vec3 &vertex : symmetric_body_mesh->vertices) {
			if (body_vertices.count(
					quantized_vertex_key(glm::vec3(-vertex.x, vertex.y, vertex.z))) == 0u) {
				report.addIssue(VehicleValidationIssue(
					VehicleDiagnosticCode::GuideConflict,
					"Body shell is not bilaterally symmetric about X=0."));
				break;
			}
		}
	}

	glm::vec3 minimum = mesh.vertices.front();
	glm::vec3 maximum = mesh.vertices.front();
	for (const glm::vec3 &vertex : mesh.vertices) {
		minimum = glm::min(minimum, vertex);
		maximum = glm::max(maximum, vertex);
	}
	const VehiclePackage &package = definition.package();
	if (minimum.x < -package.overallWidth() * 0.5f - 0.02f ||
	    maximum.x > package.overallWidth() * 0.5f + 0.02f ||
	    minimum.y < -0.01f || maximum.y > package.overallHeight() + 0.02f ||
	    minimum.z < package.rearBumperStation() - 0.02f ||
	    maximum.z > package.frontBumperStation() + 0.02f) {
		report.addIssue(VehicleValidationIssue(
			VehicleDiagnosticCode::PositioningFailure,
			"Whole-vehicle bounds exceed the declared package tolerance."));
	}

	const std::vector<std::pair<std::string, VehicleDatumType>> wheel_datums = {
		{"FrontWheelLeft", VehicleDatumType::FrontWheelCentreLeft},
		{"FrontWheelRight", VehicleDatumType::FrontWheelCentreRight},
		{"RearWheelLeft", VehicleDatumType::RearWheelCentreLeft},
		{"RearWheelRight", VehicleDatumType::RearWheelCentreRight}};
	for (const auto &wheel_datum : wheel_datums) {
		const VehiclePlacedAssembly *wheel = find_assembly(assemblies, wheel_datum.first);
		const VehicleDatum *datum = definition.datums().find(wheel_datum.second);
		if (wheel == nullptr || datum == nullptr ||
		    glm::length(
				glm::vec3(wheel->localTransform()[3]) - datum->origin()) > 1.0e-5f) {
			report.addIssue(VehicleValidationIssue(
				VehicleDiagnosticCode::PositioningFailure,
				"Wheel assembly '" + wheel_datum.first +
					"' is not aligned with its derived datum."));
		}
	}
	if (body_specification.wheelArches().size() != 4u) {
		report.addIssue(VehicleValidationIssue(
			VehicleDiagnosticCode::ArchClearanceFailure,
			"Acceptance vehicle requires four wheel arches."));
	}
	for (const WheelArchSpecification &arch : body_specification.wheelArches()) {
		if (arch.archRadius() + 1.0e-5f < wheel_specification.tireRadius()) {
			report.addIssue(VehicleValidationIssue(
				VehicleDiagnosticCode::ArchClearanceFailure,
				"Wheel arch '" + arch.identifier() +
					"' does not clear the nominal tire."));
		}
	}
	report.append(VehicleKinematicValidationService().validateJoints(joints));
	return report;
}
