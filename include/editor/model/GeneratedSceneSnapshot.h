#pragma once

#include "grammar.h"

#include <cstdint>
#include <memory>
#include <utility>

class SpatialBuildingModel;
class SmallModernBuildingModel;

class GeneratedSceneSnapshot
{
public:
	GeneratedSceneSnapshot(std::unique_ptr<GrammarDocument> grammar_document,
	                       std::uint64_t design_nonce,
	                       double evaluation_time)
		: grammar_document_(std::move(grammar_document)),
		  design_nonce_(design_nonce),
		  evaluation_time_(evaluation_time)
	{
	}

	GrammarDocument *grammarDocument()
	{
		return grammar_document_.get();
	}

	const GrammarDocument *grammarDocument() const
	{
		return grammar_document_.get();
	}

	SceneGenerationContext *generationContext()
	{
		return grammar_document_ != nullptr ? grammar_document_->context : nullptr;
	}

	const SceneGenerationContext *generationContext() const
	{
		return grammar_document_ != nullptr ? grammar_document_->context : nullptr;
	}

	const SpatialBuildingModel *spatialBuildingModel() const;
	const SmallModernBuildingModel *smallModernBuildingModel() const;

	std::uint64_t designNonce() const
	{
		return design_nonce_;
	}

	double evaluationTime() const
	{
		return evaluation_time_;
	}

private:
	std::unique_ptr<GrammarDocument> grammar_document_;
	std::uint64_t design_nonce_ = 0;
	double evaluation_time_ = 0.0;
};
