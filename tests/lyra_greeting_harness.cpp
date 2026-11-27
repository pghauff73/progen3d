#include "Context.h"
#include "GrammarRuntime.h"
#include "editor/service/GrammarCompilationService.h"
#include "geometry/model/GeneratedMeshReferenceShapeSpecification.h"
#include "grammar.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const std::string &message)
{
    if (!condition) throw std::runtime_error(message);
}

std::unique_ptr<SceneGenerationResult> compileSnapshot(const std::string &source, double seconds)
{
    SceneGenerationRequest request;
    request.source_text = source;
    request.evaluation_time = seconds;
    request.time_sample = true;
    auto result = GrammarCompilationService({}, {}).compile(request);
    require(result->succeeded(), result->diagnostics.empty() ? result->error :
            result->diagnostics.front().message);
    return result;
}

struct CharacterPose
{
    std::map<std::string, glm::mat4> transforms;
    glm::vec3 hand_position{};
};

CharacterPose inspectCharacter(const GrammarDocument &document)
{
    require(grammarUsesTime(&document), "The character must respond to the timeline.");
    const auto &instances = document.context->primitive_instances;
    require(instances.size() == 66, "Expected all 66 body and material mesh instances.");
    std::size_t triangles = 0;
    std::set<std::string> materials;
    CharacterPose pose;
    for (const auto &instance : instances) {
        const auto *reference = dynamic_cast<const GeneratedMeshReferenceShapeSpecification *>(
            instance.shape_specification.get());
        require(reference && instance.resolved_geometry && instance.resolved_geometry->hasTriangleMesh(),
                "A character part did not resolve to its triangle mesh.");
        require(pose.transforms.emplace(reference->meshKey(), instance.primary_transform).second,
                "Unexpected repeated mesh key " + reference->meshKey());
        const auto &mesh = instance.resolved_geometry->triangleMesh();
        require(!mesh.faces.empty(), "Empty mesh " + reference->meshKey());
        triangles += mesh.faces.size();
        materials.insert(instance.material_name);
        if (reference->meshKey() == "LyraHandLGloveWeave") {
            pose.hand_position = instance.position;
        }
    }
    require(triangles == 58972, "Unexpected triangle count " + std::to_string(triangles));
    require(materials.size() == 11 && materials.count("cyanemissiveplaster") && materials.count("amberemissiveplaster"),
            "The eleven material assignments must include cyan and amber emissive surfaces.");
    require(pose.transforms.count("LyraHandLGloveWeave"), "Missing presenting hand.");
    return pose;
}

void checkSeparateDocuments()
{
    const std::string rule =
        "Start -> I(GeneratedMeshReference(meshKey(Panel) topology(surface)) material(redmetallicpaint))\n";
    const std::string header = "#@mesh-v1 Panel mm Z\n#v 0 0 0\n#v 20 0 0\n#v 0 0 ";
    const std::string end = "\n#f 0 1 2\n#@endmesh\n";
    const auto first = compileSnapshot(rule + header + "30" + end, 0.0);
    const auto second = compileSnapshot(rule + header + "60" + end, 0.0);
    const auto height = [](const SceneGenerationResult &result) {
        const auto &instances = result.scene_snapshot->generationContext()->primitive_instances;
        require(instances.size() == 1 && instances.front().resolved_geometry,
                "Expected one document-local triangle.");
        return instances.front().resolved_geometry->localBoundsMaximum().y;
    };
    require(std::abs(height(*first) - 0.03f) < 1e-6f && std::abs(height(*second) - 0.06f) < 1e-6f,
            "Two open documents must resolve their own units, axes and mesh definitions.");
    GrammarDocument invalid;
    invalid.loadFromSourceText(rule + header + "30" + end + header + "60" + end);
    require(!invalid.error_details.empty(), "Duplicate local mesh keys must be diagnosed.");
    invalid.loadFromSourceText(rule + header + "30\n#f 0 1 99\n#@endmesh\n");
    require(!invalid.error_details.empty(), "Out-of-range triangle indices must be diagnosed.");
}

} // namespace

int main(int argc, char **argv)
{
    try {
        require(argc == 2, "Pass the path to the staged Lyra_Greeting.p3d.");
        std::ifstream input(argv[1], std::ios::binary);
        require(input.good(), "Cannot open the staged Lyra example.");
        const std::string source{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};

        GrammarDocument opened;
        opened.loadFromFile(argv[1]);
        require(opened.error_details.empty(), "File open failed to parse Lyra.");
        opened.addContext();
        opened.context->genPrimitives();
        opened.generateGeometry();
        const CharacterPose start = inspectCharacter(opened);
        const CharacterPose greeting = inspectCharacter(*compileSnapshot(source, 1.0)->scene_snapshot->grammarDocument());
        const CharacterPose loop = inspectCharacter(*compileSnapshot(source, 4.0)->scene_snapshot->grammarDocument());
        const float hand_travel = glm::length(greeting.hand_position - start.hand_position);
        require(std::isfinite(hand_travel) && hand_travel > 0.002f, "The left hand did not move at one second.");
        for (const auto &part : start.transforms) {
            const auto &end = loop.transforms.at(part.first);
            for (int column = 0; column < 4; ++column) {
                require(glm::length(part.second[column] - end[column]) < 1e-5f,
                        "Four-second loop did not close for " + part.first);
            }
        }
        checkSeparateDocuments();
        std::cout << "Lyra passed: 66 mesh instances, 58972 triangles, 11 materials, left-hand travel "
                  << hand_travel * 1000.0f << " mm at one second, four-second pose closure.\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "Lyra regression failed: " << error.what() << '\n';
        return 1;
    }
}
