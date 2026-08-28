#include "editor/service/GrammarCompilationService.h"

#include "Context.h"
#include "GrammarRuntime.h"
#include "grammar.h"

#include <chrono>
#include <exception>
#include <stdexcept>
#include <utility>

GrammarCompilationService::GrammarCompilationService(
	RandomSeedFunction random_seed_function,
	StructuralValidationFunction structural_validation_function)
	: random_seed_function_(std::move(random_seed_function)),
	  structural_validation_function_(std::move(structural_validation_function))
{
}

std::unique_ptr<SceneGenerationResult> GrammarCompilationService::compile(
	const SceneGenerationRequest &request) const
{
	const auto generation_started_at = std::chrono::steady_clock::now();
	auto result = std::make_unique<SceneGenerationResult>();
	result->request_id = request.request_id;
	result->design_nonce = request.design_nonce;
	result->evaluation_time = request.evaluation_time;
	result->time_sample = request.time_sample;

	try {
		if (random_seed_function_) {
			random_seed_function_(request.source_text, request.design_nonce);
		}

		auto grammar_document = std::make_unique<GrammarDocument>();
		if (!setGrammarEvaluationTime(grammar_document.get(), request.evaluation_time)) {
			throw std::runtime_error("grammar evaluation time is not finite or representable");
		}
		setGrammarEvaluationDiagnosticsQuiet(grammar_document.get(), request.time_sample);
		grammar_document->loadFromSourceText(request.source_text, "editor-buffer");

		const bool parser_succeeded = grammar_document->error_details.empty();
		const bool structure_succeeded =
			parser_succeeded &&
			(!structural_validation_function_ ||
			 structural_validation_function_(*grammar_document, request.time_sample));
		if (structure_succeeded) {
			grammar_document->addContext();
			grammar_document->context->genPrimitives();
			grammar_document->generateGeometry();
		}

		result->diagnostic_lines = grammar_document->error_lines;
		result->diagnostics = grammar_document->error_details;
		if (structure_succeeded && result->diagnostics.empty() &&
		    grammar_document->context != nullptr) {
			result->scene_snapshot = std::make_unique<GeneratedSceneSnapshot>(
				std::move(grammar_document), request.design_nonce, request.evaluation_time);
		} else if (result->diagnostics.empty()) {
			result->error = "Scene generation did not produce a complete scene context.";
		}
	}
	catch (const std::exception &exception) {
		result->error = std::string("Regeneration failed: ") + exception.what();
	}
	catch (...) {
		result->error = "Regeneration failed.";
	}

	result->generation_milliseconds =
		std::chrono::duration<double, std::milli>(
			std::chrono::steady_clock::now() - generation_started_at)
			.count();
	return result;
}
