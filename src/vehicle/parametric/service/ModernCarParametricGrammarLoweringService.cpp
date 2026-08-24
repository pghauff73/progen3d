#include "vehicle/parametric/service/ModernCarParametricGrammarLoweringService.h"

#include "vehicle/parametric/service/ModernCarFieldEvaluationService.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

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

std::string grammar_symbol(const std::string &identifier)
{
	std::string symbol;
	bool capitalize = true;
	for (unsigned char character : identifier) {
		if (!std::isalnum(character)) {
			capitalize = true;
			continue;
		}
		symbol.push_back(capitalize
			? static_cast<char>(std::toupper(character))
			: static_cast<char>(character));
		capitalize = false;
	}
	return symbol.empty() ? "Variant" : symbol;
}

std::string object_identifier(const std::string &identifier)
{
	std::string result;
	for (unsigned char character : identifier) {
		result.push_back(std::isalnum(character)
			? static_cast<char>(std::toupper(character))
			: '_');
	}
	return result;
}

std::string body_material(const ModernCarVariantDefinition &variant)
{
	const std::array<int, 3> &colour = variant.exteriorFeatures().bodyRgb();
	if (colour[2] > colour[0] && colour[2] > colour[1]) return "bluepaint";
	if (colour[0] > 170 && colour[1] > 170 && colour[2] > 170) {
		return "silverbluepolishedmetal";
	}
	return "redmetallicpaint";
}

std::string lower_body_shape(
	const ModernCarVariantDefinition &variant,
	const std::string &material,
	const std::string &detail)
{
	ModernCarFieldEvaluationService field_service;
	const VehicleLongitudinalDomain &domain = variant.body().longitudinalDomain();
	const double preview_body_vertical_correction =
		variant.wheels().radius() * 0.0588235294117647;
	constexpr std::size_t station_count = 9u;
	std::ostringstream stream;
	stream << "    T(0 " << number(preview_body_vertical_correction) << " 0)\n"
	       << "    !I(VehicleBodyShell(\n"
	       << "          thickness(0.038)\n";
	for (std::size_t index = 0u; index < station_count; ++index) {
		const double normalized = static_cast<double>(index) /
			static_cast<double>(station_count - 1u);
		const double source_x = domain.rear() +
			(domain.front() - domain.rear()) * normalized;
		const double station = source_x -
			variant.package().sourcePackageCenterStation();
		const double half_width = std::max(
			0.08, field_service.bodyHalfWidth(variant, source_x));
		const double centre_height =
			field_service.bodyCentreHeight(variant, source_x);
		const double half_height =
			field_service.bodyHalfHeight(variant, source_x);
		const double bottom = std::max(0.04, centre_height - half_height);
		const double shoulder = std::max(
			bottom + 0.04, centre_height + half_height * 0.42);
		double belt = shoulder + std::max(0.04, half_height * 0.16);
		double roof = centre_height + half_height;
		double greenhouse_width = half_width * 0.38;
		if (source_x >= domain.cabinRear() && source_x <= domain.cabinFront()) {
			belt = std::max(belt, field_service.beltHeight(variant, source_x));
			roof = std::max(roof, field_service.roofTop(variant, source_x));
			greenhouse_width = std::min(
				half_width * 0.88,
				std::max(
					0.05,
					field_service.roofHalfWidth(variant, source_x) * 0.92));
		}
		belt = std::min(belt, roof - 0.04);
		if (belt <= shoulder) belt = shoulder + 0.04;
		if (roof <= belt) roof = belt + 0.04;
		greenhouse_width = std::clamp(
			greenhouse_width, 0.04, std::max(0.04, half_width - 0.02));
		stream << "          section(" << number(station) << ' '
		       << number(half_width) << ' ' << number(bottom) << ' '
		       << number(shoulder) << ' ' << number(belt) << ' '
		       << number(roof) << ' ' << number(greenhouse_width) << ")\n";
	}
	stream << "          cap(all)\n"
	       << "          detail(" << detail << "))\n"
	       << "       material(" << material
	       << ") alpha(1) texscale(0.16))\n"
	       << "[ T(0 " << number(variant.package().height() * 0.48) << " 0)\n"
	       << "  S(" << number(variant.package().width() * 0.96) << ' '
	       << number(variant.package().height() * 0.68) << ' '
	       << number(variant.package().length() * 0.98) << ")\n"
	       << "  !I(Sphere material(" << material
	       << ") alpha(1) texscale(0.16)) ]\n"
	       << "[ T(0 " << number(variant.package().height() * 0.75) << ' '
	       << number(-variant.package().length() * 0.10) << ")\n"
	       << "  S(" << number(variant.package().width() * 0.82) << ' '
	       << number(variant.package().height() * 0.64) << ' '
	       << number(variant.package().length() * 0.64) << ")\n"
	       << "  !I(Sphere material(" << material
	       << ") alpha(1) texscale(0.16)) ]\n"
	       << "[ T(0 " << number(variant.package().height() * 0.57) << ' '
	       << number(variant.package().length() * 0.29) << ")\n"
	       << "  S(" << number(variant.package().width() * 0.90) << ' '
	       << number(variant.package().height() * 0.30) << ' '
	       << number(variant.package().length() * 0.38) << ")\n"
	       << "  !I(Sphere material(" << material
	       << ") alpha(1) texscale(0.16)) ]\n"
	       << "[ T(0 " << number(variant.package().height() * 0.38) << ' '
	       << number(variant.package().length() * 0.48) << ")\n"
	       << "  S(" << number(variant.package().width() * 0.92) << ' '
	       << number(variant.package().height() * 0.70) << ' '
	       << number(variant.package().length() * 0.04) << ")\n"
	       << "  !I(Sphere material(" << material
	       << ") alpha(1) texscale(0.16)) ]\n";
	return stream.str();
}

