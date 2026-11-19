#include "Context.h"
#include "GrammarRuntime.h"
#include "editor/service/SpatialObjectInspectionService.h"
#include "grammar.h"
#include "spatial/model/SpatialBuildingModel.h"

#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
constexpr double rad = 3.14159265358979323846 / 180;
const std::array<std::string, 3> ids{"OuterRing", "MiddleRing", "InnerRing"};
const std::array<double, 3> rates{15, -35, 50}, worldRates{15, -20, 30};
const std::array<double, 3> inertia{5.3792, 1.922, .441};

void check(bool value, const std::string &why)
{
	if (!value) throw std::runtime_error(why);
}

void near(double actual, double expected, const std::string &why, double tolerance = 2e-6)
{
	check(std::isfinite(actual) && std::abs(actual - expected) <= tolerance,
	      why + ": expected " + std::to_string(expected) + ", got " + std::to_string(actual));
}

std::unique_ptr<GrammarDocument> compile(const std::string &source, double seconds, bool valid = true)
{
	auto grammar = std::make_unique<GrammarDocument>();
	check(setGrammarEvaluationTime(grammar.get(), seconds), "Invalid test time");
	setGrammarEvaluationDiagnosticsQuiet(grammar.get(), true);
	std::istringstream input(source);
	grammar->loadFromStream(input, "coaxial drive regression");
	grammar->addContext();
	grammar->context->genPrimitives();
	grammar->generateGeometry();
	check(grammar->error_details.empty() == valid, grammar->error_details.empty()
	      ? "Expected invalid declaration" : grammar->error_details.front().message);
	return grammar;
}

SpatialObjectInspection inspect(const GrammarDocument &grammar, const std::string &id)
{
	auto model = grammar.context->spatialBuildingModel();
	check(bool(model), "Missing spatial model");
	return SpatialObjectInspectionService().inspectObject(*model, SpatialObjectId(id));
}

CoaxialDriveResponse solved(const SpatialObjectInspection &view)
{
	check(bool(view.drive_response), view.object_id + " " + view.drive_diagnostic);
	return *view.drive_response;
}

void checkSnapshot(const std::string &source, double t, double travel, double speed, double accel)
{
	auto grammar = compile(source, t);
	const auto &model = *grammar->context->spatialBuildingModel();
	check(grammar->context->primitive_instances.size() == 2210, "Photographic detail count");
	check(model.geometryBindings().size() == 2210, "Missing spatial ownership");
	check(!inspect(*grammar, "Pedestal").drive_response, "Fixed frame has a drive");
	const std::array<double, 3> initial{9, -70, 42};
	double bodyPower = 0, jointPower = 0, expectedEnergy = 0;
	for (int i = 0; i < 3; ++i) expectedEnergy += 0.5*inertia[i]*std::pow(worldRates[i]*rad*speed, 2);
	for (int i = 0; i < 3; ++i) {
		const auto view = inspect(*grammar, ids[i]);
		const auto r = solved(view);
		const auto &frame = model.objects().find(SpatialObjectId(ids[i]))->frameState().resolvedWorldTransform();
		const double yaw = (initial[i] + worldRates[i]*travel)*rad;
		near(frame[0].x, std::cos(yaw), "World pose cosine", 2e-5);
		near(frame[0].z, -std::sin(yaw), "World pose sine", 2e-5);
		near(r.hoopInertiaKgM2, inertia[i], "Hoop diameter-axis inertia");
		near(r.relativeRadPerS, rates[i]*rad*speed, "Joint rate");
		near(r.worldRadPerS, worldRates[i]*rad*speed, "World rate");
		near(r.worldRadPerS2, worldRates[i]*rad*accel, "World acceleration");
		near(r.motorRpm, rates[i]*speed*20/6, "Motor rpm", 1e-5);
		double expectedTorque = 0;
		for (int j = i; j < 3; ++j) expectedTorque += inertia[j]*worldRates[j]*rad*accel;
		near(r.hoopJointTorqueNm, expectedTorque, "Descendant inertial torque");
		near(r.chainHoopEnergyJ, expectedEnergy, "Chain energy");
		const auto picked = SpatialObjectInspectionService().inspectPrimitive(model, view.primitive_instance_indices.at(0));
		check(picked.object_id == ids[i], "Picked primitive owner");
		near(solved(picked).motorTorqueNm, r.motorTorqueNm, "Picked drive torque");
		bodyPower += r.worldRadPerS*r.worldRadPerS2*inertia[i];
		jointPower += r.hoopJointTorqueNm*r.relativeRadPerS;
	}
	near(bodyPower, jointPower, "Joint power equals rate of change of hoop energy");
}

const std::string declaration =
	"CoaxialDrive(hoop(2 1) motion(30 60) transmission(10 0.8 0.6) motor(0.01 0.02))";
const std::string box = "I(Cube material(amberwarmwood) 1 0.5)";
std::string object(const std::string &id, const std::string &body)
{
	return "Object(id(" + id + ") class(Rotor) taxonomy(Device Rotor)) [ " + body + " ]";
}

std::string edit(std::string s, const std::string &from, const std::string &to)
{
	auto at = s.find(from);
	check(at != std::string::npos, "Missing test substring " + from);
	s.replace(at, from.size(), to);
	return s;
}

