#include "ExperimentParameters.h"

#include "ManifestInput.h"
#include "RenderFiles.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace {

constexpr std::size_t maximum_candidates = 4096;

bool ascii_letter(char character)
{
	return (character >= 'A' && character <= 'Z') || (character >= 'a' && character <= 'z');
}

bool ascii_digit(char character)
{
	return character >= '0' && character <= '9';
}

bool literal_boundary(char character)
{
	return character == ' ' || character == '\t' || character == '\r' ||
	       character == '(' || character == ')' || character == '[' || character == ']' ||
	       character == ',' || character == ';';
}

bool is_numeric_token(std::string_view value)
{
	double number = 0;
	const auto parsed = std::from_chars(value.data(), value.data() + value.size(), number);
	return parsed.ec == std::errc{} && parsed.ptr == value.data() + value.size() && std::isfinite(number);
}

std::string format_value(const Json::Value &value)
{
	if (value.isString() && experiment_identifier(value.asString())) return value.asString();
	if (value.isNumeric() && std::isfinite(value.asDouble()) &&
	    std::abs(value.asDouble()) <= std::numeric_limits<float>::max()) {
		std::array<char, 512> buffer{};
		const auto converted = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value.asDouble(),
		                                     std::chars_format::fixed);
		if (converted.ec == std::errc{}) return {buffer.data(), converted.ptr};
	}
	throw std::runtime_error("Parameter values must be finite grammar numbers or bare identifiers.");
}

std::vector<Json::Value> read_axis_values(const Json::Value &entry, const std::string &label)
{
	std::vector<Json::Value> values;
	if (entry.isMember("values")) {
		const auto &array = entry["values"];
		if (!array.isArray() || array.empty() || array.size() > maximum_candidates ||
		    entry.isMember("min") || entry.isMember("max") || entry.isMember("step")) {
			throw std::runtime_error(label + " needs a nonempty values array or a min/max/step range.");
		}
		for (const auto &value : array) values.push_back(value);
	} else {
		const double lower = manifest_input::read_number(entry["min"], label + ".min");
		const double upper = manifest_input::read_number(entry["max"], label + ".max");
		const double step = manifest_input::read_number(entry["step"], label + ".step");
		const double intervals = (upper - lower) / step;
		if (!(step > 0.0 && lower <= upper && std::isfinite(intervals) && intervals < maximum_candidates) ||
		    (lower < upper && lower + step == lower)) {
			throw std::runtime_error(label + " needs an ascending finite range with a positive step.");
		}
		const auto count = static_cast<std::size_t>(std::floor(intervals + 1.0e-9)) + 1;
		for (std::size_t index = 0; index < count; ++index) {
			values.emplace_back(std::min(upper, lower + index * step));
		}
	}
	std::set<std::string> unique;
	for (const auto &value : values) {
		if (!unique.insert(format_value(value)).second) throw std::runtime_error(label + " repeats a candidate value.");
	}
	return values;
}

std::size_t annotated_literal(std::string_view code, std::string_view literal, const std::string &name)
{
	std::size_t found = std::string_view::npos;
	for (std::size_t position = code.find(literal); position != std::string_view::npos;
	     position = code.find(literal, position + literal.size())) {
		const auto after = position + literal.size();
		if ((position > 0 && !literal_boundary(code[position - 1])) ||
		    (after < code.size() && !literal_boundary(code[after]))) continue;
		if (found != std::string_view::npos) throw std::runtime_error("Ambiguous grammar literal for " + name + ".");
		found = position;
	}
	if (found == std::string_view::npos) throw std::runtime_error("Cannot find the annotated grammar literal for " + name + ".");
	return found;
}

}

bool experiment_identifier(std::string_view text)
{
	return !text.empty() && (ascii_letter(text.front()) || text.front() == '_') &&
	       std::all_of(text.begin(), text.end(), [](char character) {
		       return ascii_letter(character) || ascii_digit(character) || character == '_';
	       });
}

