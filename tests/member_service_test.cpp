#include "Context.h"
#include "GrammarRuntime.h"
#include "editor/service/SpatialObjectInspectionService.h"
#include "grammar.h"
#include "spatial/model/SpatialBuildingModel.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
void require(bool condition, const std::string &message)
{
	if (!condition) throw std::runtime_error(message);
}

void near(double actual, double expected, const std::string &label)
{
	const double tolerance = std::max(1e-11, std::abs(expected) * 1e-5);
	require(std::isfinite(actual) && std::abs(actual - expected) <= tolerance,
	        label + " expected " + std::to_string(expected) + ", got " + std::to_string(actual));
}

std::string changed(std::string source, const std::string &from, const std::string &to)
{
	const auto pos = source.find(from);
	require(pos != std::string::npos, "Missing test edit: " + from);
	source.replace(pos, from.size(), to);
	return source;
}

std::unique_ptr<GrammarDocument> generate(const std::string &source, bool shouldSucceed = true)
{
	std::istringstream input(source);
	auto grammar = std::make_unique<GrammarDocument>();
	setGrammarEvaluationDiagnosticsQuiet(grammar.get(), true);
	grammar->loadFromStream(input, "member service fixture");
	grammar->addContext();
	grammar->context->genPrimitives();
	grammar->generateGeometry();
	require(shouldSucceed == grammar->error_details.empty(),
	        grammar->error_details.empty() ? "Expected a grammar diagnostic" : grammar->error_details.front().message);
	return grammar;
}

SpatialObjectInspection inspect(const std::string &source, const std::string &id = "BookShelf")
{
	auto grammar = generate(source);
	const auto model = grammar->context->spatialBuildingModel();
	require(bool(model), "Missing spatial model");
	return SpatialObjectInspectionService().inspectObject(*model, SpatialObjectId(id));
}

MemberServiceResponse solved(const SpatialObjectInspection &inspection)
{
	require(bool(inspection.member_response), "Missing member result: " + inspection.member_diagnostic);
	return *inspection.member_response;
}

void unavailable(const std::string &source, const std::string &reason)
{
	const auto result = inspect(source);
	require(result.available && !result.member_response && result.member_diagnostic.find(reason) != std::string::npos,
	        "Unexpected unsupported-case result: " + result.member_diagnostic);
}

void checkReferenceCases(const std::string &source)
{
	auto grammar = generate(source);
	const auto &model = *grammar->context->spatialBuildingModel();
	SpatialObjectInspectionService inspector;
	const auto floor = inspector.inspectObject(model, SpatialObjectId("FloorMember"));
	const auto shelf = inspector.inspectObject(model, SpatialObjectId("BookShelf"));
	require(floor.member_material == "StructuralSteel" && shelf.member_material == "AssumedPanel", "Material ownership");
	const auto f = solved(floor), s = solved(shelf);
	near(f.areaM2, .0024, "RHS area"); near(f.secondMomentM4, 7.545e-6, "RHS second moment");
	near(f.massKg, 67.824, "RHS mass"); near(f.reactionPerSeatN, 2492.5631148, "floor reaction");
	near(f.peakMomentNm, 2243.30680332, "floor bending moment"); near(f.bendingStressPa, 22.2992724e6, "floor stress");
	near(f.midspanDeflectionM, .002006934516, "floor deflection"); near(f.pinPressurePa, 332341.749, "floor seat pressure");
	near(s.reactionPerSeatN, 111.8350366, "shelf reaction"); near(s.midspanDeflectionM, .001704542548, "shelf deflection");
	near(s.rollerPressurePa, 18639.17277, "shelf pressure");
	near(2*s.reactionPerSeatN, (s.massKg+20)*9.80665, "force balance");
	near(s.reactionPerSeatN*s.spanM, s.totalLineLoadNPerM*s.spanM*s.spanM/2, "moment balance");
	near(s.deflectionM(0), 0, "pin displacement"); near(s.deflectionM(1), 0, "roller displacement");
	near(s.deflectionM(.25), s.deflectionM(.75), "symmetric profile");
	const auto fromPrimitive = inspector.inspectPrimitive(model, shelf.primitive_instance_indices.at(0));
	near(solved(fromPrimitive).midspanDeflectionM, s.midspanDeflectionM, "primitive selection");
	require(!inspector.inspectObject(model, SpatialObjectId("Study")).member_response, "Parent acquired its child's study");
}

