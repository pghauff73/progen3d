#include "editor/service/SpatialDeclarationCompletionService.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace {

struct CallFrame
{
	std::string name;
};

bool identifier_character(char character)
{
	return std::isalnum(static_cast<unsigned char>(character)) != 0 ||
	       character == '_';
}

std::string identifier_before(const std::string &source, std::size_t parenthesis)
{
	std::size_t end = parenthesis;
	while (end > 0 &&
	       std::isspace(static_cast<unsigned char>(source[end - 1])) != 0) {
		--end;
	}
	std::size_t start = end;
	while (start > 0 && identifier_character(source[start - 1])) --start;
	return source.substr(start, end - start);
}

std::vector<CallFrame> active_call_stack(const std::string &source)
{
	std::vector<CallFrame> stack;
	bool line_comment = false;
	for (std::size_t index = 0; index < source.size(); ++index) {
		if (line_comment) {
			if (source[index] == '\n') line_comment = false;
			continue;
		}
		if (source[index] == '/' && index + 1 < source.size() &&
		    source[index + 1] == '/') {
			line_comment = true;
			++index;
			continue;
		}
		if (source[index] == '(') {
			stack.push_back({identifier_before(source, index)});
		} else if (source[index] == ')' && !stack.empty()) {
			stack.pop_back();
		}
	}
	return stack;
}

bool has_open_object_body(const std::string &source)
{
	int square_depth = 0;
	bool line_comment = false;
	for (std::size_t index = 0; index < source.size(); ++index) {
		if (line_comment) {
			if (source[index] == '\n') line_comment = false;
			continue;
		}
		if (source[index] == '/' && index + 1 < source.size() &&
		    source[index + 1] == '/') {
			line_comment = true;
			++index;
			continue;
		}
		if (source[index] == '[') ++square_depth;
		else if (source[index] == ']') square_depth = std::max(0, square_depth - 1);
	}
	return square_depth > 0;
}

std::vector<std::string> collision_layers()
{
	return {"Structure", "Envelope", "Interior", "Furniture", "Plumbing",
	        "HVAC", "Electrical", "Equipment", "Terrain", "Temporary"};
}

SpatialDeclarationCompletionContext value_completions(
	const std::vector<CallFrame> &stack)
{
	if (stack.empty()) return {};
	const std::string &field = stack.back().name;
	if (field == "layer" || field == "mask") {
		return SpatialDeclarationCompletionContext("Collision Layer", collision_layers());
	}
	if (field == "type" && stack.size() >= 2 && stack[stack.size() - 2].name == "Interface") {
		return SpatialDeclarationCompletionContext(
			"Interface Type",
			{"Support", "Bearing", "Mate", "Seat", "Insert", "Socket", "Shaft",
			 "Seal", "Fastener", "Anchor", "Hinge", "Slide", "PipePort", "DuctPort",
			 "ElectricalPort", "DataPort", "ControlPort", "DrainPort",
			 "ThermalInterface", "InspectionInterface"});
	}
	if (field == "type" && stack.size() >= 2 && stack[stack.size() - 2].name == "Connect") {
		return SpatialDeclarationCompletionContext(
			"Connection Type",
			{"Contains", "ConnectedTo", "DrainsTo", "Powers", "ServedBy", "Monitors",
			 "SupportedBy", "SeatedIn", "InsertedIn", "SealedTo", "FixedTo",
			 "AlignedWith"});
	}
	if (field == "mode") {
		return SpatialDeclarationCompletionContext(
			"P0 Position Mode", {"Touch", "Gap", "Drop"});
	}
	if (field == "direction") {
		return SpatialDeclarationCompletionContext(
			"Direction Frame",
			{"TargetInterface 0 0 -1", "World 0 -1 0", "Parent 0 -1 0",
			 "MovingObject 0 -1 0", "MovingInterface 0 0 1",
			 "TargetObject 0 -1 0"});
	}
	if (field == "region") {
		return SpatialDeclarationCompletionContext(
			"Interface Region",
			{"Point", "PlaneRectangle 1 1", "AxisSegment 1",
			 "ObjectBoundaryFace top", "ObjectBoundaryFace bottom",
			 "ObjectBoundaryFace front", "ObjectBoundaryFace back"});
	}
	return {};
}

} // namespace

SpatialDeclarationCompletionContext SpatialDeclarationCompletionService::analyze(
	const std::string &source_before_cursor) const
{
	const std::vector<CallFrame> stack = active_call_stack(source_before_cursor);
	const SpatialDeclarationCompletionContext values = value_completions(stack);
	if (values.isActive()) return values;

	if (!stack.empty()) {
		const std::string &current = stack.back().name;
		if (current == "Object") {
			return SpatialDeclarationCompletionContext(
				"Object Field",
				{"id(ObjectId)", "name(DisplayName)", "class(ObjectClass)",
				 "taxonomy(Building Assembly Component)", "container(ParentObjectId)",
				 "layer(Temporary)", "mask(Structure Interior Furniture)"});
		}
		if (current == "Interface") {
			return SpatialDeclarationCompletionContext(
				"Interface Field",
				{"id(interfaceId)", "type(InspectionInterface)", "origin(0 0 0)",
				 "normal(0 1 0)", "tangent(1 0 0)", "region(Point)",
				 "tolerance(0.0005)", "clearance(0 0 0)"});
		}
		if (current == "Connect") {
			return SpatialDeclarationCompletionContext(
				"Connection Field",
				{"id(ConnectionId)", "source(ObjectId interfaceId)",
				 "target(ObjectId interfaceId)", "type(ConnectedTo)",
				 "clearance(0 0 0)", "insertionDepth(0)"});
		}
		if (current == "Position") {
			return SpatialDeclarationCompletionContext(
				"Position Field",
				{"id(ConstraintId)", "moving(interfaceId)",
				 "target(ObjectId interfaceId)", "mode(Drop)",
				 "direction(TargetInterface 0 0 -1)", "clearance(0 0 0)",
				 "seatingDepth(0)", "maxDistance(10)", "tolerance(0.0005)",
				 "priority(0)", "mask(Structure Interior)"});
		}
		return {};
	}

	if (has_open_object_body(source_before_cursor)) {
		return SpatialDeclarationCompletionContext(
			"Spatial Object Body",
			{"Object(id(Child) class(Component) layer(Temporary) mask()) [ ]",
			 "Interface(id(interfaceId) type(InspectionInterface) origin(0 0 0) normal(0 1 0) tangent(1 0 0) region(Point) tolerance(0.0005) clearance(0 0 0))",
			 "Connect(id(ConnectionId) source(SourceObject interfaceId) target(TargetObject interfaceId) type(ConnectedTo) clearance(0 0 0) insertionDepth(0))",
			 "Position(id(ConstraintId) moving(interfaceId) target(TargetObject interfaceId) mode(Drop) direction(TargetInterface 0 0 -1) clearance(0 0 0) seatingDepth(0) maxDistance(10) tolerance(0.0005) priority(0))"});
	}
	return {};
}
