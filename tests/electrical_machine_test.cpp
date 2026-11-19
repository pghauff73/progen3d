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
        auto coupledSource = example(name);
        const auto startEnd = coupledSource.find('\n', coupledSource.find("Start ->"));
        coupledSource.insert(startEnd, "\n    [ T(1 0 0) BodyInertia(0.02 0.02 0.02) "
                                     "MotorShaft(Drive role(Rotor)) S(0.01 0.01 0.01) I(Cube material(tealmattemetal) 1 1) ]");
        auto coupled = build(coupledSource);
        auto &context = *coupled->context;
        const auto shaft = context.inspectMotorShaft("Drive").value();
        near(shaft.relativeRpm, 0, name + " starts from the body's actual speed");
        const float dt = .001f;
        context.stepSimulation(dt);
        const double liveRpm = context.inspectMotorShaft("Drive")->relativeRpm;
        require(liveRpm > 0, name + " accelerates a rigid body");
        if (!motor.cage) near(liveRpm*2*pi/60/dt, shaft.electrical.torque/.02, name + " acceleration");
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

std::string replaceOnce(std::string source, const std::string &from, const std::string &to) {
    const auto pos = source.find(from);
    require(pos != std::string::npos, "fixture field missing: " + from);
    return source.replace(pos, from.size(), to);
}

void checkReactionWheel() {
    const auto doc = build(example("ReactionWheel"));
    auto &context = *doc->context;
    auto &rotor = context.primitive_instances.at(0);
    auto &stator = context.primitive_instances.at(1);
    const auto shaft = context.inspectMotorShaft("Drive").value();
    require(!grammarUsesTime(doc.get()), "example must use physical playback");
    const double torque = shaft.electrical.torque;
    const float dt = .01f;
    rotor.sleeping = stator.sleeping = true;
    context.stepSimulation(dt);
    require(!rotor.sleeping && !stator.sleeping, "motor wakes both bodies");
    near(glm::radians(rotor.rotational_velocity.y)/dt, torque/.002, "rotor acceleration");
    near(glm::radians(stator.rotational_velocity.y)/dt, -torque/.008, "housing recoil");
    near(rotor.velocity.y, 0, "no linear rotor impulse");
    near(stator.velocity.y, 0, "no linear stator impulse");
    const double wRotor = glm::radians(rotor.rotational_velocity.y);
    const double wStator = glm::radians(stator.rotational_velocity.y);
    near(.002*wRotor, -.008*wStator, "pair angular momentum");
    near(.5*.002*wRotor*wRotor + .5*.008*wStator*wStator,
         torque*.5*(wRotor-wStator)*dt, "motor work becomes kinetic energy");
    const auto live = context.inspectMotorShaft("Drive").value();
    near(live.relativeRpm,(rotor.rotational_velocity.y-stator.rotational_velocity.y)/6, "live shaft RPM");
    near(live.electrical.convertedPower, torque*(wRotor-wStator), "relative-speed power");
    require(*live.electrical.lineBackEmf > 0, "back EMF rises from rest");
    near(std::abs(rotor.primary_transform[0][2] / glm::length(glm::vec3(rotor.primary_transform[0]))),
         std::sin(.5*torque/.002*dt*dt), "rotor orientation follows integrated acceleration");
    context.resetSimulation();
    near(context.inspectMotorShaft("Drive")->relativeRpm, 0, "reset speed");
    context.stepSimulation(dt);
    near(context.inspectMotorShaft("Drive")->relativeRpm, live.relativeRpm, "replay");
    stator.removed = true;
    require(!context.inspectMotorShaft("Drive"), "removed housing disconnects drive");
    const double before = rotor.rotational_velocity.y;
    context.stepSimulation(dt);
    near(rotor.rotational_velocity.y, before, "disconnected rotor coasts");
    context.resetSimulation();
    require(bool(context.inspectMotorShaft("Drive")), "reset reconnects housing");

    auto loaded = build(replaceOnce(example("ReactionWheel"), "MotorShaft(Drive role(Rotor)",
                                    "BodyTorque(0 -0.04 0) MotorShaft(Drive role(Rotor)"));
    loaded->context->stepSimulation(dt);
    near(glm::radians(loaded->context->primitive_instances[0].rotational_velocity.y)/dt,
         (torque-.04)/.002, "motor plus external load");
    auto tilted = build(replaceOnce(example("ReactionWheel"), "M(0.8)", "A(90 2) M(0.8)"));
    tilted->context->stepSimulation(dt);
    near(glm::radians(tilted->context->primitive_instances[0].rotational_velocity.x)/dt,
         -torque/.002, "rotated local shaft");
    near(glm::radians(tilted->context->primitive_instances[1].rotational_velocity.x)/dt,
         torque/.0012, "world reaction with different principal axis");
    auto reversed = build(replaceOnce(example("ReactionWheel"), "current(1.5)", "current(-1.5)"));
    reversed->context->stepSimulation(dt);
    near(reversed->context->primitive_instances[0].rotational_velocity.y, -before, "reverse current");
    std::cout << "Shaft test: torque " << torque << " N m; rotor " << wRotor/dt
              << " rad/s^2; housing " << wStator/dt << " rad/s^2\n";
}

