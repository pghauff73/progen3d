#include "Context.h"
#include "editor/model/PreviewGenerationStatus.h"
#include "editor/service/GrammarCompilationService.h"
#include "editor/service/SceneRegenerationCoordinator.h"
#include "geometry/model/FacialSkinMesh.h"
#include "imgui_render.h"

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
#include <thread>
#include <vector>

namespace {

void require(bool condition, const std::string &message)
{
	if (!condition) throw std::runtime_error(message);
}

std::string digest(const void *bytes, std::size_t length)
{
	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256(static_cast<const unsigned char *>(bytes), length, hash);
	std::ostringstream result;
	for (unsigned char value : hash) result << std::hex << std::setw(2) << std::setfill('0') << unsigned(value);
	return result.str();
}

class EditorSnapshotEvidence
{
public:
	Json::Value collect(const SceneGenerationResult &result) const
	{
		require(result.succeeded(), "The editor rejected an expected valid character sample.");
		const auto &scene = *result.scene_snapshot->generationContext();
		const auto cube = build_base_vertex_data();
		std::string combined;
		std::size_t triangles = 0, objects = 0, faces = 0;
		double maximum_relative_force = 0.0;
		for (const auto &part : scene.primitive_instances) {
			if (part.removed) continue;
			std::vector<GLfloat> buffer;
			scene.buildInstanceBuffer(part, cube.data(), &buffer);
			require(!buffer.empty() && buffer.size() % 24 == 0 &&
				std::all_of(buffer.begin(), buffer.end(), [](float value) { return std::isfinite(value); }),
				"The editor produced an invalid native triangle buffer.");
			combined += digest(buffer.data(), buffer.size() * sizeof(float));
			triangles += buffer.size() / 24;
			++objects;
			if (part.resolved_geometry && part.resolved_geometry->hasTriangleMesh()) {
				const auto *face = dynamic_cast<const FacialSkinMesh *>(&part.resolved_geometry->triangleMesh());
				if (face != nullptr) {
					++faces;
					require(face->mechanical_state && face->mechanical_state->solve_result.converged,
						"The editor published a face without converged native mechanics.");
					maximum_relative_force = std::max(maximum_relative_force,
						face->mechanical_state->solve_result.relative_free_force);
				}
			}
		}
		require(faces == 1, "The editor must publish exactly one native elastic face.");
		require(maximum_relative_force <= 1e-4, "The editor face violates the native force tolerance.");
		Json::Value evidence;
		evidence["requestId"] = Json::UInt64(result.request_id);
		evidence["seconds"] = result.evaluation_time;
		evidence["generationMilliseconds"] = result.generation_milliseconds;
		evidence["objectCount"] = Json::UInt64(objects);
		evidence["triangleCount"] = Json::UInt64(triangles);
		evidence["nativeFaceCount"] = Json::UInt64(faces);
		evidence["maximumRelativeFreeForce"] = maximum_relative_force;
		evidence["nativeBufferSha256"] = digest(combined.data(), combined.size());
		return evidence;
	}
};

std::unique_ptr<SceneGenerationResult> awaitResult(SceneRegenerationCoordinator &coordinator)
{
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(120);
	while (std::chrono::steady_clock::now() < deadline) {
		if (auto result = coordinator.takeCompletedResult()) {
			while (coordinator.generationInProgress()) std::this_thread::yield();
			coordinator.joinWorkerIfIdle();
			return result;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}
	throw std::runtime_error("The editor generation worker exceeded the 120-second test deadline.");
}

std::string rejectionReason(const SceneGenerationResult &result)
{
	if (!result.error.empty()) return result.error;
	if (!result.diagnostics.empty()) return result.diagnostics.front().message;
	return {};
}

} // namespace

int main(int argc, char **argv)
{
	if (argc != 3) {
		std::cerr << "Usage: lyra-facial-editor-test character.p3d editor_validation.json\n";
		return 2;
	}
	Json::Value report;
	report["schema"] = "progen3d.lyra-editor-validation.v1";
	report["path"] = "GrammarCompilationService through persistent SceneRegenerationCoordinator";
	report["viewportPixelsVerified"] = false;
	report["scope"] = "Actual native editor generation and snapshot selection; UI status wiring is compiled, not a pixel-level GUI test.";
	try {
		std::ifstream input(argv[1]);
		require(bool(input), "Could not read the integrated grammar.");
		const std::string source((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
		GrammarCompilationService compiler({}, {});
		SceneRegenerationCoordinator coordinator([&compiler](const SceneGenerationRequest &request) {
			return compiler.compile(request);
		});
		PreviewGenerationStatus presentation;
		EditorSnapshotEvidence collector;
		coordinator.requestGeneration(source, 0.0, 0, false);
		auto neutral = awaitResult(coordinator);
		const auto neutral_evidence = collector.collect(*neutral);
		report["samples"].append(neutral_evidence);
		auto retained = std::move(neutral->scene_snapshot);
		presentation.accept();

		// The first worker is allowed to finish, but its superseded result must
		// never replace the preview while a newer neutral request is queued.
		const auto superseded_id = coordinator.requestGeneration(source, 7.6, 0, true);
		const auto latest_id = coordinator.requestGeneration(source, 0.0, 0, true);
		require(superseded_id != latest_id && coordinator.hasPendingRequest(), "The editor did not queue the newer request.");
		auto superseded = awaitResult(coordinator);
		require(superseded->request_id == superseded_id && !coordinator.isLatestRequest(superseded->request_id),
			"The editor did not identify the superseded sample.");
		const auto posed_evidence = collector.collect(*superseded);
		require(posed_evidence["nativeBufferSha256"] != neutral_evidence["nativeBufferSha256"], "The character did not move at 7.6 seconds.");
		report["samples"].append(posed_evidence);
		superseded.reset();
		coordinator.startPendingRequest();
		auto replay = awaitResult(coordinator);
		require(coordinator.isLatestRequest(replay->request_id), "The replay result is not the latest request.");
		const auto replay_evidence = collector.collect(*replay);
		require(replay_evidence["nativeBufferSha256"] == neutral_evidence["nativeBufferSha256"], "Editor neutral replay changed native geometry.");
		report["samples"].append(replay_evidence);
		retained = std::move(replay->scene_snapshot);
		report["supersededResultDiscarded"] = true;
		report["neutralReplayExact"] = true;

		// Valid props before a deliberately under-iterated face must not allow
		// a faceless partial character to be published as a successful scene.
		const std::string failed_face =
			"Start -> I(Sphere(segments(16 8)) material(whitepolishedmetal) 1 1) "
			"I(FacialSkin(size(.16 .21 .045) segments(48 64) response(elastic) "
			"skinMaterial(50000 .35 .001 1000000) solve(8 .0001) bending(1) mouthMesh(quality) "
			"muscle(left_smile .87 .56 34/48 20/64 .055 .08 .5 .04 40)) material(silverpolishedmetal) 1 1)\n";
		coordinator.requestGeneration(failed_face, 1.0, 0, false);
		auto failed = awaitResult(coordinator);
		require(!failed->succeeded() && !failed->scene_snapshot, "A rejected face was published as a partial scene.");
		const std::string reason = rejectionReason(*failed);
		require(!reason.empty(), "The rejected solve has no actionable diagnostic.");
		require(reason.find("did not converge") != std::string::npos,
			"The failure fixture did not reach and reject the native elastic solve.");
		presentation.reject(failed->evaluation_time, reason);
		require(presentation.hasRejectedRequest() && retained != nullptr,
			"Retaining the prior scene did not mark the preview stale.");
		report["failedSolveRejected"] = true;
		report["failedSolveDiagnostic"] = reason;
		report["staleSnapshotLabelRequired"] = presentation.hasRejectedRequest();
		report["retainedSnapshotSeconds"] = retained->evaluationTime();

		coordinator.requestGeneration(source, 0.0, 0, false);
		auto recovery = awaitResult(coordinator);
		const auto recovery_evidence = collector.collect(*recovery);
		require(recovery_evidence["nativeBufferSha256"] == neutral_evidence["nativeBufferSha256"], "A failed solve contaminated the next valid editor sample.");
		retained = std::move(recovery->scene_snapshot);
		presentation.accept();
		require(!presentation.hasRejectedRequest(), "The successful replacement did not clear stale status.");
		report["samples"].append(recovery_evidence);
		report["failureRecoveryExact"] = true;
		report["passed"] = true;
		coordinator.shutdown();
	} catch (const std::exception &error) {
		report["passed"] = false;
		report["error"] = error.what();
	}
	std::ofstream output(argv[2]);
	require(bool(output), "Could not write editor validation report.");
	Json::StreamWriterBuilder writer;
	writer["indentation"] = "  ";
	output << Json::writeString(writer, report) << '\n';
	std::cout << (report["passed"].asBool() ? "PASS" : "FAIL") << " Lyra native editor pipeline\n";
	if (!report["passed"].asBool()) std::cerr << report["error"].asString() << '\n';
	return report["passed"].asBool() ? 0 : 1;
}