std::string lower_direct_geometry(
	const ModernCarVariantDefinition &variant,
	const std::string &prefix,
	const std::string &detail)
{
	const VehiclePackageParameters &package = variant.package();
	const VehicleWheelParameters &wheels = variant.wheels();
	const double preview_wheel_radius = wheels.radius() * 1.15;
	std::ostringstream stream;
	stream << prefix << "Geometry ->\n"
	       << "    " << prefix << "Body\n"
	       << "    " << prefix << "Wheel(" << number(-wheels.frontTrack() * 0.5)
	       << ' ' << number(package.frontAxleStation()) << " -1)\n"
	       << "    " << prefix << "Wheel(" << number(wheels.frontTrack() * 0.5)
	       << ' ' << number(package.frontAxleStation()) << " 1)\n"
	       << "    " << prefix << "Wheel(" << number(-wheels.rearTrack() * 0.5)
	       << ' ' << number(package.rearAxleStation()) << " -1)\n"
	       << "    " << prefix << "Wheel(" << number(wheels.rearTrack() * 0.5)
	       << ' ' << number(package.rearAxleStation()) << " 1)\n\n"
	       << prefix << "Body ->\n[\n"
	       << lower_body_shape(variant, body_material(variant), detail)
	       << "]\n\n"
	       << prefix << "Wheel(centerX centerZ sideSign) ->\n[\n"
	       << "    T(centerX+sideSign*" << number(wheels.width() * 0.5)
	       << ' ' << number(preview_wheel_radius) << " centerZ)\n"
	       << "    A(sideSign*90 2)\n"
	       << "    !I(Revolve(\n"
	       << "          profile(Polygon\n"
	       << "              " << number(preview_wheel_radius * 0.78) << ' '
	       << number(-wheels.width() * 0.5) << "\n"
	       << "              " << number(preview_wheel_radius * 0.93) << ' '
	       << number(-wheels.width() * 0.5) << "\n"
	       << "              " << number(preview_wheel_radius) << ' '
	       << number(-wheels.width() * 0.36) << "\n"
	       << "              " << number(preview_wheel_radius) << ' '
	       << number(wheels.width() * 0.36) << "\n"
	       << "              " << number(preview_wheel_radius * 0.93) << ' '
	       << number(wheels.width() * 0.5) << "\n"
	       << "              " << number(preview_wheel_radius * 0.78) << ' '
	       << number(wheels.width() * 0.5) << "\n"
	       << "              " << number(preview_wheel_radius * 0.70) << ' '
	       << number(wheels.width() * 0.36) << "\n"
	       << "              " << number(preview_wheel_radius * 0.70) << ' '
	       << number(-wheels.width() * 0.36) << ")\n"
	       << "          angle(0 360) segments(24) cap(all) detail(" << detail << "))\n"
	       << "       material(charcoalroughrubber) alpha(1) texscale(0.10))\n"
	       << "]\n[\n"
	       << "    T(centerX+sideSign*" << number(wheels.width() * 0.55)
	       << ' ' << number(preview_wheel_radius) << " centerZ)\n"
	       << "    A(sideSign*90 2)\n"
	       << "    !I(Revolve(\n"
	       << "          profile(Polygon 0 " << number(-wheels.width() * 0.16)
	       << ' ' << number(preview_wheel_radius * 0.70) << ' '
	       << number(-wheels.width() * 0.16) << ' '
	       << number(preview_wheel_radius * 0.70) << ' '
	       << number(wheels.width() * 0.16) << " 0 "
	       << number(wheels.width() * 0.16) << ")\n"
	       << "          angle(0 360) segments(24) cap(all) detail(" << detail << "))\n"
	       << "       material(charcoalroughmetal) alpha(1) texscale(0.08))\n"
	       << "]\n";
	return stream.str();
}

} // namespace

