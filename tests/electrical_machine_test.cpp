#include "Context.h"
#include "GrammarRuntime.h"
#include "editor/controller/PreviewCameraController.h"
#include "electrical/service/ElectricalFieldEvaluator.h"
#include "grammar.h"
#include "lighting/model/LightingSceneDefinition.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
constexpr double pi = 3.14159265358979323846;
void require(bool condition, const std::string &message) {
    if (!condition) throw std::runtime_error(message);
}
void near(double value, double expected, const std::string &what) {
    const double tolerance = 2e-6 * std::max(1e-10, std::abs(expected));
    require(std::isfinite(value) && std::abs(value - expected) <= tolerance,
            what + ": " + std::to_string(value) + " vs " + std::to_string(expected));
}
std::unique_ptr<GrammarDocument> build(const std::string &source, bool valid = true, double time = 0) {
    auto doc = std::make_unique<GrammarDocument>();
    setGrammarEvaluationDiagnosticsQuiet(doc.get(), true);
    setGrammarEvaluationTime(doc.get(), time);
    doc->loadFromSourceText(source, "electrical machine");
    doc->addContext();
    doc->context->genPrimitives();
    doc->generateGeometry();
    if (valid && !doc->error_details.empty()) throw std::runtime_error(doc->error_details.front().message);
    if (!valid) {
        require(!doc->error_details.empty() && doc->error_details.front().line >= 0, "expected a source diagnostic");
        const auto &model = doc->context->lightingSceneDefinition().electricalControlGraph().machines();
        require(model.wires.empty() && model.coils.empty() && model.magnets.empty() && model.motors.empty() &&
                doc->context->primitive_instances.empty(), "partial scene committed");
    }
    return doc;
}
std::string example(const std::string &name) {
    std::ifstream in("examples/ElectricalMachines/" + name + ".p3d");
    require(bool(in), "missing example " + name);
    std::ostringstream source;
    source << in.rdbuf();
    return source.str();
}
const ElectricalMachineModel &model(const GrammarDocument &doc) {
    return doc.context->lightingSceneDefinition().electricalControlGraph().machines();
}
void checkFields() {
    const auto doc = build(example("GappedFieldCoil"));
    const auto &scene = model(*doc);
    const auto coil = scene.coils.at("Pickup");
    const auto wire = scene.wires.at("Wire");
    require(doc->context->primitive_instances.size() == 45, "visible coil turns and core");
    const auto bounds = doc->context->getSceneBounds();
    PreviewCameraController camera;
    camera.fitToBounds(0, 0, 0, bounds.half_extents.x, bounds.half_extents.y,
                       bounds.half_extents.z, 500, 480, PreviewProjectionConfiguration{});
    require(camera.distance() > .03 && camera.distance() < .25, "fit real-size electrical parts");
    auto sample = ElectricalFieldEvaluator::winding(coil, wire);
    const double length = 40 * std::hypot(2*pi*.008, .00066);
    near(sample.wireLength, length, "helical length");
    near(sample.resistance, length / (58e6 * pi * .0006 * .0006 / 4), "wire R");
    near(sample.conductorField * sample.wireLength, 1.6 * sample.resistance, "line integral of E");
    near(sample.insulationField, 80000, "electric field across insulation");
    near(sample.displacement, 8.8541878188e-12 * 3.4 * 80000, "dielectric displacement");
    near(sample.gapField * .001 + sample.coreField * .119, 40 * 1.6, "Ampere loop");
    near(sample.inductance * 1.6, 40 * sample.flux, "flux linkage");
    near(sample.storedEnergy, .5 * sample.inductance * 1.6 * 1.6, "field energy");
    require(!sample.beyondCoreLimit, "ordinary N87 flux flagged");
    auto changedCoil = coil;
    changedCoil.current = 100;
    require(ElectricalFieldEvaluator::winding(changedCoil, wire).beyondCoreLimit, "saturation flag");
    changedCoil.current = 0;
    near(ElectricalFieldEvaluator::winding(changedCoil, wire).inductance, sample.inductance, "zero-current L");
    auto hot = wire;
    hot.temperature += 50;
    near(hot.resistancePerMetre() / wire.resistancePerMetre(), 1 + .00393*50, "temperature coefficient");
    hot = wire;
    hot.conductor = ElectricalMaterialCatalog::conductor("Aluminium");
    require(hot.resistancePerMetre() > wire.resistancePerMetre(), "aluminium resistance");
    changedCoil = coil;
    changedCoil.core = ElectricalMaterialCatalog::core("Air");
    require(ElectricalFieldEvaluator::winding(changedCoil, wire).fluxDensity < sample.fluxDensity, "core substitution");
    changedCoil = coil;
    changedCoil.current = -coil.current;
    near(ElectricalFieldEvaluator::winding(changedCoil, wire).flux, -sample.flux, "coil polarity");
    std::cout << "Coil " << sample.resistance << " ohm, " << sample.inductance*1000 << " mH, "
              << sample.fluxDensity << " T, " << sample.ohmicPower << " W\n";
}

