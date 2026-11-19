#include "Context.h"
#include "grammar.h"
#include "GrammarRuntime.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
void require(bool condition, const char *message)
{
	if (!condition) throw std::runtime_error(message);
}

void near(double value, double expected, double tolerance, const char *message)
{
	if (!(std::abs(value - expected) <= tolerance)) {
		std::cerr << message << ": measured " << value << ", expected " << expected << '\n';
		throw std::runtime_error(message);
	}
}

void load(GrammarDocument &grammar, std::istream &input)
{
	setGrammarEvaluationDiagnosticsQuiet(&grammar, true);
	grammar.loadFromStream(input, "surface-contact-check");
	if (grammar.error_details.empty()) {
		grammar.addContext();
		grammar.context->genPrimitives();
		grammar.generateGeometry();
	}
}

void compile(GrammarDocument &grammar, const std::string &source)
{
	std::istringstream input(source);
	load(grammar, input);
	for (const auto &issue : grammar.error_details) std::cerr << issue.message << '\n';
	require(grammar.error_details.empty(), "grammar failed");
}

double energy(const Context &scene)
{
	double total = 0;
	for (const auto &part : scene.primitive_instances) {
		const auto m = scene.getSurfaceMotionBalance(part);
		total += m.translation_energy + m.rotation_energy + m.gravity_potential +
		         part.surface_state.elastic_energy + part.surface_state.friction_heat;
	}
	return total;
}

glm::dvec3 angularMomentum(const Context &scene)
{
	glm::dvec3 total(0);
	for (const auto &body : scene.primitive_instances) {
		const auto motion = scene.getSurfaceMotionBalance(body);
		total += motion.spin_momentum + glm::cross(glm::dvec3(scene.getInstanceCollisionGeometry(body).center_of_mass),
		                                          motion.linear_momentum);
	}
	return total;
}

void impact(double dt)
{
	GrammarDocument grammar;
	compile(grammar, "Start -> [ T(-0.52 0 0) V(1 0 0) Body ] [ T(0.52 0 0) V(-1 0 0) Body ]\n"
	                 "Body -> P(2) ContactSurface(4000 0 0) I(Cube material(plaster))");
	auto &scene = *grammar.context;
	auto &a = scene.primitive_instances.at(0);
	auto &b = scene.primitive_instances.at(1);
	near(a.surface_state.volume, 1, 1e-6, "box volume");
	near(a.mass, 2, 1e-6, "density mass");
	near(a.surface_state.inertia[2][2], 1.0/3, 1e-6, "box inertia");
	double compression = 0, peak_stored = 0;
	for (int i=0; i<std::lround(0.12/dt); ++i) {
		scene.stepSimulation(static_cast<float>(dt));
		compression = std::max(compression, a.surface_state.normal_compression);
		peak_stored = std::max(peak_stored, a.surface_state.elastic_energy+b.surface_state.elastic_energy);
		near(glm::length(a.surface_state.force+b.surface_state.force),0,1e-6,"equal opposite force");
	}
	near(compression, 0.02236068, 0.0002, "analytical peak compression per layer");
	near(peak_stored,2.0,0.04,"kinetic to elastic potential");
	near(a.velocity.x,-1,0.005,"elastic rebound"); near(b.velocity.x,1,0.005,"elastic rebound");
	near(energy(scene),2,0.015,"elastic energy after separation");
	std::cout << "elastic: dt=" << dt << " peak layer=" << compression << " m, energy=" << energy(scene) << " J\n";
}

