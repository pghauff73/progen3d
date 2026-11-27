#pragma once

#include <cstddef>
#include <functional>
#include <string>

class GrammarDocument;
struct GrammarDiagnostic;

class GrammarStructureValidationService
{
public:
	using DiagnosticSink = std::function<void(const GrammarDiagnostic &)>;

	static bool validate(GrammarDocument *grammar, bool quiet,
	                     const DiagnosticSink &diagnostic_sink = {});
	static std::size_t findCommentStart(const std::string &line);
};
