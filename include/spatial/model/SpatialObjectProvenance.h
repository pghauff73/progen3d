#pragma once

#include <string>
#include <utility>

class SpatialObjectProvenance {
public:
	SpatialObjectProvenance() = default;
	SpatialObjectProvenance(std::string source_name,
	                        int start_line,
	                        int start_column,
	                        int end_line,
	                        int end_column)
		: source_name_(std::move(source_name)),
		  start_line_(start_line),
		  start_column_(start_column),
		  end_line_(end_line),
		  end_column_(end_column) {}

	const std::string &sourceName() const { return source_name_; }
	int startLine() const { return start_line_; }
	int startColumn() const { return start_column_; }
	int endLine() const { return end_line_; }
	int endColumn() const { return end_column_; }

private:
	std::string source_name_;
	int start_line_ = -1;
	int start_column_ = 0;
	int end_line_ = -1;
	int end_column_ = 0;
};
