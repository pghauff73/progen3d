#include "StlCatalog.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <sstream>
#include <string>
#include <system_error>
#include <unordered_map>
#include <unordered_set>

#include "AppPaths.h"
#include "geometry/service/ProceduralShapeCatalogRepository.h"

std::string derive_stl_category_path(const std::string &category);

std::vector<std::string> part_class_names;
std::unordered_set<std::string> part_class_name_set;
std::vector<std::string> stl_part_names;
std::unordered_set<std::string> stl_part_name_set;
std::vector<std::string> stl_category_names;
std::vector<StlCatalogEntry> stl_catalog_entries;
std::unordered_map<std::string, StlCatalogEntry> stl_catalog_lookup;
std::unordered_map<std::string, std::vector<std::string>> stl_part_names_by_category;
StlCategoryTreeNode stl_category_tree_root;
bool has_stl_category_tree = false;

namespace {

void debugout(const std::string &message)
{
	std::cerr << message << '\n';
}

bool read_text_file(const std::string &path, std::string *text)
{
	if (text == nullptr) {
		return false;
	}

	std::ifstream fin(path);
	if (!fin.is_open()) {
		return false;
	}

	std::stringstream buffer;
	buffer << fin.rdbuf();
	*text = buffer.str();
	return true;
}

std::vector<std::string> default_part_class_catalog()
{
	std::vector<std::string> names = {
		"Cube", "CubeX", "CubeY", "CubeZ", "Cylinder", "Sphere"};
	const ProceduralShapeCatalogRepository procedural_catalog;
	for (const std::string &name : procedural_catalog.completionNames()) {
		if (std::find(names.begin(), names.end(), name) == names.end()) {
			names.push_back(name);
		}
	}
	return names;
}

std::string json_escape_string(const std::string &value)
{
	std::string escaped;
	escaped.reserve(value.size() + 8);
	for (char character : value) {
		switch (character) {
		case '\\':
			escaped += "\\\\";
			break;
		case '"':
			escaped += "\\\"";
			break;
		case '\n':
			escaped += "\\n";
			break;
		case '\r':
			escaped += "\\r";
			break;
		case '\t':
			escaped += "\\t";
			break;
		default:
			escaped.push_back(character);
			break;
		}
	}
	return escaped;
}

std::string lowercase_copy(std::string value)
{
	for (char &character : value) {
		character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
	}
	return value;
}

std::vector<std::string> split_string_non_empty(const std::string &value, char delimiter)
{
	std::vector<std::string> parts;
	std::stringstream stream(value);
	std::string part;
	while (std::getline(stream, part, delimiter)) {
		if (!part.empty()) {
			parts.push_back(part);
		}
	}
	return parts;
}

std::string join_string_tokens(const std::vector<std::string> &tokens, char delimiter)
{
	std::string result;
	for (std::size_t index = 0; index < tokens.size(); ++index) {
		if (index > 0) {
			result.push_back(delimiter);
		}
		result += tokens[index];
	}
	return result;
}

void sort_and_unique_strings(std::vector<std::string> *values)
{
	if (values == nullptr) {
		return;
	}
	std::sort(values->begin(), values->end());
	values->erase(std::unique(values->begin(), values->end()), values->end());
}

std::vector<std::string> parse_json_string_array(const std::string &json_text)
{
	std::vector<std::string> values;
	bool in_string = false;
	bool escaping = false;
	std::string current;

	for (char character : json_text) {
		if (!in_string) {
			if (character == '"') {
				in_string = true;
				current.clear();
			}
			continue;
		}

		if (escaping) {
			current.push_back(character);
			escaping = false;
			continue;
		}

		if (character == '\\') {
			escaping = true;
			continue;
		}
		if (character == '"') {
			values.push_back(current);
			in_string = false;
			continue;
		}

		current.push_back(character);
	}

	return values;
}

bool is_json_whitespace(char character)
{
	return character == ' ' || character == '\t' || character == '\n' || character == '\r';
}

std::string trim_json_copy(const std::string &value)
{
	std::size_t start = 0;
	while (start < value.size() && is_json_whitespace(value[start])) {
		++start;
	}
	std::size_t end = value.size();
	while (end > start && is_json_whitespace(value[end - 1])) {
		--end;
	}
	return value.substr(start, end - start);
}

std::string parse_json_string_literal(const std::string &json_text,
                                      std::size_t start,
                                      std::size_t *end_index = nullptr)
{
	if (start >= json_text.size() || json_text[start] != '"') {
		if (end_index != nullptr) {
			*end_index = start;
		}
		return "";
	}

	std::string value;
	bool escaping = false;
	for (std::size_t index = start + 1; index < json_text.size(); ++index) {
		const char character = json_text[index];
		if (escaping) {
			switch (character) {
			case 'n':
				value.push_back('\n');
				break;
			case 'r':
				value.push_back('\r');
				break;
			case 't':
				value.push_back('\t');
				break;
			case '\\':
				value.push_back('\\');
				break;
			case '"':
				value.push_back('"');
				break;
			case '/':
				value.push_back('/');
				break;
			case 'b':
				value.push_back('\b');
				break;
			case 'f':
				value.push_back('\f');
				break;
			case 'u':
				for (int hex_index = 0; hex_index < 4 && index + 1 < json_text.size(); ++hex_index) {
					++index;
				}
				value.push_back('?');
				break;
			default:
				value.push_back(character);
				break;
			}
			escaping = false;
			continue;
		}
		if (character == '\\') {
			escaping = true;
			continue;
		}
		if (character == '"') {
			if (end_index != nullptr) {
				*end_index = index + 1;
			}
			return value;
		}
		value.push_back(character);
	}

	if (end_index != nullptr) {
		*end_index = json_text.size();
	}
	return value;
}

std::vector<std::string> extract_top_level_json_objects(const std::string &json_text)
{
	std::vector<std::string> objects;
	const std::size_t array_start = json_text.find('[');
	if (array_start == std::string::npos) {
		return objects;
	}

	bool in_string = false;
	bool escaping = false;
	int object_depth = 0;
	int array_depth = 0;
	std::size_t object_start = std::string::npos;
	for (std::size_t index = array_start + 1; index < json_text.size(); ++index) {
		const char character = json_text[index];
		if (in_string) {
			if (escaping) {
				escaping = false;
				continue;
			}
			if (character == '\\') {
				escaping = true;
				continue;
			}
			if (character == '"') {
				in_string = false;
			}
			continue;
		}

		if (character == '"') {
			in_string = true;
			continue;
		}
		if (character == '{') {
			if (object_depth == 0 && array_depth == 0) {
				object_start = index;
			}
			++object_depth;
			continue;
		}
		if (character == '}') {
			if (object_depth == 0) {
				continue;
			}
			--object_depth;
			if (object_depth == 0 && array_depth == 0 && object_start != std::string::npos) {
				objects.push_back(json_text.substr(object_start, index - object_start + 1));
				object_start = std::string::npos;
			}
			continue;
		}
		if (character == '[' && object_depth > 0) {
			++array_depth;
			continue;
		}
		if (character == ']' && array_depth > 0) {
			--array_depth;
		}
	}
	return objects;
}

std::unordered_map<std::string, std::string> parse_top_level_json_string_properties(
	const std::string &object_text)
{
	std::unordered_map<std::string, std::string> properties;
	const std::size_t object_start = object_text.find('{');
	const std::size_t object_end = object_text.rfind('}');
	if (object_start == std::string::npos || object_end == std::string::npos || object_end <= object_start) {
		return properties;
	}

	std::size_t index = object_start + 1;
	while (index < object_end) {
		while (index < object_end &&
		       (is_json_whitespace(object_text[index]) || object_text[index] == ',')) {
			++index;
		}
		if (index >= object_end || object_text[index] != '"') {
			break;
		}

		std::size_t key_end = index;
		const std::string key = parse_json_string_literal(object_text, index, &key_end);
		index = key_end;
		while (index < object_end && is_json_whitespace(object_text[index])) {
			++index;
		}
		if (index >= object_end || object_text[index] != ':') {
			break;
		}
		++index;
		while (index < object_end && is_json_whitespace(object_text[index])) {
			++index;
		}

		const std::size_t value_start = index;
		bool in_string = false;
		bool escaping = false;
		int nested_object_depth = 0;
		int nested_array_depth = 0;
		while (index < object_end) {
			const char character = object_text[index];
			if (in_string) {
				if (escaping) {
					escaping = false;
					++index;
					continue;
				}
				if (character == '\\') {
					escaping = true;
					++index;
					continue;
				}
				if (character == '"') {
					in_string = false;
				}
				++index;
				continue;
			}

			if (character == '"') {
				in_string = true;
				++index;
				continue;
			}
			if (character == '{') {
				++nested_object_depth;
				++index;
				continue;
			}
			if (character == '}') {
				if (nested_object_depth == 0 && nested_array_depth == 0) {
					break;
				}
				nested_object_depth = std::max(0, nested_object_depth - 1);
				++index;
				continue;
			}
			if (character == '[') {
				++nested_array_depth;
				++index;
				continue;
			}
			if (character == ']') {
				nested_array_depth = std::max(0, nested_array_depth - 1);
				++index;
				continue;
			}
			if (character == ',' && nested_object_depth == 0 && nested_array_depth == 0) {
				break;
			}
			++index;
		}

		const std::string raw_value = trim_json_copy(object_text.substr(value_start, index - value_start));
		if (!raw_value.empty() && raw_value.front() == '"') {
			properties[key] = parse_json_string_literal(raw_value, 0);
		} else {
			properties[key] = raw_value;
		}
		if (index < object_end && object_text[index] == ',') {
			++index;
		}
	}

	return properties;
}

float parse_json_number(const std::string &value, float fallback = 0.0f)
{
	const std::string trimmed = trim_json_copy(value);
	if (trimmed.empty()) {
		return fallback;
	}
	try {
		return std::stof(trimmed);
	} catch (...) {
		return fallback;
	}
}

std::array<int, 3> parse_json_int_array3(const std::string &value)
{
	std::array<int, 3> result{0, 0, 0};
	const std::string trimmed = trim_json_copy(value);
	const std::size_t start = trimmed.find('[');
	const std::size_t end = trimmed.rfind(']');
	if (start == std::string::npos || end == std::string::npos || end <= start) {
		return result;
	}

	std::stringstream stream(trimmed.substr(start + 1, end - start - 1));
	for (std::size_t index = 0; index < result.size(); ++index) {
		std::string token;
		if (!std::getline(stream, token, ',')) {
			break;
		}
		result[index] = static_cast<int>(std::lround(parse_json_number(token, 0.0f)));
	}
	return result;
}

std::array<float, 3> parse_json_xyz_object(const std::string &value)
{
	std::array<float, 3> result{0.0f, 0.0f, 0.0f};
	const std::unordered_map<std::string, std::string> fields =
		parse_top_level_json_string_properties(value);
	const auto x_it = fields.find("x");
	const auto y_it = fields.find("y");
	const auto z_it = fields.find("z");
	if (x_it != fields.end()) {
		result[0] = parse_json_number(x_it->second, 0.0f);
	}
	if (y_it != fields.end()) {
		result[1] = parse_json_number(y_it->second, 0.0f);
	}
	if (z_it != fields.end()) {
		result[2] = parse_json_number(z_it->second, 0.0f);
	}
	return result;
}

std::array<float, 3> parse_json_size_object(const std::string &value)
{
	std::array<float, 3> result{0.0f, 0.0f, 0.0f};
	const std::unordered_map<std::string, std::string> fields =
		parse_top_level_json_string_properties(value);
	const auto sx_it = fields.find("sx");
	const auto sy_it = fields.find("sy");
	const auto sz_it = fields.find("sz");
	if (sx_it != fields.end()) {
		result[0] = parse_json_number(sx_it->second, 0.0f);
	}
	if (sy_it != fields.end()) {
		result[1] = parse_json_number(sy_it->second, 0.0f);
	}
	if (sz_it != fields.end()) {
		result[2] = parse_json_number(sz_it->second, 0.0f);
	}
	return result;
}

std::array<float, 3> parse_json_extents_object(const std::string &value)
{
	std::array<float, 3> result{0.0f, 0.0f, 0.0f};
	const std::unordered_map<std::string, std::string> fields =
		parse_top_level_json_string_properties(value);
	const auto length_it = fields.find("length");
	const auto width_it = fields.find("width");
	const auto height_it = fields.find("height");
	if (length_it != fields.end()) {
		result[0] = parse_json_number(length_it->second, 0.0f);
	}
	if (width_it != fields.end()) {
		result[1] = parse_json_number(width_it->second, 0.0f);
	}
	if (height_it != fields.end()) {
		result[2] = parse_json_number(height_it->second, 0.0f);
	}
	return result;
}

StlCatalogEntry::StlGeometry parse_stl_geometry(const std::string &value)
{
	StlCatalogEntry::StlGeometry geometry;
	const std::unordered_map<std::string, std::string> fields =
		parse_top_level_json_string_properties(value);
	const auto bounding_box_it = fields.find("bounding_box");
	if (bounding_box_it != fields.end()) {
		const std::unordered_map<std::string, std::string> bbox_fields =
			parse_top_level_json_string_properties(bounding_box_it->second);
		const auto min_it = bbox_fields.find("min");
		const auto max_it = bbox_fields.find("max");
		const auto size_it = bbox_fields.find("size");
		const auto center_it = bbox_fields.find("center");
		if (min_it != bbox_fields.end()) {
			geometry.bounding_box.min = parse_json_xyz_object(min_it->second);
		}
		if (max_it != bbox_fields.end()) {
			geometry.bounding_box.max = parse_json_xyz_object(max_it->second);
		}
		if (size_it != bbox_fields.end()) {
			geometry.bounding_box.size = parse_json_xyz_object(size_it->second);
		}
		if (center_it != bbox_fields.end()) {
			geometry.bounding_box.center = parse_json_xyz_object(center_it->second);
		}
	}
	const auto center_of_mass_it = fields.find("center_of_mass");
	if (center_of_mass_it != fields.end()) {
		geometry.center_of_mass = parse_json_xyz_object(center_of_mass_it->second);
	}
	return geometry;
}

std::vector<StlCatalogEntry::ConnectionPoint> parse_connection_points(const std::string &array_text)
{
	std::vector<StlCatalogEntry::ConnectionPoint> points;
	for (const std::string &object_text : extract_top_level_json_objects(array_text)) {
		const std::unordered_map<std::string, std::string> fields =
			parse_top_level_json_string_properties(object_text);
		StlCatalogEntry::ConnectionPoint point;
		const auto name_it = fields.find("name");
		const auto type_it = fields.find("type");
		const auto axis_it = fields.find("axis_vector");
		const auto movement_it = fields.find("movement_type");
		const auto center_it = fields.find("center");
		const auto center_numeric_it = fields.find("center_numeric");
		const auto extents_numeric_it = fields.find("extents_numeric");
		const auto diameter_numeric_it = fields.find("diameter_numeric");
		const auto width_numeric_it = fields.find("width_numeric");
		const auto notes_it = fields.find("notes");
		if (name_it != fields.end()) {
			point.name = name_it->second;
		}
		if (type_it != fields.end()) {
			point.type = type_it->second;
		}
		if (axis_it != fields.end()) {
			point.axis_vector = parse_json_int_array3(axis_it->second);
		}
		if (movement_it != fields.end()) {
			point.movement_type = movement_it->second;
		}
		if (center_it != fields.end()) {
			point.center = parse_json_xyz_object(center_it->second);
		} else if (center_numeric_it != fields.end()) {
			point.center = parse_json_xyz_object(center_numeric_it->second);
		}
		if (extents_numeric_it != fields.end()) {
			point.extents_numeric.size = parse_json_extents_object(extents_numeric_it->second);
		}
		if (diameter_numeric_it != fields.end()) {
			point.diameter_numeric = parse_json_number(diameter_numeric_it->second, 0.0f);
		}
		if (width_numeric_it != fields.end()) {
			point.width_numeric = parse_json_number(width_numeric_it->second, 0.0f);
		}
		if (notes_it != fields.end()) {
			point.notes = notes_it->second;
		}
		if (!point.name.empty()) {
			points.push_back(std::move(point));
		}
	}
	return points;
}

std::vector<StlCatalogEntry::ConnectionSurface> parse_connection_surfaces(const std::string &array_text)
{
	std::vector<StlCatalogEntry::ConnectionSurface> surfaces;
	for (const std::string &object_text : extract_top_level_json_objects(array_text)) {
		const std::unordered_map<std::string, std::string> fields =
			parse_top_level_json_string_properties(object_text);
		StlCatalogEntry::ConnectionSurface surface;
		const auto name_it = fields.find("name");
		const auto source_it = fields.find("source_connection_point");
		const auto shape_it = fields.find("shape");
		const auto plane_it = fields.find("plane");
		const auto axis_it = fields.find("axis_vector");
		const auto center_it = fields.find("center");
		const auto center_numeric_it = fields.find("center_numeric");
		const auto bounds_numeric_it = fields.find("bounds_numeric");
		const auto diameter_numeric_it = fields.find("diameter_numeric");
		const auto thread_type_it = fields.find("thread_type");
		const auto thread_pitch_numeric_it = fields.find("thread_pitch_numeric");
		const auto thread_height_numeric_it = fields.find("thread_height_numeric");
		const auto thread_depth_numeric_it = fields.find("thread_depth_numeric");
		const auto notes_it = fields.find("notes");
		if (name_it != fields.end()) {
			surface.name = name_it->second;
		}
		if (source_it != fields.end()) {
			surface.source_connection_point = source_it->second;
		}
		if (shape_it != fields.end()) {
			surface.shape = shape_it->second;
		}
		if (plane_it != fields.end()) {
			surface.plane = plane_it->second;
		}
		if (axis_it != fields.end()) {
			surface.axis_vector = parse_json_int_array3(axis_it->second);
		}
		if (center_it != fields.end()) {
			surface.center = parse_json_xyz_object(center_it->second);
		} else if (center_numeric_it != fields.end()) {
			surface.center = parse_json_xyz_object(center_numeric_it->second);
		}
		if (bounds_numeric_it != fields.end()) {
			surface.bounds_numeric.size = parse_json_size_object(bounds_numeric_it->second);
		}
		if (diameter_numeric_it != fields.end()) {
			surface.diameter_numeric = parse_json_number(diameter_numeric_it->second, 0.0f);
		}
		if (thread_type_it != fields.end()) {
			surface.thread_type = thread_type_it->second;
		}
		if (thread_pitch_numeric_it != fields.end()) {
			surface.thread_pitch_numeric = parse_json_number(thread_pitch_numeric_it->second, 0.0f);
		}
		if (thread_height_numeric_it != fields.end()) {
			surface.thread_height_numeric = parse_json_number(thread_height_numeric_it->second, 0.0f);
		}
		if (thread_depth_numeric_it != fields.end()) {
			surface.thread_depth_numeric = parse_json_number(thread_depth_numeric_it->second, 0.0f);
		}
		if (notes_it != fields.end()) {
			surface.notes = notes_it->second;
		}
		if (!surface.name.empty()) {
			surfaces.push_back(std::move(surface));
		}
	}
	return surfaces;
}

std::vector<StlCatalogEntry::DetectedConnection> parse_scad_detected_connections(
	const std::string &array_text)
{
	std::vector<StlCatalogEntry::DetectedConnection> features;
	for (const std::string &object_text : extract_top_level_json_objects(array_text)) {
		const std::unordered_map<std::string, std::string> fields =
			parse_top_level_json_string_properties(object_text);
		StlCatalogEntry::DetectedConnection feature;
		const auto name_it = fields.find("name");
		const auto primitive_it = fields.find("primitive");
		const auto feature_role_it = fields.find("feature_role");
		const auto axis_it = fields.find("axis_vector");
		const auto plane_it = fields.find("plane");
		const auto center_it = fields.find("center");
		const auto bounds_numeric_it = fields.find("bounds_numeric");
		const auto diameter_numeric_it = fields.find("diameter_numeric");
		const auto length_numeric_it = fields.find("length_numeric");
		const auto outer_diameter_numeric_it = fields.find("outer_diameter_numeric");
		const auto inner_diameter_numeric_it = fields.find("inner_diameter_numeric");
		const auto metric_size_it = fields.find("metric_size");
		const auto thread_type_it = fields.find("thread_type");
		const auto thread_pitch_numeric_it = fields.find("thread_pitch_numeric");
		const auto source_file_it = fields.find("source_file");
		const auto source_module_it = fields.find("source_module");
		const auto evidence_it = fields.find("evidence");
		if (name_it != fields.end()) {
			feature.name = name_it->second;
		}
		if (primitive_it != fields.end()) {
			feature.primitive = primitive_it->second;
		}
		if (feature_role_it != fields.end()) {
			feature.feature_role = feature_role_it->second;
		}
		if (axis_it != fields.end()) {
			feature.axis_vector = parse_json_int_array3(axis_it->second);
		}
		if (plane_it != fields.end()) {
			feature.plane = plane_it->second;
		}
		if (center_it != fields.end()) {
			feature.center = parse_json_xyz_object(center_it->second);
		}
		if (bounds_numeric_it != fields.end()) {
			feature.bounds_numeric.size = parse_json_size_object(bounds_numeric_it->second);
		}
		if (diameter_numeric_it != fields.end()) {
			feature.diameter_numeric = parse_json_number(diameter_numeric_it->second, 0.0f);
		}
		if (length_numeric_it != fields.end()) {
			feature.length_numeric = parse_json_number(length_numeric_it->second, 0.0f);
		}
		if (outer_diameter_numeric_it != fields.end()) {
			feature.outer_diameter_numeric = parse_json_number(outer_diameter_numeric_it->second, 0.0f);
		}
		if (inner_diameter_numeric_it != fields.end()) {
			feature.inner_diameter_numeric = parse_json_number(inner_diameter_numeric_it->second, 0.0f);
		}
		if (metric_size_it != fields.end()) {
			feature.metric_size = metric_size_it->second;
		}
		if (thread_type_it != fields.end()) {
			feature.thread_type = thread_type_it->second;
		}
		if (thread_pitch_numeric_it != fields.end()) {
			feature.thread_pitch_numeric = parse_json_number(thread_pitch_numeric_it->second, 0.0f);
		}
		if (source_file_it != fields.end()) {
			feature.source_file = source_file_it->second;
		}
		if (source_module_it != fields.end()) {
			feature.source_module = source_module_it->second;
		}
		if (evidence_it != fields.end()) {
			feature.evidence = evidence_it->second;
		}
		if (!feature.name.empty()) {
			features.push_back(std::move(feature));
		}
	}
	return features;
}

bool stl_name_contains(const std::string &value, std::initializer_list<const char *> needles)
{
	for (const char *needle : needles) {
		if (needle != nullptr && value.find(needle) != std::string::npos) {
			return true;
		}
	}
	return false;
}

std::string dot_path_from_stl_category_path(const std::string &category_path)
{
	std::string dot_path = category_path;
	std::replace(dot_path.begin(), dot_path.end(), '/', '.');
	return dot_path;
}

StlCatalogEntry normalize_stl_catalog_entry(const std::string &name,
                                            const std::string &category,
                                            const std::string &description = "",
                                            const std::string &short_description = "",
                                            const std::string &tooltip = "",
                                            const std::string &source_file = "",
                                            const std::string &source_module = "",
                                            const std::string &implementation_file = "",
                                            const std::string &implementation_module = "")
{
	StlCatalogEntry entry;
	entry.name = name;
	entry.category = sanitize_stl_category(category.empty() ? infer_stl_category(name) : category);
	entry.category_path = derive_stl_category_path(entry.category);
	entry.category_depth = static_cast<int>(split_string_non_empty(entry.category_path, '/').size());
	entry.qualified_name = entry.category.empty() ? entry.name : entry.category + "." + entry.name;
	entry.description = description;
	entry.short_description = short_description;
	entry.tooltip = tooltip;
	entry.source_file = source_file;
	entry.source_module = source_module;
	entry.implementation_file = implementation_file;
	entry.implementation_module = implementation_module;
	return entry;
}

StlCategoryTreeNode parse_stl_category_tree_node(const std::string &object_text)
{
	StlCategoryTreeNode node;
	const std::unordered_map<std::string, std::string> fields =
		parse_top_level_json_string_properties(object_text);
	const auto name_it = fields.find("name");
	const auto path_it = fields.find("path");
	const auto path_string_it = fields.find("path_string");
	const auto count_it = fields.find("count");
	const auto categories_it = fields.find("categories");
	const auto qualified_names_it = fields.find("qualified_names");
	const auto names_it = fields.find("names");
	const auto children_it = fields.find("children");
	if (name_it != fields.end()) {
		node.name = name_it->second;
	}
	if (path_it != fields.end()) {
		node.path = parse_json_string_array(path_it->second);
	}
	if (path_string_it != fields.end()) {
		node.path_string = path_string_it->second;
	}
	if (count_it != fields.end()) {
		node.count = static_cast<int>(std::lround(parse_json_number(count_it->second, 0.0f)));
	}
	if (categories_it != fields.end()) {
		node.categories = parse_json_string_array(categories_it->second);
	}
	if (qualified_names_it != fields.end()) {
		node.qualified_names = parse_json_string_array(qualified_names_it->second);
	}
	if (names_it != fields.end()) {
		node.names = parse_json_string_array(names_it->second);
	}
	if (children_it != fields.end()) {
		for (const std::string &child_text : extract_top_level_json_objects(children_it->second)) {
			node.children.push_back(parse_stl_category_tree_node(child_text));
		}
	}
	return node;
}

void normalize_stl_category_tree_node(StlCategoryTreeNode *node)
{
	if (node == nullptr) {
		return;
	}
	sort_and_unique_strings(&node->categories);
	sort_and_unique_strings(&node->qualified_names);
	sort_and_unique_strings(&node->names);
	for (StlCategoryTreeNode &child : node->children) {
		normalize_stl_category_tree_node(&child);
	}
	std::sort(node->children.begin(),
	          node->children.end(),
	          [](const StlCategoryTreeNode &lhs, const StlCategoryTreeNode &rhs) {
		          return lhs.name < rhs.name;
	          });
}

void append_catalog_entry_to_stl_category_tree(StlCategoryTreeNode *root, const StlCatalogEntry &entry)
{
	if (root == nullptr || entry.name.empty()) {
		return;
	}

	root->count += 1;
	const std::string category_path =
		entry.category_path.empty() ? derive_stl_category_path(entry.category) : entry.category_path;
	const std::vector<std::string> tokens = split_string_non_empty(category_path, '/');
	StlCategoryTreeNode *cursor = root;
	for (const std::string &token : tokens) {
		auto child_it = std::find_if(cursor->children.begin(),
		                             cursor->children.end(),
		                             [&](const StlCategoryTreeNode &child) {
			                             return child.name == token;
		                             });
		if (child_it == cursor->children.end()) {
			StlCategoryTreeNode child;
			child.name = token;
			child.path = cursor->path;
			child.path.push_back(token);
			child.path_string = join_string_tokens(child.path, '/');
			cursor->children.push_back(std::move(child));
			child_it = cursor->children.end() - 1;
		}
		child_it->count += 1;
		cursor = &(*child_it);
	}

	cursor->categories.push_back(entry.category);
	cursor->qualified_names.push_back(entry.qualified_name);
	cursor->names.push_back(entry.name);
}

void rebuild_stl_category_tree_from_catalog()
{
	StlCategoryTreeNode root;
	root.name = "root";
	root.path_string.clear();
	for (const StlCatalogEntry &entry : stl_catalog_entries) {
		append_catalog_entry_to_stl_category_tree(&root, entry);
	}
	normalize_stl_category_tree_node(&root);
	stl_category_tree_root = std::move(root);
	has_stl_category_tree = !stl_category_tree_root.children.empty();
}

void append_part_class_names(const std::vector<std::string> &names)
{
	for (const std::string &name : names) {
		if (name.empty() || name.rfind("STL.", 0) == 0) {
			continue;
		}
		if (part_class_name_set.insert(name).second) {
			part_class_names.push_back(name);
		}
	}
}

bool parse_startup_progress_line(const std::string &line, float *progress, std::string *detail)
{
	static const std::string prefix = "PROGRESS\t";
	if (line.rfind(prefix, 0) != 0) {
		return false;
	}

	const std::size_t completed_end = line.find('\t', prefix.size());
	if (completed_end == std::string::npos) {
		return false;
	}
	const std::size_t total_end = line.find('\t', completed_end + 1);
	if (total_end == std::string::npos) {
		return false;
	}

	const std::string completed_text = line.substr(prefix.size(), completed_end - prefix.size());
	const std::string total_text = line.substr(completed_end + 1, total_end - completed_end - 1);
	const std::string message = line.substr(total_end + 1);
	const float completed = parse_json_number(completed_text, 0.0f);
	const float total = std::max(parse_json_number(total_text, 1.0f), 1.0f);
	if (progress != nullptr) {
		*progress = std::clamp(completed / total, 0.0f, 1.0f);
	}
	if (detail != nullptr) {
		*detail = message;
	}
	return true;
}

} // namespace

