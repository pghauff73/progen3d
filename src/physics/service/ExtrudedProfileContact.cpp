#include "physics/service/ExtrudedProfileContact.h"
#include "Context.h"
#include "geometry/model/ExtrudeProfileShapeSpecification.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {
using Point = glm::dvec2;
using Vector = glm::dvec3;
using Extrusion = ExtrudeProfileShapeSpecification;

struct ContactPlane {
	Vector x, y, z;
	Point project(const Vector &p) const { return {glm::dot(p,x), glm::dot(p,y)}; }
};

struct ProjectedProfile {
	std::vector<Point> boundary;
	Point low{std::numeric_limits<double>::max()}, high{std::numeric_limits<double>::lowest()};
	double axial_min, axial_max;

	ProjectedProfile(const PrimitiveInstance &body, const ContactPlane &plane) {
		const auto &shape = static_cast<const Extrusion &>(*body.shape_specification);
		const glm::dmat4 transform(body.primary_transform);
		for (const auto &v : shape.profile().outerLoop().points()) {
			const Point p = plane.project(Vector(transform * glm::dvec4(v.x,v.y,0,1)));
			boundary.push_back(p);
			low = glm::min(low,p);
			high = glm::max(high,p);
		}
		axial_min = glm::dot(Vector(transform[3]),plane.z);
		axial_max = axial_min + shape.depth() * glm::dot(Vector(transform[2]),plane.z);
		if (axial_max < axial_min) std::swap(axial_min,axial_max);
	}
};

struct ProfilePenetration {
	double depth = 0;
	Point point{0}, normal{0};

	void testVertex(const Point &p, const ProjectedProfile &other, double sign) {
		if (p.x < other.low.x || p.x > other.high.x || p.y < other.low.y || p.y > other.high.y) return;
		bool inside = false;
		double minimum = std::numeric_limits<double>::max();
		Point closest(0);
		auto a = other.boundary.back();
		for (const auto &b : other.boundary) {
			const Point edge = b-a;
			if ((a.y>p.y) != (b.y>p.y) && p.x < a.x + (p.y-a.y)*edge.x/edge.y) inside = !inside;
			const double t = glm::clamp(glm::dot(p-a,edge)/glm::dot(edge,edge),0.0,1.0);
			const Point q = a+t*edge;
			const double d2 = glm::dot(p-q,p-q);
			if (d2 < minimum) { minimum = d2; closest = q; }
			a=b;
		}
		if (inside && minimum > depth*depth && minimum > 1e-22) {
			depth = std::sqrt(minimum);
			normal = sign * (p-closest)/depth;
			point = 0.5*(p+closest);
		}
	}
	void sample(const ProjectedProfile &profile, const ProjectedProfile &other, double sign) {
		auto previous = profile.boundary.back();
		for (const auto &p : profile.boundary) {
			testVertex(p,other,sign);
			testVertex((p+previous)*0.5,other,sign);
			previous=p;
		}
	}
};
} // namespace

bool ExtrudedProfileContact::accepts(const PrimitiveInstance &body)
{
	const auto *shape = dynamic_cast<const Extrusion *>(body.shape_specification.get());
	if (!shape || !shape->requestsClosedGeometry() || !shape->profile().innerLoops().empty() ||
	    !body.surface_material.onShaft()) return false;
	const glm::dmat3 axes(body.primary_transform);
	const Vector face_normal = glm::normalize(glm::cross(axes[0],axes[1]));
	return std::abs(glm::dot(face_normal,body.surface_material.shaft_axis)) > 1.0-1e-6 &&
	       std::abs(glm::dot(face_normal,glm::normalize(axes[2]))) > 1.0-1e-6;
}

CollisionContact ExtrudedProfileContact::detect(const PrimitiveInstance &a, const PrimitiveInstance &b)
{
	const Vector axis = a.surface_material.shaft_axis;
	if (std::abs(glm::dot(axis,b.surface_material.shaft_axis)) < 1.0-1e-6)
		return CollisionContact::createSeparated();
	const Vector x = glm::normalize(Vector(a.primary_transform[0]));
	const ContactPlane plane{x,glm::normalize(glm::cross(axis,x)),axis};
	const ProjectedProfile first(a,plane), second(b,plane);
	const double low = std::max(first.axial_min,second.axial_min), high = std::min(first.axial_max,second.axial_max);
	if (high <= low) return CollisionContact::createSeparated();
	ProfilePenetration contact;
	contact.sample(first,second,1);
	contact.sample(second,first,-1);
	if (contact.depth <= 0) return CollisionContact::createSeparated();
	const auto normal = plane.x*contact.normal.x+plane.y*contact.normal.y;
	const auto point = plane.x*contact.point.x+plane.y*contact.point.y+axis*(0.5*(low+high));
	return CollisionContact::createIntersecting(glm::vec3(normal),static_cast<float>(contact.depth),glm::vec3(point));
}