ParameterGrid ParameterGrid::parse(const Json::Value &specification, const std::string &label)
{
	ParameterGrid grid;
	if (specification.isNull()) return grid;
	if (!specification.isArray()) throw std::runtime_error(label + " must be a parameter array.");
	std::set<std::string> names;
	for (const auto &entry : specification) {
		manifest_input::check_fields(entry, {"name", "values", "min", "max", "step"}, label);
		const auto name = manifest_input::read_string(entry["name"], label + ".name");
		if (!experiment_identifier(name) || !names.insert(name).second) {
			throw std::runtime_error(label + " needs unique parameter identifiers.");
		}
		auto values = read_axis_values(entry, label + "." + name);
		if (values.size() > maximum_candidates / grid.size_) {
			throw std::runtime_error(label + " exceeds 4096 candidate combinations.");
		}
		grid.size_ *= values.size();
		grid.axes_.push_back({name, std::move(values)});
	}
	return grid;
}

void ParameterGrid::forEach(const std::function<void(const Json::Value &)> &evaluate) const
{
	for (std::size_t index = 0; index < size_; ++index) {
		Json::Value candidate(Json::objectValue);
		std::size_t remaining = index;
		for (auto axis = axes_.rbegin(); axis != axes_.rend(); ++axis) {
			candidate[axis->name] = axis->values[remaining % axis->values.size()];
			remaining /= axis->values.size();
		}
		evaluate(candidate);
	}
}

ParameterizedGrammar::ParameterizedGrammar(const std::filesystem::path &source)
	: source_(render_files::read_text(source))
{
	for (std::size_t start = 0; start < source_.size();) {
		const auto newline = source_.find('\n', start);
		const auto end = newline == std::string::npos ? source_.size() : newline;
		const std::string_view line(source_.data() + start, end - start);
		const auto comment = std::min(line.find('#'), line.find("//"));
		if (comment != std::string_view::npos && line[comment] == '#') {
			std::istringstream annotation{std::string(line.substr(comment + 1))};
			std::string marker, name, original, extra;
			annotation >> marker;
			if (marker == "@param") {
				annotation >> name >> original;
				const bool numeric = is_numeric_token(original);
				if (!experiment_identifier(name) || (!numeric && !experiment_identifier(original)) ||
				    (annotation >> extra)) {
					throw std::runtime_error("Expected a grammar annotation of the form # @param name literal.");
				}
				const auto offset = annotated_literal(line.substr(0, comment), original, name);
				const auto annotation_value = line.find_last_not_of(" \t\r") + 1 - original.size();
				if (!bindings_.emplace(name, Binding{start + offset, start + annotation_value, original, numeric}).second) {
					throw std::runtime_error("Duplicate grammar parameter annotation: " + name);
				}
			}
		}
		start = end + 1;
	}
}

void ParameterizedGrammar::checkBindings(const ParameterGrid &grid) const
{
	for (const auto &axis : grid.axes()) {
		const auto binding = bindings_.find(axis.name);
		if (binding == bindings_.end()) throw std::runtime_error("No grammar annotation for " + axis.name + ".");
		for (const auto &value : axis.values) {
			if (value.isNumeric() != binding->second.numeric) {
				throw std::runtime_error("Parameter " + axis.name + " must keep the annotated literal's type.");
			}
		}
	}
}

std::string ParameterizedGrammar::apply(const Json::Value &values) const
{
	struct Edit {
		std::size_t offset;
		std::size_t length;
		std::string replacement;
	};
	std::vector<Edit> edits;
	for (const auto &name : values.getMemberNames()) {
		const auto &binding = bindings_.at(name);
		const auto replacement = format_value(values[name]);
		edits.push_back({binding.literal, binding.original.size(), replacement});
		edits.push_back({binding.annotation_value, binding.original.size(), replacement});
	}
	std::sort(edits.begin(), edits.end(), [](const Edit &first, const Edit &second) {
		return first.offset > second.offset;
	});
	std::string result = source_;
	for (const auto &edit : edits) result.replace(edit.offset, edit.length, edit.replacement);
	return result;
}