std::string sanitize_stl_category(const std::string &category)
{
	std::string sanitized;
	sanitized.reserve(category.size());
	bool previous_was_separator = false;
	for (char character : category) {
		if (std::isalnum(static_cast<unsigned char>(character)) != 0) {
			sanitized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(character))));
			previous_was_separator = false;
			continue;
		}
		if (!sanitized.empty() && !previous_was_separator) {
			sanitized.push_back('_');
			previous_was_separator = true;
		}
	}
	while (!sanitized.empty() && sanitized.back() == '_') {
		sanitized.pop_back();
	}
	return sanitized.empty() ? "misc" : sanitized;
}

std::string derive_stl_category_path(const std::string &category)
{
	std::string path = sanitize_stl_category(category);
	std::replace(path.begin(), path.end(), '_', '/');
	return path;
}

std::string infer_stl_category(const std::string &name)
{
	const std::string lowered = lowercase_copy(name);
	if (lowered.rfind("test_", 0) == 0) {
		return "examples_tests";
	}
	if (lowered == "color_demo" || lowered == "demo_3d_gears" || lowered == "nema_demo") {
		return "examples_demos";
	}
	if (lowered == "frame_ref" || lowered == "name_tag") {
		return "examples_reference";
	}
	if (stl_name_contains(lowered, {"focus_ring", "camera", "manfrotto"})) {
		if (lowered.find("assembly") != std::string::npos) {
			return "camera_assemblies";
		}
		if (stl_name_contains(lowered, {"focus_ring", "lens"})) {
			return "camera_optics";
		}
		if (stl_name_contains(lowered, {"bracket", "manfrotto"})) {
			return "camera_mounts";
		}
		return "camera_housings";
	}
	if (lowered.find("ribbon_clamp") != std::string::npos) {
		return "cable_management_clamps";
	}
	if (lowered.find("grommet") != std::string::npos) {
		return "cable_management_grommets";
	}
	if (stl_name_contains(lowered, {"mask"})) {
		return "fabrication_masks";
	}
	if (stl_name_contains(lowered, {"joiner", "snap_lock", "snap_socket"})) {
		return "mechanical_joiners";
	}
	if (stl_name_contains(lowered, {"printer_fan", "e3d_fan", "fan_duct", "fan_guard", "fan_holes"})) {
		return "printer_parts_cooling";
	}
	if (stl_name_contains(lowered, {"printer_hotend", "hotend", "e3dv6", "bowden_connector"})) {
		return "printer_parts_hotends";
	}
	if (stl_name_contains(lowered, {"usb", "panel_usba"})) {
		return "connectors_data_usb";
	}
	if (lowered.find("rj45") != std::string::npos) {
		return "connectors_data_network";
	}
	if (stl_name_contains(lowered, {"molex", "terminal_35", "vero_pin", "wire_link", "usd"})) {
		return "connectors_signal_wire_to_board";
	}
	if (lowered.find("fuseholder") != std::string::npos) {
		return "connectors_power_fuseholders";
	}
	if (lowered.find("socket_box") != std::string::npos) {
		return "connectors_power_outlets";
	}
	if (stl_name_contains(lowered, {"barrel_jack", "power_jack", "jack"})) {
		return "connectors_power_jacks";
	}
	if (stl_name_contains(lowered, {"press_fit_socket", "socket"})) {
		return "connectors_mounting_interfaces";
	}
	if (lowered.find("electronics_enclosure") != std::string::npos) {
		return "electronics_enclosures";
	}
	if (lowered.find("ssr") != std::string::npos) {
		return "electronics_power_control";
	}
	if (stl_name_contains(lowered, {"carrier", "pcb_spacer", "pcb_mount_washer"})) {
		return "electronics_mounting";
	}
	if (lowered.find("pcb") != std::string::npos) {
		return "electronics_pcb_modules";
	}
	if (stl_name_contains(lowered, {"hygrometer", "microview", "trimpot", "buzzer", "chip", "solder_meniscus"})) {
		return "electronics_components";
	}
	if (stl_name_contains(lowered, {"nema17", "stepper"})) {
		return "motors_steppers";
	}
	if (stl_name_contains(lowered, {"towerprosg90", "servo"})) {
		return "motors_servos";
	}
	if (lowered.find("gear_motor") != std::string::npos) {
		return "motors_geared";
	}
	if (lowered == "opengrab" || lowered == "opengrab_target") {
		return "motors_actuators";
	}
	if (lowered.find("motor") != std::string::npos) {
		return "motors_generic";
	}
	if (stl_name_contains(lowered, {"leadnut_housing"})) {
		return "drivetrain_nut_housings";
	}
	if (stl_name_contains(lowered, {"leadnut", "ballnut"}) || lowered == "nut") {
		return "drivetrain_nuts";
	}
	if (stl_name_contains(lowered, {"leadscrew", "threaded_rod", "trapezoidal", "metric_threaded_rod"})) {
		return "drivetrain_screw_drives";
	}
	if (lowered.find("pulley") != std::string::npos) {
		return "drivetrain_pulleys";
	}
	if (lowered.find("rack") != std::string::npos) {
		return "drivetrain_racks";
	}
	if (stl_name_contains(lowered, {"hirth", "coupling"})) {
		return "drivetrain_couplings";
	}
	if (stl_name_contains(lowered, {"gear", "meshing_double_helix"})) {
		return "drivetrain_gears";
	}
	if (stl_name_contains(lowered, {"linear_bearing", "linearbearing", "linear_carriage", "linear_rail", "supported_rail", "sbr_rail", "sbr_bearing_block"})) {
		return "motion_linear_guides";
	}
	if (stl_name_contains(lowered, {"pillow_block", "shaft_support"})) {
		return "motion_supports";
	}
	if (lowered.find("smooth_rod") != std::string::npos) {
		return "motion_shafts";
	}
	if (stl_name_contains(lowered, {"bearing"})) {
		return "motion_bearings";
	}
	if (stl_name_contains(lowered, {"extrusion_2020", "printer_extrusion"})) {
		return "motion_structural_extrusions";
	}
	if (stl_name_contains(lowered, {"corner_block", "fixing_block", "2screw_block"})) {
		return "printed_hardware_structural_blocks";
	}
	if (lowered.find("handle") != std::string::npos) {
		return "printed_hardware_handles";
	}
	if (lowered.find("foot") != std::string::npos) {
		return "printed_hardware_feet";
	}
	if (stl_name_contains(lowered, {"screw_knob"})) {
		return "printed_hardware_knobs";
	}
	if (lowered.find("strap_end") != std::string::npos) {
		return "printed_hardware_strap_fittings";
	}
	if (stl_name_contains(lowered, {"door_hinge", "door_latch"})) {
		return "printed_hardware_closures";
	}
	if (stl_name_contains(lowered, {"pillar", "post_4mm", "insert_foot", "d_pillar", "press_fit_peg"})) {
		return "printed_hardware_mounting";
	}
	if (lowered == "fack2spm" || lowered == "flex") {
		return "printed_hardware_misc";
	}
	if (stl_name_contains(lowered, {"metric_nut", "nuthole", "bolthole", "fb_screw", "screw_lug"})) {
		return "fasteners_hardware";
	}
	if (stl_name_contains(lowered,
	                      {"cube", "cylinder", "sphere", "spheroid", "wedge", "tube", "prism",
	                       "pyramid", "triangle", "teardrop", "torus", "box", "octagon",
	                       "pentagon", "hexagon", "heptagon", "decagon", "dodecagon",
	                       "nonagon", "hendecagon", "cone", "ellipsoid", "elliptical", "egg",
	                       "auger", "axle", "rod", "post", "reinforcement", "dislocatebox",
	                       "roundedbox", "ball_groove", "shapes_", "regular_shapes"})) {
		return "geometry_primitives";
	}
	return "misc_functional";
}