std::string ModernCarParametricGrammarLoweringService::lowerVariantRule(
	const ModernCarVariantDefinition &variant,
	const std::string &rule_name,
	double placement_z,
	const std::string &container_identifier) const
{
	const VehiclePackageParameters &package = variant.package();
	const VehicleWheelParameters &wheels = variant.wheels();
	std::ostringstream stream;
	stream << rule_name << " ->\n[\n"
	       << "    T(0 0 " << number(placement_z) << ")\n"
	       << "    Object(\n"
	       << "        id(MCPOMV1_" << object_identifier(variant.identifier()) << ")\n"
	       << "        name(" << grammar_symbol(variant.displayName()) << ")\n"
	       << "        class(ModernCarParametricVariant)\n"
	       << "        container(" << container_identifier << ")\n"
	       << "        taxonomy(Vehicle ParametricCandidate ModernCar "
	       << grammar_symbol(variant.identifier()) << ")\n"
	       << "        layer(Equipment)\n"
	       << "        mask(Equipment Terrain Temporary)\n"
	       << "    )\n    [\n"
	       << "        Interface(id(packageCentre) type(InspectionInterface)\n"
	       << "                  origin(0 0 0) normal(0 1 0) tangent(0 0 1)\n"
	       << "                  region(Point) tolerance(0.001))\n"
	       << "        Interface(id(frontAxle) type(Bearing)\n"
	       << "                  origin(0 " << number(wheels.radius()) << ' '
	       << number(package.frontAxleStation())
	       << ") normal(1 0 0) tangent(0 1 0)\n"
	       << "                  region(Point) tolerance(0.002))\n"
	       << "        Interface(id(rearAxle) type(Bearing)\n"
	       << "                  origin(0 " << number(wheels.radius()) << ' '
	       << number(package.rearAxleStation())
	       << ") normal(1 0 0) tangent(0 1 0)\n"
	       << "                  region(Point) tolerance(0.002))\n"
	       << "        MCPOMv1VehicleCandidate("
	       << number(package.length()) << ' ' << number(package.width()) << ' '
	       << number(package.height()) << ' ' << number(package.wheelbase()) << ' '
	       << number(wheels.frontTrack()) << ' ' << number(wheels.rearTrack()) << ' '
	       << number(package.groundClearance()) << ' ' << number(wheels.radius()) << ' '
	       << number(wheels.width()) << ")\n"
	       << "    ]\n]\n\n";
	return stream.str();
}