void checkDriveTrain()
{
	auto single = [&](std::string d) {
		return solved(inspect(*compile("Start -> " + object("Ring", d + " " + box) + "\n", 0), "Ring"));
	};
	auto forward = single(declaration);
	near(forward.motorTorqueNm, (60*rad+.02)/8+.01*10*60*rad, "Forward efficiency and rotor inertia");
	const auto brake = single(edit(declaration, "30 60", "30 -60"));
	near(brake.motorTorqueNm, (-60*rad+.02)*.6/10-.01*10*60*rad, "Backdriving efficiency");
	check(brake.motorPowerW < 0, "Braking shaft power");
	const auto reverse = single(edit(declaration, "10 0.8", "-10 0.8"));
	near(reverse.motorRpm, -forward.motorRpm, "Reversing gear");
	near(reverse.motorPowerW, forward.motorPowerW, "Gear sense preserves power");
	const auto rest = single(edit(declaration, "30 60", "0 0"));
	near(rest.motorTorqueNm, 0, "Stationary centred hoop");
	const auto start = single(edit(declaration, "30 60", "0 60"));
	near(start.motorTorqueNm, forward.motorTorqueNm, "Starting in the commanded direction");
	near(start.motorPowerW, 0, "Zero mechanical power at rest");

	// Two moving carriers, signed ratio on the second motor, lossless gearing.
	std::string ideal = edit(edit(declaration, "10 0.8 0.6", "10 1 1"), "0.01 0.02", "0.01 0");
	const std::string child = edit(edit(ideal, "30 60", "-50 -15"), "10 1 1", "-7 1 1");
	auto grammar = compile("Start -> " + object("A", ideal + " " + object("Adapter", object("B",child+" "+box)))+"\n", 0);
	const auto a = solved(inspect(*grammar, "A")), b = solved(inspect(*grammar, "B"));
	near(b.rotorWorldRadPerS2, (60+105)*rad, "Rotor acceleration includes rotating carrier");
	near(b.rotorWorldRadPerS, (30+350)*rad, "Rotor velocity includes rotating carrier");
	near(a.gearboxOutputTorqueNm-a.hoopJointTorqueNm, .01*165*rad, "Parent carries child rotor torque");
	near(a.motorPowerW+b.motorPowerW,
	     a.worldRadPerS*a.worldRadPerS2+b.worldRadPerS*b.worldRadPerS2+
	     .01*(a.rotorWorldRadPerS*a.rotorWorldRadPerS2+b.rotorWorldRadPerS*b.rotorWorldRadPerS2),
	     "Motor shaft power equals derivative of hoop plus rotor energy");
	near(a.chainRotorEnergyJ, .005*(std::pow(300*rad,2)+std::pow(380*rad,2)), "Rotor energy in world frame");
}

void checkDiagnostics()
{
	auto unavailable = [&](const std::string &body, const std::string &reason) {
		const auto v = inspect(*compile("Start -> "+object("A",body)+"\n",0),"A");
		check(v.available && !v.drive_response && v.drive_diagnostic.find(reason) != std::string::npos,
		      "Unexpected drive diagnostic: " + v.drive_diagnostic);
	};
	unavailable(edit(declaration, "2 1", "0 1") + " " + box, "positive");
	unavailable(edit(declaration, "10 0.8", "0 0.8") + " " + box, "ratio");
	unavailable(edit(declaration, "0.8 0.6", "1.1 0.6") + " " + box, "efficiencies");
	unavailable(edit(declaration, "0.01 0.02", "-1 0.02") + " " + box, "nonnegative");
	const auto child = object("B",declaration + " " + box);
	unavailable(declaration+" [ A(5 0) "+child+" ]", "rigid");
	unavailable(declaration+" [ S(2 2 2) "+child+" ]", "rigid");
	unavailable(declaration+" [ T(0.1 0 0) "+child+" ]", "shaft line");
	unavailable(declaration+" "+child+" "+object("C",declaration+" "+box), "serial chain");
	for (const auto &invalid : {box+" "+declaration, object("A",box+" "+declaration+" "+declaration),
	                           object("A",edit(declaration,"hoop(2 1)", "hoop(2 1) hoop(2 1)")),
	                           object("A",edit(declaration,"hoop(2 1)", ""))}) {
		const auto grammar = compile("Start -> "+invalid+"\n",0,false);
		check(grammar->context->primitive_instances.empty(), "Invalid drive partially executed");
	}
}
}

int main()
{
	try {
		std::ifstream file("examples/Mechanics/CoaxialRingDisplay.p3d");
		check(bool(file), "Missing ring grammar");
		std::ostringstream source; source << file.rdbuf();
		checkDriveTrain();
		checkDiagnostics();
		checkSnapshot(source.str(), 0, 0, 0, .5);
		checkSnapshot(source.str(), 1, .25, .5, .5);
		checkSnapshot(source.str(), 2, 1, 1, 0);
		checkSnapshot(source.str(), 12, 11, 1, 0);
		checkSnapshot(source.str(), 72, 71, 1, -.5);
		checkSnapshot(source.str(), 73, 71.75, .5, -.5);
		checkSnapshot(source.str(), 74, 72, 0, .5);
		auto custom = compile(edit(source.str(), "15 -35 50", "16 -35 50"), 75);
		const double yaw = (9 + 16*72.25)*rad;
		const auto outer = inspect(*custom, "OuterRing");
		near(outer.resolved_world_transform[0], std::cos(yaw), "Edited speed carries travel across cycles", 2e-5);
		near(outer.resolved_world_transform[2], -std::sin(yaw), "Edited speed keeps world heading", 2e-5);
		std::cout << "Coaxial drive grammar, geometry, power and inspector checks passed.\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