std::vector<StlCatalogEntry> collect_stl_catalog_entries(const std::string &stls_dir)
{
	std::vector<StlCatalogEntry> entries;
	std::error_code error;
	const std::filesystem::path root(stls_dir);
	if (!std::filesystem::exists(root, error) || !std::filesystem::is_directory(root, error)) {
		return entries;
	}

	std::unordered_set<std::string> seen_names;
	for (std::filesystem::recursive_directory_iterator it(root, error);
	     !error && it != std::filesystem::recursive_directory_iterator();
	     it.increment(error)) {
		const std::filesystem::directory_entry &file_entry = *it;
		const std::filesystem::file_status status = file_entry.status(error);
		if (error || !std::filesystem::is_regular_file(status)) {
			continue;
		}
		if (lowercase_copy(file_entry.path().extension().string()) != ".stl") {
			continue;
		}

		const std::string name = file_entry.path().stem().string();
		if (name.empty() || !seen_names.insert(lowercase_copy(name)).second) {
			continue;
		}

		StlCatalogEntry entry = normalize_stl_catalog_entry(name, "");
		const std::filesystem::path relative_parent =
			std::filesystem::relative(file_entry.path().parent_path(), root, error);
		if (!error && !relative_parent.empty() && relative_parent != ".") {
			entry.category_path = relative_parent.lexically_normal().generic_string();
			entry.category = dot_path_from_stl_category_path(entry.category_path);
			entry.category_depth =
				static_cast<int>(split_string_non_empty(entry.category_path, '/').size());
			entry.qualified_name = entry.category.empty() ? entry.name : entry.category + "." + entry.name;
		} else {
			error.clear();
		}
		entries.push_back(std::move(entry));
	}

	std::sort(entries.begin(),
	          entries.end(),
	          [](const StlCatalogEntry &lhs, const StlCatalogEntry &rhs) {
		          if (lhs.category != rhs.category) {
			          return lhs.category < rhs.category;
		          }
		          return lhs.name < rhs.name;
	          });
	return entries;
}

