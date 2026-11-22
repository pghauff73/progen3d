#include "Context.h"
#include "GrammarRuntime.h"
#include "grammar.h"
#include "imgui_render.h"
#include "geometry/model/FacialSkinMesh.h"
#include "geometry/model/FacialSkinShapeSpecification.h"
#include "geometry/service/FacialElasticModelAssembler.h"
#include "geometry/service/FacialElasticSolver.h"
#include "geometry/service/FacialSkinAdmissibility.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <json/json.h>
#include <openssl/sha.h>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

using SteadyClock = std::chrono::steady_clock;

void require(bool condition, const std::string &message)
{
	if (!condition) throw std::runtime_error(message);
}

double secondsSince(SteadyClock::time_point start)
{
	return std::chrono::duration<double>(SteadyClock::now() - start).count();
}

std::string digest(const void *bytes, std::size_t length)
{
	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256(static_cast<const unsigned char *>(bytes), length, hash);
	std::ostringstream result;
	for (unsigned char value : hash) result << std::hex << std::setw(2) << std::setfill('0') << unsigned(value);
	return result.str();
}

std::string readSource(const std::string &path)
{
	std::ifstream input(path);
	require(bool(input), "Could not read timing fixture: " + path);
	return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

class FacialEquilibriumTiming
{
public:
	Json::Value measure(const ScenePrimitiveInstance &instance) const
	{
		const auto *face = dynamic_cast<const FacialSkinMesh *>(&instance.resolved_geometry->triangleMesh());
		const auto *specification = dynamic_cast<const FacialSkinShapeSpecification *>(instance.shape_specification.get());
		require(face && face->mechanical_state && specification, "A timing fixture must contain the native fitted elastic face.");
		const auto &reference_state = *face->mechanical_state;
		Mesh neutral_mesh = *face;
		for (std::size_t index = 0; index < neutral_mesh.vertices.size(); ++index)
			neutral_mesh.vertices[index] = glm::vec3(reference_state.system.rest_positions[index]);
		const auto &surface_tags = instance.resolved_geometry->faceSurfaceTags();
		const auto assembly_start = SteadyClock::now();
		const auto assembled = FacialElasticModelAssembler().assemble(*specification, neutral_mesh, surface_tags);
		const double assembly_seconds = secondsSince(assembly_start);
		FacialSkinAdmissibility admissibility(neutral_mesh, surface_tags, face->landmarks,
			assembled.system.fixed_vertices, specification->size().x);
		require(admissibility.minimumGlobeClearance(assembled.system.rest_positions) >= 0.0,
			"Timing fixture rest geometry fails native eye clearance.");
		FacialElasticSolverSettings settings;
		settings.max_iterations = specification->elasticSolveSettings().maximum_iterations;
		settings.relative_force_tolerance = specification->elasticSolveSettings().relative_force_tolerance;
		const auto solve_start = SteadyClock::now();
		const auto solved = FacialElasticSolver().solve(assembled.system, settings,
			[&admissibility](const auto &positions) { return admissibility.accepts(positions); });
		const double solve_seconds = secondsSince(solve_start);
		require(solved.converged && solved.positions.size() == reference_state.solve_result.positions.size(),
			"The separately timed native equilibrium did not reproduce an accepted solution.");
		double maximum_error = 0.0;
		for (std::size_t index = 0; index < solved.positions.size(); ++index)
			maximum_error = std::max(maximum_error,
				glm::length(solved.positions[index] - reference_state.solve_result.positions[index]));
		require(maximum_error == 0.0 && solved.iterations == reference_state.solve_result.iterations &&
			solved.relative_free_force == reference_state.solve_result.relative_free_force,
			"The timed reconstruction differs from the actual native generated equilibrium.");
		Json::Value evidence;
		evidence["modelAssemblySeconds"] = assembly_seconds;
		evidence["equilibriumSolveSeconds"] = solve_seconds;
		evidence["solveStartsAtImmutableRestState"] = true;
		evidence["maximumPositionDifferenceFromNativeSolveM"] = maximum_error;
		evidence["iterations"] = Json::UInt64(solved.iterations);
		evidence["relativeFreeForce"] = solved.relative_free_force;
		evidence["nativeSolveReproducedExactly"] = true;
		evidence["scope"] = "Separate repeat of native model assembly and equilibrium solve, with the same native admissibility callback. Timings exclude rest-mesh copy and admissibility-object construction.";
		return evidence;
	}
};

class NativeComponentTiming
{
public:
	Json::Value evaluate(const std::string &source, bool expects_face) const
	{
		GrammarDocument grammar;
		setGrammarEvaluationDiagnosticsQuiet(&grammar, true);
		require(setGrammarEvaluationTime(&grammar, 0.0), "Timing fixture time was rejected.");
		const auto parse_start = SteadyClock::now();
		grammar.loadFromSourceText(source, "lyra-component-timing");
		const double parse_seconds = secondsSince(parse_start);
		require(grammar.error_details.empty(), "Timing fixture grammar was rejected.");
		const auto expansion_start = SteadyClock::now();
		require(grammar.reexpandAtTime(0.0), "Timing fixture repeated expansion failed.");
		const double expansion_seconds = secondsSince(expansion_start);
		const auto generation_start = SteadyClock::now();
		grammar.addContext();
		grammar.context->genPrimitives();
		grammar.generateGeometry();
		const double generation_seconds = secondsSince(generation_start);
		require(grammar.error_details.empty(), "Timing fixture native generation failed.");
		Json::Value evidence;
		evidence["parseIncludingFirstExpansionSeconds"] = parse_seconds;
		evidence["repeatedExpansionSeconds"] = expansion_seconds;
		evidence["nativeGenerationSeconds"] = generation_seconds;
		std::size_t objects = 0, triangles = 0, native_faces = 0;
		std::string combined;
		const auto cube = build_base_vertex_data();
		const auto buffer_start = SteadyClock::now();
		const ScenePrimitiveInstance *facial_instance = nullptr;
		for (const auto &instance : grammar.context->primitive_instances) {
			if (instance.removed) continue;
			std::vector<GLfloat> buffer;
			grammar.context->buildInstanceBuffer(instance, cube.data(), &buffer);
			require(!buffer.empty() && buffer.size() % 24 == 0 &&
				std::all_of(buffer.begin(), buffer.end(), [](float value) { return std::isfinite(value); }),
				"Timing fixture produced an invalid native buffer.");
			const auto hash = digest(buffer.data(), buffer.size() * sizeof(float));
			combined += hash;
			evidence["instanceBufferSha256"].append(hash);
			++objects;
			triangles += buffer.size() / 24;
			if (instance.resolved_geometry && instance.resolved_geometry->hasTriangleMesh() &&
				dynamic_cast<const FacialSkinMesh *>(&instance.resolved_geometry->triangleMesh())) {
				++native_faces;
				facial_instance = &instance;
			}
		}
		evidence["bufferConstructionAndHashSeconds"] = secondsSince(buffer_start);
		evidence["objectCount"] = Json::UInt64(objects);
		evidence["triangleCount"] = Json::UInt64(triangles);
		evidence["nativeFaceCount"] = Json::UInt64(native_faces);
		evidence["nativeBufferSha256"] = digest(combined.data(), combined.size());
		require(native_faces == (expects_face ? 1 : 0), "Unexpected native face count in timing fixture.");
		if (expects_face) {
			require(objects == 1, "Face-only timing fixture contains another primitive.");
			evidence["separateEquilibriumRepeat"] = FacialEquilibriumTiming().measure(*facial_instance);
		}
		return evidence;
	}
};

} // namespace