std::string ModernCarParametricGrammarLoweringService::lowerVehicleCandidateRules() const
{
	return R"GRAMMAR(MCPOMv1VehicleCandidate(length width height wheelbase trackFront trackRear groundClearance wheelRadius wheelWidth) ->
    MCPOMv1CandidateBody(length width height groundClearance)
    MCPOMv1CandidateWheel(-trackFront/2 wheelbase/2 -1 wheelRadius wheelWidth)
    MCPOMv1CandidateWheel(trackFront/2 wheelbase/2 1 wheelRadius wheelWidth)
    MCPOMv1CandidateWheel(-trackRear/2 -wheelbase/2 -1 wheelRadius wheelWidth)
    MCPOMv1CandidateWheel(trackRear/2 -wheelbase/2 1 wheelRadius wheelWidth)

MCPOMv1CandidateBody(length width height groundClearance) ->
[
    T(0 groundClearance-0.15*height/1.46812 0)
    S(width/1.816 height/1.46812 length/4.353)
    !I(VehicleBodyShell(
          thickness(0.038)
          section(-2.1765 0.70 0.24 0.60 0.68 0.76 0.28)
          section(-1.92 0.84 0.18 0.76 0.90 1.11 0.50)
          section(-1.70 0.91 0.15 0.88 1.00 1.34 0.64)
          section(-0.74 0.92 0.14 0.92 1.05 1.46 0.68)
          section(0.18 0.91 0.14 0.91 1.03 1.42 0.66)
          section(0.78 0.90 0.16 0.86 0.96 1.16 0.56)
          section(1.45 0.84 0.20 0.76 0.84 0.92 0.40)
          section(2.1765 0.70 0.27 0.60 0.67 0.74 0.26)
          cap(all)
          detail(LOD1))
       material(redmetallicpaint) alpha(1) texscale(0.16))
]
[
    T(0 height*0.48 0)
    S(width*0.96 height*0.68 length*0.98)
    !I(Sphere material(redmetallicpaint) alpha(1) texscale(0.16))
]
[
    T(0 height*0.75 -length*0.10)
    S(width*0.82 height*0.64 length*0.64)
    !I(Sphere material(redmetallicpaint) alpha(1) texscale(0.16))
]
[
    T(0 height*0.57 length*0.29)
    S(width*0.90 height*0.30 length*0.38)
    !I(Sphere material(redmetallicpaint) alpha(1) texscale(0.16))
]
[
    T(0 height*0.38 length*0.48)
    S(width*0.92 height*0.70 length*0.04)
    !I(Sphere material(redmetallicpaint) alpha(1) texscale(0.16))
]

MCPOMv1CandidateWheel(centerX centerZ sideSign radius width) ->
[
    T(centerX+sideSign*width/2 radius centerZ)
    A(sideSign*90 2)
    !I(Revolve(
          profile(Polygon
              radius*0.78 -width/2
              radius*0.93 -width/2
              radius -width*0.36
              radius width*0.36
              radius*0.93 width/2
              radius*0.78 width/2
              radius*0.70 width*0.36
              radius*0.70 -width*0.36)
          angle(0 360)
          segments(24)
          cap(all)
          detail(LOD1))
       material(charcoalroughrubber) alpha(1) texscale(0.10))
]
[
    T(centerX+sideSign*width*0.55 radius centerZ)
    A(sideSign*90 2)
    !I(Revolve(
          profile(Polygon 0 -width*0.16 radius*0.70 -width*0.16
                          radius*0.70 width*0.16 0 width*0.16)
          angle(0 360)
          segments(24)
          cap(all)
          detail(LOD1))
       material(charcoalroughmetal) alpha(1) texscale(0.08))
]
)GRAMMAR";
}