void checkLoadCases(const std::string &source)
{
	const auto s = solved(inspect(source));
	const auto stiff = solved(inspect(changed(source, "6000000000", "12000000000")));
	near(2*stiff.midspanDeflectionM, s.midspanDeflectionM, "doubling E");
	const auto centre = solved(inspect(changed(source, "load(0 booksKg 0)", "load(0 booksKg 100)")));
	near(centre.reactionPerSeatN, s.reactionPerSeatN+50, "midpoint point force");
	near(centre.midspanDeflectionM-s.midspanDeflectionM, 100*std::pow(s.spanM,3)/(48*s.flexuralRigidityNm2), "point-load deflection");
	const auto unloaded = changed(source, "0.018 20)", "0.018 0)");
	const auto longer = solved(inspect(changed(unloaded, "Shelf(0.8", "Shelf(1.6")));
	near(longer.midspanDeflectionM, 16*solved(inspect(unloaded)).midspanDeflectionM, "constant w span scaling");

	auto mm = changed(source, "Shelf(0.8 0.30 0.018", "Shelf(800 300 18");
	mm = changed(mm, "metresPerUnit(1)\n            material(AssumedPanel", "metresPerUnit(0.001)\n            material(AssumedPanel");
	mm = changed(mm, "Seats(span breadth 0.02)", "Seats(span breadth 20)");
	const auto metric = solved(inspect(mm));
	near(metric.massKg, s.massKg, "mm to metres mass"); near(metric.midspanDeflectionM, s.midspanDeflectionM, "mm to metres deflection");
	near(metric.rollerPressurePa, s.rollerPressurePa, "mm to metres contact area");
	const auto scaled = solved(inspect(changed(unloaded, "T(0 0.52 0)", "S(2 2 2)")));
	near(scaled.massKg, 8*solved(inspect(unloaded)).massKg, "uniform scene scaling");
	auto pressure = changed(source, "region(PlaneRectangle seatLength breadth))\n    [ T", "region(PlaneRectangle 2*seatLength breadth))\n    [ T");
	const auto unequal = solved(inspect(pressure)); near(unequal.rollerPressurePa*2, unequal.pinPressurePa, "separate seat areas");
}

void checkMissingEvidence(const std::string &source)
{
	unavailable(changed(source, "section(breadth depth 0)", "section(breadth depth depth)"), "hollow wall");
	unavailable(changed(source, "material(AssumedPanel 6000000000", "material(AssumedPanel 0"), "Modulus");
	unavailable(changed(source, "T(0 0.52 0)", "S(1 2 1)"), "uniformly scaled");
	unavailable(changed(source, "T(0 0.52 0)", "A(90 2)"), "level seats");
	near(solved(inspect(changed(source, "T(0 0.52 0)", "A(37 1)"))).midspanDeflectionM,
	     .001704542548, "yaw placement");
	unavailable(changed(source, "type(Seat) origin(-span", "type(Mate) origin(-span"), "existing seat");
	unavailable(changed(source, "supports(LeftSeat RightSeat) metresPerUnit(1)\n            material(AssumedPanel", "supports(LeftSeat Missing) metresPerUnit(1)\n            material(AssumedPanel"), "existing seat");
	unavailable(changed(source,"Seats(span breadth 0.02)", "Seats(span breadth 0.02) Interface(id(Pin3) type(Seat)) Interface(id(Pin4) type(Seat))"), "Additional support");
	const std::string study = "PinRollerMember(supports(A B) material(Panel 1 1) section(1 1 0) load(0 0 0) metresPerUnit(1))";
	auto orphan = generate("Start -> I(Cube material(amberwarmwood) 1 0.5) " + study + "\n", false);
	require(orphan->context->primitive_instances.empty(), "Orphan study partially executed");
	auto duplicate = generate(changed(source, "Seats(span breadth 0.02)", study + " Seats(span breadth 0.02)"), false);
	require(duplicate->context->primitive_instances.empty(), "Duplicate study partially executed");
	auto malformed = generate(changed(source, "section(breadth depth wall)", "section(breadth depth wall) section(1 1 0)"), false);
	require(malformed->error_details.front().line >= 0, "Missing source location for invalid declaration");
}
}

int main()
{
	try {
		std::ifstream file("examples/Mechanics/PinRollerMembers.p3d");
		require(bool(file), "Cannot open reference grammar");
		std::ostringstream text; text << file.rdbuf();
		checkReferenceCases(text.str());
		checkLoadCases(text.str());
		checkMissingEvidence(text.str());
		std::cout << "Member service analysis passed the native grammar and inspector checks.\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