int main(int argc, char **argv)
{
	if (argc != 4) {
		std::cerr << "Usage: lyra-facial-timing-test body_only.p3d face_only.p3d timing.json\n";
		return 2;
	}
	Json::Value report;
	report["schema"] = "progen3d.lyra-native-component-timing.v1";
	report["compiler"] = __VERSION__;
#ifdef __OPTIMIZE__
	report["harnessOptimizationEnabled"] = true;
#else
	report["harnessOptimizationEnabled"] = false;
#endif
	report["performanceTargetTested"] = false;
	report["buildScope"] = "The harness uses its recorded compiler; linked objects can have different optimization settings. Record the actual build separately; this is not a uniform-release benchmark.";
	report["cacheScope"] = "Three fresh GrammarDocument/SceneGenerationContext evaluations within one process, all starting at rest; operating-system and allocator caches are not flushed. Workspace load is uncontrolled.";
	report["meshUploadSeconds"] = Json::nullValue;
	report["nativeDrawingSeconds"] = Json::nullValue;
	report["displayTimingStatus"] = "Not exercised by the supported headless native-evaluation path; no GPU context or displayed-frame-rate claim.";
	try {
		NativeComponentTiming timer;
		for (bool facial : {false, true}) {
			const auto source = readSource(argv[facial ? 2 : 1]);
			const std::string label = facial ? "faceOnly" : "bodyOnly";
			report[label]["sourceSha256"] = digest(source.data(), source.size());
			std::string first_hash;
			for (int repetition = 0; repetition < 3; ++repetition) {
				auto sample = timer.evaluate(source, facial);
				const auto hash = sample["nativeBufferSha256"].asString();
				if (repetition == 0) first_hash = hash;
				require(hash == first_hash, "Repeated component timing changed the actual native geometry.");
				sample["repetition"] = repetition;
				report[label]["samples"].append(sample);
			}
			report[label]["repeatedBuffersIdentical"] = true;
		}
		report["passed"] = true;
		report["passScope"] = "Native validity, exact repeated geometry, and exact reconstructed equilibrium; no timing threshold is a gate.";
	} catch (const std::exception &error) {
		report["passed"] = false;
		report["error"] = error.what();
	}
	std::ofstream output(argv[3]);
	if (!output) { std::cerr << "Could not write timing receipt.\n"; return 2; }
	Json::StreamWriterBuilder writer;
	writer["indentation"] = "  ";
	output << Json::writeString(writer, report) << '\n';
	std::cout << (report["passed"].asBool() ? "PASS" : "FAIL") << " native component timing evidence\n";
	if (!report["passed"].asBool()) std::cerr << report["error"].asString() << '\n';
	return report["passed"].asBool() ? 0 : 1;
}