std::vector<StlCatalogEntry> parse_stl_catalog_entries(const std::string &json_text)
{
	std::vector<StlCatalogEntry> entries;
	const std::size_t array_start = json_text.find('[');
	if (array_start == std::string::npos) {
		return entries;
	}

	const std::size_t first_item = json_text.find_first_not_of(" \t\r\n", array_start + 1);
	if (first_item == std::string::npos || json_text[first_item] == ']') {
		return entries;
	}

	if (json_text[first_item] == '"') {
		for (const std::string &name : parse_json_string_array(json_text)) {
			if (!name.empty()) {
				entries.push_back(normalize_stl_catalog_entry(name, ""));
			}
		}
		std::sort(entries.begin(),
		          entries.end(),
		          [](const StlCatalogEntry &lhs, const StlCatalogEntry &rhs) {
			          if (lhs.category != rhs.category) {
				          return lhs.category < rhs.category;
			          }
			          return lhs.name < rhs.name;
		          });
		return entries;
	}

	for (const std::string &object_text : extract_top_level_json_objects(json_text)) {
		const std::unordered_map<std::string, std::string> fields =
			parse_top_level_json_string_properties(object_text);
		const auto name_it = fields.find("name");
		if (name_it == fields.end() || name_it->second.empty()) {
			continue;
		}
		const auto category_it = fields.find("category");
		const auto description_it = fields.find("description");
		const auto short_description_it = fields.find("short_description");
		const auto tooltip_it = fields.find("tooltip");
		const auto category_path_it = fields.find("category_path");
		const auto category_depth_it = fields.find("category_depth");
		const auto source_file_it = fields.find("source_file");
		const auto source_module_it = fields.find("source_module");
		const auto implementation_file_it = fields.find("implementation_file");
		const auto implementation_module_it = fields.find("implementation_module");
		StlCatalogEntry entry = normalize_stl_catalog_entry(
			name_it->second,
			category_it != fields.end() ? category_it->second : "",
			description_it != fields.end() ? description_it->second : "",
			short_description_it != fields.end() ? short_description_it->second : "",
			tooltip_it != fields.end() ? tooltip_it->second : "",
			source_file_it != fields.end() ? source_file_it->second : "",
			source_module_it != fields.end() ? source_module_it->second : "",
			implementation_file_it != fields.end() ? implementation_file_it->second : "",
			implementation_module_it != fields.end() ? implementation_module_it->second : "");
		if (category_path_it != fields.end() && !category_path_it->second.empty()) {
			entry.category_path = category_path_it->second;
		}
		if (category_depth_it != fields.end()) {
			entry.category_depth = static_cast<int>(
				std::lround(parse_json_number(category_depth_it->second, static_cast<float>(entry.category_depth))));
		} else {
			entry.category_depth = static_cast<int>(split_string_non_empty(entry.category_path, '/').size());
		}
		const auto stl_geometry_it = fields.find("stl_geometry");
		if (stl_geometry_it != fields.end()) {
			entry.stl_geometry = parse_stl_geometry(stl_geometry_it->second);
		}
		const auto connection_points_it = fields.find("connection_points");
		if (connection_points_it != fields.end()) {
			entry.connection_points = parse_connection_points(connection_points_it->second);
		}
		const auto connection_surfaces_it = fields.find("connection_surfaces");
		if (connection_surfaces_it != fields.end()) {
			entry.connection_surfaces = parse_connection_surfaces(connection_surfaces_it->second);
		}
		const auto scad_detected_connections_it = fields.find("scad_detected_connections");
		if (scad_detected_connections_it != fields.end()) {
			entry.scad_detected_connections =
				parse_scad_detected_connections(scad_detected_connections_it->second);
		}
		entries.push_back(std::move(entry));
	}

	std::sort(entries.begin(),
	          entries.end(),
	          [](const StlCatalogEntry &lhs, const StlCatalogEntry &rhs) {
		          if (lhs.category != rhs.category) {
			          return lhs.category < rhs.category;
		          }
		          return lhs.name < rhs.name;
	          });
	return entries;
}

