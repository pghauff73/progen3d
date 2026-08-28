#pragma once

#include "grammar.h"

#include <cstddef>
#include <vector>

class DocumentDiagnosticCollection
{
public:
	void clear();
	void addLine(int line_number);
	void addDiagnostic(const GrammarDiagnostic &diagnostic);

	bool empty() const;
	std::size_t size() const;
	const std::vector<int> &lineNumbers() const;
	const std::vector<GrammarDiagnostic> &diagnostics() const;

private:
	std::vector<int> line_numbers_;
	std::vector<GrammarDiagnostic> diagnostics_;
};
