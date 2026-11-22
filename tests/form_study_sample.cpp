// Export one native grammar-time sample for the FormStudies lessons and tests.
#include "Context.h"
#include "GrammarRuntime.h"
#include "geometry/model/SweepDiskShapeSpecification.h"
#include "geometry/model/FacialSkinShapeSpecification.h"
#include "geometry/service/FacialSkinMeshGenerator.h"
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

namespace {

class NativeSceneObjWriter
{
public:
    void writeScene(std::ostream &out, const SceneGenerationContext &scene) const;

private:
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
            const auto landmarks = FacialSkinMeshGenerator().evaluateLandmarks(*face);
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
    if (argc != 4) {
        std::cerr << "Usage: form-study-sample grammar.p3d seconds output.obj\n";
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