void obliqueImpact()
{
	GrammarDocument grammar;
	std::ifstream input("examples/surface_contact_transfer.p3d");
	require(input.good(),"missing demo grammar"); load(grammar,input);
	require(grammar.error_details.empty(),"demo did not compile");
	auto &scene = *grammar.context;
	auto &a = scene.primitive_instances.at(0);
	auto &b = scene.primitive_instances.at(1);
	const auto initial_p = scene.getSurfaceMotionBalance(a).linear_momentum;
	const auto initial_l = angularMomentum(scene);
	const double initial_energy = energy(scene);
	near(a.surface_material.normal_stiffness,4000,0.001,"Young modulus to normal stiffness");
	near(a.surface_material.shear_stiffness,1600,0.001,"Poisson ratio to shear stiffness");
	const double start_temp = a.surface_state.temperature;
	const double dt=0.00025;
	double max_shear = 0, max_torque=0;
	for (int i=0; i<1200; ++i) {
		scene.stepSimulation(dt);
		max_shear = std::max(max_shear, a.surface_state.shear_displacement);
		max_torque = std::max(max_torque, glm::length(a.surface_state.torque));
	}
	const double heat = a.surface_state.friction_heat + b.surface_state.friction_heat;
	require(heat>0 && max_shear>0 && max_torque>0 && glm::length(a.rotational_velocity)>0, "glancing transfer");
	require(a.surface_state.temperature < start_temp && b.surface_state.temperature>295, "hot to cold transfer");
	near(a.surface_state.conducted_heat,-b.surface_state.conducted_heat,1e-9,"conduction conservation");
	double thermal_change = 0;
	for (const auto &body : scene.primitive_instances)
		thermal_change += body.mass * body.surface_material.specific_heat *
		                  (body.surface_state.temperature - body.surface_material.initial_temperature);
	near(thermal_change,heat,1e-8,"calorimetry");
	near(glm::length(scene.getSurfaceMotionBalance(a).linear_momentum+scene.getSurfaceMotionBalance(b).linear_momentum-initial_p),0,4e-5,"linear momentum");
	near(glm::length(angularMomentum(scene)-initial_l),0,1e-4,"angular momentum about origin");
	near(energy(scene),initial_energy,0.012*initial_energy,"mechanical plus dissipated energy");
	std::cout << "oblique: torque=" << max_torque << " Nm, peak shear=" << max_shear
	          << " m, dissipated=" << heat << " J, conducted=" << b.surface_state.conducted_heat
	          << " J, energy residual=" << energy(scene)-initial_energy << " J\n";
	scene.resetSimulation();
	near(a.surface_state.temperature,start_temp,0,"reset temperature");
	near(a.surface_state.friction_heat,0,0,"reset heat"); near(glm::length(a.rotational_velocity),0,0,"reset spin");
	for(int i=0; i<1200; ++i) scene.stepSimulation(dt);
	near(a.surface_state.friction_heat+b.surface_state.friction_heat,heat,1e-9,"deterministic replay");
}

double toothTransfer(double dt)
{
	GrammarDocument grammar;
	std::ifstream input("examples/gear_surface_transfer.p3d");
	require(input.good(),"missing gear grammar"); load(grammar,input);
	for (const auto &issue : grammar.error_details) std::cerr << issue.message << '\n';
	require(grammar.error_details.empty(),"gear grammar did not compile");
	auto &scene = *grammar.context;
	auto &pinion = scene.primitive_instances.at(0);
	auto &wheel = scene.primitive_instances.at(1);
	const double initial = energy(scene);
	const auto pinion_center = scene.getInstanceCollisionGeometry(pinion).center_of_mass;
	const auto wheel_center = scene.getInstanceCollisionGeometry(wheel).center_of_mass;
	require(initial>0 && wheel.rotational_velocity.z==0,"gear start");
	double impulse=0, peak=0, work_on_wheel=0;
	double peak_wheel_speed=0, max_energy_error=0;
	for (int i=0; i<std::lround(0.04/dt); ++i) {
		const double old_spin = glm::radians(double(wheel.rotational_velocity.z));
		scene.stepSimulation(dt);
		impulse += wheel.surface_state.torque.z*dt;
		work_on_wheel += 0.5*(old_spin + glm::radians(double(wheel.rotational_velocity.z)))*wheel.surface_state.torque.z*dt;
		peak = std::max(peak,glm::length(wheel.surface_state.force));
		peak_wheel_speed = std::min(peak_wheel_speed,double(wheel.rotational_velocity.z));
		max_energy_error = std::max(max_energy_error,std::abs(energy(scene)-initial));
	}
	const auto wheel_motion = scene.getSurfaceMotionBalance(wheel);
	std::cout << "gear: pinion=" << pinion.rotational_velocity.z << " deg/s, wheel=" << wheel.rotational_velocity.z
	          << " deg/s, peak tooth force=" << peak << " N, wheel work=" << work_on_wheel
	          << " J, peak wheel speed=" << peak_wheel_speed << " deg/s, max energy residual=" << max_energy_error << " J, initial=" << initial << " J, residual=" << energy(scene)-initial << " J\n";
	require(peak>0 && peak_wheel_speed < -10 && wheel_motion.rotation_energy >= 0,"wheel must receive tooth torque");
	near(wheel_motion.spin_momentum.z,impulse,2e-8,"wheel angular impulse");
	near(wheel_motion.rotation_energy,work_on_wheel,1e-8,"wheel work-energy");
	near(glm::length(scene.getInstanceCollisionGeometry(pinion).center_of_mass-pinion_center),0,2e-6,"pinion bearing");
	near(glm::length(scene.getInstanceCollisionGeometry(wheel).center_of_mass-wheel_center),0,2e-6,"wheel bearing");
	near(max_energy_error,0,0.012*initial,"gear energy balance over both flanks");
	near(wheel.surface_state.friction_heat,0,0,"frictionless tooth heat");
	return max_energy_error / initial;
}