std::string ModernCarParametricGrammarLoweringService::lowerFamilyPreview(
	const ModernCarFamilyDefinition &family) const
{
	std::ostringstream stream;
	stream << "# MCP_OMv1 executable family preview generated by ModernCarParametricGrammarLoweringService.\n"
	       << "# +X right, +Y up, +Z forward; each origin is its ground-projected package centre.\n\n"
	       << "Start ->\n    MCPOMv1FamilyPreview\n\n"
	       << "MCPOMv1FamilyPreview ->\n[\n"
	       << "    Object(id(MCPOMV1_FAMILY_PREVIEW) name(McpOmv1FamilyPreview)\n"
	       << "           class(ModernCarParametricFamily)\n"
	       << "           taxonomy(Vehicle ParametricCandidate Family)\n"
	       << "           layer(Equipment) mask(Equipment Terrain Temporary))\n    [\n"
	       << "        MCPOMv1Ground\n";
	for (const ModernCarVariantDefinition &variant : family.variants()) {
		stream << "        MCPOMv1" << grammar_symbol(variant.identifier()) << "\n";
	}
	stream << "    ]\n]\n\n"
	       << "MCPOMv1Ground ->\n[\n"
	       << "    T(0 -0.025 0) S(1.4 0.025 10.1)\n"
	       << "    !I(CubeY material(lightgreyroughconcrete) alpha(1) texscale(0.55))\n"
	       << "]\n\n";
	const std::vector<double> placements{7.4, 2.5, -2.5, -7.5};
	for (std::size_t index = 0u; index < family.variants().size(); ++index) {
		stream << lowerVariantRule(
			family.variants()[index],
			"MCPOMv1" + grammar_symbol(family.variants()[index].identifier()),
			index < placements.size() ? placements[index] : 0.0,
			"MCPOMV1_FAMILY_PREVIEW");
	}
	stream << lowerVehicleCandidateRules();
	return stream.str();
}

std::string ModernCarParametricGrammarLoweringService::lowerVariantPreview(
	const ModernCarVariantDefinition &variant) const
{
	const std::string prefix = "MCPOMv1" + grammar_symbol(variant.identifier());
	std::ostringstream stream;
	stream << "# MCP_OMv1 executable " << variant.identifier() << " preview.\n"
	       << "# +X right, +Y up, +Z forward; origin is ground package centre.\n\n"
	       << "Start ->\n    " << prefix << "Preview\n\n"
	       << prefix << "Preview ->\n[\n"
	       << "    Object(id(" << object_identifier(prefix) << "_PREVIEW)\n"
	       << "           name(" << grammar_symbol(variant.displayName()) << ")\n"
	       << "           class(ModernCarParametricVariant)\n"
	       << "           taxonomy(Vehicle ParametricCandidate ModernCar)\n"
	       << "           layer(Equipment) mask(Equipment Terrain Temporary))\n    [\n"
	       << "        Interface(id(packageCentre) type(InspectionInterface)\n"
	       << "                  origin(0 0 0) normal(0 1 0) tangent(0 0 1)\n"
	       << "                  region(Point) tolerance(0.001))\n"
	       << "        " << prefix << "Geometry\n"
	       << "    ]\n]\n\n"
	       << lower_direct_geometry(variant, prefix, "LOD2");
	return stream.str();
}

