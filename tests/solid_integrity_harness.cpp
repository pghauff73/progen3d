#include "geometry/service/SolidIntegrity.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void require(bool condition, const char *message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

sic::Mesh closed_tetrahedron()
{
    sic::Mesh mesh;
    mesh.id = "ClosedTetrahedron";
    mesh.assigned = true;
    mesh.vertices = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    mesh.faces = {{0, 2, 1}, {0, 1, 3}, {0, 3, 2}, {1, 2, 3}};
    return mesh;
}

void check_boundary_warning(const sic::Mesh &closed)
{
    sic::Mesh open = closed;
    open.id = "OpenTetrahedron";
    open.line = 7;
    open.column = 4;
    open.end_column = 24;
    open.faces.pop_back();

    const sic::Report report = sic::validate({closed, open});
    require(report.succeeded(), "An open mesh must not block compilation.");
    require(report.valid_meshes == 1 && report.contacts.empty(),
            "Only the closed mesh should participate in solid contact analysis.");
    require(report.diagnostics.size() == 1, "Expected one boundary warning.");
    const auto &warning = report.diagnostics.front();
    require(warning.code == "SIC001" && !warning.error && warning.id == open.id,
            "The boundary diagnostic must name the open mesh and be a warning.");
    require(warning.line == 7 && warning.column == 4 && warning.end_column == 24,
            "The warning must retain the declaration's source range.");
    require(warning.message.find("3 boundary edges") != std::string::npos &&
                warning.message.find("single incident triangle") != std::string::npos &&
                warning.message.find("missing caps or faces") != std::string::npos &&
                warning.message.find("seams") != std::string::npos,
            "The warning must describe the observed defect and possible causes.");
}

void check_invalid_geometry_remains_blocking(const sic::Mesh &closed)
{
    sic::Mesh damaged = closed;
    damaged.id = "NonmanifoldTetrahedron";
    damaged.vertices.push_back({0, 0, -1});
    damaged.faces.push_back({0, 1, 4});
    const auto branched = sic::validate({damaged});
    require(!branched.succeeded(), "A non-manifold edge must still block compilation.");
    require(std::any_of(branched.diagnostics.begin(), branched.diagnostics.end(),
                        [](const sic::Diagnostic &diagnostic) {
                            return diagnostic.code == "SIC002" && diagnostic.error;
                        }), "Expected a blocking non-manifold diagnostic.");

    damaged = closed;
    damaged.id = "DegenerateTetrahedron";
    damaged.faces.push_back({0, 0, 1});
    const auto degenerate = sic::validate({damaged});
    require(!degenerate.succeeded(), "A degenerate face must still block compilation.");
    require(std::any_of(degenerate.diagnostics.begin(), degenerate.diagnostics.end(),
                        [](const sic::Diagnostic &diagnostic) {
                            return diagnostic.code == "SIC003" && diagnostic.error;
                        }), "Expected a blocking invalid-geometry diagnostic.");
}

}

int main()
{
    const auto closed = closed_tetrahedron();
    const auto closed_report = sic::validate({closed});
    require(closed_report.succeeded() && closed_report.valid_meshes == 1 &&
                closed_report.diagnostics.empty(), "A closed solid should compile without warnings.");
    check_boundary_warning(closed);
    check_invalid_geometry_remains_blocking(closed);
    std::cout << "Solid integrity warning checks passed.\n";
}
