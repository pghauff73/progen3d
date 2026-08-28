#include "VegetationDelimitedSourceTableReader.h"

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

using Issue = VegetationMeasuredSourceDecodeIssue;
using IssueCode = VegetationMeasuredSourceDecodeIssueCode;

std::optional<std::vector<std::string>> parse_record(
	const std::string &line,
	char delimiter)
{
	std::vector<std::string> fields;
	std::string field;
	bool quoted = false;
	for (std::size_t index = 0u; index < line.size(); ++index) {
		const char character = line[index];
		if (quoted) {
			if (character == '"') {
				if (index + 1u < line.size() && line[index + 1u] == '"') {
					field.push_back('"');
					++index;
				} else {
					quoted = false;
				}
			} else {
				field.push_back(character);
			}
			continue;
		}
		if (character == '"' && field.empty()) {
			quoted = true;
		} else if (character == delimiter) {
			fields.push_back(field);
			field.clear();
		} else {
			field.push_back(character);
		}
	}
	if (quoted) return std::nullopt;
	fields.push_back(field);
	return fields;
}

std::vector<std::string> source_lines(const std::string &payload)
{
	std::vector<std::string> lines;
	std::string line;
	for (char character : payload) {
		if (character == '\n') {
			if (!line.empty() && line.back() == '\r') line.pop_back();
			lines.push_back(line);
			line.clear();
		} else {
			line.push_back(character);
		}
	}
	if (!line.empty()) {
		if (line.back() == '\r') line.pop_back();
		lines.push_back(line);
	}
	while (!lines.empty() && lines.back().empty()) lines.pop_back();
	return lines;
}

}

std::optional<VegetationDelimitedSourceTable>
VegetationDelimitedSourceTableReader::read(
	const std::string &payload,
	char delimiter,
	std::vector<VegetationMeasuredSourceDecodeIssue> &issues) const
{
	const std::vector<std::string> lines = source_lines(payload);
	if (lines.empty()) {
		issues.emplace_back(
			IssueCode::MalformedSourcePayload, std::string(),
			"Delimited source table is empty.");
		return std::nullopt;
	}
	const auto header = parse_record(lines.front(), delimiter);
	if (!header.has_value() || header->empty()) {
		issues.emplace_back(
			IssueCode::MalformedSourcePayload, "header",
			"Delimited source table header is malformed.");
		return std::nullopt;
	}
	std::vector<std::vector<std::string>> records;
	for (std::size_t line_index = 1u; line_index < lines.size(); ++line_index) {
		if (lines[line_index].empty()) continue;
		const auto record = parse_record(lines[line_index], delimiter);
		if (!record.has_value()) {
			issues.emplace_back(
				IssueCode::MalformedSourcePayload,
				"line-" + std::to_string(line_index + 1u),
				"Delimited source table record has an unterminated quoted field.");
			continue;
		}
		records.push_back(*record);
	}
	if (!issues.empty()) return std::nullopt;
	return VegetationDelimitedSourceTable(*header, std::move(records));
}

std::optional<double> VegetationDelimitedSourceTableReader::readDecimal(
	const std::string &value,
	const std::string &record_identifier,
	const std::string &field_name,
	std::vector<VegetationMeasuredSourceDecodeIssue> &issues) const
{
	errno = 0;
	char *end = nullptr;
	const double parsed = std::strtod(value.c_str(), &end);
	if (end == value.c_str() || *end != '\0' || errno == ERANGE ||
	    !std::isfinite(parsed)) {
		issues.emplace_back(
			IssueCode::NonFiniteValue, record_identifier,
			"Delimited source field " + field_name +
				" must contain one finite decimal value.");
		return std::nullopt;
	}
	return parsed;
}

std::optional<long long> VegetationDelimitedSourceTableReader::readInteger(
	const std::string &value,
	const std::string &record_identifier,
	const std::string &field_name,
	std::vector<VegetationMeasuredSourceDecodeIssue> &issues) const
{
	errno = 0;
	char *end = nullptr;
	const long long parsed = std::strtoll(value.c_str(), &end, 10);
	if (end == value.c_str() || *end != '\0' || errno == ERANGE) {
		issues.emplace_back(
			IssueCode::InvalidValueRange, record_identifier,
			"Delimited source field " + field_name +
				" must contain one integer value.");
		return std::nullopt;
	}
	return parsed;
}
