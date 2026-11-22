// Export one native grammar-time sample for the FormStudies lessons and tests.
#include "Context.h"
#include "GrammarRuntime.h"
#include "geometry/model/SweepDiskShapeSpecification.h"
#include "geometry/model/FacialSkinShapeSpecification.h"
#include "geometry/model/FacialSkinMesh.h"
#include "geometry/service/FacialSkinMeshGenerator.h"
#include "geometry/service/FacialElasticSolver.h"
#include "grammar.h"
#include "imgui_render.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>
#include <stdexcept>
#include <vector>
#include <json/json.h>

namespace {

class NativeSceneObjWriter
{
public:
    void writeScene(std::ostream &out, const SceneGenerationContext &scene) const;

private:
	static Json::Value pointValue(const glm::dvec3 &point)
	{
		Json::Value result(Json::arrayValue);
		for (int axis = 0; axis < 3; ++axis) result.append(point[axis]);
		return result;
	}

	static Json::Value uvValue(const glm::vec2 &point)
	{
		Json::Value result(Json::arrayValue);
		result.append(point.x); result.append(point.y);
		return result;
	}

	static void writeElasticEvidence(std::ostream &out,
		const FacialSkinShapeSpecification &specification,
		const FacialSkinMesh &mesh, const glm::mat4 &transform)
	{
		if (!mesh.mechanical_state) return;
		const auto &state = *mesh.mechanical_state;
		const auto &solve = state.solve_result;
		const auto &energy = solve.final_evaluation;
		Json::Value report(Json::objectValue);
		report["shapeKey"] = specification.key().canonicalValue();
		report["canonicalText"] = specification.canonicalText();
		report["converged"] = solve.converged;
		report["iterations"] = Json::UInt64(solve.iterations);
		report["terminationReason"] = solve.termination_reason;
		report["relativeFreeForce"] = solve.relative_free_force;
		report["freeForceL2N"] = solve.free_force_l2_n;
		report["forceScaleN"] = solve.force_scale_n;
		const auto initial_evaluation = FacialElasticSolver().evaluate(state.system, state.system.rest_positions);
		double initial_free_force_squared = 0.0;
		for (std::size_t index = 0; index < state.system.rest_positions.size(); ++index) {
			if (!state.system.fixed_vertices[index]) initial_free_force_squared +=
				glm::dot(initial_evaluation.gradient_n[index], initial_evaluation.gradient_n[index]);
		}
		report["initialFreeForceL2N"] = std::sqrt(initial_free_force_squared);
		const std::vector<glm::dvec3> mesh_positions(mesh.vertices.begin(), mesh.vertices.end());
		const auto mesh_energy = FacialElasticSolver().evaluate(state.system, mesh_positions);
		double mesh_force_squared = 0.0, maximum_quantization = 0.0;
		for (std::size_t index = 0; index < mesh_positions.size(); ++index) {
			if (!state.system.fixed_vertices[index])
				mesh_force_squared += glm::dot(mesh_energy.gradient_n[index], mesh_energy.gradient_n[index]);
			maximum_quantization = std::max(maximum_quantization,
				glm::length(mesh_positions[index] - solve.positions[index]));
		}
		report["meshFreeForceL2N"] = std::sqrt(mesh_force_squared);
		report["meshRelativeFreeForce"] = std::sqrt(mesh_force_squared) / solve.force_scale_n;
		report["maximumPositionQuantizationM"] = maximum_quantization;
		report["solverPrecision"] = "float64; authoritative mechanical state";
		report["meshPrecision"] = "float32; presentation geometry, separately re-evaluated";
		report["initialEnergyJ"] = solve.accepted_energies_j.front();
		report["finalEnergyJ"] = energy.total_energy_j;
		report["membraneEnergyJ"] = energy.membrane_energy_j;
		report["supportEnergyJ"] = energy.support_energy_j;
		report["muscleEnergyJ"] = energy.muscle_energy_j;
		report["bendingEnergyJ"] = energy.bending_energy_j;
		report["maximumBendingCurvatureChangePerM"] = energy.maximum_bending_curvature_change_per_m;
		report["bendingScale"] = specification.bendingSettings().stiffness_scale;
		report["mouthMeshMode"] = specification.mouthMeshMode() == FacialMouthMeshMode::Quality ? "quality" : "legacy";
		const auto &bending = state.bending_assembly;
		report["bendingAssembly"]["activeStencils"] = Json::UInt64(bending.active_stencils);
		report["bendingAssembly"]["fixedOnlyTriangles"] = Json::UInt64(bending.fixed_only_triangles);
		report["bendingAssembly"]["fixedOnlyUnreconstructableTriangles"] = Json::UInt64(bending.fixed_only_unreconstructable_triangles);
		report["bendingAssembly"]["expandedStencils"] = Json::UInt64(bending.expanded_stencils);
		report["bendingAssembly"]["maximumPatchVertices"] = Json::UInt64(bending.maximum_patch_vertices);
		report["bendingAssembly"]["maximumNeighborhoodRings"] = Json::UInt64(bending.maximum_neighborhood_rings);
		report["bendingAssembly"]["maximumReconstructionCondition"] = bending.maximum_reconstruction_condition;
		report["bendingAssembly"]["maximumPolynomialReproductionError"] = bending.maximum_polynomial_reproduction_error;
		report["bendingAssembly"]["maximumRestCurvaturePerM"] = bending.maximum_rest_curvature_per_m;
		report["bendingAssembly"]["maximumPatchHeightRatio"] = bending.maximum_patch_height_ratio;
		report["maximumGreenStrain"] = energy.maximum_green_strain;
		report["mechanicalFrame"] = "head-local SI; displayed points include scene transforms";
		double total_area = 0.0;
		std::size_t fixed_count = 0;
		for (std::size_t index = 0; index < state.system.rest_positions.size(); ++index) {
			total_area += state.rest_vertex_areas_m2[index];
			if (state.system.fixed_vertices[index]) ++fixed_count;
		}
		report["totalRestAreaM2"] = total_area;
		report["hardAttachmentVertices"] = Json::UInt64(fixed_count);
		const auto &material = specification.elasticMaterial();
		report["material"]["youngModulusPa"] = material.young_modulus_pa;
		report["material"]["poissonRatio"] = material.poisson_ratio;
		report["material"]["thicknessM"] = material.thickness_m;
		report["material"]["supportNPerM3"] = material.support_stiffness_n_per_m3;
		report["muscles"] = Json::Value(Json::arrayValue);
		for (std::size_t index = 0; index < state.system.muscles.size(); ++index) {
			const auto &muscle = state.system.muscles[index];
			const auto &resolved = energy.muscle_states[index];
			Json::Value item(Json::objectValue);
			item["name"] = muscle.name;
			item["activation"] = muscle.activation;
			item["maximumShortening"] = muscle.maximum_shortening;
			item["stiffnessNPerM"] = muscle.stiffness_n_per_m;
			item["origin"] = pointValue(glm::dvec3(transform * glm::vec4(glm::vec3(muscle.origin), 1)));
			item["insertion"] = pointValue(glm::dvec3(transform * glm::vec4(glm::vec3(resolved.insertion_position), 1)));
			item["restLengthM"] = muscle.rest_length_m;
			item["targetLengthM"] = resolved.target_length_m;
			item["lengthM"] = resolved.length_m;
			item["tensionN"] = resolved.tension_n;
			item["energyJ"] = resolved.energy_j;
			item["insertionVertices"] = Json::UInt64(muscle.insertions.size());
			report["muscles"].append(item);
		}
		report["attachments"] = Json::Value(Json::arrayValue);
		for (const auto &attachment : specification.attachments()) {
			Json::Value item(Json::objectValue);
			item["name"] = attachment.name;
			item["centerUv"] = uvValue(attachment.center_uv);
			item["radiusUv"] = uvValue(attachment.influence_radius_uv);
			item["stiffnessNPerM"] = attachment.stiffness_n_per_m;
			report["attachments"].append(item);
		}
		report["tissueRegions"] = Json::Value(Json::arrayValue);
		for (const auto &region : specification.tissueRegions()) {
			Json::Value item(Json::objectValue);
			item["name"] = region.name;
			item["centerUv"] = uvValue(region.center_uv);
			item["radiusUv"] = uvValue(region.influence_radius_uv);
			item["youngScale"] = region.young_modulus_scale;
			item["supportScale"] = region.support_stiffness_scale;
			report["tissueRegions"].append(item);
		}
		Json::StreamWriterBuilder writer;
		writer["indentation"] = "";
		out << "# facial_elastic " << Json::writeString(writer, report) << '\n';
	}

