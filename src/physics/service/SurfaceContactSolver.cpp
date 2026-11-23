#include "physics/service/SurfaceContactSolver.h"
#include "Context.h"

#include <algorithm>
#include <cmath>

namespace {
using Vector = glm::dvec3;
using Matrix = glm::dmat3;

double series(double first, double second)
{
	return first > 0 && second > 0 ? 1.0 / (1.0 / first + 1.0 / second) : 0.0;
}

class ContactArm {
public:
	ContactArm(PrimitiveInstance &body, const Vector &arm)
		: body_(body), arm_(arm), inverse_mass_(body.immovable || body.surface_material.onShaft() ? 0.0 : 1.0 / body.mass),
		  inverse_inertia_(0.0) {
		if (body.immovable) return;
		const auto &axis = body.surface_material.shaft_axis;
		const auto &inertia = body.surface_state.inertia;
		if (body.surface_material.onShaft())
			inverse_inertia_ = glm::outerProduct(axis,axis) / glm::dot(axis,inertia*axis);
		else
			inverse_inertia_ = glm::inverse(inertia);
	}

	Vector velocity() const {
		return Vector(body_.velocity) + glm::cross(glm::radians(Vector(body_.rotational_velocity)), arm_);
	}
	Vector mobility(const Vector &impulse) const {
		return inverse_mass_ * impulse + glm::cross(inverse_inertia_ * glm::cross(arm_, impulse), arm_);
	}
	void apply(const Vector &impulse, double seconds) {
		auto &state = body_.surface_state;
		const Vector angular_impulse = glm::cross(arm_, impulse);
		state.force += impulse / seconds;
		state.torque += angular_impulse / seconds;
		body_.velocity = Vector(body_.velocity) + inverse_mass_ * impulse;
		body_.rotational_velocity = Vector(body_.rotational_velocity) + glm::degrees(inverse_inertia_ * angular_impulse);
	}
private:
	PrimitiveInstance &body_;
	Vector arm_;
	double inverse_mass_;
	Matrix inverse_inertia_;
};

void recordDeformation(PrimitiveInstance &body, double normal_force, double shear_force)
{
	const auto &m = body.surface_material;
	auto &s = body.surface_state;
	const double normal_depth = normal_force / m.normal_stiffness;
	const double shear_depth = m.shear_stiffness > 0 ? shear_force / m.shear_stiffness : 0;
	s.normal_compression = std::max(s.normal_compression, normal_depth);
	s.shear_displacement = std::max(s.shear_displacement, shear_depth);
	s.elastic_energy += 0.5 * (normal_force * normal_depth + shear_force * shear_depth);
}

void exchangeHeat(PrimitiveInstance &a, PrimitiveInstance &b, double generated, double seconds)
{
	const double capacity_a = a.mass * a.surface_material.specific_heat;
	const double capacity_b = b.mass * b.surface_material.specific_heat;
	const double capacity = series(capacity_a, capacity_b);
	const double conductance = series(a.surface_material.conductance, b.surface_material.conductance);
	auto &sa = a.surface_state;
	auto &sb = b.surface_state;
	// Exact two-lump conduction over this interval. Sliding heat is split equally.
	const double received = (sb.temperature - sa.temperature) * capacity *
	                        -std::expm1(-conductance * seconds / capacity);
	sa.conducted_heat += received;
	sb.conducted_heat -= received;
	sa.friction_heat += generated / 2;
	sb.friction_heat += generated / 2;
	sa.temperature += (received + generated / 2) / capacity_a;
	sb.temperature += (-received + generated / 2) / capacity_b;
}

Vector transportShear(const SurfaceContactPatch &patch, const Vector &normal)
{
	// Minimum rotation between successive contact normals (Rodrigues formula).
	const double cosine = glm::clamp(glm::dot(patch.normal, normal), -1.0, 1.0);
	if (cosine < -0.999999) return -patch.elastic_shear;
	const Vector axis = glm::cross(patch.normal, normal);
	const Vector rotated = patch.elastic_shear + glm::cross(axis, patch.elastic_shear) +
	                       glm::cross(axis, glm::cross(axis, patch.elastic_shear)) / (1.0 + cosine);
	return rotated - normal * glm::dot(normal, rotated);
}
} // namespace