bool load_stl_category_tree(const std::string &path)
{
	std::string json_text;
	if (!read_text_file(path, &json_text)) {
		return false;
	}

	StlCategoryTreeNode loaded_root = parse_stl_category_tree_node(json_text);
	normalize_stl_category_tree_node(&loaded_root);
	if (loaded_root.name.empty()) {
		return false;
	}

	stl_category_tree_root = std::move(loaded_root);
	has_stl_category_tree = true;
	debugout("Loaded STL category tree: " + path + " (" +
	         std::to_string(stl_category_tree_root.children.size()) + " roots)");
	return true;
}

void set_part_class_catalog(const std::vector<std::string> &names)
{
	part_class_names.clear();
	part_class_name_set.clear();
	append_part_class_names(default_part_class_catalog());
	append_part_class_names(names);
}

bool load_part_class_catalog(const std::string &path)
{
	std::string json_text;
	if (!read_text_file(path, &json_text)) {
		set_part_class_catalog({});
		debugout("Using default part class catalog");
		return false;
	}

	std::vector<std::string> loaded_names = parse_json_string_array(json_text);
	set_part_class_catalog(loaded_names);
	debugout("Loaded part class catalog: " + path + " (" + std::to_string(part_class_names.size()) + " entries)");
	return true;
}