std::string ModernCarParametricGrammarLoweringService::lowerMvp26ReferenceAdapter(
	const ModernCarVariantDefinition &reference_variant) const
{
	const std::string prefix = "MCPOMv1ReferenceAdapter";
	const VehiclePackageParameters &package = reference_variant.package();
	std::ostringstream stream;
	stream << "# MCP_OMv1 to MVP2.6 reference adapter generated by ModernCarParametricGrammarLoweringService.\n\n"
	       << "Start ->\n    MCPOMv1Mvp26ReferenceAdapter\n\n"
	       << "MCPOMv1Mvp26ReferenceAdapter ->\n[\n"
	       << "    Object(id(MCPOMV1_MVP26_REFERENCE_ADAPTER)\n"
	       << "           name(McpOmv1Mvp26ReferenceAdapter)\n"
	       << "           class(VehicleMvp26ParametricSourceAdapter)\n"
	       << "           taxonomy(Vehicle MVP26 ParametricSource Adapter)\n"
	       << "           layer(Equipment) mask(Equipment Terrain Temporary))\n    [\n"
	       << "        MCPOMv1AdapterGround\n"
	       << "        MCPOMv1SourceFrameEvidence\n"
	       << "        MVP26ReferenceCandidate\n"
	       << "    ]\n]\n\n"
	       << "MCPOMv1AdapterGround ->\n[\n"
	       << "    T(0 -0.018 0) S(2.6 0.018 3.0)\n"
	       << "    !I(CubeY material(lightgreyroughconcrete) alpha(1) texscale(0.45))\n"
	       << "]\n\n"
	       << "MCPOMv1SourceFrameEvidence ->\n[\n"
	       << "    Object(id(MCPOMV1_SOURCE_FRAME) name(McsM1ToMcpVehicleCoordinateFrame)\n"
	       << "           class(ModernCarCoordinateFrame)\n"
	       << "           taxonomy(Vehicle ParametricSource CoordinateFrame AxisMapping)\n"
	       << "           layer(Temporary) mask(Equipment Temporary))\n    [\n"
	       << "        Interface(id(mcsmPositiveXFront) type(InspectionInterface) origin(0 0 0.45) normal(0 0 1) tangent(1 0 0) region(Point) tolerance(0.0001))\n"
	       << "        Interface(id(mcsmPositiveYRight) type(InspectionInterface) origin(0.45 0 0) normal(1 0 0) tangent(0 1 0) region(Point) tolerance(0.0001))\n"
	       << "        Interface(id(mcsmPositiveZUp) type(InspectionInterface) origin(0 0.45 0) normal(0 1 0) tangent(0 0 1) region(Point) tolerance(0.0001))\n"
	       << "    ]\n]\n\n"
	       << "MVP26ReferenceCandidate ->\n[\n"
	       << "    Object(id(MVP26_MCPOMV1_REFERENCE_CANDIDATE)\n"
	       << "           name(Mvp26McpOmv1ReferenceCandidate)\n"
	       << "           class(VehicleMvp26ParametricCandidate)\n"
	       << "           taxonomy(Vehicle MVP26 ParametricSource Hatchback Reference)\n"
	       << "           layer(Equipment) mask(Equipment Terrain Temporary))\n    [\n"
	       << "        Interface(id(packageCentre) type(InspectionInterface) origin(0 0 0) normal(0 1 0) tangent(0 0 1) region(Point) tolerance(0.001))\n"
	       << "        Interface(id(packageMaximum) type(InspectionInterface) origin("
	       << number(package.width() * 0.5) << ' ' << number(package.height()) << ' '
	       << number(package.length() * 0.5)
	       << ") normal(0 1 0) tangent(0 0 1) region(Point) tolerance(0.002))\n"
	       << "        " << prefix << "Geometry\n"
	       << "        MVP26ParametricSourceEvidence\n"
	       << "    ]\n]\n\n"
	       << "MVP26ParametricSourceEvidence ->\n[\n"
	       << "    Object(id(MVP26_MCPOMV1_SOURCE_EVIDENCE) name(McpOmv1ReferenceSourceEvidence)\n"
	       << "           class(VehicleParametricSourceEvidence)\n"
	       << "           taxonomy(Vehicle MVP26 SourceManifest Package Curves Sections Observations)\n"
	       << "           layer(Temporary) mask(Equipment Temporary))\n    [\n"
	       << "        Interface(id(sourceManifestRequired) type(InspectionInterface) origin(-0.24 0.06 0) normal(0 1 0) tangent(1 0 0) region(Point) tolerance(0.0001))\n"
	       << "        Interface(id(characterCurveNetworkRequired) type(InspectionInterface) origin(-0.08 0.06 0) normal(0 1 0) tangent(1 0 0) region(Point) tolerance(0.0001))\n"
	       << "        Interface(id(bodySectionNetworkRequired) type(InspectionInterface) origin(0.08 0.06 0) normal(0 1 0) tangent(1 0 0) region(Point) tolerance(0.0001))\n"
	       << "        Interface(id(observationSetRequired) type(InspectionInterface) origin(0.24 0.06 0) normal(0 1 0) tangent(1 0 0) region(Point) tolerance(0.0001))\n"
	       << "    ]\n]\n\n"
	       << lower_direct_geometry(reference_variant, prefix, "LOD2");
	return stream.str();
}
