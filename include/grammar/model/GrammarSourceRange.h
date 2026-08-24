#pragma once

class GrammarSourceRange
{
public:
	GrammarSourceRange() = default;
	GrammarSourceRange(int start_line,
	                   int start_column,
	                   int end_line,
	                   int end_column)
		: start_line_(start_line),
		  start_column_(start_column),
		  end_line_(end_line),
		  end_column_(end_column)
	{
	}

	int startLine() const { return start_line_; }
	int startColumn() const { return start_column_; }
	int endLine() const { return end_line_; }
	int endColumn() const { return end_column_; }
	bool isKnown() const { return start_line_ >= 0; }

private:
	int start_line_ = -1;
	int start_column_ = 0;
	int end_line_ = -1;
	int end_column_ = 0;
};
