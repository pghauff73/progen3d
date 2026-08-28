#include "vehicle/mcsmv2/service/VehicleWheelGrammarLoweringService.h"

#include <cmath>
#include <iomanip>
#include <sstream>

namespace {

std::string number(double value)
{
	if (std::fabs(value) < 5.0e-10) value = 0.0;
	std::ostringstream stream;
	stream << std::fixed << std::setprecision(6) << value;
	std::string text = stream.str();
	while (!text.empty() && text.back() == '0') text.pop_back();
	if (!text.empty() && text.back() == '.') text.pop_back();
	return text.empty() || text == "-0" ? "0" : text;
}

} // namespace

std::string VehicleWheelGrammarLoweringService::lowerWheelRule(
	const ModernCarSemanticVariant &variant,
	const std::string &rule_name) const
{
	const VehicleWheelParameters &wheel = variant.wheelMotion().wheelGeometry();
	const double radius = wheel.radius();
	const double width = wheel.width();
	std::ostringstream stream;
	stream << rule_name << "(centerX centerZ sideSign) ->\n"
	       << "[\n"
	       << "    T(centerX+sideSign*" << number(width * 0.5) << ' '
	       << number(radius) << " centerZ)\n"
	       << "    A(sideSign*90 2)\n"
	       << "    !I(Revolve(\n"
	       << "          profile(Polygon\n"
	       << "              " << number(radius * 0.78) << ' ' << number(-width * 0.5) << "\n"
	       << "              " << number(radius * 0.93) << ' ' << number(-width * 0.5) << "\n"
	       << "              " << number(radius) << ' ' << number(-width * 0.36) << "\n"
	       << "              " << number(radius) << ' ' << number(width * 0.36) << "\n"
	       << "              " << number(radius * 0.93) << ' ' << number(width * 0.5) << "\n"
	       << "              " << number(radius * 0.78) << ' ' << number(width * 0.5) << "\n"
	       << "              " << number(radius * 0.70) << ' ' << number(width * 0.36) << "\n"
	       << "              " << number(radius * 0.70) << ' ' << number(-width * 0.36) << ")\n"
	       << "          angle(0 360) segments(24) cap(all) detail(LOD2))\n"
	       << "       material(charcoalroughrubber) alpha(1) texscale(0.10))\n"
	       << "]\n"
	       << "[\n"
	       << "    T(centerX+sideSign*" << number(width * 0.55) << ' '
	       << number(radius) << " centerZ)\n"
	       << "    A(sideSign*90 2)\n"
	       << "    !I(Revolve(\n"
	       << "          profile(Polygon 0 " << number(-width * 0.16) << ' '
	       << number(radius * 0.70) << ' ' << number(-width * 0.16) << ' '
	       << number(radius * 0.70) << ' ' << number(width * 0.16)
	       << " 0 " << number(width * 0.16) << ")\n"
	       << "          angle(0 360) segments(24) cap(all) detail(LOD2))\n"
	       << "       material(charcoalroughmetal) alpha(1) texscale(0.08))\n"
	       << "]\n";
	return stream.str();
}