    static void writePoint(std::ostream &out, const glm::vec3 &point)
    {
        out << point.x << ' ' << point.y << ' ' << point.z;
    }

    static void writeCorner(std::ostream &out, std::size_t vertex_index)
    {
        // Position, texture coordinate and normal records have a one-to-one
        // correspondence, including across object boundaries.
        out << vertex_index << '/' << vertex_index << '/' << vertex_index;
    }
};

void NativeSceneObjWriter::writeScene(std::ostream &out, const SceneGenerationContext &scene) const
{
    const auto cube = build_base_vertex_data();
    std::size_t firstVertex = 1;
    std::size_t index = 0;
    out << std::setprecision(std::numeric_limits<float>::max_digits10);
    for (const auto &part : scene.primitive_instances) {
        if (part.removed) {
            continue;
        }
        out << "o part_" << index++ << "\nusemtl " << part.material_name << '\n';
        if (const auto *face = dynamic_cast<const FacialSkinShapeSpecification *>(part.shape_specification.get())) {
            const auto *native_face = part.resolved_geometry && part.resolved_geometry->hasTriangleMesh()
                ? dynamic_cast<const FacialSkinMesh *>(&part.resolved_geometry->triangleMesh()) : nullptr;
            const auto landmarks = native_face ? native_face->landmarks : FacialSkinMeshGenerator().evaluateLandmarks(*face);
            if (native_face) writeElasticEvidence(out, *face, *native_face, part.primary_transform);
            const auto write_landmark = [&](const char *name, const glm::vec3 &point) {
                out << "# facial_landmark " << name << ' ';
                writePoint(out, part.primary_transform * glm::vec4(point, 1.0f));
                out << '\n';
            };
            write_landmark("left_eyeball_center", landmarks.left_eyeball_center);
            write_landmark("right_eyeball_center", landmarks.right_eyeball_center);
            write_landmark("left_eye_outer_corner", landmarks.left_eye_outer_corner);
            write_landmark("left_eye_inner_corner", landmarks.left_eye_inner_corner);
            write_landmark("left_eye_upper_margin", landmarks.left_eye_upper_margin);
            write_landmark("left_eye_lower_margin", landmarks.left_eye_lower_margin);
            write_landmark("right_eye_outer_corner", landmarks.right_eye_outer_corner);
            write_landmark("right_eye_inner_corner", landmarks.right_eye_inner_corner);
            write_landmark("right_eye_upper_margin", landmarks.right_eye_upper_margin);
            write_landmark("right_eye_lower_margin", landmarks.right_eye_lower_margin);
            write_landmark("mouth_left_corner", landmarks.mouth_left_corner);
            write_landmark("mouth_right_corner", landmarks.mouth_right_corner);
            write_landmark("mouth_upper_margin", landmarks.mouth_upper_margin);
            write_landmark("mouth_lower_margin", landmarks.mouth_lower_margin);
            out << "# facial_eyeball_radius_local " << landmarks.eyeball_radius << '\n';
        }
        const auto *rod = dynamic_cast<const SweepDiskShapeSpecification *>(part.shape_specification.get());
        if (rod) {
            out << "# centreline ";
            for (const auto &point : {rod->pathPoints().front(), rod->pathPoints().back()}) {
                writePoint(out, part.primary_transform * glm::vec4(point, 1.0f));
                out << ' ';
            }
            out << '\n';
        }
        std::vector<GLfloat> vertices;
        scene.buildInstanceBuffer(part, cube.data(), &vertices);
        // The desktop preview interleaves position, normal and UV for each triangle.
        if (vertices.empty() || vertices.size() % 24 != 0 ||
            !std::all_of(vertices.begin(), vertices.end(), [](float v) { return std::isfinite(v); })) {
            throw std::runtime_error("Invalid native triangle buffer");
        }
        const auto end = firstVertex + vertices.size() / 8;
        for (std::size_t i = 0; i < vertices.size(); i += 8) {
            out << "v ";
            writePoint(out, {vertices[i], vertices[i + 1], vertices[i + 2]});
            out << '\n';
        }
        for (std::size_t i = 0; i < vertices.size(); i += 8) {
            out << "vt " << vertices[i + 6] << ' ' << vertices[i + 7] << '\n';
        }
        for (std::size_t i = 0; i < vertices.size(); i += 8) {
            out << "vn ";
            // Preserve the evaluated native normals. Reconstructing normals
            // from the exported triangle soup would lose smooth skin shading.
            writePoint(out, {vertices[i + 3], vertices[i + 4], vertices[i + 5]});
            out << '\n';
        }
        for (auto i = firstVertex; i < end; i += 3) {
            out << "f ";
            writeCorner(out, i);
            out << ' ';
            writeCorner(out, i + 1);
            out << ' ';
            writeCorner(out, i + 2);
            out << '\n';
        }
        firstVertex = end;
    }
}

} // namespace

