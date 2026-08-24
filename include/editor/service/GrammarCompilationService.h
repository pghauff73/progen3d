#pragma once

#include "editor/model/SceneGenerationRequest.h"
#include "editor/model/SceneGenerationResult.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

class GrammarDocument;

class GrammarCompilationService
{
public:
	using RandomSeedFunction =
		std::function<void(const std::string &source_text, std::uint64_t design_nonce)>;
	using StructuralValidationFunction =
		std::function<bool(GrammarDocument &grammar_document, bool quiet)>;

	GrammarCompilationService(RandomSeedFunction random_seed_function,
	                          StructuralValidationFunction structural_validation_function);

	std::unique_ptr<SceneGenerationResult> compile(
		const SceneGenerationRequest &request) const;

private:
	RandomSeedFunction random_seed_function_;
	StructuralValidationFunction structural_validation_function_;
};
