#pragma once

#include "vegetation/model/VegetationMeasuredSourceDecodeReport.h"

#include <optional>
#include <string>
#include <vector>

class VegetationDelimitedSourceTable
{
public:
	VegetationDelimitedSourceTable(
		std::vector<std::string> column_names,
		std::vector<std::vector<std::string>> records)
		: column_names_(std::move(column_names)), records_(std::move(records))
	{
	}

	const std::vector<std::string> &columnNames() const
	{
		return column_names_;
	}
	const std::vector<std::vector<std::string>> &records() const
	{
		return records_;
	}

private:
	std::vector<std::string> column_names_;
	std::vector<std::vector<std::string>> records_;
};

class VegetationDelimitedSourceTableReader
{
public:
	std::optional<VegetationDelimitedSourceTable> read(
		const std::string &payload,
		char delimiter,
		std::vector<VegetationMeasuredSourceDecodeIssue> &issues) const;

	std::optional<double> readDecimal(
		const std::string &value,
		const std::string &record_identifier,
		const std::string &field_name,
		std::vector<VegetationMeasuredSourceDecodeIssue> &issues) const;
	std::optional<long long> readInteger(
		const std::string &value,
		const std::string &record_identifier,
		const std::string &field_name,
		std::vector<VegetationMeasuredSourceDecodeIssue> &issues) const;
};