int main(int argc, char **argv)
{
    const bool require_facial_skin = argc == 5 && std::string(argv[4]) == "--require-facial-skin";
    if (argc != 4 && !require_facial_skin) {
        std::cerr << "Usage: form-study-sample grammar.p3d seconds output.obj [--require-facial-skin]\n";
        return 2;
    }
    try {
        std::ifstream input(argv[1]);
        if (!input) {
            throw std::runtime_error("Could not open grammar");
        }
        std::size_t consumed = 0;
        const std::string timeArgument(argv[2]);
        const double seconds = std::stod(timeArgument, &consumed);
        GrammarDocument grammar;
        if (consumed != timeArgument.size() || !setGrammarEvaluationTime(&grammar, seconds)) {
            throw std::runtime_error("Invalid time");
        }
        setGrammarEvaluationDiagnosticsQuiet(&grammar, true);
        grammar.loadFromStream(input, argv[1]);
        if (grammar.error_details.empty()) {
            grammar.addContext();
            grammar.context->genPrimitives();
            grammar.generateGeometry();
        }
        for (const auto &issue : grammar.error_details) {
            std::cerr << argv[1] << ':' << issue.line + 1 << ": " << issue.message << '\n';
        }
        if (!grammar.error_details.empty() || !grammar.context || grammar.context->primitive_instances.empty()) {
            return 1;
        }

        // A failed procedural face can leave valid eye/oral props in the scene.
        // Shell studies require the requested native skin before treating that
        // partial scene as a successful export. The generic sampler keeps its
        // existing behavior unless this explicit contract is requested.
        if (require_facial_skin) {
            const bool has_facial_skin = std::any_of(grammar.context->primitive_instances.begin(),
                grammar.context->primitive_instances.end(), [](const auto &part) {
                    return !part.removed && part.resolved_geometry && part.resolved_geometry->hasTriangleMesh() &&
                        dynamic_cast<const FacialSkinMesh *>(&part.resolved_geometry->triangleMesh()) != nullptr;
                });
            if (!has_facial_skin) {
                throw std::runtime_error("Required native FacialSkin sample is absent; see grammar execution diagnostics.");
            }
        }

        std::ofstream output;
        output.exceptions(std::ios::badbit | std::ios::failbit);
        output.open(argv[3]);
        output.imbue(std::locale::classic());
        NativeSceneObjWriter().writeScene(output, *grammar.context);
        output.close();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
