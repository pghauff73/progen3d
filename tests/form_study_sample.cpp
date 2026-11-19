// Export one native grammar-time sample for the FormStudies lessons and tests.
#include "Context.h"
#include "GrammarRuntime.h"
#include "geometry/model/SweepDiskShapeSpecification.h"
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

void writePoint(std::ostream &out, const glm::vec3 &point)
{
    out << point.x << ' ' << point.y << ' ' << point.z;
}

void writeScene(std::ostream &out, const SceneGenerationContext &scene)
{
    const auto cube = build_base_vertex_data();
    std::size_t firstVertex = 1;
    std::size_t index = 0;
    out << std::setprecision(std::numeric_limits<float>::max_digits10);
    for (const auto &part : scene.primitive_instances) {
        out << "o part_" << index++ << "\nusemtl " << part.material_name << '\n';
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
        for (auto i = firstVertex; i < end; i += 3) {
            out << "f " << i << ' ' << i + 1 << ' ' << i + 2 << '\n';
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
        writeScene(output, *grammar.context);
        output.close();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