void checkMotors() {
    for (const std::string name : {"BrushedDC", "RadialInrunner", "RadialOutrunner", "SixStepOutrunner", "AxialFlux", "InductionCage"}) {
        const auto doc = build(example(name));
        const std::map<std::string, std::size_t> partCounts{
            {"BrushedDC",27}, {"RadialInrunner",27}, {"RadialOutrunner",42}, {"SixStepOutrunner",42},
            {"AxialFlux",37}, {"InductionCage",53}
        };
        require(doc->context->primitive_instances.size() == partCounts.at(name), name + " geometry");
        const auto &scene = model(*doc);
        auto motor = scene.motors.at("Drive");
        const auto point = ElectricalFieldEvaluator::motor(motor, scene);
        const auto &coil = scene.coils.at(motor.windingId);
        const auto &wire = scene.wires.at(coil.wireId);
        const double r = ElectricalFieldEvaluator::winding(coil, wire).resistance;
        double lossFactor = 1.5;
        if (name == "BrushedDC") lossFactor = 1;
        if (name == "SixStepOutrunner") lossFactor = 2;
        near(point.windingLoss, lossFactor*r*coil.current*coil.current, "winding loss");
        require(point.torque > 0 && point.convertedPower > 0, name + " positive motoring point");
        if (motor.cage) {
            near(*point.synchronousRpm, 1500, "synchronous speed");
            near(*point.slip, 1.0/30, "slip");
            near(*point.rotorFrequency, 50.0/30, "slip frequency");
            near(point.torque * 1500*2*pi/60, point.convertedPower + *point.rotorLoss, "induction power balance");
            motor.rpm = 1500;
            auto sync = ElectricalFieldEvaluator::motor(motor, scene);
            near(sync.torque, 0, "synchronous torque"); near(*sync.rotorLoss, 0, "synchronous loss");
            motor.rpm = 1600;
            auto gen = ElectricalFieldEvaluator::motor(motor, scene);
            require(gen.torque < 0 && *gen.rotorLoss > 0, "generating torque and rotor loss");
            motor.rpm = 0;
            auto stall = ElectricalFieldEvaluator::motor(motor, scene);
            near(*stall.rotorLoss, stall.torque * 1500*2*pi/60, "stall rotor power");
            motor.cage->conductor = ElectricalMaterialCatalog::conductor("Copper");
            require(ElectricalFieldEvaluator::motor(motor, scene).torque != stall.torque, "bar materials affect torque");
        } else {
            auto magnet = scene.magnets.at(motor.magnetId);
            const auto field = ElectricalFieldEvaluator::magnet(magnet);
            near(-field.magnetField*magnet.length, field.gapField*(magnet.gap + magnet.returnLength/magnet.returnCore->relativePermeability), "PM loop");
            near(field.fluxDensity, magnet.material->remanence / (1 + magnet.material->recoilPermeability *
                 (magnet.gap + magnet.returnLength/magnet.returnCore->relativePermeability)/magnet.length), "PM load line");
            magnet.polarity = -1;
            near(ElectricalFieldEvaluator::magnet(magnet).flux, -field.flux, "magnet reversal");
            double electricalPower = *point.lineBackEmf * point.lineCurrent * (name == "BrushedDC" ? 1 : std::sqrt(3.0));
            if (name == "SixStepOutrunner") {
                electricalPower = *point.lineBackEmf * coil.current;
                near(point.lineCurrent, std::sqrt(2.0/3.0)*coil.current, "120-degree conduction RMS");
                near(*point.lineBackEmf, 2*(12/(5*pi))*motor.polePairs*coil.turns*motor.windingFactor*field.flux*motor.rpm*2*pi/60,
                     "trapezoid plateau voltage");
            }
            near(point.convertedPower, electricalPower, "ideal conversion power");
            auto reversed = scene;
            reversed.magnets.at(motor.magnetId).polarity = -1;
            near(ElectricalFieldEvaluator::motor(motor, reversed).torque, -point.torque, "PM torque reversal");
            auto stopped = motor;
            stopped.rpm = 0;
            const auto atRest = ElectricalFieldEvaluator::motor(stopped, scene);
            near(atRest.torque, point.torque, "current-imposed stall torque");
            near(*atRest.lineBackEmf, 0, "zero-speed EMF");
            if (name != "BrushedDC" && name != "SixStepOutrunner") {
                motor.connection = WindingConnection::Delta;
                const auto delta = ElectricalFieldEvaluator::motor(motor, scene);
                motor.connection = WindingConnection::Wye;
                const auto wye = ElectricalFieldEvaluator::motor(motor, scene);
                near(*wye.lineBackEmf / *delta.lineBackEmf, std::sqrt(3.0), "wye/delta voltage");
                near(delta.lineCurrent / wye.lineCurrent, std::sqrt(3.0), "delta/wye current");
            }
        }
        std::cout << name << " " << point.torque << " N m, " << point.convertedPower << " W converted\n";
    }
}
void checkAuthoring() {
    const std::string wire = "MagnetWire(Cu material(Copper) insulation(Polyimide) diameter(0.001) thickness(0.00002))";
    const std::string coil = "FieldCoil(C wire(Cu) core(Air) turns(10) turnLength(0.1) coreLength(0.1) area(0.0001) gap(0) current(t))";
    const std::string cube = " I(Cube material(tealmattemetal) 1 1) ";
    const std::string source = "Start -> " + coil + " " + wire + cube + "LightSwitch(S1)";
    const auto doc = build(source, true, 2.5);
    near(model(*doc).coils.at("C").current, 2.5, "time expression and forward wire reference");
    require(grammarUsesTime(doc.get()), "electrical time expression recognised");
    auto graph = doc->context->lightingSceneDefinition().electricalControlGraph();
    require(graph.switches().size() == 1, "lighting coexists");
    graph.clear();
    require(graph.machines().wires.empty() && graph.switches().empty(), "clear resets graph");
    for (const std::string bad : {wire + " " + wire, coil,
         std::string("MagnetWire(X material(Steel) insulation(Air) diameter(1) thickness(1))"),
         std::string("MagnetWire(X material(Copper) insulation(Air) diameter(0) thickness(1))"),
         std::string("FieldCoil(X turns(2) turns(3))"), std::string("MagnetWire(X mystery(1))"),
         std::string("MagnetWire(X material(Copper))")}) {
        build("Start -> " + cube + bad, false);
    }
    auto sourceWith = [](std::string source, const std::string &from, const std::string &to) {
        source.replace(source.find(from), from.size(), to); return source;
    };
    build(sourceWith(source, "turns(10)", "turns(1.5)"), false);
    build(sourceWith(source, "core(Air)", "core(Unknown)"), false);
    build(sourceWith(source, "current(t)", "current(unknownValue)"), false);
    build(sourceWith(example("InductionCage"), "frequency(50)", "frequency(0)"), false);
    build(sourceWith(example("RadialOutrunner"), "magnet(Pole)", "magnet(Absent)"), false);
    build(sourceWith(example("BrushedDC"), "connection(Armature)", "connection(Wye)"), false);
    build(sourceWith(example("AxialFlux"), "rpm(rpm)", "rpm(rpm) barEmf(0)"), false);
    build(sourceWith(example("SixStepOutrunner"), "connection(Wye)", "connection(Delta)"), false);
    auto legacy = build("Start -> FieldCoil(2)\nFieldCoil(n) -> S(n 1 1)" + cube);
    require(legacy->context->primitive_instances.size() == 1, "legacy positional rule collision");
    build("Start ->" + cube);
}
}
int main() {
    try { checkFields(); checkMotors(); checkAuthoring(); }
    catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
    std::cout << "Electrical machine checks passed.\n";
}