void heatBetweenFixedBodies()
{
	GrammarDocument grammar;
	compile(grammar,
	        "Start -> [ T(-0.49 0 0) M(2) ContactThermal(340 100 40) !I(Cube material(plaster)) ] "
	        "[ T(0.49 0 0) M(1) ContactThermal(300 100 40) !I(Cube material(plaster)) ]");
	auto &scene = *grammar.context;
	for (int i = 0; i < 100; ++i) scene.stepSimulation(0.001f);
	const auto &hot = scene.primitive_instances.at(0);
	const auto &cold = scene.primitive_instances.at(1);
	const double capacity = 1.0 / (1.0 / 200.0 + 1.0 / 100.0);
	const double expected = 40.0 * capacity * -std::expm1(-20.0 * 0.1 / capacity);
	near(cold.surface_state.conducted_heat, expected, 1e-4, "fixed body heat transfer");
	near(hot.surface_state.temperature, 340.0 - expected / 200.0, 1e-5, "hot body temperature");
	near(cold.surface_state.temperature, 300.0 + expected / 100.0, 1e-5, "cold body temperature");
	near(glm::length(cold.velocity), 0.0, 0.0, "fixed thermal support stays in place");
	near(cold.surface_state.friction_heat, 0.0, 0.0, "conduction produces no friction heat");
}

void separatedGears()
{
	std::ifstream file("examples/gear_surface_transfer.p3d");
	std::ostringstream original;
	original << file.rdbuf();
	auto source = original.str();
	const std::string centre = "T(0.03 0 0)";
	const auto at = source.find(centre);
	require(at != std::string::npos, "wheel centre missing from demo");
	source.replace(at, centre.size(), "T(0.15 0 0)");
	GrammarDocument grammar;
	compile(grammar, source);
	auto &scene = *grammar.context;
	for (int i = 0; i < 80; ++i) scene.stepSimulation(0.0001f);
	const auto &pinion = scene.primitive_instances.at(0);
	const auto &wheel = scene.primitive_instances.at(1);
	near(pinion.rotational_velocity.z, 90.0, 1e-5, "unloaded pinion keeps spinning");
	near(wheel.rotational_velocity.z, 0.0, 0.0, "separated wheel stays at rest");
	near(glm::length(wheel.surface_state.torque), 0.0, 0.0, "separated wheel receives no torque");
}

void externallyDrivenShaft()
{
	GrammarDocument grammar;
	compile(grammar, "Start -> M(2) BodyInertia(0.12 0.12 0.20) BodyForce(5 0 0) "
	                 "BodyTorque(0 0 0.4) ContactShaft(0 0 1) ContactSurface(1000 0 0) I(Cube material(plaster))");
	auto &scene = *grammar.context;
	scene.stepSimulation(0.01f);
	const auto &shaft = scene.primitive_instances.at(0);
	const auto motion = scene.getSurfaceMotionBalance(shaft);
	near(glm::length(shaft.velocity), 0, 0, "shaft bears the transverse force");
	near(motion.spin_momentum.z, 0.004, 2e-7, "authored torque advances shaft angular momentum");
	near(glm::radians(double(shaft.rotational_velocity.z)), 0.02, 1e-6, "authored shaft inertia");
	near(motion.rotation_energy, 0.00004, 2e-8, "drive work becomes kinetic energy");
}

void gravityAndGrammar()
{
	GrammarDocument grammar;
	compile(grammar, "Start -> G(9.81 0 -1 0) [ T(0 2 0) S(2 1 3) ContactSurface(5000 0 0) P(2) I(Cube material(plaster)) ] [ T(12 0 0) !I(Cube material(plaster)) ]");
	auto &scene = *grammar.context;
	auto &body = scene.primitive_instances.at(0);
	require(!scene.primitive_instances.at(1).surface_material.enabled,"pending setting leaked");
	near(body.surface_state.volume,6,1e-5,"scaled volume");near(body.mass,12,1e-5,"scaled mass");
	near(body.surface_state.inertia[0][0],10,1e-5,"nonuniform box inertia");
	const double initial=energy(scene);
	for(int i=0;i<100;++i) scene.stepSimulation(.001f);
	near(body.velocity.y,-.981,2e-5,"free fall");
	near(energy(scene),initial,.005,"gravitational to kinetic energy");
	for(const char *invalid : {"ContactSurface(-1 1 0)", "ContactSurface(1 -1 0)", "ContactThermal(300 0 1)", "ElasticModulus(1 0.5 0)", "ContactLayer(0 1)", "ContactShaft(0 0 0)", "ElasticModulus(4000 0.25 0.4)", "ContactShaft(0 0 1) V(1 0 0)"}) {
		GrammarDocument bad; std::istringstream in(std::string("Start -> ")+invalid+" I(Cube material(plaster))");
		load(bad,in);require(!bad.error_details.empty(),"invalid material accepted");
	}
}
}

int main()
{
	try {
		impact(0.0005); impact(0.00025); obliqueImpact(); gravityAndGrammar();
		heatBetweenFixedBodies(); separatedGears(); externallyDrivenShaft();
		const double coarse = toothTransfer(0.0001);
		const double fine = toothTransfer(0.000025);
		require(fine < coarse, "gear energy residual must shrink with timestep");
		std::cout << "Surface contact checks passed\n";
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n'; return 1;
	}
}
