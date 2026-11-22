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
#include <chrono>
#include <filesystem>
#include <map>
#include <random>
#include <set>
#include <sstream>
#include <tuple>
#include <openssl/sha.h>
#include <stdexcept>
#include <vector>
#include <json/json.h>

extern std::mt19937_64 grammar_rng;

namespace {

class NativeSceneObjWriter
{
public:
    void writeScene(std::ostream &out, const SceneGenerationContext &scene,
                    const Json::Value *primitive_evidence = nullptr) const;

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

public:
	static Json::Value elasticEvidence(
		const FacialSkinShapeSpecification &specification,
		const FacialSkinMesh &mesh, const glm::mat4 &transform)
	{
		if (!mesh.mechanical_state) return Json::Value();
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
		return report;
	}

private:
	static void writeElasticEvidence(std::ostream &out,
		const FacialSkinShapeSpecification &specification,
		const FacialSkinMesh &mesh, const glm::mat4 &transform)
	{
		const Json::Value report = elasticEvidence(specification, mesh, transform);
		if (report.isNull()) return;
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

void NativeSceneObjWriter::writeScene(std::ostream &out, const SceneGenerationContext &scene,
                                     const Json::Value *primitive_evidence) const
{
    const auto cube = build_base_vertex_data();
    std::size_t firstVertex = 1;
    std::size_t index = 0;
    out << std::setprecision(std::numeric_limits<float>::max_digits10);
    for (const auto &part : scene.primitive_instances) {
        if (part.removed) {
            continue;
        }
        out << "o part_" << index << "\nusemtl " << part.material_name << '\n';
        if (primitive_evidence != nullptr) {
            Json::StreamWriterBuilder writer;
            writer["indentation"] = "";
            out << "# native_primitive " << Json::writeString(writer, (*primitive_evidence)[Json::ArrayIndex(index)]) << '\n';
        }
        ++index;
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


using NativeSampleClock = std::chrono::steady_clock;

class NativeEvidenceEncoding
{
public:
    static double elapsedSeconds(NativeSampleClock::time_point start)
    {
        return std::chrono::duration<double>(NativeSampleClock::now() - start).count();
    }
    static std::string digestBytes(const void *data, std::size_t size)
    {
        unsigned char bytes[SHA256_DIGEST_LENGTH];
        SHA256(static_cast<const unsigned char *>(data), size, bytes);
        std::ostringstream text;
        text << std::hex << std::setfill('0');
        for (unsigned char value : bytes) text << std::setw(2) << static_cast<unsigned>(value);
        return text.str();
    }
    static std::string digestText(const std::string &text) { return digestBytes(text.data(), text.size()); }
    static Json::Value point(const glm::vec3 &point)
    {
        Json::Value result(Json::arrayValue);
        for (int axis = 0; axis < 3; ++axis) result.append(point[axis]);
        return result;
    }
    static Json::Value matrix(const glm::mat4 &matrix)
    {
        Json::Value result(Json::arrayValue);
        for (int column = 0; column < 4; ++column)
            for (int row = 0; row < 4; ++row) result.append(matrix[column][row]);
        return result;
    }
};
using NativeSourceRange = std::tuple<int, int, int, int>;

class NativeSourceProvenanceIndex
{
public:
    explicit NativeSourceProvenanceIndex(const GrammarDocument &grammar)
    {
        for (const auto *rule : grammar.rule_list) {
            std::map<NativeSourceRange, const GrammarActionToken *> declarations;
            collectRule(*rule, declarations);
            std::size_t ordinal = 0;
            for (const auto &[range, token] : declarations) {
                Json::Value source;
                source["rule"] = rule->rule_name;
                source["primitiveOrdinal"] = Json::UInt64(ordinal++);
                source["declarationSha256"] = NativeEvidenceEncoding::digestText(sourceText(grammar, range));
                by_range_[range] = std::move(source);
            }
        }
    }

    Json::Value identify(const ScenePrimitiveInstance &part) const
    {
        const NativeSourceRange range{part.source_start_line, part.source_start_column,
                                      part.source_end_line, part.source_end_column};
        const auto found = by_range_.find(range);
        if (found == by_range_.end()) throw std::runtime_error("Native primitive has no parsed source identity");
        Json::Value source = found->second;
        source["startLine"] = part.source_start_line + 1;
        source["startColumn"] = part.source_start_column;
        return source;
    }

private:
    static void collectToken(const GrammarActionToken &token,
                             std::map<NativeSourceRange, const GrammarActionToken *> &declarations)
    {
        if (token.token_name == "I") {
            declarations[{token.source_start_line, token.source_start_column,
                          token.source_end_line, token.source_end_column}] = &token;
        }
        if (token.isConditionalToken()) {
            for (const auto *child : token.getTrueBranchTokens()) if (child) collectToken(*child, declarations);
            for (const auto *child : token.getFalseBranchTokens()) if (child) collectToken(*child, declarations);
        }
        if (const auto *scope = dynamic_cast<const SpatialObjectScopeToken *>(&token)) {
            for (const auto *child : scope->body_tokens) if (child) collectToken(*child, declarations);
        }
    }

    static void collectRule(const GrammarRuleDefinition &rule,
                            std::map<NativeSourceRange, const GrammarActionToken *> &declarations)
    {
        for (int section = 0; section < 3; ++section)
            for (const auto *token : rule.section_tokens[section]) if (token) collectToken(*token, declarations);
        if (rule.alternate) collectRule(*rule.alternate, declarations);
    }

    static std::string sourceText(const GrammarDocument &grammar, const NativeSourceRange &range)
    {
        const auto [first, first_column, last, last_column] = range;
        std::string result;
        for (int line = first; line <= last; ++line) {
            const auto &text = grammar.lines.at(line);
            const std::size_t begin = line == first ? first_column : 0;
            const std::size_t end = line == last ? last_column : text.size();
            result += text.substr(begin, end - begin);
            result += '\n';
        }
        return result;
    }

    std::map<NativeSourceRange, Json::Value> by_range_;
};

class NativeSceneEvidenceCollector
{
public:
    Json::Value collect(const GrammarDocument &grammar, const NativeSourceProvenanceIndex &provenance,
                        const std::vector<std::string> &capture_rules) const
    {
        Json::Value report;
        report["instances"] = Json::Value(Json::arrayValue);
        const auto cube = build_base_vertex_data();
        std::size_t triangle_count = 0, vertex_count = 0;
        for (const auto &part : grammar.context->primitive_instances) {
            if (part.removed) continue;
            Json::Value item;
            item["schema"] = "progen3d.native-primitive.v1";
            item["objectName"] = "part_" + std::to_string(report["instances"].size());
            item["source"] = provenance.identify(part);
            item["type"] = part.type;
            item["material"] = part.material_name;
            item["alpha"] = part.alpha;
            item["texscale"] = part.texscale;
            item["matrixLayout"] = "column-major";
            item["primaryTransform"] = NativeEvidenceEncoding::matrix(part.primary_transform);
            item["secondaryTransform"] = NativeEvidenceEncoding::matrix(part.secondary_transform);
            item["size"] = NativeEvidenceEncoding::point(part.size);
            item["size2"] = NativeEvidenceEncoding::point(part.size2);
            item["dualScales"] = Json::Value(Json::arrayValue);
            item["dualTranslations"] = Json::Value(Json::arrayValue);
            for (int axis = 0; axis < 3; ++axis) {
                item["dualScales"].append(NativeEvidenceEncoding::point(part.dual_scales[axis]));
                item["dualTranslations"].append(NativeEvidenceEncoding::point(part.dual_translations[axis]));
            }
            item["shapeKeySha256"] = part.shape_specification
                ? NativeEvidenceEncoding::digestText(part.shape_specification->key().canonicalValue()) : "";
            std::vector<GLfloat> buffer;
            grammar.context->buildInstanceBuffer(part, cube.data(), &buffer);
            if (buffer.empty() || buffer.size() % 24 != 0 ||
                !std::all_of(buffer.begin(), buffer.end(), [](float value) { return std::isfinite(value); }))
                throw std::runtime_error("Invalid actual native triangle buffer for " + item["objectName"].asString());
            item["nativeBufferSha256"] = NativeEvidenceEncoding::digestBytes(buffer.data(), buffer.size() * sizeof(float));
            item["triangles"] = Json::UInt64(buffer.size() / 24);
            item["expandedVertices"] = Json::UInt64(buffer.size() / 8);
            glm::vec3 lower(std::numeric_limits<float>::infinity()), upper(-std::numeric_limits<float>::infinity());
            double normal_error = 0.0;
            const bool capture = matchesRule(item["source"]["rule"].asString(), capture_rules);
            if (capture) {
                item["geometry"]["positions"] = Json::Value(Json::arrayValue);
                item["geometry"]["normals"] = Json::Value(Json::arrayValue);
            }
            for (std::size_t index = 0; index < buffer.size(); index += 8) {
                const glm::vec3 point(buffer[index], buffer[index + 1], buffer[index + 2]);
                lower = glm::min(lower, point); upper = glm::max(upper, point);
                normal_error = std::max(normal_error, std::fabs(glm::length(
                    glm::dvec3(buffer[index + 3], buffer[index + 4], buffer[index + 5])) - 1.0));
                if (capture) for (int axis = 0; axis < 3; ++axis) {
                    item["geometry"]["positions"].append(buffer[index + axis]);
                    item["geometry"]["normals"].append(buffer[index + 3 + axis]);
                }
            }
            item["boundsMinimum"] = NativeEvidenceEncoding::point(lower);
            item["boundsMaximum"] = NativeEvidenceEncoding::point(upper);
            item["maximumNormalUnitError"] = normal_error;
            if (part.resolved_geometry && part.resolved_geometry->hasTriangleMesh()) {
                const Mesh &mesh = part.resolved_geometry->triangleMesh();
                item["nativeVertexCount"] = Json::UInt64(mesh.vertices.size());
                item["nativeTriangleCount"] = Json::UInt64(mesh.faces.size());
                item["nativeGeometrySha256"] = localMeshDigest(mesh);
                if (buffer.size() != mesh.faces.size() * 24)
                    throw std::runtime_error("Native mesh and evaluated triangle buffer disagree");
                double maximum_transform_error = 0.0;
                const glm::dmat4 transform(part.primary_transform);
                for (std::size_t triangle = 0; triangle < mesh.faces.size(); ++triangle) {
                    for (int corner = 0; corner < 3; ++corner) {
                        const auto local = glm::dvec4(mesh.vertices[mesh.faces[triangle][corner]], 1.0);
                        const glm::dvec3 expected(transform * local);
                        const std::size_t offset = (triangle * 3 + corner) * 8;
                        const glm::dvec3 actual(buffer[offset], buffer[offset + 1], buffer[offset + 2]);
                        maximum_transform_error = std::max(maximum_transform_error, glm::length(actual - expected));
                    }
                }
                item["maximumNativePositionTransformErrorM"] = maximum_transform_error;
                if (const auto *face = dynamic_cast<const FacialSkinMesh *>(&mesh)) {
                    const auto *specification = dynamic_cast<const FacialSkinShapeSpecification *>(part.shape_specification.get());
                    if (!specification) throw std::runtime_error("Native FacialSkin lacks its typed specification");
                    item["facial"]["canonicalText"] = specification->canonicalText();
                    item["facial"]["mechanics"] = NativeSceneObjWriter::elasticEvidence(*specification, *face, part.primary_transform);
                    item["facial"]["eyeballRadiusLocalM"] = face->landmarks.eyeball_radius;
                    appendLandmarks(item["facial"], face->landmarks, part.primary_transform);
                    if (capture) appendNativeFace(item["facial"], *face, *part.resolved_geometry);
                }
            }
            triangle_count += buffer.size() / 24; vertex_count += buffer.size() / 8;
            report["instances"].append(std::move(item));
        }
        report["objectCount"] = report["instances"].size();
        report["triangleCount"] = Json::UInt64(triangle_count);
        report["expandedVertexCount"] = Json::UInt64(vertex_count);
        report["allActualNativeBuffersFinite"] = true;
        return report;
    }
private:
    static std::string localMeshDigest(const Mesh &mesh)
    {
        // Encode components explicitly; GLM vector padding is not evidence.
        std::string bytes;
        const auto append_value = [&bytes](const auto &value) {
            bytes.append(reinterpret_cast<const char *>(&value), sizeof(value));
        };
        for (const auto *points : {&mesh.vertices, &mesh.normals, &mesh.texcoords}) {
            append_value(static_cast<std::uint64_t>(points->size()));
            for (const auto &point : *points)
                for (int axis = 0; axis < 3; ++axis) append_value(point[axis]);
        }
        append_value(static_cast<std::uint64_t>(mesh.faces.size()));
        for (const auto &face : mesh.faces)
            for (int corner = 0; corner < 3; ++corner) append_value(face[corner]);
        return NativeEvidenceEncoding::digestText(bytes);
    }

    static bool matchesRule(const std::string &rule, const std::vector<std::string> &patterns)
    {
        for (const auto &pattern : patterns) {
            if (pattern == rule) return true;
            if (!pattern.empty() && pattern.back() == '*' && rule.compare(0, pattern.size() - 1, pattern, 0, pattern.size() - 1) == 0) return true;
        }
        return false;
    }
    static void appendLandmarks(Json::Value &record, const FacialSkinLandmarks &landmarks, const glm::mat4 &transform)
    {
        const std::vector<std::pair<std::string, glm::vec3>> points = {
            {"left_eyeball_center", landmarks.left_eyeball_center}, {"right_eyeball_center", landmarks.right_eyeball_center},
            {"left_eye_outer_corner", landmarks.left_eye_outer_corner}, {"left_eye_inner_corner", landmarks.left_eye_inner_corner},
            {"left_eye_upper_margin", landmarks.left_eye_upper_margin}, {"left_eye_lower_margin", landmarks.left_eye_lower_margin},
            {"right_eye_outer_corner", landmarks.right_eye_outer_corner}, {"right_eye_inner_corner", landmarks.right_eye_inner_corner},
            {"right_eye_upper_margin", landmarks.right_eye_upper_margin}, {"right_eye_lower_margin", landmarks.right_eye_lower_margin},
            {"mouth_left_corner", landmarks.mouth_left_corner}, {"mouth_right_corner", landmarks.mouth_right_corner},
            {"mouth_upper_margin", landmarks.mouth_upper_margin}, {"mouth_lower_margin", landmarks.mouth_lower_margin}};
        for (const auto &[name, point] : points) {
            record["localLandmarks"][name] = NativeEvidenceEncoding::point(point);
            record["worldLandmarks"][name] = NativeEvidenceEncoding::point(glm::vec3(transform * glm::vec4(point, 1)));
        }
    }
    static void appendNativeFace(Json::Value &record, const FacialSkinMesh &face, const ResolvedPrimitiveGeometry &geometry)
    {
        auto &mesh = record["nativeMesh"];
        for (std::size_t index = 0; index < face.vertices.size(); ++index) {
            for (int axis = 0; axis < 3; ++axis) {
                mesh["positions"].append(face.vertices[index][axis]);
                mesh["normals"].append(face.normals[index][axis]);
            }
            mesh["materialUv"].append(face.texcoords[index].x);
            mesh["materialUv"].append(face.texcoords[index].y);
        }
        for (const auto &triangle : face.faces)
            for (int corner = 0; corner < 3; ++corner) mesh["triangles"].append(triangle[corner]);
        for (const auto &tag : geometry.faceSurfaceTags()) mesh["surfaceBoundaries"].append(tag.boundaryIndex());
        if (face.mechanical_state) {
            const auto &state = *face.mechanical_state;
            for (std::size_t index = 0; index < state.system.rest_positions.size(); ++index) {
                for (int axis = 0; axis < 3; ++axis) {
                    mesh["restPositions"].append(state.system.rest_positions[index][axis]);
                    mesh["doublePositions"].append(state.solve_result.positions[index][axis]);
                }
                if (state.system.fixed_vertices[index]) mesh["fixedVertexIds"].append(Json::UInt64(index));
            }
        }
    }
};

class NativeBatchSampleRunner
{
public:
    int run(const std::string &source_path, const std::string &request_path, const std::string &report_path) const
    {
        std::ifstream request_stream(request_path);
        Json::Value request;
        Json::CharReaderBuilder reader;
        std::string error;
        if (!request_stream || !Json::parseFromStream(reader, request_stream, &request, &error))
            throw std::runtime_error("Could not read native batch request: " + error);
        const auto &samples = request["samples"];
        if (!samples.isArray() || samples.empty() || samples.size() > 512)
            throw std::runtime_error("Native batch requires 1 to 512 samples");
        std::set<std::string> identifiers;
        for (const auto &sample : samples) {
            const std::string id = sample["id"].asString();
            if (!sample["id"].isString() || id.empty() || !identifiers.insert(id).second || !sample["seconds"].isNumeric() || !std::isfinite(sample["seconds"].asDouble()))
                throw std::runtime_error("Native batch sample requires a unique id and finite numeric seconds");
            if (sample.isMember("obj") && (!sample["obj"].isString() || sample["obj"].asString().empty()))
                throw std::runtime_error("Native batch OBJ path must be a nonempty string");
        }
        std::vector<std::string> capture_rules;
        if (request.isMember("captureRules")) {
            if (!request["captureRules"].isArray()) throw std::runtime_error("captureRules must be an array");
            for (const auto &rule : request["captureRules"]) {
                if (!rule.isString()) throw std::runtime_error("captureRules must contain strings");
                capture_rules.push_back(rule.asString());
            }
        }
        if (request.isMember("requireFacialSkin") && !request["requireFacialSkin"].isBool())
            throw std::runtime_error("requireFacialSkin must be Boolean");
        if (request.isMember("seed") && !request["seed"].isUInt64())
            throw std::runtime_error("Native batch seed must be a nonnegative integer");
        const std::uint64_t seed = request.get("seed", Json::UInt64(std::mt19937_64::default_seed)).asUInt64();
        const std::filesystem::path destination(report_path);
        const auto absolute_path = [](const std::filesystem::path &path) {
            return std::filesystem::weakly_canonical(std::filesystem::absolute(path));
        };
        const auto source_absolute = absolute_path(source_path);
        const auto request_absolute = absolute_path(request_path);
        const auto report_absolute = absolute_path(destination);
        if (report_absolute == source_absolute || report_absolute == request_absolute)
            throw std::runtime_error("Native batch report must not overwrite its input");
        std::set<std::filesystem::path> output_paths{report_absolute};
        for (const auto &sample : samples) if (sample.isMember("obj")) {
            std::filesystem::path path(sample["obj"].asString());
            if (path.is_relative()) path = destination.parent_path() / path;
            const auto absolute = absolute_path(path);
            if (absolute == source_absolute || absolute == request_absolute || !output_paths.insert(absolute).second)
                throw std::runtime_error("Native batch output paths must be distinct from inputs and each other");
        }
        if (!destination.parent_path().empty()) std::filesystem::create_directories(destination.parent_path());
        // Do not leave a prior successful report behind if this request fails.
        std::filesystem::remove(destination);
        Json::Value report;
        report["schema"] = "progen3d.native-batch.v1";
        report["source"] = source_path;
        report["parseOnce"] = true;
        report["parseIncludesFirstExpansion"] = true;
        report["seed"] = Json::UInt64(seed);
        report["randomStatePolicy"] = "Same declared seed before each native expansion";
        report["nativeBufferEncoding"] = "native-endian float32 triangle soup; interleaved position3 normal3 uv2";
        report["samples"] = Json::Value(Json::arrayValue);
        GrammarDocument grammar;
        setGrammarEvaluationDiagnosticsQuiet(&grammar, true);
        if (!setGrammarEvaluationTime(&grammar, samples[0]["seconds"].asDouble())) throw std::runtime_error("Invalid first sample time");
        grammar_rng.seed(seed);
        const auto parse_started = NativeSampleClock::now();
        grammar.loadFromFile(source_path);
        report["parseSeconds"] = NativeEvidenceEncoding::elapsedSeconds(parse_started);
        if (!grammar.error_details.empty() || grammar.rule_list.empty())
            throw std::runtime_error(grammar.error_details.empty() ? "No valid parsed rules" : grammar.error_details.front().message);
        const NativeSourceProvenanceIndex provenance(grammar);
        bool all_passed = true;
        for (Json::ArrayIndex index = 0; index < samples.size(); ++index) {
            const auto &sample = samples[index];
            Json::Value result;
            result["id"] = sample["id"];
            result["seconds"] = sample["seconds"];
            std::filesystem::path obj_path;
            if (sample.isMember("obj")) {
                obj_path = sample["obj"].asString();
                if (obj_path.is_relative()) obj_path = destination.parent_path() / obj_path;
                if (!obj_path.parent_path().empty()) std::filesystem::create_directories(obj_path.parent_path());
                std::filesystem::remove(obj_path);
            }
            try {
                const auto expansion_started = NativeSampleClock::now();
                grammar_rng.seed(seed);
                if (index != 0 && !grammar.reexpandAtTime(sample["seconds"].asDouble()))
                    throw std::runtime_error(grammar.error_details.empty() ? "Native time expansion failed" : grammar.error_details.front().message);
                result["timing"]["expansionSeconds"] = NativeEvidenceEncoding::elapsedSeconds(expansion_started);
                const auto generation_started = NativeSampleClock::now();
                grammar.addContext();
                grammar.context->genPrimitives();
                grammar.generateGeometry();
                result["timing"]["generationSeconds"] = NativeEvidenceEncoding::elapsedSeconds(generation_started);
                if (!grammar.error_details.empty() || grammar.context->primitive_instances.empty())
                    throw std::runtime_error(grammar.error_details.empty() ? "No native scene instances" : grammar.error_details.front().message);
                const auto audit_started = NativeSampleClock::now();
                result["scene"] = NativeSceneEvidenceCollector().collect(grammar, provenance, capture_rules);
                bool has_face = false;
                for (const auto &part : result["scene"]["instances"]) has_face |= part.isMember("facial");
                if (request.get("requireFacialSkin", false).asBool() && !has_face)
                    throw std::runtime_error("Required native FacialSkin sample is absent; see grammar execution diagnostics");
                result["timing"]["nativeBufferAuditSeconds"] = NativeEvidenceEncoding::elapsedSeconds(audit_started);
                if (!obj_path.empty()) {
                    auto lightweight = result["scene"]["instances"];
                    for (auto &part : lightweight) { part.removeMember("geometry"); if (part.isMember("facial")) part["facial"].removeMember("nativeMesh"); }
                    const auto export_started = NativeSampleClock::now();
                    std::ofstream output(obj_path);
                    output.exceptions(std::ios::badbit | std::ios::failbit);
                    output.imbue(std::locale::classic());
                    NativeSceneObjWriter().writeScene(output, *grammar.context, &lightweight);
                    output.close();
                    result["obj"] = obj_path.string();
                    result["timing"]["objExportSeconds"] = NativeEvidenceEncoding::elapsedSeconds(export_started);
                }
                result["passed"] = true;
            } catch (const std::exception &failure) {
                result["passed"] = false;
                result["error"] = failure.what();
                if (!obj_path.empty()) std::filesystem::remove(obj_path);
                all_passed = false;
            }
            for (const auto &issue : grammar.error_details) {
                Json::Value diagnostic;
                diagnostic["line"] = issue.line + 1;
                diagnostic["message"] = issue.message;
                result["diagnostics"].append(diagnostic);
            }
            report["samples"].append(std::move(result));
        }
        report["passed"] = all_passed;
        Json::StreamWriterBuilder writer;
        writer["indentation"] = "";
        std::ofstream output(report_path);
        output.exceptions(std::ios::badbit | std::ios::failbit);
        output << Json::writeString(writer, report) << '\n';
        return all_passed ? 0 : 1;
    }
};

} // namespace

int main(int argc, char **argv)
{
    if (argc == 5 && std::string(argv[1]) == "--batch") {
        try { return NativeBatchSampleRunner().run(argv[2], argv[3], argv[4]); }
        catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
    }
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