void checkInductionDynamics() {
    const std::string source = example("InductionSpinUp");
    auto doc = build(source);
    auto &ctx = *doc->context;
    const double startTorque = ctx.inspectMotorShaft("Drive")->electrical.torque;
    require(startTorque > 0, "start torque");
    double previous = 0;
    for (int i=0; i<120; ++i) {
        ctx.stepSimulation(1.f/120);
        const double rpm = ctx.inspectMotorShaft("Drive")->relativeRpm;
        require(rpm >= previous-1e-3 && rpm < 1500.01, "induction acceleration approaches synchronous speed");
        previous = rpm;
    }
    require(previous > 100 && ctx.inspectMotorShaft("Drive")->electrical.torque < startTorque, "live slip feedback");
    near(ctx.primitive_instances[1].rotational_velocity.y, 0, "grounded support");
    // Zero leakage gives exact exponential slip decay. Check a stiff, finite step
    // against that analytic solution, then repeat with the rotor above synchronism.
    const auto resistive = replaceOnce(source, "barLeakage(0.000005)", "barLeakage(0)");
    for (const double rpm : {0., 1500., 2000.}) {
        auto rig = build(replaceOnce(resistive, "VR(0 0 0)", "VR(0 "+std::to_string(6*rpm)+" 0)"));
        auto &r = *rig->context;
        near(r.inspectMotorShaft("Drive")->relativeRpm, rpm, "authored rotor speed");
        const double rate = r.inspectMotorShaft("Drive")->electrical.torque;
        if (rpm == 2000.) require(rate < 0, "generator braking torque");
        const double seconds = .1f;
        double expected = rpm;
        if (rpm != 1500.) {
            const double inertia = r.primitive_instances[0].airflow->principalInertia.y;
            const double decay = rate / (inertia * (1500-rpm)*2*pi/60);
            expected = 1500 - (1500-rpm)*std::exp(-seconds*decay);
        }
        r.stepSimulation(seconds);
        near(r.inspectMotorShaft("Drive")->relativeRpm, expected, "exact resistive-cage transient");
        r.resetSimulation();
        near(r.inspectMotorShaft("Drive")->relativeRpm, rpm, "restore authored rotor speed");
    }
    std::cout << "Induction spin-up: " << previous << " rpm at 1 s; synchronous speed 1500 rpm\n";
}

void checkShaftBindings() {
    const std::string source = example("ReactionWheel");
    const std::string rotor = "MotorShaft(Drive role(Rotor) axis(0 1 0))";
    const std::string stator = "MotorShaft(Drive role(Stator))";
    for (const auto &bad : {
            replaceOnce(source, rotor, "MotorShaft(Missing role(Rotor))"),
            replaceOnce(source, rotor, "MotorShaft(Drive role(Other))"),
            replaceOnce(source, rotor, "MotorShaft(Drive role(Rotor) axis(0 0 0))"),
            replaceOnce(source, stator, rotor),
            replaceOnce(source, rotor, ""),
            replaceOnce(source, rotor, rotor+" "+stator),
            replaceOnce(source, stator, "MotorShaft(Drive role(Stator) axis(0 1 0))"),
            replaceOnce(source, "BodyInertia(0.0004 0.002 0.0018)", ""),
            replaceOnce(source, "I(Cube material(copperbrushedmetal)", "!I(Cube material(copperbrushedmetal)"),
            source+"    "+rotor+"\n"}) build(bad, false);
    auto single = build(replaceOnce(source, stator, ""));
    auto &ctx = *single->context;
    ctx.stepSimulation(.01f);
    require(ctx.primitive_instances[0].rotational_velocity.y > 0, "grounded by omission");
    near(ctx.primitive_instances[1].rotational_velocity.y, 0, "unbound body unaffected");

    const auto split = source.find("    [ T(0 0.035");
    const auto housing = source.find("    [ T(0 -0.035");
    const auto decl = source.find("    MagnetWire");
    const auto forward = source.substr(0, decl) + source.substr(housing) +
                         source.substr(split,housing-split) + source.substr(decl,split-decl);
    auto reordered = build(forward);
    reordered->context->stepSimulation(.01f);
    near(reordered->context->primitive_instances[1].rotational_velocity.y,
         ctx.primitive_instances[0].rotational_velocity.y, "forward reference and ordering");
    require(!build(example("SixStepOutrunner"))->context->inspectMotorShaft("Drive"),
            "prescribed-speed examples remain uncoupled");
    auto legacy = build("Start -> VR(0 12000 0) I(Cube material(charcoalmattemetal) 1 1)");
    near(legacy->context->primitive_instances.at(0).rotational_velocity.y, 720, "legacy speed limit");
}
}
int main() {
    try { checkFields(); checkMotors(); checkAuthoring(); checkReactionWheel(); checkInductionDynamics(); checkShaftBindings(); }
    catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
    std::cout << "Electrical machine checks passed.\n";
}
