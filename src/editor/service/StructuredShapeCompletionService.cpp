#include "editor/service/StructuredShapeCompletionService.h"

#include "geometry/service/ProceduralShapeCatalogRepository.h"

#include <algorithm>
#include <cctype>
#include <regex>
#include <string>
#include <vector>

namespace {

struct CallFrame
{
	std::string name;
	std::size_t open_parenthesis = 0;
};

bool identifier_character(char character)
{
	return std::isalnum(static_cast<unsigned char>(character)) != 0 ||
	       character == '_';
}

std::string identifier_before(
	const std::string &source,
	std::size_t parenthesis)
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
			stack.push_back({identifier_before(source, index), index});
		} else if (source[index] == ')' && !stack.empty()) {
			stack.pop_back();
		}
	}
	return stack;
}

std::vector<std::string> declared_profile_names(const std::string &source)
{
	static const std::regex profile_expression(
		R"(profile\s*\(\s*([A-Za-z_][A-Za-z0-9_]*))");
	std::vector<std::string> names;
	for (std::sregex_iterator match(source.begin(), source.end(), profile_expression), end;
	     match != end; ++match) {
		const std::string name = (*match)[1].str();
		if (std::find(names.begin(), names.end(), name) == names.end()) {
			names.push_back(name);
		}
	}
	return names;
}

}

ShapeCompletionContext StructuredShapeCompletionService::analyze(
	const std::string &source_before_cursor) const
{
	const std::vector<CallFrame> stack = active_call_stack(source_before_cursor);
	std::size_t instance_index = stack.size();
	for (std::size_t index = stack.size(); index > 0; --index) {
		if (stack[index - 1].name == "I" || stack[index - 1].name == "!I") {
			instance_index = index - 1;
			break;
		}
	}
	if (instance_index == stack.size() || instance_index + 1 >= stack.size()) {
		return {};
	}

	const CallFrame &shape_frame = stack[instance_index + 1];
	const std::string &shape_name = shape_frame.name;
	const ProceduralShapeCatalogRepository catalog;
	if (catalog.find(shape_name) == nullptr) return {};

	const CallFrame &current_frame = stack.back();
	if (shape_name == "Extrude") {
		if (current_frame.name == "Extrude") {
			return ShapeCompletionContext(
				shape_name,
				{"Rect(1 1) 1 cap(all)",
				 "RoundedRect(1 1 0.1 4) 1 cap(all)",
				 "Circle(0.5 32) 1 cap(all)",
				 "Ellipse(0.5 0.25 32) 1 cap(all)",
				 "ChamferRect(1 1 0.1) 1 cap(all)",
				 "Polygon(-0.5 -0.5 0.5 -0.5 0.5 0.5 -0.5 0.5) 1 cap(all)"});
		}
		return {};
	}
	if (shape_name != "AxialProfile") {
		return current_frame.name == shape_name
			? ShapeCompletionContext(shape_name, catalog.optionCompletions(shape_name))
			: ShapeCompletionContext();
	}

	if (current_frame.name == "AxialProfile") {
		return ShapeCompletionContext(
			shape_name,
			{"axis(y)",
			 "profile(Name polygon(-0.5 -0.5 0.5 -0.5 0.5 0.5 -0.5 0.5))",
			 "at(0 Name)",
			 "hold(1)",
			 "linear(1 Name)",
			 "step(Name)",
			 "cap(all)"});
	}
	if (current_frame.name == "profile") {
		return ShapeCompletionContext(
			shape_name,
			{"polygon(-0.5 -0.5 0.5 -0.5 0.5 0.5 -0.5 0.5)"});
	}
	if (current_frame.name == "at" || current_frame.name == "linear" ||
	    current_frame.name == "step") {
		std::vector<std::string> completions = declared_profile_names(
			source_before_cursor.substr(shape_frame.open_parenthesis));
		completions.push_back("center(0 0)");
		completions.push_back("scale(1 1)");
		completions.push_back("rotate(0)");
		return ShapeCompletionContext(shape_name, std::move(completions));
	}
	return {};
}
