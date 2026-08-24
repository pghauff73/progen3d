#pragma once

class SourceRange
{
public:
	int start_line = -1;
	int start_column = 0;
	int end_line = -1;
	int end_column = 0;

	bool isValid() const
	{
		return start_line >= 0 && end_line >= start_line;
	}
};
