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
#include <sstream>
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
    float largest_seam_gap = 0.0f;
};

using MeshSeam = std::vector<std::pair<std::string, std::size_t>>;

std::vector<MeshSeam> readCharacterSeams(const std::string &source)
{
    std::vector<MeshSeam> seams;
    std::istringstream lines(source);
    for (std::string line; std::getline(lines, line);) {
        if (line.rfind("#@mesh-seam-v1 ", 0) != 0) continue;
        std::istringstream row(line);
        std::string tag, key;
        std::size_t index;
        row >> tag;
        MeshSeam seam;
        while (row >> key >> index) seam.emplace_back(key, index);
        require(seam.size() >= 2, "A Lyra seam point needs at least two vertices.");
        seams.push_back(std::move(seam));
    }
    require(seams.size() == 627, "Expected all 627 corresponding seam points.");
    return seams;
}

CharacterPose inspectCharacter(const GrammarDocument &document, const std::vector<MeshSeam> &seams)
{
    require(grammarUsesTime(&document), "The character must respond to the timeline.");
    const auto &instances = document.context->primitive_instances;
    require(instances.size() == 66, "Expected all 66 body and material mesh instances.");
    std::size_t triangles = 0;
    std::set<std::string> materials;
    std::map<std::string, const ScenePrimitiveInstance *> instances_by_key;
    CharacterPose pose;
    for (const auto &instance : instances) {
        const auto *reference = dynamic_cast<const GeneratedMeshReferenceShapeSpecification *>(
            instance.shape_specification.get());
        require(reference && instance.resolved_geometry && instance.resolved_geometry->hasTriangleMesh(),
                "A character part did not resolve to its triangle mesh.");
        require(pose.transforms.emplace(reference->meshKey(), instance.primary_transform).second,
                "Unexpected repeated mesh key " + reference->meshKey());
        instances_by_key.emplace(reference->meshKey(), &instance);
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
    for (const auto &seam : seams) {
        const auto worldPosition = [&](const auto &vertex) {
            const auto &instance = *instances_by_key.at(vertex.first);
            const auto &position = instance.resolved_geometry->triangleMesh().vertices.at(vertex.second);
            return glm::vec3(instance.primary_transform * glm::vec4(position, 1.0f));
        };
        const auto first = worldPosition(seam.front());
        for (const auto &vertex : seam) {
            const float gap = glm::length(worldPosition(vertex) - first);
            require(std::isfinite(gap) && gap < 1e-6f, "Open seam at " + vertex.first);
            pose.largest_seam_gap = std::max(pose.largest_seam_gap, gap);
        }
    }
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
    invalid.loadFromSourceText(rule + header + "30" + end + "#@mesh-seam-v1 Panel 0 Missing 0\n");
    require(!invalid.error_details.empty(), "A seam referencing an unknown mesh must be diagnosed.");
}

} // namespace

int main(int argc, char **argv)
{
    try {
        require(argc == 2, "Pass the path to the staged Lyra_Greeting.p3d.");
        std::ifstream input(argv[1], std::ios::binary);
        require(input.good(), "Cannot open the staged Lyra example.");
        const std::string source{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
        const auto seams = readCharacterSeams(source);

        GrammarDocument opened;
        opened.loadFromFile(argv[1]);
        require(opened.error_details.empty(), "File open failed to parse Lyra.");
        opened.addContext();
        opened.context->genPrimitives();
        opened.generateGeometry();
        const CharacterPose start = inspectCharacter(opened, seams);
        float largest_seam_gap = start.largest_seam_gap;
        CharacterPose greeting, loop;
        for (int frame = 1; frame <= 16; ++frame) {
            const auto result = compileSnapshot(source, frame / 4.0);
            auto pose = inspectCharacter(*result->scene_snapshot->grammarDocument(), seams);
            largest_seam_gap = std::max(largest_seam_gap, pose.largest_seam_gap);
            if (frame == 4) greeting = std::move(pose);
            if (frame == 16) loop = std::move(pose);
        }
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
                  << hand_travel * 1000.0f << " mm at one second, four-second pose closure, maximum seam gap "
                  << largest_seam_gap * 1000.0f << " mm across 17 poses.\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "Lyra regression failed: " << error.what() << '\n';
        return 1;
    }
}