bool write_stl_catalog_file(const std::string &path, const std::vector<StlCatalogEntry> &entries)
{
	const std::filesystem::path output_path(path);
	const std::filesystem::path parent = output_path.parent_path();
	if (!parent.empty()) {
		std::error_code error;
		std::filesystem::create_directories(parent, error);
		if (error) {
			return false;
		}
	}

	std::ofstream fout(path);
	if (!fout.is_open()) {
		return false;
	}

	fout << "[\n";
	for (std::size_t index = 0; index < entries.size(); ++index) {
		const StlCatalogEntry &entry = entries[index];
		fout << "  {\n";
		fout << "    \"name\": \"" << json_escape_string(entry.name) << "\",\n";
		fout << "    \"category\": \"" << json_escape_string(entry.category) << "\"\n";
		fout << "  }";
		if (index + 1 < entries.size()) {
			fout << ",";
		}
		fout << "\n";
	}
	fout << "]\n";
	return true;
}

void generate_stl_part_catalog(const std::string &stls_dir,
                               const std::string &catalog_path,
                               const std::string &tree_path,
                               const StartupProgressCallback &progress_callback)
{
	const std::filesystem::path script_path =
		progen3d_resource_path(std::filesystem::path("scripts") / "build_stl_catalog.py");
	const std::filesystem::path libraries_dir = progen3d_resource_path("libraries");
	if (std::filesystem::exists(script_path)) {
		std::string command =
			"python3 -u \"" + script_path.string() + "\" --libraries-dir \"" +
			libraries_dir.string() + "\" --stls-dir \"" + stls_dir + "\" --output \"" + catalog_path + "\"";
		if (!tree_path.empty()) {
			command += " --tree-output \"" + tree_path + "\"";
		}
		command += " 2>&1";

		bool keep_reporting = true;
		if (progress_callback) {
			keep_reporting = progress_callback(0.0f, "Scanning SCAD libraries.");
		}

		FILE *pipe = popen(command.c_str(), "r");
		int result = -1;
		if (pipe != nullptr) {
			std::array<char, 1024> buffer{};
			while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
				std::string line(buffer.data());
				while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
					line.pop_back();
				}
				if (line.empty()) {
					continue;
				}

				float subprogress = 0.0f;
				std::string detail;
				if (parse_startup_progress_line(line, &subprogress, &detail)) {
					if (keep_reporting && progress_callback) {
						keep_reporting = progress_callback(subprogress, detail);
					}
					continue;
				}

				debugout(line);
			}
			result = pclose(pipe);
		} else {
			debugout("Unable to stream STL catalog builder progress, running without live updates");
			result = std::system(command.c_str());
		}
		if (result == 0) {
			debugout("Generated STL part catalog with metadata: " + catalog_path);
			return;
		}
		debugout("Metadata STL catalog builder failed, falling back to basic catalog generation");
	}

	if (progress_callback) {
		progress_callback(0.2f, "Metadata builder unavailable. Scanning STL files for a fallback catalog.");
	}
	const std::vector<StlCatalogEntry> entries = collect_stl_catalog_entries(stls_dir);
	if (progress_callback) {
		progress_callback(0.8f, "Writing fallback STL catalog.");
	}
	if (!write_stl_catalog_file(catalog_path, entries)) {
		debugout("Failed to write STL part catalog: " + catalog_path);
		return;
	}
	if (progress_callback) {
		progress_callback(1.0f, "Fallback STL catalog ready.");
	}
	debugout("Generated fallback STL part catalog: " + catalog_path + " (" + std::to_string(entries.size()) + " entries)");
}

