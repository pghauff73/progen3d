#include "Context.h"
#include "GrammarRuntime.h"
#include "grammar.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

namespace {
void require(bool condition, const std::string &label) {
    if (!condition) throw std::runtime_error(label);
}
void near(double actual, double expected, double tolerance, const std::string &label) {
    if (!(std::abs(actual-expected) <= tolerance))
        throw std::runtime_error(label + ": got " + std::to_string(actual) + ", expected " + std::to_string(expected));
}
std::unique_ptr<GrammarDocument> compile(std::istream &input, bool valid=true) {
    auto grammar = std::make_unique<GrammarDocument>();
    setGrammarEvaluationDiagnosticsQuiet(grammar.get(),true);
    grammar->loadFromStream(input,"flexible body test");
    grammar->addContext();
    grammar->context->genPrimitives();
    grammar->generateGeometry();
    if (valid && !grammar->error_details.empty()) throw std::runtime_error(grammar->error_details.front().message);
    if (!valid) {
        require(!grammar->error_details.empty(),"invalid flexible scene accepted");
        require(grammar->context->primitive_instances.empty(),"invalid scene partly emitted");
    }
    return grammar;
}
std::unique_ptr<GrammarDocument> scene(const std::string &actions, bool valid=true) {
    std::istringstream input("Start -> "+actions+"\n");
    return compile(input,valid);
}

void checkSurface(const Mesh &mesh) {
    std::map<std::pair<int,int>,int> counts, orientations;
    for (auto face : mesh.faces) {
        const auto a=mesh.vertices.at(face.x), b=mesh.vertices.at(face.y), c=mesh.vertices.at(face.z);
        require(glm::length(glm::cross(b-a,c-a))>0,"collapsed or nonfinite surface triangle");
        for (int j=0; j<3; ++j) {
            const int start=face[j], end=face[(j+1)%3];
            auto key=std::minmax(start,end);
            ++counts[key];
            orientations[key] += start < end ? 1 : -1;
        }
    }
    require(!counts.empty(),"empty mesh");
    for (auto &edge : counts) require(edge.second==2 && orientations[edge.first]==0,"open or reversed mesh seam");
}
void checkIntegratedScene() {
    std::ifstream input("examples/FlexibleBodies/WireBeltSheet.p3d");
    require(bool(input),"missing example");
    auto grammar=compile(input);
    auto &c=*grammar->context;
    require(c.primitive_instances.size()==3,"example requires three objects");
    const auto &wire=c.primitive_instances[0], &belt=c.primitive_instances[1], &sheet=c.primitive_instances[2];
    for (const auto &b : c.primitive_instances) {
        require(b.flexible_body.has_value() && b.mass>0,"flexible object attachment and mass");
        checkSurface(b.resolved_geometry->triangleMesh());
    }
    const float dummyVertices[288]={};
    const auto before=c.buildExportMesh(dummyVertices);
    const auto bounds=c.getInstanceBounds(sheet);
    const auto initialMin=bounds.min.z;
    auto run=[&]{ for (int i=0;i<60;++i) c.stepSimulation(1.f/120); };
    run();
    const auto &w=wire.flexible_body->particles(), &b=belt.flexible_body->particles(), &s=sheet.flexible_body->particles();
    require(w[10].position.z < w[10].restPosition.z-.005,"wire sags");
    require(s[6*9+4].position.z < -.01,"sheet sags");
    require(b[4*3+1].position.z < b[4*3+1].restPosition.z-.002,"belt span sags");
    for (const auto &p:b) for (double x:{0.,.65}) require(std::hypot(p.position.x-x,p.position.z)>.13999,"guide clearance");
    for (auto i:{0,20}) near(glm::distance(w[i].position,w[i].restPosition),0,0,"wire anchor");
    for (auto i:{0,8,108,116}) near(glm::distance(s[i].position,s[i].restPosition),0,0,"sheet anchor");
    for (const auto &part:c.primitive_instances) checkSurface(part.resolved_geometry->triangleMesh());
    require(c.getInstanceBounds(sheet).min.z < initialMin-.01,"deformed scene bounds");
    const auto after=c.buildExportMesh(dummyVertices);
    require(before.vertices!=after.vertices && before.faces==after.faces,"live export topology and deformation");
    const double sag=w[10].position.z;
    c.resetSimulation();
    require(c.buildExportMesh(dummyVertices).vertices==before.vertices,"reset mesh");
    run();
    near(wire.flexible_body->particles()[10].position.z,sag,1e-12,"deterministic replay");
}

void checkAxialBenchmark() {
    FlexibleBodySpecification spec;
    spec.material = {1000,100000,12};
    spec.dimensions = glm::dvec3(1,.01,0);
    spec.grid = glm::ivec2(4,0);
    spec.pins = {{0,0}};
    // Distributed self-weight in the axial direction. Tip extension = rho*g*L^2/(2E).
    for (const double modulus : {100000., 200000.}) {
        spec.material.youngModulus = modulus;
        for (const int frequency : {60,120}) {
            FlexibleBody wire(FlexibleTopology::Wire,spec);
            near(wire.totalMass(),1000*3.141592653589793*.005*.005,1e-12,"wire mass");
            for (int i=0; i<4*frequency; ++i) wire.advance(1./frequency,{9.81,0,0});
            const double extension=wire.particles().back().position.x-1;
            near(extension,1000*9.81/(2*modulus),.00035,"self-weight axial extension");
            std::cout << "E=" << modulus << " Pa, " << frequency << " Hz: extension " << extension << " m\n";
        }
    }
}
void checkWireBending() {
    FlexibleBodySpecification spec;
    spec.material = {1000,1e7,14};
    spec.grid = glm::ivec2(2,0);
    spec.pins = {{0,0},{1,0}};
    // One free tip and one angular spring. For small rotations, delta = W*l^3/(EI).
    for (double diameter : {.01,.02}) {
        spec.dimensions = glm::dvec3(.1,diameter,0);
        FlexibleBody wire(FlexibleTopology::Wire,spec);
        for (int i=0;i<1200;++i) wire.advance(1./240,{0,0,-9.81});
        const auto &tip = wire.particles().back();
        const double inertia = 3.141592653589793*std::pow(diameter,4)/64;
        const double predicted = tip.mass*9.81*std::pow(.05,3)/(spec.material.youngModulus*inertia);
        near(-tip.position.z,predicted,predicted*.01,"angular bending equilibrium");
    }
}
void checkBeltCirculation() {
    auto grammar = scene("FlexBeltSpeed(0.3) I(FlexibleBelt 0 1 1)");
    auto &context = *grammar->context;
    const auto &body = *context.primitive_instances.front().flexible_body;
    const auto &upper = body.particles()[4*2], &lower = body.particles()[20*2];
    near(upper.velocity.x, .3, 1e-7, "upper run circulation");
    near(lower.velocity.x, -.3, 1e-7, "return run circulation");
    context.stepSimulation(.01f);
    require(upper.position.x>upper.restPosition.x && lower.position.x<lower.restPosition.x,"belt travels around the loop");
    context.resetSimulation();
    near(lower.velocity.x,-.3,1e-7,"belt circulation resets");
}
void checkSceneSemantics() {
    auto fixture=scene("G(2 0 0 -1) [ T(1 2 3) A(90 0) S(2 2 2) P(1000) FlexDimensions(1 0.5 0.01) I(ElasticSheet 0 1 1) ] M(3) V(1 0 0) I(FlexibleWire 0 1 1) !I(ElasticSheet 0 1 1) I(Cube 0 1 1)");
    auto &c=*fixture->context;
    require(c.primitive_instances.size()==4,"mixed scene");
    auto &sheet=c.primitive_instances[0], &wire=c.primitive_instances[1], &fixed=c.primitive_instances[2];
    near(sheet.mass,40,.0001,"scaled sheet mass and density");
    near(wire.mass,3,1e-7,"mass override");
    const auto before=c.getInstanceCenter(sheet);
    c.stepSimulation(.01f);
    const auto after=c.getInstanceCenter(sheet);
    require(after.z<before.z && std::abs(after.y-before.y)<1e-5,"gravity in rotated placement");
    require(wire.flexible_body->particles().front().position.x>.009,"initial velocity");
    for (const auto &p:fixed.flexible_body->particles()) require(p.position==p.restPosition,"immovable object");
    require(!c.primitive_instances[3].flexible_body,"rigid neighbour remains rigid");
}
void checkRejections() {
    const std::string prefix="I(Cube 0 1 1) ";
    for (const std::string &actions : {
        "FlexGrid(2.5 0) I(FlexibleWire 0 1 1)", "FlexDimensions(0 1 1) I(ElasticSheet 0 1 1)",
        "FlexPin(32 0) I(FlexibleBelt 0 1 1)", "FlexGrid(7 1) I(FlexibleBelt 0 1 1)",
        "FlexGuideRadius(0.1) I(ElasticSheet 0 1 1)", "FlexMaterial(1 0 0) I(FlexibleWire 0 1 1)",
        "FlexPin(0 2) I(FlexibleWire 0 1 1)", "FlexGrid(128 32) I(ElasticSheet 0 1 1)",
        "FlexBeltSpeed(1) I(FlexibleWire 0 1 1)", "FlexPin(0 0)", "FlexPin(0 0) I(Cube 0 1 1)", "S(1 2 3) I(ElasticSheet 0 1 1)",
        "BodyInertia(1 1 1) I(FlexibleWire 0 1 1)", "[ FlexPin(0 0) ] I(FlexibleWire 0 1 1)"})
        scene(prefix+actions,false);
}
}

int main() {
    try {
        checkAxialBenchmark();
        checkIntegratedScene();
        checkWireBending();
        checkBeltCirculation();
        checkSceneSemantics();
        checkRejections();
        std::cout << "Flexible body checks passed.\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
