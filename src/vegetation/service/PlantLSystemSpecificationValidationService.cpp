#include "vegetation/service/PlantLSystemSpecificationValidationService.h"

#include <array>
#include <cmath>
#include <limits>
#include <unordered_set>

namespace {

std::size_t symbol_index(LSystemSymbol symbol)
{
	return static_cast<std::size_t>(symbol);
}

bool finite_positive(float value)
{
	return std::isfinite(value) && value > 0.0f;
}

}

bool PlantLSystemSpecificationValidationService::validate(
	const PlantLSystemSpecification &specification,
	std::string *diagnostic) const
{
	if (specification.axiom().empty()) {
		if (diagnostic != nullptr) {
			*diagnostic = "Plant L-system requires a non-empty axiom.";
		}
		return false;
	}
	bool contains_forward = false;
	for (LSystemSymbol symbol : specification.axiom()) {
		contains_forward = contains_forward || symbol == LSystemSymbol::Forward;
	}
	if (!contains_forward) {
		if (diagnostic != nullptr) {
			*diagnostic = "Plant L-system axiom must contain at least one Forward symbol.";
		}
		return false;
	}
	if (specification.productionRules().empty()) {
		if (diagnostic != nullptr) {
			*diagnostic = "Plant L-system requires at least one production rule.";
		}
		return false;
	}
	std::unordered_set<int> predecessors;
	for (const LSystemProductionRule &rule : specification.productionRules()) {
		if (rule.successor().empty()) {
			if (diagnostic != nullptr) {
				*diagnostic = "Plant L-system production rules require non-empty successors.";
			}
			return false;
		}
		if (!predecessors.insert(static_cast<int>(rule.predecessor())).second) {
			if (diagnostic != nullptr) {
				*diagnostic = "Plant L-system production-rule predecessors must be unique.";
			}
			return false;
		}
	}
	if (specification.iterationCount() >
	    complexity_limits_.maximumGrowthIterations()) {
		if (diagnostic != nullptr) {
			*diagnostic = "Plant L-system iteration count exceeds the vegetation safety limit.";
		}
		return false;
	}
	if (!std::isfinite(specification.turnAngleDegrees()) ||
	    !finite_positive(specification.stepLength()) ||
	    !finite_positive(specification.baseRadius()) ||
	    !finite_positive(specification.terminalRadius()) ||
	    specification.terminalRadius() > specification.baseRadius() ||
	    !finite_positive(specification.radiusConservationExponent())) {
		if (diagnostic != nullptr) {
			*diagnostic =
				"Plant L-system requires a finite angle, positive step/radii/gamma, and terminal radius no larger than base radius.";
		}
		return false;
	}

	std::array<const std::vector<LSystemSymbol> *, 5> successors{};
	for (const LSystemProductionRule &rule : specification.productionRules()) {
		successors[symbol_index(rule.predecessor())] = &rule.successor();
	}
	std::vector<LSystemSymbol> sentence = specification.axiom();
	if (sentence.size() > complexity_limits_.maximumLSystemSymbols()) {
		if (diagnostic != nullptr) {
			*diagnostic = "Plant L-system axiom exceeds the symbol safety limit.";
		}
		return false;
	}
	for (std::size_t iteration = 0u;
	     iteration < specification.iterationCount();
	     ++iteration) {
		std::size_t next_size = 0u;
		for (LSystemSymbol symbol : sentence) {
			const auto *successor = successors[symbol_index(symbol)];
			const std::size_t contribution = successor == nullptr
				? 1u : successor->size();
			if (contribution > complexity_limits_.maximumLSystemSymbols() - next_size) {
				if (diagnostic != nullptr) {
					*diagnostic = "Plant L-system expansion exceeds the symbol safety limit.";
				}
				return false;
			}
			next_size += contribution;
		}
		std::vector<LSystemSymbol> next;
		next.reserve(next_size);
		for (LSystemSymbol symbol : sentence) {
			const auto *successor = successors[symbol_index(symbol)];
			if (successor == nullptr) next.push_back(symbol);
			else next.insert(next.end(), successor->begin(), successor->end());
		}
		sentence = std::move(next);
	}

	std::size_t stack_depth = 0u;
	std::size_t forward_count = 0u;
	for (LSystemSymbol symbol : sentence) {
		if (symbol == LSystemSymbol::Forward) ++forward_count;
		else if (symbol == LSystemSymbol::PushState) ++stack_depth;
		else if (symbol == LSystemSymbol::PopState) {
			if (stack_depth == 0u) {
				if (diagnostic != nullptr) {
					*diagnostic = "Plant L-system sentence pops an empty turtle-state stack.";
				}
				return false;
			}
			--stack_depth;
		}
	}
	if (stack_depth != 0u) {
		if (diagnostic != nullptr) {
			*diagnostic = "Plant L-system sentence leaves turtle-state pushes unmatched.";
		}
		return false;
	}
	if (forward_count == 0u) {
		if (diagnostic != nullptr) {
			*diagnostic = "Plant L-system expansion must contain at least one Forward symbol.";
		}
		return false;
	}
	if (forward_count + 1u > complexity_limits_.maximumBranchNodes() ||
	    forward_count > complexity_limits_.maximumBranchSegments()) {
		if (diagnostic != nullptr) {
			*diagnostic = "Plant L-system expansion exceeds BranchGraph safety limits.";
		}
		return false;
	}
	if (diagnostic != nullptr) diagnostic->clear();
	return true;
}
