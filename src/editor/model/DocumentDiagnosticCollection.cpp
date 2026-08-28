#include "editor/model/DocumentDiagnosticCollection.h"

#include <algorithm>

namespace {

bool describes_same_diagnostic(const GrammarDiagnostic &left,
	                            const GrammarDiagnostic &right)
{
	return left.line == right.line &&
	       left.start_column == right.start_column &&
	       left.end_column == right.end_column &&
	       left.message == right.message;
}

}

void DocumentDiagnosticCollection::clear()
{
	line_numbers_.clear();
	diagnostics_.clear();
}

void DocumentDiagnosticCollection::addLine(int line_number)
{
	if (line_number < 0 ||
	    std::find(line_numbers_.begin(), line_numbers_.end(), line_number) != line_numbers_.end()) {
		return;
	}
	line_numbers_.push_back(line_number);
}

void DocumentDiagnosticCollection::addDiagnostic(const GrammarDiagnostic &diagnostic)
{
	if (diagnostic.line < 0) {
		return;
	}
	addLine(diagnostic.line);
	for (GrammarDiagnostic &existing : diagnostics_) {
		if (describes_same_diagnostic(existing, diagnostic)) {
			return;
		}
		if (existing.line == diagnostic.line &&
		    existing.start_column == diagnostic.start_column &&
		    existing.end_column == diagnostic.end_column) {
			if (diagnostic.message.size() > existing.message.size()) {
				existing.message = diagnostic.message;
			}
			return;
		}
	}
	diagnostics_.push_back(diagnostic);
}

bool DocumentDiagnosticCollection::empty() const
{
	return diagnostics_.empty();
}

std::size_t DocumentDiagnosticCollection::size() const
{
	return diagnostics_.size();
}

const std::vector<int> &DocumentDiagnosticCollection::lineNumbers() const
{
	return line_numbers_;
}

const std::vector<GrammarDiagnostic> &DocumentDiagnosticCollection::diagnostics() const
{
	return diagnostics_;
}
