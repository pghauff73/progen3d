#pragma once

#include <string>

class GrammarDocument;

// Immutable editor/debug view of a value sampled by one GrammarDocument.
// The active lexical environment is never exposed or shared between documents.
struct GrammarRuntimeVariableSnapshot {
    std::string name;
    float value = 0.0f;
    float min = 0.0f;
    float max = 0.0f;
    bool integer = false;
    int instance_count = 0;
    std::string defining_rule;
};

// P2 evaluates a grammar as a deterministic snapshot F(source, seed, t).
// Time is measured in seconds. The value is a pure input, not an accumulated
// mutable variable inside the grammar environment.
struct GrammarEvaluationState {
    double time_seconds = 0.0;
    bool uses_time = false;
    bool quiet_diagnostics = false;
};

bool getGrammarRuntimeVariableSnapshot(const GrammarDocument *grammar,
                                       const std::string &name,
                                       GrammarRuntimeVariableSnapshot *out_snapshot);
void clearGrammarRuntimeVariableSnapshots(const GrammarDocument *grammar);

// Sets the built-in immutable `t` value used by the next rebuild/expansion.
// Returns false for a null grammar or a non-finite / float-unrepresentable time.
bool setGrammarEvaluationTime(GrammarDocument *grammar, double time_seconds);
double getGrammarEvaluationTime(const GrammarDocument *grammar);
bool setGrammarEvaluationDiagnosticsQuiet(GrammarDocument *grammar, bool quiet);
bool grammarEvaluationDiagnosticsQuiet(const GrammarDocument *grammar);
bool grammarUsesTime(const GrammarDocument *grammar);
bool getGrammarEvaluationState(const GrammarDocument *grammar,
                               GrammarEvaluationState *out_state);
void clearGrammarEvaluationState(const GrammarDocument *grammar);