void append_stl_part_catalog(const std::string &path, const std::string &tree_path)
{
	std::string json_text;
	if (!read_text_file(path, &json_text)) {
		return;
	}

	const std::vector<StlCatalogEntry> loaded_entries = parse_stl_catalog_entries(json_text);
	stl_catalog_entries.clear();
	stl_catalog_lookup.clear();
	stl_category_names.clear();
	stl_part_names.clear();
	stl_part_name_set.clear();
	stl_part_names_by_category.clear();
	has_stl_category_tree = false;
	stl_category_tree_root = {};
	for (const StlCatalogEntry &entry : loaded_entries) {
		if (entry.name.empty()) {
			continue;
		}
		const std::string lookup_key = lowercase_copy(entry.qualified_name);
		if (!stl_part_name_set.insert(entry.qualified_name).second) {
			continue;
		}
		stl_catalog_entries.push_back(entry);
		stl_part_names.push_back(entry.qualified_name);
		stl_catalog_lookup[lookup_key] = entry;
		stl_catalog_lookup[lowercase_copy(entry.name)] = entry;
		if (!entry.category_path.empty()) {
			const std::string tree_qualified_name =
				dot_path_from_stl_category_path(entry.category_path) + "." + entry.name;
			stl_catalog_lookup[lowercase_copy(tree_qualified_name)] = entry;
		}
		auto &category_bucket = stl_part_names_by_category[entry.category];
		if (category_bucket.empty()) {
			stl_category_names.push_back(entry.category);
		}
		category_bucket.push_back(entry.qualified_name);
	}

	std::vector<std::string> prefixed_names;
	prefixed_names.reserve(stl_catalog_entries.size() * 3);
	for (const StlCatalogEntry &entry : stl_catalog_entries) {
		prefixed_names.push_back("STL." + entry.qualified_name);
		prefixed_names.push_back("STL." + entry.name);
		if (!entry.category_path.empty()) {
			prefixed_names.push_back("STL." + dot_path_from_stl_category_path(entry.category_path) + "." + entry.name);
		}
	}
	append_part_class_names(prefixed_names);
	std::sort(stl_category_names.begin(), stl_category_names.end());
	stl_category_names.erase(std::unique(stl_category_names.begin(), stl_category_names.end()), stl_category_names.end());
	for (auto &pair : stl_part_names_by_category) {
		std::sort(pair.second.begin(), pair.second.end());
	}
	if (!tree_path.empty() && !load_stl_category_tree(tree_path)) {
		rebuild_stl_category_tree_from_catalog();
		debugout("Fell back to an in-memory STL category tree for autocomplete");
	} else if (tree_path.empty()) {
		rebuild_stl_category_tree_from_catalog();
	}
	debugout("Loaded STL part catalog: " + path + " (" + std::to_string(stl_catalog_entries.size()) +
	         " entries across " + std::to_string(stl_category_names.size()) + " categories)");
}
