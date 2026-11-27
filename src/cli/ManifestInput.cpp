#include "ManifestInput.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>

namespace manifest_input {

Json::Value read_json(const std::filesystem::path &path)
{
	std::ifstream input(path, std::ios::binary);
	if (!input) throw std::runtime_error("Cannot open " + path.u8string());
	input.exceptions(std::ios::badbit);
	Json::CharReaderBuilder parser;
	Json::CharReaderBuilder::strictMode(&parser.settings_);
	parser["skipBom"] = true;
	Json::Value result;
	std::string error;
	if (!Json::parseFromStream(parser, input, &result, &error)) {
		throw std::runtime_error("Invalid JSON in " + path.u8string() + ": " + error);
	}
	return result;
}

std::filesystem::path resolve_path(const std::filesystem::path &directory,
                                   const Json::Value &value, const char *field)
{
	if (!value.isString()) throw std::runtime_error(std::string(field) + " must be a path string.");
	const std::string text = value.asString();
	if (text.empty() || text.find('\0') != std::string::npos) {
		throw std::runtime_error(std::string(field) + " must be a nonempty path.");
	}
	const auto path = std::filesystem::u8path(text);
	if (!path.is_absolute() && (path.has_root_name() || path.has_root_directory())) {
		throw std::runtime_error(std::string(field) + " must be absolute or relative to its manifest.");
	}
	return (directory / path).lexically_normal();
}

bool is_unsigned_integer(const Json::Value &value)
{
	return (value.type() == Json::intValue || value.type() == Json::uintValue) && value.isUInt64();
}

void check_fields(const Json::Value &object, std::initializer_list<std::string_view> fields,
                  const std::string &location)
{
	if (!object.isObject()) throw std::runtime_error(location + " must be a JSON object.");
	for (const auto &name : object.getMemberNames()) {
		if (std::find(fields.begin(), fields.end(), name) == fields.end()) {
			throw std::runtime_error("Unknown field: " + location + "." + name);
		}
	}
}

std::string read_string(const Json::Value &value, const std::string &location)
{
	if (!value.isString() || value.asString().empty() || value.asString().find('\0') != std::string::npos) {
		throw std::runtime_error(location + " must be a nonempty string.");
	}
	return value.asString();
}

double read_number(const Json::Value &value, const std::string &location)
{
	if (!value.isNumeric() || !std::isfinite(value.asDouble())) {
		throw std::runtime_error(location + " must be a finite number.");
	}
	return value.asDouble();
}

}