bool SurfaceContactSolver::initializeInertia(PrimitiveInstance &body, const CollisionGeometry &geometry)
{
	if (geometry.analytic_sphere_radius > 0) {
		const double r = geometry.analytic_sphere_radius;
		body.surface_state.inertia = body.surface_state.initial_inertia = Matrix(0.4 * body.mass * r * r);
		body.surface_state.temperature = body.surface_material.initial_temperature;
		return true;
	}
	// Uniform-density tetrahedral second moment about the mesh centre of mass.
	// The signed-volume ratio also works with consistently reversed winding.
	const Vector center(geometry.center_of_mass);
	double volume = 0;
	Matrix moment(0.0);
	for (const auto &triangle : geometry.triangles) {
		if (!triangle.valid) continue;
		const Vector a = Vector(triangle.vertices[0]) - center;
		const Vector b = Vector(triangle.vertices[1]) - center;
		const Vector c = Vector(triangle.vertices[2]) - center;
		const double tetra_volume = glm::dot(a, glm::cross(b, c)) / 6.0;
		const Vector sum = a + b + c;
		moment += (glm::outerProduct(a,a) + glm::outerProduct(b,b) + glm::outerProduct(c,c) +
		           glm::outerProduct(sum,sum)) * (tetra_volume / 20.0);
		volume += tetra_volume;
	}
	if (!std::isfinite(volume) || volume == 0.0) return false;
	moment *= static_cast<double>(body.mass) / volume;
	const Matrix inertia = Matrix(moment[0][0] + moment[1][1] + moment[2][2]) - moment;
	const double determinant = glm::determinant(inertia);
	if (!(inertia[0][0] > 0 && inertia[0][0]*inertia[1][1] > inertia[0][1]*inertia[0][1] &&
	      determinant > 0 && std::isfinite(determinant))) return false;
	body.surface_state.inertia = body.surface_state.initial_inertia = inertia;
	body.surface_state.temperature = body.surface_material.initial_temperature;
	return true;
}

void SurfaceContactSolver::advance(PrimitiveInstance &a, PrimitiveInstance &b,
                                  const Vector &ca, const Vector &cb, const Vector &point,
                                  const Vector &normal, double depth, double h, SurfaceContactPatch &patch)
{
	ContactArm first(a, point-ca), second(b, point-cb);
	const double kn = series(a.surface_material.normal_stiffness, b.surface_material.normal_stiffness);
	const double kt = series(a.surface_material.shear_stiffness, b.surface_material.shear_stiffness);
	const double friction = std::min(a.surface_material.friction, b.surface_material.friction);
	const auto mobility = [&](const Vector &direction) { return first.mobility(direction) + second.mobility(direction); };
	const double normal_mobility = glm::dot(normal, mobility(normal));
	// Positions already drifted. Correct the predicted overlap for the contact impulse.
	const double normal_force = kn * depth / (1.0 + h*h*kn*normal_mobility);
	const Vector normal_impulse = h * normal_force * normal;
	first.apply(-normal_impulse, h);
	second.apply(normal_impulse, h);

	const Matrix tangent = Matrix(1.0) - glm::outerProduct(normal, normal);
	const Vector previous = transportShear(patch, normal);
	const Vector slip_velocity = tangent * (second.velocity() - first.velocity());
	Matrix response(1.0);
	for (int i=0; i<3; ++i) response[i] += h*h*kt * (tangent * mobility(tangent[i]));
	Vector shear = glm::inverse(response) * (previous + h * slip_velocity);
	const double trial_force = kt * glm::length(shear);
	const double limit = friction * normal_force;
	if (trial_force > limit) shear *= limit / trial_force;
	if (kt == 0) shear = Vector(0);
	const Vector shear_impulse = -h * kt * shear;
	first.apply(-shear_impulse, h);
	second.apply(shear_impulse, h);

	const Vector plastic_slip = previous + h * (tangent * (second.velocity()-first.velocity())) - shear;
	const double heat = trial_force > limit ? std::max(0.0, kt * glm::dot(shear, plastic_slip)) : 0.0;
	exchangeHeat(a, b, heat, h);
	recordDeformation(a, normal_force, kt * glm::length(shear));
	recordDeformation(b, normal_force, kt * glm::length(shear));
	patch = {normal, shear, 0.5 * kt * glm::dot(shear, shear)};
}

void SurfaceContactSolver::release(PrimitiveInstance &a, PrimitiveInstance &b, const SurfaceContactPatch &patch)
{
	// A disappearing patch relaxes any residual tangential spring energy locally.
	exchangeHeat(a, b, patch.shear_energy, 0.0);
}
