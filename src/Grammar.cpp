#include <fstream>
#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <list>
#include <limits>
#include <string>
#include <unordered_map>
#include <stack>
#include <vector>
#include <random>
#include <chrono>
#include <cmath>
#include <exception>
#include <memory>
#include <mutex>
#include <unordered_set>
#include <utility>

#include "PLYWriter.h"
#include "Context.h"
#include "Scope.h"
#include "grammar.h"
#include "GrammarRuntime.h"
#include "Solution.h"
#include "grammar/model/AxialProfileDescriptorSyntax.h"
#include "grammar/model/ExtrudeProfileDescriptorSyntax.h"
#include "grammar/service/ShapeSpecificationEvaluator.h"
#include "grammar/service/ShapeSpecificationParser.h"
#include "grammar/service/LightingDeclarationParser.h"
#include "grammar/service/SpatialConnectionDeclarationParser.h"
#include "grammar/service/SpatialConstraintDeclarationParser.h"
#include "grammar/service/SpatialInterfaceDeclarationParser.h"
#include "grammar/service/SpatialObjectSpecificationParser.h"
#include "grammar/service/VehicleJointDeclarationParser.h"
#include "spatial/model/CollisionPositionConstraint.h"
#include "spatial/model/CollisionParticipationPolicy.h"
#include "spatial/model/ConstraintDegreeOfFreedomState.h"
#include "spatial/model/InterfaceCompatibilityProfile.h"
#include "spatial/model/SpatialClearanceRequirement.h"
#include "spatial/model/SpatialConnection.h"
#include "spatial/model/SpatialConnectionState.h"
#include "spatial/model/SpatialConnectionType.h"
#include "spatial/model/SpatialDirection.h"
#include "spatial/model/SpatialInterface.h"
#include "spatial/model/SpatialInterfaceFrame.h"
#include "spatial/model/SpatialInterfaceRegion.h"
#include "spatial/model/SpatialInterfaceType.h"
#include "spatial/model/SpatialObjectClass.h"
#include "spatial/model/SpatialObjectIdentity.h"
#include "spatial/model/SpatialObjectProvenance.h"
#include "spatial/model/SpatialTaxonomyPath.h"
#include "spatial/relationship/SpatialInterfaceReference.h"
#include "electrical/model/LightEmitterDefinition.h"
#include "electrical/model/LightFixtureObject.h"
#include "electrical/model/LightSwitch.h"
#include "electrical/model/LightingCircuit.h"
#include "lighting/model/SceneLight.h"
#include "vehicle/model/VehicleJoint.h"

#include <glm/gtc/matrix_transform.hpp>





extern void errorout(std::string error_str);
extern void debugout(const std::string &message);

extern std::mt19937_64 grammar_rng;
std::vector<std::string> breakup(std::string input,std::string delimiter);
std::string removeSpaces(std::string word);
static std::vector<std::string> split_rule_tokens(const std::string &rule_str);
bool isnumber(const std::string& s);
static bool is_identifier_token_string(const std::string &text);
static bool is_identifier_start_char(char c);
static bool is_identifier_char(char c);
static bool is_supported_math_function_name(const std::string &token);

static bool value_fits_finite_float(double value)
{
	if(!std::isfinite(value))return false;
	const double float_limit = static_cast<double>(std::numeric_limits<float>::max());
	if(value > float_limit || value < -float_limit)return false;
	const float converted = static_cast<float>(value);
	if(!std::isfinite(converted))return false;
	// Do not silently collapse a non-zero expression to zero through float underflow.
	return value == 0.0 || converted != 0.0f;
}

static bool parse_finite_number(const std::string &text, double *value)
{
	if(value == NULL || text.empty())return false;
	errno = 0;
	char *end = NULL;
	const double parsed = std::strtod(text.c_str(), &end);
	if(end == text.c_str() || end == NULL || *end != '\0' || errno == ERANGE ||
	   !value_fits_finite_float(parsed)){
		return false;
	}
	*value = parsed;
	return true;
}


namespace {

std::mutex grammar_runtime_snapshot_mutex;
std::unordered_map<const GrammarDocument *,
                   std::unordered_map<std::string, GrammarRuntimeVariableSnapshot>>
    grammar_runtime_snapshots;

std::mutex grammar_evaluation_state_mutex;
std::unordered_map<const GrammarDocument *, GrammarEvaluationState>
    grammar_evaluation_states;

} // namespace

bool getGrammarRuntimeVariableSnapshot(const GrammarDocument *grammar,
                                       const std::string &name,
                                       GrammarRuntimeVariableSnapshot *out_snapshot)
{
	if(grammar == NULL || out_snapshot == NULL)return false;
	std::lock_guard<std::mutex> lock(grammar_runtime_snapshot_mutex);
	const auto grammar_it = grammar_runtime_snapshots.find(grammar);
	if(grammar_it == grammar_runtime_snapshots.end())return false;
	const auto value_it = grammar_it->second.find(name);
	if(value_it == grammar_it->second.end())return false;
	*out_snapshot = value_it->second;
	return true;
}

void clearGrammarRuntimeVariableSnapshots(const GrammarDocument *grammar)
{
	if(grammar == NULL)return;
	std::lock_guard<std::mutex> lock(grammar_runtime_snapshot_mutex);
	grammar_runtime_snapshots.erase(grammar);
}

bool setGrammarEvaluationTime(GrammarDocument *grammar, double time_seconds)
{
	if(grammar == NULL || !value_fits_finite_float(time_seconds))return false;
	std::lock_guard<std::mutex> lock(grammar_evaluation_state_mutex);
	grammar_evaluation_states[grammar].time_seconds = time_seconds;
	return true;
}

double getGrammarEvaluationTime(const GrammarDocument *grammar)
{
	if(grammar == NULL)return 0.0;
	std::lock_guard<std::mutex> lock(grammar_evaluation_state_mutex);
	const auto found = grammar_evaluation_states.find(grammar);
	return found == grammar_evaluation_states.end() ? 0.0 : found->second.time_seconds;
}

bool setGrammarEvaluationDiagnosticsQuiet(GrammarDocument *grammar, bool quiet)
{
	if(grammar == NULL)return false;
	std::lock_guard<std::mutex> lock(grammar_evaluation_state_mutex);
	grammar_evaluation_states[grammar].quiet_diagnostics = quiet;
	return true;
}

bool grammarEvaluationDiagnosticsQuiet(const GrammarDocument *grammar)
{
	if(grammar == NULL)return false;
	std::lock_guard<std::mutex> lock(grammar_evaluation_state_mutex);
	const auto found = grammar_evaluation_states.find(grammar);
	return found != grammar_evaluation_states.end() && found->second.quiet_diagnostics;
}

bool grammarUsesTime(const GrammarDocument *grammar)
{
	if(grammar == NULL)return false;
	std::lock_guard<std::mutex> lock(grammar_evaluation_state_mutex);
	const auto found = grammar_evaluation_states.find(grammar);
	return found != grammar_evaluation_states.end() && found->second.uses_time;
}

bool getGrammarEvaluationState(const GrammarDocument *grammar,
                               GrammarEvaluationState *out_state)
{
	if(grammar == NULL || out_state == NULL)return false;
	std::lock_guard<std::mutex> lock(grammar_evaluation_state_mutex);
	const auto found = grammar_evaluation_states.find(grammar);
	if(found == grammar_evaluation_states.end()){
		*out_state = GrammarEvaluationState{};
		return false;
	}
	*out_state = found->second;
	return true;
}

void clearGrammarEvaluationState(const GrammarDocument *grammar)
{
	if(grammar == NULL)return;
	std::lock_guard<std::mutex> lock(grammar_evaluation_state_mutex);
	grammar_evaluation_states.erase(grammar);
}

static void set_grammar_uses_time(const GrammarDocument *grammar, bool uses_time)
{
	if(grammar == NULL)return;
	std::lock_guard<std::mutex> lock(grammar_evaluation_state_mutex);
	grammar_evaluation_states[grammar].uses_time = uses_time;
}

static void store_runtime_variable_snapshot(const GrammarDocument *grammar,
                                            const GrammarRuntimeVariableSnapshot &snapshot)
{
	if(grammar == NULL || snapshot.name.empty())return;
	std::lock_guard<std::mutex> lock(grammar_runtime_snapshot_mutex);
	auto &per_grammar = grammar_runtime_snapshots[grammar];
	GrammarRuntimeVariableSnapshot stored = snapshot;
	const auto previous = per_grammar.find(snapshot.name);
	if(previous != per_grammar.end() &&
	   stored.instance_count <= previous->second.instance_count){
		stored.instance_count = previous->second.instance_count + 1;
	}
	per_grammar[snapshot.name] = std::move(stored);
}

struct RuntimeVariableValue {
	std::string name;
	float value = 0.0f;
	float min = 0.0f;
	float max = 0.0f;
	bool integer = false;
	bool resampleable = false;
	int instance_count = 0;
};

class GrammarLexicalEnvironment {
public:
	GrammarLexicalEnvironment()
	{
		frames_.emplace_back();
	}

	void pushFrame()
	{
		frames_.emplace_back();
	}

	void popFrame()
	{
		if(frames_.size() > 1){
			frames_.pop_back();
		}
		else if(!frames_.empty()){
			frames_.front().clear();
		}
	}

	RuntimeVariableValue *lookup(const std::string &name)
	{
		for(auto frame = frames_.rbegin(); frame != frames_.rend(); ++frame){
			auto value = frame->find(name);
			if(value != frame->end())return &value->second;
		}
		return NULL;
	}

	const RuntimeVariableValue *lookup(const std::string &name) const
	{
		for(auto frame = frames_.rbegin(); frame != frames_.rend(); ++frame){
			auto value = frame->find(name);
			if(value != frame->end())return &value->second;
		}
		return NULL;
	}

	RuntimeVariableValue *lookupOuter(const std::string &name)
	{
		if(frames_.size() <= 1)return lookup(name);
		for(std::size_t index = frames_.size() - 1; index > 0; --index){
			auto value = frames_[index - 1].find(name);
			if(value != frames_[index - 1].end())return &value->second;
		}
		return NULL;
	}

	RuntimeVariableValue &defineCurrent(RuntimeVariableValue value)
	{
		if(frames_.empty())frames_.emplace_back();
		auto &frame = frames_.back();
		auto existing = frame.find(value.name);
		if(existing != frame.end()){
			value.instance_count = std::max(value.instance_count,
			                                existing->second.instance_count + 1);
			existing->second = std::move(value);
			return existing->second;
		}
		auto inserted = frame.emplace(value.name, std::move(value));
		return inserted.first->second;
	}

	bool setVisibleValue(const std::string &name, float value)
	{
		RuntimeVariableValue *variable = lookup(name);
		if(variable == NULL)return false;
		variable->value = value;
		return true;
	}

	std::unordered_map<std::string, float> visibleValues() const
	{
		std::unordered_map<std::string, float> values;
		for (const auto &frame : frames_) {
			for (const auto &entry : frame) values[entry.first] = entry.second.value;
		}
		return values;
	}

private:
	std::vector<std::unordered_map<std::string, RuntimeVariableValue>> frames_;
};

struct RuleSemanticSymbols {
	std::unordered_set<std::string> parameters;
	std::unordered_set<std::string> variables;
	std::unordered_set<std::string> rerolls;
};

struct GrammarSemanticTables {
	std::unordered_map<std::string, Rule *> rules;
	std::unordered_map<const Rule *, RuleSemanticSymbols> per_rule;
	std::unordered_map<const Rule *, std::unordered_set<Rule *>> calls;
	std::unordered_map<const Rule *, std::unordered_set<std::string>> potential_visible_names;
	std::unordered_set<std::string> all_variable_names;
};

static const std::unordered_set<std::string> kGrammarBuiltinVariableNames = {"t"};

static bool is_reserved_builtin_variable_name(const std::string &name)
{
	return kGrammarBuiltinVariableNames.find(name) != kGrammarBuiltinVariableNames.end();
}

struct GrammarRuntimeState {
	Grammar *grammar = NULL;
	GrammarLexicalEnvironment environment;
	GrammarSemanticTables symbols;
};

static thread_local GrammarRuntimeState *active_grammar_runtime = NULL;

static GrammarRuntimeState *runtime_for(Grammar *grammar)
{
	if(active_grammar_runtime == NULL || active_grammar_runtime->grammar != grammar){
		return NULL;
	}
	return active_grammar_runtime;
}

static const RuntimeVariableValue *lookup_runtime_variable(Grammar *grammar,
                                                           const std::string &name)
{
	GrammarRuntimeState *runtime = runtime_for(grammar);
	return runtime == NULL ? NULL : runtime->environment.lookup(name);
}

// P0 generation safety limits. These allow large architectural grammars while
// placing hard ceilings on recursive and multiplicative expansion.
static constexpr std::size_t kMaxGrammarRecursionDepth = 256;
static constexpr std::size_t kMaxGrammarRuleInvocations = 250000;
static constexpr std::size_t kMaxGrammarExpansionWork = 1000000;
static constexpr std::size_t kMaxExpandedActionCount = 250000;
static constexpr std::size_t kMaxGeneratedPrimitiveCount = 50000;
static constexpr int kMaxRuleRepeatCount = 100000;

struct ExpansionBudgetState {
	Grammar *grammar = NULL;
	std::size_t recursion_depth = 0;
	std::size_t rule_invocations = 0;
	std::size_t work_units = 0;
	std::size_t emitted_actions = 0;
	std::size_t emitted_primitives = 0;
	bool aborted = false;
	std::vector<std::string> call_stack;
};

static thread_local ExpansionBudgetState *active_expansion_budget = NULL;

static std::string trim_copy(const std::string &input) {
	size_t start = input.find_first_not_of(" \t\r\n");
	if(start == std::string::npos)return "";
	size_t end = input.find_last_not_of(" \t\r\n");
	return input.substr(start, end - start + 1);
}

static std::size_t find_comment_start(const std::string &line)
{
	const std::size_t hash_comment = line.find('#');
	const std::size_t slash_comment = line.find("//");
	if(hash_comment == std::string::npos)return slash_comment;
	if(slash_comment == std::string::npos)return hash_comment;
	return std::min(hash_comment, slash_comment);
}

static std::string trim_code_copy(const std::string &line)
{
	const std::size_t comment_start = find_comment_start(line);
	if(comment_start == std::string::npos){
		return trim_copy(line);
	}
	return trim_copy(line.substr(0, comment_start));
}

static bool is_blank_or_comment_line(const std::string &line) {
	return trim_code_copy(line).empty();
}

static std::string normalize_grammar_spacing(const std::string &input) {
	std::string output;
	output.reserve(input.size() * 2);
	for(size_t i = 0; i < input.size(); ++i){
		char c = input[i];
		if(c == '\t' || c == '\r' || c == '\n'){
			output += ' ';
			continue;
		}
		if(c == '-' && i + 1 < input.size() && input[i + 1] == '>'){
			output += " -> ";
			++i;
			continue;
		}
		if(c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' ||
		   c == '|' || c == ';' || c == '?' || c == ':'){
			output += ' ';
			output += c;
			output += ' ';
			continue;
		}
		output += c;
	}

	std::string collapsed;
	collapsed.reserve(output.size());
	bool last_was_space = false;
	for(char c : output){
		if(std::isspace(static_cast<unsigned char>(c))){
			if(!last_was_space){
				collapsed += ' ';
				last_was_space = true;
			}
		}
		else{
			collapsed += c;
			last_was_space = false;
		}
	}
	return trim_copy(collapsed);
}

static const char *kDeferredBareMaterialPrefix = "@progen3d:bare-material:";

struct ParsedMaterialArgument {
	enum class Kind {
		Material,
		IndexExpression,
		DeferredBareIdentifier
	};
	Kind kind = Kind::Material;
	std::string value;
};

static bool unwrap_named_argument(const std::string &text,
                                  const std::string &name,
                                  std::string *inner)
{
	if(inner == NULL)return false;
	const std::string prefix = name + "(";
	if(text.size() <= prefix.size() || text.rfind(prefix, 0) != 0 || text.back() != ')'){
		return false;
	}
	*inner = trim_copy(text.substr(prefix.size(), text.size() - prefix.size() - 1));
	return !inner->empty();
}

static std::string unquote_material_name(const std::string &text)
{
	if(text.size() >= 2 &&
	   ((text.front() == '"' && text.back() == '"') ||
	    (text.front() == '\'' && text.back() == '\''))){
		return text.substr(1, text.size() - 2);
	}
	return text;
}

static ParsedMaterialArgument parse_material_argument(const std::string &raw)
{
	ParsedMaterialArgument parsed;
	const std::string text = trim_copy(raw);
	std::string inner;
	if(unwrap_named_argument(text, "material", &inner)){
		parsed.kind = ParsedMaterialArgument::Kind::Material;
		parsed.value = unquote_material_name(inner);
		return parsed;
	}
	if(unwrap_named_argument(text, "index", &inner)){
		parsed.kind = ParsedMaterialArgument::Kind::IndexExpression;
		parsed.value = inner;
		return parsed;
	}
	if(text.size() >= 2 &&
	   ((text.front() == '"' && text.back() == '"') ||
	    (text.front() == '\'' && text.back() == '\''))){
		parsed.kind = ParsedMaterialArgument::Kind::Material;
		parsed.value = unquote_material_name(text);
		return parsed;
	}
	if(isnumber(text)){
		parsed.kind = ParsedMaterialArgument::Kind::IndexExpression;
		parsed.value = text;
		return parsed;
	}
	if(is_identifier_token_string(text)){
		parsed.kind = ParsedMaterialArgument::Kind::DeferredBareIdentifier;
		parsed.value = text;
		return parsed;
	}
	parsed.kind = ParsedMaterialArgument::Kind::IndexExpression;
	parsed.value = text;
	return parsed;
}

static bool assign_instance_expression(Token *token,
	                                   std::size_t argument_index,
	                                   const std::string &expression,
	                                   float default_value = 0.0f)
{
	if(token == NULL || argument_index >= token->var_names.size())return false;
	while(token->arguments.size() <= argument_index){
		token->arguments.push_back(default_value);
	}
	double literal_value = 0.0;
	if(parse_finite_number(expression, &literal_value)){
		token->arguments[argument_index] = static_cast<float>(literal_value);
		token->var_names[argument_index].clear();
	}
	else{
		token->arguments[argument_index] = default_value;
		token->var_names[argument_index] = expression;
	}
	return true;
}

static bool parse_instance_appearance_arguments(
	Token *token,
	const std::vector<std::string> &raw_arguments,
	std::string *diagnostic)
{
	if(token == NULL || raw_arguments.empty()){
		if(diagnostic != NULL){
			*diagnostic = "Instance syntax requires a material argument.";
		}
		return false;
	}

	const ParsedMaterialArgument material_argument =
		parse_material_argument(removeSpaces(raw_arguments[0]));
	if(material_argument.value.empty()){
		if(diagnostic != NULL)*diagnostic = "Instance material argument is empty.";
		return false;
	}

	std::size_t appearance_offset = 0;
	if(material_argument.kind == ParsedMaterialArgument::Kind::Material){
		token->material_name = material_argument.value;
	}
	else if(material_argument.kind == ParsedMaterialArgument::Kind::DeferredBareIdentifier){
		token->material_name = std::string(kDeferredBareMaterialPrefix) +
		                       material_argument.value;
	}
	else{
		if(!assign_instance_expression(token, 0, material_argument.value))return false;
		appearance_offset = 1;
	}

	bool has_named_appearance = false;
	bool has_positional_appearance = false;
	bool alpha_assigned = false;
	bool texscale_assigned = false;
	std::vector<std::pair<std::string, std::string>> parsed_arguments;
	for(std::size_t index = 1; index < raw_arguments.size(); ++index){
		const std::string raw = removeSpaces(raw_arguments[index]);
		std::string inner;
		if(unwrap_named_argument(raw, "alpha", &inner)){
			has_named_appearance = true;
			if(alpha_assigned){
				if(diagnostic != NULL)*diagnostic = "Instance alpha is declared more than once.";
				return false;
			}
			alpha_assigned = true;
			parsed_arguments.emplace_back("alpha", inner);
		}
		else if(unwrap_named_argument(raw, "texscale", &inner)){
			has_named_appearance = true;
			if(texscale_assigned){
				if(diagnostic != NULL)*diagnostic = "Instance texture scale is declared more than once.";
				return false;
			}
			texscale_assigned = true;
			parsed_arguments.emplace_back("texscale", inner);
		}
		else{
			has_positional_appearance = true;
			parsed_arguments.emplace_back("positional", raw);
		}
	}

	if(has_named_appearance && has_positional_appearance){
		if(diagnostic != NULL){
			*diagnostic = "Named and positional alpha or texture-scale arguments cannot be mixed.";
		}
		return false;
	}
	if(has_positional_appearance && parsed_arguments.size() > 2){
		if(diagnostic != NULL)*diagnostic = "Instance accepts at most alpha and texture-scale appearance arguments.";
		return false;
	}

	if(has_named_appearance){
		for(const auto &parsed : parsed_arguments){
			const std::size_t slot = appearance_offset +
				(parsed.first == "alpha" ? 0u : 1u);
			const float default_value = parsed.first == "alpha" ? 0.0f : 0.125f;
			if(!assign_instance_expression(token, slot, parsed.second, default_value))return false;
		}
	}
	else{
		for(std::size_t index = 0; index < parsed_arguments.size(); ++index){
			const std::size_t slot = appearance_offset + index;
			const float default_value = index == 0 ? 0.0f : 0.125f;
			if(!assign_instance_expression(token,
			                               slot,
			                               parsed_arguments[index].second,
			                               default_value))return false;
		}
	}
	return true;
}

static bool is_deferred_bare_material(const std::string &material_name)
{
	return material_name.rfind(kDeferredBareMaterialPrefix, 0) == 0;
}

static std::string deferred_bare_material_name(const std::string &material_name)
{
	return is_deferred_bare_material(material_name)
		? material_name.substr(std::char_traits<char>::length(kDeferredBareMaterialPrefix))
		: material_name;
}

static std::string legacy_material_name_for_index(int index) {
	static const std::vector<std::string> legacy_materials = {
		"warmwhitematteplaster",
		"pastelbluemattepaint",
		"lightgreyroughconcrete",
		"softbeigemattesandstone",
		"darkslatesmoothstone",
		"bluewhiteglossysmoothglass",
		"amberwarmwood",
		"steelshinyroughmetal",
		"creamtiledceramic",
		"terracottaroughceramic",
		"sagegreenmattepaint",
		"lightoakwood",
		"offwhiteceramic",
		"silverbluepolishedmetal",
		"charcoalroughmetal",
		"softpinkmatteplaster",
		"mintglossysmoothtile",
		"smokytransparentglass",
		"bluewhiteshinysmoothmetal"
	};

	if(index >= 0 && index < static_cast<int>(legacy_materials.size())){
		return legacy_materials[static_cast<std::size_t>(index)];
	}
	return "material" + std::to_string(index);
}

static bool starts_new_rule(const std::string &trimmed_line) {
	if(trimmed_line.empty())return false;
	if(trimmed_line.rfind("->", 0) == 0)return false;
	return trimmed_line.find("->") != std::string::npos;
}

struct LogicalRule {
	std::string content;
	int start_line;
	int end_line;
	std::vector<SourceTokenSpan> tokens;
};

static int first_non_whitespace_column(const std::string &line)
{
	const std::size_t start = line.find_first_not_of(" \t\r\n");
	return start == std::string::npos ? 0 : static_cast<int>(start);
}

static int line_end_column(const std::string &line)
{
	std::size_t end = find_comment_start(line);
	if(end == std::string::npos){
		end = line.size();
	}
	while(end > 0 && std::isspace(static_cast<unsigned char>(line[end - 1]))){
		--end;
	}
	return end == 0 ? 1 : static_cast<int>(end);
}

static GrammarDiagnostic make_line_diagnostic(const Grammar *grammar,
                                              int line_number,
                                              const std::string &message)
{
	GrammarDiagnostic diagnostic;
	diagnostic.line = line_number;
	diagnostic.message = message;
	if(grammar != NULL && line_number >= 0 &&
	   line_number < static_cast<int>(grammar->lines.size())){
		const std::string &line = grammar->lines[static_cast<std::size_t>(line_number)];
		diagnostic.start_column = first_non_whitespace_column(line);
		diagnostic.end_column = std::max(diagnostic.start_column + 1, line_end_column(line));
	}
	return diagnostic;
}

static GrammarDiagnostic make_token_diagnostic(const SourceTokenSpan &token,
                                               const std::string &message)
{
	GrammarDiagnostic diagnostic;
	diagnostic.line = token.line;
	diagnostic.start_column = token.start_column;
	diagnostic.end_column = std::max(token.start_column + 1, token.end_column);
	diagnostic.message = message;
	return diagnostic;
}

static std::vector<SourceTokenSpan> tokenize_rule_source(const std::vector<std::string> &source_lines,
                                                         int start_line,
                                                         int end_line)
{
	std::vector<SourceTokenSpan> tokens;
	if(start_line < 0 || end_line < start_line)return tokens;

	for(int line_number = start_line;
	    line_number <= end_line &&
	    line_number < static_cast<int>(source_lines.size());
	    ++line_number){
		const std::string &line = source_lines[static_cast<std::size_t>(line_number)];
		if(is_blank_or_comment_line(line))continue;

		const std::size_t code_end_raw = find_comment_start(line);
		const std::size_t code_end =
			code_end_raw == std::string::npos ? line.size() : code_end_raw;
		const std::size_t start = line.find_first_not_of(" \t\r\n");
		if(start == std::string::npos || start >= code_end)continue;

		std::string current_token;
		int current_start_column = -1;
		const auto flush_token = [&]() {
			if(current_token.empty())return;
			tokens.push_back(
				{current_token,
				 line_number,
				 current_start_column,
				 current_start_column + static_cast<int>(current_token.size())});
			current_token.clear();
			current_start_column = -1;
		};

		for(std::size_t column = start; column < code_end; ++column){
			const char c = line[column];
			if(std::isspace(static_cast<unsigned char>(c))){
				flush_token();
				continue;
			}
			if(c == '-' && column + 1 < code_end && line[column + 1] == '>'){
				flush_token();
				tokens.push_back({"->",
				                 line_number,
				                 static_cast<int>(column),
				                 static_cast<int>(column) + 2});
				++column;
				continue;
			}
			if(c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' ||
			   c == '|' || c == ';' || c == '?' || c == ':'){
				flush_token();
				tokens.push_back({std::string(1, c),
				                 line_number,
				                 static_cast<int>(column),
				                 static_cast<int>(column) + 1});
				continue;
			}
			if(current_token.empty()){
				current_start_column = static_cast<int>(column);
			}
			current_token += c;
		}

		flush_token();
	}

	return tokens;
}

static std::string join_token_texts(const std::vector<SourceTokenSpan> &tokens,
                                    std::size_t start_index,
                                    std::size_t end_index)
{
	if(start_index >= end_index || start_index >= tokens.size())return "";
	end_index = std::min(end_index, tokens.size());

	std::string output;
	for(std::size_t index = start_index; index < end_index; ++index){
		if(!output.empty())output += ' ';
		output += tokens[index].text;
	}
	return output;
}

static std::vector<SourceTokenSpan> slice_token_spans(const std::vector<SourceTokenSpan> &tokens,
                                                      std::size_t start_index,
                                                      std::size_t end_index)
{
	if(start_index >= end_index || start_index >= tokens.size())return {};
	end_index = std::min(end_index, tokens.size());
	return std::vector<SourceTokenSpan>(tokens.begin() + start_index,
	                                    tokens.begin() + end_index);
}

static int find_rule_end_line(const std::vector<std::string> &source_lines, int start_line)
{
	if(start_line < 0 || start_line >= static_cast<int>(source_lines.size()))return start_line;
	for(int line_number = start_line + 1; line_number < static_cast<int>(source_lines.size()); ++line_number){
		const std::string &line = source_lines[static_cast<std::size_t>(line_number)];
		if(is_blank_or_comment_line(line))continue;
		if(starts_new_rule(trim_code_copy(line))){
			return line_number - 1;
		}
	}
	return static_cast<int>(source_lines.size()) - 1;
}

static std::size_t find_token_subsequence(const std::vector<SourceTokenSpan> &tokens,
                                          const std::vector<std::string> &needle_tokens)
{
	if(needle_tokens.empty() || needle_tokens.size() > tokens.size())return std::string::npos;
	for(std::size_t index = 0; index + needle_tokens.size() <= tokens.size(); ++index){
		bool matches = true;
		for(std::size_t token_index = 0; token_index < needle_tokens.size(); ++token_index){
			if(tokens[index + token_index].text != needle_tokens[token_index]){
				matches = false;
				break;
			}
		}
		if(matches)return index;
	}
	return std::string::npos;
}

static GrammarDiagnostic locate_rule_section_diagnostic(const Grammar *grammar,
                                                        int rule_start_line,
                                                        const std::string &section_text,
                                                        std::size_t section_token_index,
                                                        const std::string &message)
{
	if(grammar == NULL){
		GrammarDiagnostic diagnostic;
		diagnostic.line = rule_start_line;
		diagnostic.message = message;
		return diagnostic;
	}

	const std::vector<std::string> section_tokens = split_rule_tokens(section_text);
	if(section_tokens.empty()){
		return make_line_diagnostic(grammar, rule_start_line, message);
	}

	const int rule_end_line = find_rule_end_line(grammar->lines, rule_start_line);
	const std::vector<SourceTokenSpan> rule_tokens =
		tokenize_rule_source(grammar->lines, rule_start_line, rule_end_line);
	const std::size_t rule_token_start = find_token_subsequence(rule_tokens, section_tokens);
	if(rule_token_start == std::string::npos){
		return make_line_diagnostic(grammar, rule_start_line, message);
	}

	if(section_token_index >= section_tokens.size()){
		section_token_index = section_tokens.size() - 1;
	}
	if(rule_token_start + section_token_index >= rule_tokens.size()){
		return make_line_diagnostic(grammar, rule_start_line, message);
	}
	return make_token_diagnostic(rule_tokens[rule_token_start + section_token_index], message);
}

static std::vector<LogicalRule> build_logical_rules(const std::vector<std::string> &source_lines) {
	std::vector<LogicalRule> logical_rules;
	int current_rule_start_line = -1;

	const auto finalize_rule = [&](int end_line) {
		if(current_rule_start_line < 0 || end_line < current_rule_start_line)return;
		LogicalRule logical_rule;
		logical_rule.start_line = current_rule_start_line;
		logical_rule.end_line = end_line;
		logical_rule.tokens = tokenize_rule_source(source_lines, current_rule_start_line, end_line);
		logical_rule.content = join_token_texts(logical_rule.tokens, 0, logical_rule.tokens.size());
		if(!logical_rule.content.empty()){
			logical_rules.push_back(std::move(logical_rule));
		}
		current_rule_start_line = -1;
	};

	for(int line_num = 0; line_num < static_cast<int>(source_lines.size()); ++line_num){
		const std::string &raw_line = source_lines[static_cast<std::size_t>(line_num)];
		if(is_blank_or_comment_line(raw_line))continue;
		std::string trimmed = trim_code_copy(raw_line);
		if(current_rule_start_line < 0){
			current_rule_start_line = line_num;
			continue;
		}
		if(starts_new_rule(trimmed)){
			finalize_rule(line_num - 1);
			current_rule_start_line = line_num;
		}
	}

	finalize_rule(static_cast<int>(source_lines.size()) - 1);

	return logical_rules;
}

static void report_grammar_issue(Grammar *grammar,
                                 const GrammarDiagnostic &diagnostic,
                                 const std::string &context = "")
{
	if(grammar != NULL){
		if(diagnostic.line >= 0 &&
		   std::find(grammar->error_lines.begin(), grammar->error_lines.end(), diagnostic.line) ==
		       grammar->error_lines.end()){
			grammar->error_lines.push_back(diagnostic.line);
		}

		bool has_detail = false;
		for(const auto &detail : grammar->error_details){
			if(detail.line == diagnostic.line &&
			   detail.start_column == diagnostic.start_column &&
			   detail.end_column == diagnostic.end_column &&
			   detail.message == diagnostic.message){
				has_detail = true;
				break;
			}
		}
		if(!has_detail){
			grammar->error_details.push_back(diagnostic);
		}
	}

	std::stringstream ss;
	ss << "Grammar error";
	if(diagnostic.line >= 0){
		ss << " [line " << (diagnostic.line + 1);
		if(diagnostic.start_column >= 0){
			ss << ", column " << (diagnostic.start_column + 1);
		}
		ss << "]";
	}
	ss << ": " << diagnostic.message;
	if(!context.empty()){
		ss << " :: " << trim_copy(context);
	}
	errorout(ss.str());
}

static void report_grammar_issue(Grammar *grammar,
                                 int line_number,
                                 const std::string &message,
                                 const std::string &context = "")
{
	report_grammar_issue(grammar, make_line_diagnostic(grammar, line_number, message), context);
}

static GrammarDiagnostic make_action_token_diagnostic(const Token *token,
                                                       const std::string &message)
{
	GrammarDiagnostic diagnostic;
	diagnostic.message = message;
	if(token != NULL){
		diagnostic.line = token->source_start_line;
		diagnostic.start_column = token->source_start_column;
		diagnostic.end_column = std::max(token->source_start_column + 1,
		                                 token->source_end_column);
	}
	return diagnostic;
}

static std::string expansion_call_path(const ExpansionBudgetState &state,
                                       const std::string &next_rule = "")
{
	std::ostringstream path;
	const std::size_t max_visible_frames = 12;
	const std::size_t start =
		state.call_stack.size() > max_visible_frames
			? state.call_stack.size() - max_visible_frames
			: 0;
	if(start > 0){
		path << "... -> ";
	}
	for(std::size_t index = start; index < state.call_stack.size(); ++index){
		if(index > start)path << " -> ";
		path << state.call_stack[index];
	}
	if(!next_rule.empty()){
		if(!state.call_stack.empty())path << " -> ";
		path << next_rule;
	}
	return path.str();
}

static void abort_expansion(Grammar *grammar,
                            const Token *token,
                            const std::string &message,
                            const std::string &next_rule = "")
{
	ExpansionBudgetState *state = active_expansion_budget;
	if(state != NULL){
		if(state->aborted)return;
		state->aborted = true;
	}

	std::string detailed_message = message;
	if(state != NULL){
		const std::string path = expansion_call_path(*state, next_rule);
		if(!path.empty()){
			detailed_message += " Expansion path: " + path + ".";
		}
	}
	report_grammar_issue(grammar,
	                     make_action_token_diagnostic(token, detailed_message));
}

class ScopedExpansionBudgetSession {
public:
	ScopedExpansionBudgetSession(Grammar *grammar, ExpansionBudgetState *provided_state = NULL)
		: previous_(active_expansion_budget)
	{
		if(provided_state != NULL){
			provided_state->grammar = grammar;
			active_expansion_budget = provided_state;
		}
		else if(active_expansion_budget == NULL){
			owned_state_.grammar = grammar;
			active_expansion_budget = &owned_state_;
		}
	}

	~ScopedExpansionBudgetSession()
	{
		active_expansion_budget = previous_;
	}

private:
	ExpansionBudgetState *previous_ = NULL;
	ExpansionBudgetState owned_state_;
};

class ScopedRuleExpansion {
public:
	ScopedRuleExpansion(Grammar *grammar, Rule *rule, const Token *call_token)
		: state_(active_expansion_budget)
	{
		if(state_ == NULL || rule == NULL || state_->aborted)return;

		if(++state_->rule_invocations > kMaxGrammarRuleInvocations){
			abort_expansion(grammar,
			                call_token,
			                "Grammar expansion exceeded the rule invocation budget of " +
			                    std::to_string(kMaxGrammarRuleInvocations) + ".",
			                rule->rule_name);
			return;
		}
		if(state_->recursion_depth >= kMaxGrammarRecursionDepth){
			abort_expansion(grammar,
			                call_token,
			                "Grammar expansion exceeded the recursion depth limit of " +
			                    std::to_string(kMaxGrammarRecursionDepth) + ".",
			                rule->rule_name);
			return;
		}

		++state_->recursion_depth;
		state_->call_stack.push_back(rule->rule_name);
		entered_ = true;
	}

	~ScopedRuleExpansion()
	{
		if(!entered_ || state_ == NULL)return;
		if(!state_->call_stack.empty())state_->call_stack.pop_back();
		if(state_->recursion_depth > 0)--state_->recursion_depth;
	}

	bool entered() const { return entered_; }

private:
	ExpansionBudgetState *state_ = NULL;
	bool entered_ = false;
};

class ScopedGrammarRuntimeSession {
public:
	ScopedGrammarRuntimeSession(Grammar *grammar,
	                            GrammarRuntimeState *provided_state = NULL)
		: previous_(active_grammar_runtime)
	{
		GrammarRuntimeState *selected = provided_state;
		if(selected == NULL &&
		   (active_grammar_runtime == NULL || active_grammar_runtime->grammar != grammar)){
			owned_state_.grammar = grammar;
			selected = &owned_state_;
		}
		if(selected != NULL){
			selected->grammar = grammar;
			if(selected->symbols.rules.empty() && grammar != NULL){
				for(Rule *rule : grammar->rule_list){
					if(rule != NULL && selected->symbols.rules.find(rule->rule_name) == selected->symbols.rules.end()){
						selected->symbols.rules.emplace(rule->rule_name, rule);
					}
				}
			}
			active_grammar_runtime = selected;
		}
	}

	~ScopedGrammarRuntimeSession()
	{
		active_grammar_runtime = previous_;
	}

private:
	GrammarRuntimeState *previous_ = NULL;
	GrammarRuntimeState owned_state_;
};

class ScopedLexicalFrame {
public:
	explicit ScopedLexicalFrame(GrammarRuntimeState *runtime)
		: runtime_(runtime)
	{
		if(runtime_ != NULL){
			runtime_->environment.pushFrame();
			active_ = true;
		}
	}

	~ScopedLexicalFrame()
	{
		if(active_ && runtime_ != NULL){
			runtime_->environment.popFrame();
		}
	}

private:
	GrammarRuntimeState *runtime_ = NULL;
	bool active_ = false;
};

static bool consume_expansion_work(Grammar *grammar,
                                   const Token *token,
                                   std::size_t amount = 1)
{
	ExpansionBudgetState *state = active_expansion_budget;
	if(state == NULL)return true;
	if(state->aborted)return false;
	if(amount > kMaxGrammarExpansionWork -
	                std::min(state->work_units, kMaxGrammarExpansionWork)){
		abort_expansion(grammar,
		                token,
		                "Grammar expansion exceeded the work budget of " +
		                    std::to_string(kMaxGrammarExpansionWork) + " steps.");
		return false;
	}
	state->work_units += amount;
	return true;
}

static bool reserve_expanded_action(Grammar *grammar, const Token *token)
{
	ExpansionBudgetState *state = active_expansion_budget;
	if(state == NULL)return true;
	if(state->aborted)return false;
	if(++state->emitted_actions > kMaxExpandedActionCount){
		abort_expansion(grammar,
		                token,
		                "Grammar expansion exceeded the action budget of " +
		                    std::to_string(kMaxExpandedActionCount) + ".");
		return false;
	}
	if(token != NULL && token->token_name == "I" &&
	   ++state->emitted_primitives > kMaxGeneratedPrimitiveCount){
		abort_expansion(grammar,
		                token,
		                "Grammar expansion exceeded the primitive budget of " +
		                    std::to_string(kMaxGeneratedPrimitiveCount) + ".");
		return false;
	}
	return true;
}

static bool expansion_is_aborted()
{
	return active_expansion_budget != NULL && active_expansion_budget->aborted;
}

static bool validate_source_structure_before_expansion(Grammar *grammar)
{
	if(grammar == NULL)return false;
	bool valid = true;
	const auto add_issue = [&](const GrammarDiagnostic &diagnostic) {
		valid = false;
		report_grammar_issue(grammar, diagnostic);
	};

	if(grammar->lines.empty()){
		GrammarDiagnostic diagnostic;
		diagnostic.message = "Document is empty.";
		add_issue(diagnostic);
		return false;
	}

	// Reject parser-hostile escape characters before any grammar object or
	// random variable can be created.
	for(int line_index = 0; line_index < static_cast<int>(grammar->lines.size()); ++line_index){
		const std::string &line = grammar->lines[static_cast<std::size_t>(line_index)];
		const std::size_t comment_start = find_comment_start(line);
		const std::size_t code_end =
			comment_start == std::string::npos ? line.size() : comment_start;
		for(std::size_t column = 0; column < code_end; ++column){
			if(line[column] != '\\')continue;
			add_issue(
				{line_index,
				 static_cast<int>(column),
				 static_cast<int>(column) + 1,
				 "Unexpected '\\' character in grammar source."});
		}
	}

	struct DelimiterState {
		char delimiter = '\0';
		int line = -1;
		int column = 0;
	};

	const auto closing_for = [](char delimiter) {
		switch(delimiter){
		case '(':
			return ')';
		case '[':
			return ']';
		case '{':
			return '}';
		default:
			return '\0';
		}
	};

	// Delimiters are scoped to a logical rule. Without this boundary, an open
	// scope in one rule could be incorrectly "closed" by the next rule and pass
	// validation even though either production is independently unsafe.
	const std::vector<LogicalRule> logical_rules = build_logical_rules(grammar->lines);
	for(const LogicalRule &logical_rule : logical_rules){
		std::vector<DelimiterState> stack;
		for(const SourceTokenSpan &token : logical_rule.tokens){
			if(token.text.size() != 1)continue;
			const char delimiter = token.text[0];
			if(delimiter == '(' || delimiter == '[' || delimiter == '{'){
				stack.push_back({delimiter, token.line, token.start_column});
				continue;
			}
			if(delimiter != ')' && delimiter != ']' && delimiter != '}')continue;

			if(stack.empty()){
				add_issue(
					{token.line,
					 token.start_column,
					 token.end_column,
					 std::string("Unmatched closing delimiter '") + delimiter + "'."});
				continue;
			}

			const DelimiterState open = stack.back();
			if(closing_for(open.delimiter) == delimiter){
				stack.pop_back();
				continue;
			}

			add_issue(
				{token.line,
				 token.start_column,
				 token.end_column,
				 std::string("Mismatched closing delimiter '") + delimiter +
				     "'. Expected '" + closing_for(open.delimiter) + "'."});
		}

		for(const DelimiterState &open : stack){
			add_issue(
				{open.line,
				 open.column,
				 open.column + 1,
				 std::string("Unclosed delimiter '") + open.delimiter + "'."});
		}
	}

	return valid;
}

static const Token *first_rule_token(const Rule *rule)
{
	if(rule == NULL)return NULL;
	for(int section = 0; section < 3; ++section){
		if(!rule->section_tokens[section].empty())return rule->section_tokens[section].front();
	}
	if(rule->alternate != NULL){
		for(int section = 0; section < 3; ++section){
			if(!rule->alternate->section_tokens[section].empty()){
				return rule->alternate->section_tokens[section].front();
			}
		}
	}
	return NULL;
}

static void collect_random_declarations(const std::vector<Token *> &tokens,
                                        RuleSemanticSymbols *rule_symbols,
                                        GrammarSemanticTables *tables)
{
	if(rule_symbols == NULL || tables == NULL)return;
	for(Token *token : tokens){
		if(token == NULL)continue;
		if(auto *object_token = dynamic_cast<SpatialObjectScopeToken *>(token)){
			collect_random_declarations(object_token->body_tokens, rule_symbols, tables);
			continue;
		}
		if(token->isConditionalToken()){
			collect_random_declarations(token->getTrueBranchTokens(), rule_symbols, tables);
			collect_random_declarations(token->getFalseBranchTokens(), rule_symbols, tables);
			continue;
		}
		if(token->token_name != "R")continue;
		rule_symbols->variables.insert(token->var_name);
		tables->all_variable_names.insert(token->var_name);
	}
}


static void collect_rule_call_edges(const std::vector<Token *> &tokens,
                                    Rule *caller,
                                    GrammarSemanticTables *tables)
{
	if(caller == NULL || tables == NULL)return;
	for(Token *token : tokens){
		if(token == NULL)continue;
		if(auto *object_token = dynamic_cast<SpatialObjectScopeToken *>(token)){
			collect_rule_call_edges(object_token->body_tokens, caller, tables);
			continue;
		}
		if(token->isConditionalToken()){
			collect_rule_call_edges(token->getTrueBranchTokens(), caller, tables);
			collect_rule_call_edges(token->getFalseBranchTokens(), caller, tables);
			continue;
		}
		if(token->isRule().empty())continue;
		const auto called = tables->rules.find(token->token_name);
		if(called != tables->rules.end()){
			tables->calls[caller].insert(called->second);
		}
	}
}

static void compute_potential_lexical_visibility(GrammarSemanticTables *tables)
{
	if(tables == NULL)return;
	for(const auto &entry : tables->per_rule){
		const Rule *rule = entry.first;
		const RuleSemanticSymbols &symbols = entry.second;
		std::unordered_set<std::string> &visible = tables->potential_visible_names[rule];
		visible.insert(kGrammarBuiltinVariableNames.begin(), kGrammarBuiltinVariableNames.end());
		visible.insert(symbols.parameters.begin(), symbols.parameters.end());
		visible.insert(symbols.variables.begin(), symbols.variables.end());
		visible.insert(symbols.rerolls.begin(), symbols.rerolls.end());
		if(rule != NULL)visible.insert(rule->rule_name + "_count");
	}

	bool changed = true;
	while(changed){
		changed = false;
		for(const auto &call_entry : tables->calls){
			const Rule *caller = call_entry.first;
			const auto caller_visible = tables->potential_visible_names.find(caller);
			if(caller_visible == tables->potential_visible_names.end())continue;
			for(Rule *callee : call_entry.second){
				std::unordered_set<std::string> &callee_visible =
					tables->potential_visible_names[callee];
				const std::size_t before = callee_visible.size();
				callee_visible.insert(caller_visible->second.begin(), caller_visible->second.end());
				if(callee_visible.size() != before)changed = true;
			}
		}
	}
}

struct ExpressionIdentifierUse {
	std::string name;
	bool function_call = false;
};

static std::vector<ExpressionIdentifierUse> expression_identifier_uses(const std::string &expression)
{
	std::vector<ExpressionIdentifierUse> uses;
	std::size_t index = 0;
	while(index < expression.size()){
		const unsigned char c = static_cast<unsigned char>(expression[index]);
		if(std::isspace(c) != 0 || expression[index] == '&'){
			++index;
			continue;
		}
		if(std::isdigit(c) != 0 || expression[index] == '.'){
			const char *begin = expression.c_str() + index;
			char *end_number = NULL;
			errno = 0;
			(void)std::strtod(begin, &end_number);
			if(end_number != begin){
				index = static_cast<std::size_t>(end_number - expression.c_str());
				continue;
			}
		}
		if(!is_identifier_start_char(expression[index])){
			++index;
			continue;
		}
		const std::size_t start = index++;
		while(index < expression.size() && is_identifier_char(expression[index]))++index;
		std::size_t lookahead = index;
		while(lookahead < expression.size() &&
		      std::isspace(static_cast<unsigned char>(expression[lookahead])) != 0){
			++lookahead;
		}
		uses.push_back({expression.substr(start, index - start),
		                lookahead < expression.size() && expression[lookahead] == '('});
	}
	return uses;
}

static void validate_expression_symbols(Grammar *grammar,
                                        const Token *token,
                                        const std::string &expression,
                                        const std::string &purpose,
                                        const std::unordered_set<std::string> &visible_names)
{
	if(grammar == NULL || trim_copy(expression).empty())return;
	std::unordered_set<std::string> reported;
	for(const ExpressionIdentifierUse &use : expression_identifier_uses(expression)){
		if(!use.function_call && use.name == "t"){
			set_grammar_uses_time(grammar, true);
		}
		if(use.function_call){
			if(!is_supported_math_function_name(use.name) && reported.insert("fn:" + use.name).second){
				report_grammar_issue(
					grammar,
					make_action_token_diagnostic(
						token,
						"Invalid " + purpose + " expression '" + expression +
						    "': unsupported function '" + use.name + "'."));
			}
			continue;
		}
		if(visible_names.find(use.name) == visible_names.end() &&
		   reported.insert("var:" + use.name).second){
			report_grammar_issue(
				grammar,
				make_action_token_diagnostic(
					token,
					"Invalid " + purpose + " expression '" + expression +
					    "': undefined variable '" + use.name + "'."));
		}
	}
}

static bool is_symbolic_shape_argument(const ShapeOptionSyntax &option,
	                                   std::size_t argument_index)
{
	const std::string &option_name = option.optionName();
	if(option_name == "topology" || option_name == "close" ||
	   option_name == "mapping" || option_name == "axis" ||
	   option_name == "side" || option_name == "cap" ||
	   option_name == "frame" || option_name == "end" ||
	   option_name == "profile" || option_name == "species" ||
	   option_name == "architecture" || option_name == "state" ||
	   option_name == "generation" || option_name == "lAxiom" ||
	   option_name == "lRule" ||
	   option_name == "meshKey" ||
	   option_name == "detail" || option_name == "mode" ||
	   option_name == "collision" || option_name == "attachment" ||
	   option_name == "region" || option_name == "face" ||
	   option_name == "orientation" || option_name == "layer" ||
	   option_name == "mask"){
		return true;
	}
	if((option_name == "target" || option_name == "surface") &&
	   argument_index == 0)return true;
	if(option_name == "section" && argument_index == 1)return true;
	if(option_name == "station" && argument_index == 1)return true;
	if(option_name == "nominalSection" && argument_index == 0)return true;
	if((option_name == "host" || option_name == "opening") &&
	   argument_index == 0)return true;
	if(option_name == "plane" && argument_index == 0)return true;
	if(option_name == "obstacle" &&
	   (argument_index == 0 || argument_index == 7))return true;
	if(option_name == "leaf" && argument_index == 0)return true;
	if(option_name == "flower" && argument_index == 0)return true;
	if(option_name == "inflorescence" && argument_index == 0)return true;
	if(option_name == "leafArray" && argument_index == 4)return true;
	if(option_name == "organArray" &&
	   (argument_index == 0 || argument_index == 1 || argument_index == 7))return true;
	if(option_name == "phyllotaxis" && argument_index == 0)return true;
	if(option_name == "growth" &&
	   (argument_index == 3 || argument_index == 5 || argument_index == 7))return true;
	if(option_name == "tropism" && argument_index == 0)return true;
	if(option_name == "crown" && argument_index == 0)return true;
	if(option_name == "crownObstacle" && argument_index == 0)return true;
	if(option_name == "clip" && argument_index == 4)return true;
	if(option_name == "chord" && argument_index == 2)return true;
	return false;
}

static void validate_shape_descriptor_expressions(
	Grammar *grammar,
	const Token *token,
	const std::unordered_set<std::string> &visible_names)
{
	if(grammar == NULL || token == NULL || !token->shape_descriptor_syntax)return;
	const auto *axial_profile = dynamic_cast<const AxialProfileDescriptorSyntax *>(
		token->shape_descriptor_syntax.get());
	if(axial_profile != NULL){
		for(const AxialProfilePolygonSyntax &profile : axial_profile->profiles()){
			for(const GeometryExpression &coordinate : profile.coordinates()){
				validate_expression_symbols(
					grammar,
					token,
					coordinate.sourceText(),
					"AxialProfile polygon coordinate",
					visible_names);
			}
		}
		for(const AxialProfileLevelSyntax &level : axial_profile->levels()){
			if(level.transition() != AxialTransitionKind::Step){
				validate_expression_symbols(
					grammar, token, level.axialPosition().sourceText(),
					"AxialProfile axial position", visible_names);
			}
			if(level.transition() == AxialTransitionKind::Hold)continue;
			const AxialProfileTransformSyntax &transform = level.transform();
			const GeometryExpression *expressions[] = {
				&transform.centerX(), &transform.centerZ(),
				&transform.scaleX(), &transform.scaleZ(),
				&transform.rotation()};
			for(const GeometryExpression *expression : expressions){
				validate_expression_symbols(
					grammar, token, expression->sourceText(),
					"AxialProfile section transform", visible_names);
			}
		}
		return;
	}
	const auto *extrude_profile = dynamic_cast<const ExtrudeProfileDescriptorSyntax *>(
		token->shape_descriptor_syntax.get());
	if(extrude_profile != NULL){
		for(const GeometryExpression &argument : extrude_profile->profile().arguments()){
			validate_expression_symbols(
				grammar, token, argument.sourceText(),
				"Extrude profile argument", visible_names);
		}
		validate_expression_symbols(
			grammar, token, extrude_profile->depth().sourceText(),
			"Extrude depth", visible_names);
		return;
	}
	for(const ShapeOptionSyntax &option : token->shape_descriptor_syntax->options()){
		for(std::size_t argument_index = 0;
		    argument_index < option.arguments().size();
		    ++argument_index){
			if(is_symbolic_shape_argument(option, argument_index))continue;
			validate_expression_symbols(
				grammar,
				token,
				option.arguments()[argument_index].sourceText(),
				"shape option '" + option.optionName() + "'",
				visible_names);
		}
	}
}

static void resolve_deferred_material(Grammar *grammar,
                                      Token *token,
                                      const std::unordered_set<std::string> &known_value_names)
{
	if(grammar == NULL || token == NULL || token->token_name != "I" ||
	   !is_deferred_bare_material(token->material_name)){
		return;
	}
	const std::string name = deferred_bare_material_name(token->material_name);
	if(known_value_names.find(name) == known_value_names.end()){
		// Legacy bare material names remain supported, but their meaning no longer
		// depends on which values happened to have been sampled while parsing.
		token->material_name = name;
		return;
	}

	// A bare identifier that resolves to a declared grammar value retains the
	// legacy material-index behaviour. Explicit material(name) and index(expr)
	// are always preferred because they are unambiguous.
	if(token->arguments.size() >= token->var_names.size()){
		report_grammar_issue(
			grammar,
			make_action_token_diagnostic(
				token,
				"Instance material index leaves no room for alpha or texture-scale arguments."));
		return;
	}
	for(std::size_t index = token->var_names.size() - 1; index > 0; --index){
		token->var_names[index] = token->var_names[index - 1];
	}
	token->var_names[0] = name;
	token->arguments.insert(token->arguments.begin(), 0.0f);
	token->material_name.clear();
}

static void validate_random_literal_range(Grammar *grammar, const Token *token)
{
	if(grammar == NULL || token == NULL || token->token_name != "R")return;
	double minimum = 0.0;
	double maximum = 0.0;
	if(!parse_finite_number(trim_copy(token->var_names[0]), &minimum) ||
	   !parse_finite_number(trim_copy(token->var_names[1]), &maximum)){
		return;
	}
	if(minimum > maximum){
		report_grammar_issue(
			grammar,
			make_action_token_diagnostic(
				token,
				"Random-variable minimum " + std::to_string(minimum) +
				    " exceeds maximum " + std::to_string(maximum) + "."));
		return;
	}
	if(token->integer){
		const double integer_min = static_cast<double>(std::numeric_limits<int>::lowest());
		const double integer_max = static_cast<double>(std::numeric_limits<int>::max());
		if(minimum < integer_min || maximum > integer_max){
			report_grammar_issue(
				grammar,
				make_action_token_diagnostic(
					token,
					"Integer random-variable range exceeds the supported int range."));
		}
		else if(std::ceil(minimum) > std::floor(maximum)){
			report_grammar_issue(
				grammar,
				make_action_token_diagnostic(
					token,
					"Integer random-variable range contains no integer value."));
		}
	}
}

static void validate_semantic_tokens(Grammar *grammar,
                                     Rule *owning_rule,
                                     const std::vector<Token *> &tokens,
                                     const GrammarSemanticTables &tables,
                                     const RuleSemanticSymbols &rule_symbols,
                                     const std::unordered_set<std::string> &visible_names)
{
	if(grammar == NULL || owning_rule == NULL)return;
	for(Token *token : tokens){
		if(token == NULL)continue;
		if(auto *object_token = dynamic_cast<SpatialObjectScopeToken *>(token)){
			validate_semantic_tokens(grammar,
			                         owning_rule,
			                         object_token->body_tokens,
			                         tables,
			                         rule_symbols,
			                         visible_names);
			continue;
		}
		if(token->isConditionalToken()){
			validate_expression_symbols(grammar,
			                            token,
			                            token->getConditionExpression(),
			                            "conditional",
			                            visible_names);
			validate_semantic_tokens(grammar,
			                         owning_rule,
			                         token->getTrueBranchTokens(),
			                         tables,
			                         rule_symbols,
			                         visible_names);
			validate_semantic_tokens(grammar,
			                         owning_rule,
			                         token->getFalseBranchTokens(),
			                         tables,
			                         rule_symbols,
			                         visible_names);
			continue;
		}

		if(token->token_name == "R"){
			const std::string counter_name = owning_rule->rule_name + "_count";
			if(is_reserved_builtin_variable_name(token->var_name)){
				report_grammar_issue(
					grammar,
					make_action_token_diagnostic(
						token,
						"Random variable '" + token->var_name +
						    "' uses a reserved built-in variable name."));
			}
			if(rule_symbols.parameters.find(token->var_name) != rule_symbols.parameters.end()){
				report_grammar_issue(
					grammar,
					make_action_token_diagnostic(
						token,
						"Random variable '" + token->var_name +
						    "' collides with a parameter of rule '" + owning_rule->rule_name + "'."));
			}
			if(rule_symbols.rerolls.find(token->var_name) != rule_symbols.rerolls.end()){
				report_grammar_issue(
					grammar,
					make_action_token_diagnostic(
						token,
						"Random variable '" + token->var_name +
						    "' collides with a reroll binding of rule '" + owning_rule->rule_name + "'."));
			}
			if(token->var_name == counter_name){
				report_grammar_issue(
					grammar,
					make_action_token_diagnostic(
						token,
						"Random variable '" + token->var_name +
						    "' uses the reserved iteration-counter name for rule '" +
						    owning_rule->rule_name + "'."));
			}
			validate_random_literal_range(grammar, token);
			validate_expression_symbols(grammar, token, token->var_names[0], "random minimum", visible_names);
			validate_expression_symbols(grammar, token, token->var_names[1], "random maximum", visible_names);
			continue;
		}

		if(token->isRule() != ""){
			const auto called = tables.rules.find(token->token_name);
			if(called == tables.rules.end()){
				report_grammar_issue(
					grammar,
					make_action_token_diagnostic(
						token,
						"Undefined rule reference '" + token->token_name + "'."));
			}
			else if(token->rule_arguments.size() != called->second->parameter_names.size()){
				report_grammar_issue(
					grammar,
					make_action_token_diagnostic(
						token,
						"Rule '" + token->token_name + "' expects " +
						    std::to_string(called->second->parameter_names.size()) +
						    " argument(s), but received " +
						    std::to_string(token->rule_arguments.size()) + "."));
			}
			for(const std::string &argument : token->rule_arguments){
				validate_expression_symbols(grammar, token, argument, "rule argument", visible_names);
			}
			continue;
		}

		resolve_deferred_material(grammar, token, visible_names);
		validate_shape_descriptor_expressions(grammar, token, visible_names);
		for(const std::string &expression : token->var_names){
			validate_expression_symbols(grammar, token, expression, "action", visible_names);
		}
		if(token->token_name == "G"){
			validate_expression_symbols(grammar, token, token->var_name, "gravity", visible_names);
		}
		else if(token->token_name == "A" && token->arguments.size() == 4){
			validate_expression_symbols(grammar,
			                            token,
			                            token->var_name,
			                            "rotation upper bound",
			                            visible_names);
		}
	}
}

static bool build_and_validate_semantic_tables(Grammar *grammar,
                                               GrammarSemanticTables *out_tables)
{
	if(grammar == NULL || out_tables == NULL)return false;
	GrammarSemanticTables tables;
	tables.all_variable_names.insert(kGrammarBuiltinVariableNames.begin(),
	                                kGrammarBuiltinVariableNames.end());

	for(Rule *rule : grammar->rule_list){
		if(rule == NULL)continue;
		if(is_reserved_builtin_variable_name(rule->rule_name)){
			report_grammar_issue(
				grammar,
				make_action_token_diagnostic(
					first_rule_token(rule),
					"Rule name '" + rule->rule_name +
					    "' is reserved for a built-in grammar value."));
		}
		const auto inserted = tables.rules.emplace(rule->rule_name, rule);
		if(!inserted.second){
			report_grammar_issue(
				grammar,
				make_action_token_diagnostic(
					first_rule_token(rule),
					"Duplicate rule definition '" + rule->rule_name + "'."));
			continue;
		}

		RuleSemanticSymbols &symbols = tables.per_rule[rule];
		for(const std::string &parameter : rule->parameter_names){
			if(is_reserved_builtin_variable_name(parameter)){
				report_grammar_issue(
					grammar,
					make_action_token_diagnostic(
						first_rule_token(rule),
						"Parameter '" + parameter + "' of rule '" + rule->rule_name +
						    "' uses a reserved built-in variable name."));
			}
			symbols.parameters.insert(parameter);
			tables.all_variable_names.insert(parameter);
		}
		for(int index = 0; index < rule->var_counter; ++index){
			const std::string &reroll_name = rule->var_names[index];
			if(is_reserved_builtin_variable_name(reroll_name)){
				report_grammar_issue(
					grammar,
					make_action_token_diagnostic(
						first_rule_token(rule),
						"Reroll binding '" + reroll_name + "' of rule '" +
						    rule->rule_name + "' uses a reserved built-in variable name."));
			}
			symbols.rerolls.insert(reroll_name);
			tables.all_variable_names.insert(reroll_name);
		}
		for(int section = 0; section < 3; ++section){
			collect_random_declarations(rule->section_tokens[section], &symbols, &tables);
		}
		if(rule->alternate != NULL){
			for(int section = 0; section < 3; ++section){
				collect_random_declarations(rule->alternate->section_tokens[section], &symbols, &tables);
			}
		}
	}

	for(Rule *rule : grammar->rule_list){
		if(rule == NULL)continue;
		for(int section = 0; section < 3; ++section){
			collect_rule_call_edges(rule->section_tokens[section], rule, &tables);
		}
		if(rule->alternate != NULL){
			for(int section = 0; section < 3; ++section){
				collect_rule_call_edges(rule->alternate->section_tokens[section], rule, &tables);
			}
		}
	}
	compute_potential_lexical_visibility(&tables);

	for(Rule *rule : grammar->rule_list){
		if(rule == NULL)continue;
		const auto symbols_it = tables.per_rule.find(rule);
		if(symbols_it == tables.per_rule.end())continue;
		const RuleSemanticSymbols &symbols = symbols_it->second;
		const auto visible_it = tables.potential_visible_names.find(rule);
		const std::unordered_set<std::string> visible_names =
			visible_it == tables.potential_visible_names.end()
				? std::unordered_set<std::string>{rule->rule_name + "_count"}
				: visible_it->second;

		if(!std::isfinite(rule->probability) || rule->probability < 0.0f || rule->probability > 1.0f){
			report_grammar_issue(
				grammar,
				make_action_token_diagnostic(
					first_rule_token(rule),
					"Rule '" + rule->rule_name +
					    "' probability must be a finite number between 0 and 1."));
		}
		if(rule->alternate != NULL &&
		   (!std::isfinite(rule->alternate->probability) ||
		    rule->alternate->probability < 0.0f || rule->alternate->probability > 1.0f)){
			report_grammar_issue(
				grammar,
				make_action_token_diagnostic(
					first_rule_token(rule->alternate),
					"Alternate probability for rule '" + rule->rule_name +
					    "' must be a finite number between 0 and 1."));
		}
		if(!rule->var_name.empty()){
			validate_expression_symbols(grammar,
			                            first_rule_token(rule),
			                            rule->var_name,
			                            "repeat count",
			                            visible_names);
		}
		for(int section = 0; section < 3; ++section){
			validate_semantic_tokens(grammar,
			                         rule,
			                         rule->section_tokens[section],
			                         tables,
			                         symbols,
			                         visible_names);
		}
		if(rule->alternate != NULL){
			for(int section = 0; section < 3; ++section){
				validate_semantic_tokens(grammar,
				                         rule,
				                         rule->alternate->section_tokens[section],
				                         tables,
				                         symbols,
				                         visible_names);
			}
		}
	}

	if(!grammar->rule_list.empty() && !grammar->rule_list.front()->parameter_names.empty()){
		Rule *entry = grammar->rule_list.front();
		report_grammar_issue(
			grammar,
			make_action_token_diagnostic(
				first_rule_token(entry),
				"Entry rule '" + entry->rule_name + "' expects " +
				    std::to_string(entry->parameter_names.size()) +
				    " argument(s), but entry rules receive no arguments."));
	}

	*out_tables = std::move(tables);
	return grammar->error_details.empty();
}

void GrammarDocument::ensure_start_rule_is_first()
{
	// Look for a rule named "Start" (case-insensitive)
	for (size_t i = 0; i < rule_list.size(); ++i) {
		if (rule_list[i]->rule_name == "Start" || rule_list[i]->rule_name == "start") {
			// If "Start" rule is not already first, move it to the front
			if (i > 0) {
				Rule *start_rule = rule_list[i];
				// Remove from current position
				rule_list.erase(rule_list.begin() + static_cast<std::ptrdiff_t>(i));
				// Insert at the beginning
				rule_list.insert(rule_list.begin(), start_rule);

				std::stringstream ss;
				ss<<"Start rule found and moved to beginning: "<<start_rule->rule_name<<std::endl;
				debugout(trim_copy(ss.str()));
			} else {
				std::stringstream ss;
				ss<<"Start rule is already first: "<<rule_list[0]->rule_name<<std::endl;
				debugout(trim_copy(ss.str()));
			}
			return;
		}
	}

	// No "Start" rule found, use first rule as entry point
	if (!rule_list.empty()) {
		std::stringstream ss;
		ss<<"No 'Start' rule found, using first rule as entry point: "<<rule_list[0]->rule_name<<std::endl;
		debugout(trim_copy(ss.str()));
	}
}

static void parse_rule_lines_into_grammar(Grammar *grammar, const std::vector<LogicalRule> &logical_rules) {
	for(const LogicalRule &logical_rule : logical_rules){
		if(logical_rule.content.empty())continue;

		std::vector<std::size_t> arrow_indices;
		for(std::size_t token_index = 0; token_index < logical_rule.tokens.size(); ++token_index){
			if(logical_rule.tokens[token_index].text == "->"){
				arrow_indices.push_back(token_index);
			}
		}

		if(arrow_indices.empty()){
			report_grammar_issue(grammar,
			                     make_line_diagnostic(grammar,
			                                          logical_rule.start_line,
			                                          "Rule is missing a '->' production arrow."),
			                     logical_rule.content);
			continue;
		}
		if(arrow_indices.size() > 2){
			report_grammar_issue(grammar,
			                     make_token_diagnostic(logical_rule.tokens[arrow_indices[2]],
			                                           "Rule has too many '->' clauses. Only one alternate production is supported."),
			                     logical_rule.content);
			continue;
		}

		const std::size_t primary_arrow_index = arrow_indices[0];
		const std::string header_text = join_token_texts(logical_rule.tokens, 0, primary_arrow_index);
		if(header_text.empty()){
			report_grammar_issue(grammar,
			                     make_token_diagnostic(logical_rule.tokens[primary_arrow_index],
			                                           "Rule is missing a left-hand side name."),
			                     logical_rule.content);
			continue;
		}

		std::istringstream lin(header_text + " ->");
		std::string rulename;
		if(!(lin >> rulename) || !is_identifier_token_string(rulename)){
			report_grammar_issue(grammar,
			                     make_token_diagnostic(logical_rule.tokens[0],
			                                           "Rule name must be a valid identifier."),
			                     logical_rule.content);
			continue;
		}

		const std::size_t primary_body_start = primary_arrow_index + 1;
		const std::size_t primary_body_end =
			arrow_indices.size() > 1 ? arrow_indices[1] : logical_rule.tokens.size();
		const std::string primary_body =
			join_token_texts(logical_rule.tokens, primary_body_start, primary_body_end);
		const std::vector<SourceTokenSpan> primary_body_tokens =
			slice_token_spans(logical_rule.tokens, primary_body_start, primary_body_end);
		if(primary_body.empty()){
			report_grammar_issue(grammar,
			                     make_token_diagnostic(logical_rule.tokens[primary_arrow_index],
			                                           "Rule '" + rulename + "' has an empty production body."),
			                     logical_rule.content);
			continue;
		}

		Rule *rule = new Rule(rulename, 1);
		const std::size_t error_count_before_body = grammar->error_details.size();
		std::string line =
			grammar->ruleBody(rule,
			                 lin,
			                 primary_body,
			                 logical_rule.start_line,
			                 header_text,
			                 &primary_body_tokens);
		if(line.empty() || grammar->error_details.size() != error_count_before_body){
			delete rule;
			continue;
		}
		if(arrow_indices.size() > 1){
			const std::string alternate_body =
				join_token_texts(logical_rule.tokens, arrow_indices[1] + 1, logical_rule.tokens.size());
			if(alternate_body.empty()){
				report_grammar_issue(grammar,
				                     make_token_diagnostic(logical_rule.tokens[arrow_indices[1]],
				                                           "Rule '" + rulename + "' has an empty alternate production."),
				                     logical_rule.content);
			}
			else{
				const std::vector<SourceTokenSpan> alternate_body_tokens =
					slice_token_spans(logical_rule.tokens, arrow_indices[1] + 1, logical_rule.tokens.size());
				const std::size_t error_count_before_alternate = grammar->error_details.size();
				grammar->ruleAlternate(rule,
				                      alternate_body,
				                      logical_rule.start_line,
				                      &alternate_body_tokens);
				if(grammar->error_details.size() != error_count_before_alternate && rule->alternate != NULL){
					delete rule->alternate;
					rule->alternate = NULL;
				}
			}
		}
		grammar->rule_list.push_back(rule);
	}
}

std::string removeSpaces(std::string word) {
    std::string newWord;
    newWord.reserve(word.size());
    for (char character : word) {
        if (character != ' ') {
            newWord += character;
        }
    }
    return newWord;
}

std::vector<std::string> breakup(std::string input,std::string delimiter){
		std::vector<std::string> output;
	
		int pos=-1;
		
		while( (pos = input.find(delimiter))!=-1){
	
			std::string str=input.substr(0, pos);
			
			input.erase(0, pos + delimiter.length());
		
		    output.push_back(str);
		}
		//input=removeSpaces(input);
		//if(input!="")
		output.push_back(input);
		return output;
}



bool checkAlpha(std::string str){
    for(char character : str)
        if( !isalpha(static_cast<unsigned char>(character)) ||
            !isspace(static_cast<unsigned char>(character)) )
            return true;
    return false;
}
bool isnumber(const std::string& s)
{
	double value = 0.0;
	return parse_finite_number(trim_copy(s), &value);
}

static bool is_identifier_start_char(char c)
{
	const unsigned char character = static_cast<unsigned char>(c);
	return std::isalpha(character) != 0 || c == '_';
}

static bool is_identifier_char(char c)
{
	const unsigned char character = static_cast<unsigned char>(c);
	return std::isalnum(character) != 0 || c == '_';
}

static bool is_identifier_token_string(const std::string &text)
{
	if(text.empty() || !is_identifier_start_char(text[0]))return false;
	for(std::size_t index = 1; index < text.size(); ++index){
		if(!is_identifier_char(text[index]))return false;
	}
	return true;
}

static bool is_supported_instance_type_name(const std::string &text)
{
	return text == "Cube" || text == "CubeX" || text == "CubeY" ||
	       text == "CubeZ" ||
	       ShapeSpecificationParser::isSupportedShapeName(text);
}

static float sample_variable_value(float min, float max, bool integer)
{
	if(integer){
		const int min_value = static_cast<int>(std::ceil(min));
		const int max_value = static_cast<int>(std::floor(max));
		if(max_value < min_value){
			return static_cast<float>(min_value);
		}
		std::uniform_int_distribution<int> unif(min_value, max_value);
		return static_cast<float>(unif(grammar_rng));
	}

	std::uniform_real_distribution<double> unif(0.0, 1.0);
	const double minimum = static_cast<double>(min);
	const double maximum = static_cast<double>(max);
	return static_cast<float>(unif(grammar_rng) * (maximum - minimum) + minimum);
}

static void append_rule_token(Rule *rule, int section, std::vector<Token *> *target, Token *token)
{
	if(token == NULL)return;
	if(target != NULL){
		target->push_back(token);
	}
	else{
		rule->addToken(token, section);
	}
}

static std::vector<std::string> split_rule_tokens(const std::string &rule_str)
{
	std::vector<std::string> tokens;
	std::istringstream lin(normalize_grammar_spacing(rule_str));
	std::string token;
	while(lin >> token){
		tokens.push_back(token);
	}
	return tokens;
}

static std::string parse_parenthesized_expression(const std::vector<std::string> &raw_tokens, std::size_t *index)
{
	if(index == NULL || *index >= raw_tokens.size() || raw_tokens[*index] != "("){
		throw(1);
	}

	++(*index);
	int depth = 1;
	std::string expression;
	while(*index < raw_tokens.size()){
		const std::string &token = raw_tokens[*index];
		++(*index);
		if(token == "("){
			++depth;
			expression += token;
			continue;
		}
		if(token == ")"){
			--depth;
			if(depth == 0){
				return removeSpaces(expression);
			}
			expression += token;
			continue;
		}
		expression += token;
	}

	throw(1);
}

static bool is_math_operator_char(char c) noexcept
{
	return c == '+' || c == '-' || c == '*' || c == '/' || c == '^';
}

static char last_non_space_char(const std::string &text)
{
	for(std::size_t index = text.size(); index > 0; --index){
		const char c = text[index - 1];
		if(!std::isspace(static_cast<unsigned char>(c))){
			return c;
		}
	}
	return '\0';
}

static bool expression_tail_requires_more(const std::string &expression)
{
	const char tail = last_non_space_char(expression);
	return tail == '\0' || is_math_operator_char(tail) || tail == '(';
}

static bool token_begins_expression_continuation(const std::string &token)
{
	const std::string trimmed = trim_copy(token);
	if(trimmed.empty())return false;
	if(trimmed.size() == 1){
		return is_math_operator_char(trimmed[0]);
	}
	if((trimmed[0] == '+' || trimmed[0] == '-')){
		if(isnumber(trimmed))return false;
		if(is_identifier_token_string(trimmed.substr(1)))return false;
	}
	return trimmed[0] == '*' || trimmed[0] == '/' || trimmed[0] == '^';
}

static bool is_supported_math_function_name(const std::string &token)
{
	return token == "sin" || token == "cos" || token == "tan" ||
	       token == "abs" || token == "sqrt" || token == "floor" ||
	       token == "ceil" || token == "min" || token == "max" ||
	       token == "clamp" || token == "wrap" || token == "lerp" ||
	       token == "cycle" || token == "pingpong" || token == "pulse" ||
	       token == "accelerate" || token == "decelerate" ||
	       token == "ease" || token == "smoother" ||
	       token == "swing" || token == "oscillate" || token == "spin" ||
	       token == "radians" || token == "degrees";
}

static bool token_ends_with_supported_math_function_name(const std::string &token)
{
	std::size_t function_name_start = token.size();
	while(function_name_start > 0 &&
	      is_identifier_char(token[function_name_start - 1])){
		--function_name_start;
	}
	if(function_name_start == token.size())return false;
	return is_supported_math_function_name(token.substr(function_name_start));
}

static void assign_token_source_range(Token *token,
                                      const std::vector<SourceTokenSpan> *raw_token_spans,
                                      std::size_t start_index,
                                      std::size_t end_index)
{
	if(token == NULL || raw_token_spans == NULL || raw_token_spans->empty() ||
	   start_index >= raw_token_spans->size()){
		return;
	}

	end_index = std::min(end_index, raw_token_spans->size());
	if(end_index <= start_index){
		end_index = start_index + 1;
	}

	const SourceTokenSpan &start_span =
		(*raw_token_spans)[start_index];
	const SourceTokenSpan &end_span =
		(*raw_token_spans)[end_index - 1];
	token->source_start_line = start_span.line;
	token->source_start_column = start_span.start_column;
	token->source_end_line = end_span.line;
	token->source_end_column = end_span.end_column;
}

static GrammarSourceRange resolve_grammar_source_range(
	const std::vector<SourceTokenSpan> *raw_token_spans,
	std::size_t start_index,
	std::size_t end_index)
{
	if (raw_token_spans == nullptr || raw_token_spans->empty() ||
	    start_index >= raw_token_spans->size()) {
		return {};
	}
	end_index = std::min(end_index, raw_token_spans->size());
	if (end_index <= start_index) end_index = start_index + 1;
	const SourceTokenSpan &start_span = (*raw_token_spans)[start_index];
	const SourceTokenSpan &end_span = (*raw_token_spans)[end_index - 1];
	return GrammarSourceRange(
		start_span.line,
		start_span.start_column,
		end_span.line,
		end_span.end_column);
}

// Example: `DSX ( 1 1 (r-w)/r )` must keep `(r-w)/r` as one argument expression.
static std::string parse_expression_argument(const std::vector<std::string> &raw_tokens,
                                             std::size_t *index,
                                             const std::string &terminator)
{
	if(index == NULL || *index >= raw_tokens.size()){
		throw(1);
	}

	std::string expression;
	std::string last_token;
	int depth = 0;

	while(*index < raw_tokens.size()){
		const std::string token = raw_tokens[*index];
		if(token == terminator && depth == 0){
			break;
		}

		expression += token;
		last_token = token;
		++(*index);

		if(token == "("){
			++depth;
		}
		else if(token == ")"){
			if(depth == 0){
				throw(1);
			}
			--depth;
		}

		if(depth > 0){
			continue;
		}
		if(*index >= raw_tokens.size()){
			break;
		}

		const std::string &next_token = raw_tokens[*index];
		if(next_token == terminator){
			break;
		}
		if(expression_tail_requires_more(expression)){
			continue;
		}
		if(token_begins_expression_continuation(next_token)){
			continue;
		}
		if(next_token == "(" &&
		   (is_identifier_token_string(last_token) ||
		    token_ends_with_supported_math_function_name(last_token))){
			continue;
		}

		break;
	}

	if(depth != 0 || expression.empty()){
		throw(1);
	}

	return removeSpaces(expression);
}

static void parse_tokens_into(Grammar *grammar,
                              Rule *rule,
                              const std::vector<std::string> &raw_tokens,
                              const std::vector<SourceTokenSpan> *raw_token_spans,
                              std::size_t *index,
                              int section,
                              std::vector<Token *> *target,
                              const std::string &stop_token);





GrammarVariableDefinition::GrammarVariableDefinition(std::string name,float min,float max,bool i){
	this->var_name=std::move(name);
	this->min=min;
	this->max=max;
	this->value=min;
	this->integer=i;
}

float GrammarVariableDefinition::getRandom(){
	return sample_variable_value(min, max, integer);
}

struct ExpressionEvaluationResult {
	bool ok = false;
	float value = std::numeric_limits<float>::quiet_NaN();
	std::size_t error_offset = 0;
	std::string message;
};

class CheckedGrammarExpressionParser {
public:
	CheckedGrammarExpressionParser(
		Grammar *grammar,
		std::string input,
		const Token *token)
		: grammar_(grammar), input_(std::move(input)), token_(token)
	{
	}

	ExpressionEvaluationResult parse()
	{
		ExpressionEvaluationResult result;
		skipWhitespace();
		if(atEnd()){
			fail("expression is empty");
		}
		const double value = parseExpression();
		skipWhitespace();
		if(valid_ && !atEnd()){
			fail(std::string("unexpected token '") + peek() + "'");
		}
		if(valid_ && !std::isfinite(value)){
			fail("result is not finite");
		}
		if(valid_ && !value_fits_finite_float(value)){
			fail("result is outside the finite float range");
		}
		result.ok = valid_;
		result.error_offset = error_offset_;
		result.message = error_message_;
		if(valid_){
			result.value = static_cast<float>(value);
		}
		return result;
	}

private:
	double parseExpression()
	{
		double value = parseTerm();
		while(valid_){
			skipWhitespace();
			if(match('+')){
				value += parseTerm();
				continue;
			}
			if(match('-')){
				value -= parseTerm();
				continue;
			}
			break;
		}
		return value;
	}

	double parseTerm()
	{
		double value = parseUnary();
		while(valid_){
			skipWhitespace();
			if(match('*')){
				value *= parseUnary();
				continue;
			}
			if(match('/')){
				const double divisor = parseUnary();
				if(!valid_)return 0.0;
				if(std::fabs(divisor) <= 1.0e-12){
					fail("division by zero");
					return 0.0;
				}
				value /= divisor;
				continue;
			}
			break;
		}
		return value;
	}

	double parseUnary()
	{
		skipWhitespace();
		if(match('+'))return parseUnary();
		if(match('-'))return -parseUnary();
		return parsePower();
	}

	double parsePower()
	{
		double value = parsePrimary();
		skipWhitespace();
		if(match('^')){
			const double exponent = parseUnary();
			if(!valid_)return 0.0;
			value = std::pow(value, exponent);
			if(!std::isfinite(value)){
				fail("power operation produced a non-finite result");
				return 0.0;
			}
		}
		return value;
	}

	double parsePrimary()
	{
		skipWhitespace();
		if(atEnd()){
			fail("expected a number, variable, function, or parenthesized expression");
			return 0.0;
		}

		if(match('(')){
			const double value = parseExpression();
			skipWhitespace();
			if(!match(')')){
				fail("missing closing ')'");
				return 0.0;
			}
			return value;
		}

		if(match('&')){
			skipWhitespace();
			if(atEnd() || !is_identifier_start_char(peek())){
				fail("'&' must be followed by a random-variable name");
				return 0.0;
			}
			const std::string name = parseIdentifier();
			const RuntimeVariableValue *variable = lookup_runtime_variable(grammar_, name);
			if(variable == NULL){
				fail("undefined random variable '" + name + "'");
				return 0.0;
			}
			if(!variable->resampleable){
				fail("variable '" + name + "' is immutable and cannot be resampled");
				return 0.0;
			}
			return sample_variable_value(variable->min, variable->max, variable->integer);
		}

		if(is_identifier_start_char(peek())){
			const std::string identifier = parseIdentifier();
			skipWhitespace();
			if(match('(')){
				return parseFunction(identifier);
			}
			const RuntimeVariableValue *variable = lookup_runtime_variable(grammar_, identifier);
			if(variable != NULL)return variable->value;
			if(token_ != NULL){
				const auto captured = token_->captured_variables.find(identifier);
				if(captured != token_->captured_variables.end())return captured->second;
			}
			if(variable == NULL){
				fail("undefined variable '" + identifier + "'");
				return 0.0;
			}
			return variable->value;
		}

		return parseNumber();
	}

	double parseFunction(const std::string &name)
	{
		std::vector<double> arguments;
		skipWhitespace();
		if(!match(')')){
			while(valid_){
				arguments.push_back(parseExpression());
				if(!valid_)return 0.0;
				skipWhitespace();
				if(match(')'))break;
				if(!match(',')){
					fail("function '" + name + "' requires comma-separated arguments");
					return 0.0;
				}
			}
		}

		const auto arity_is = [&](std::size_t count) {
			return arguments.size() == count;
		};
		const auto arity_between = [&](std::size_t minimum, std::size_t maximum) {
			return arguments.size() >= minimum && arguments.size() <= maximum;
		};
		const auto fail_arity = [&](const std::string &expected) {
			fail("function '" + name + "' expects " + expected + " argument(s), but received " +
			     std::to_string(arguments.size()));
			return 0.0;
		};
		const auto clamp01 = [](double value) {
			return std::clamp(value, 0.0, 1.0);
		};
		const auto validate_period = [&](double period) {
			if(period > 0.0 && std::isfinite(period))return true;
			fail("function '" + name + "' requires period > 0");
			return false;
		};
		const auto cycle_phase = [&](double time, double period, double phase) {
			// Reduce time before normalization. This avoids a large quotient
			// erasing the fractional phase when t has grown far beyond one cycle.
			const double time_fraction = std::fmod(time, period) / period;
			double wrapped = std::fmod(time_fraction + phase, 1.0);
			if(wrapped < 0.0)wrapped += 1.0;
			return wrapped;
		};
		const auto validate_bounds = [&](double minimum, double maximum, bool strict) {
			const bool valid = strict ? maximum > minimum : maximum >= minimum;
			if(valid)return true;
			fail("function '" + name + "' requires maximum " +
			     std::string(strict ? ">" : ">=") + " minimum");
			return false;
		};

		if(name == "sin" || name == "cos" || name == "tan" ||
		   name == "abs" || name == "sqrt" || name == "floor" ||
		   name == "ceil" || name == "ease" || name == "smoother" ||
		   name == "radians" || name == "degrees"){
			if(!arity_is(1))return fail_arity("1");
			const double value = arguments[0];
			if(name == "sin")return std::sin(value);
			if(name == "cos")return std::cos(value);
			if(name == "tan")return std::tan(value);
			if(name == "abs")return std::fabs(value);
			if(name == "floor")return std::floor(value);
			if(name == "ceil")return std::ceil(value);
			if(name == "sqrt"){
				if(value < 0.0){
					fail("sqrt() received a negative argument");
					return 0.0;
				}
				return std::sqrt(value);
			}
			if(name == "radians")return value * (3.14159265358979323846264338327950288 / 180.0);
			if(name == "degrees")return value * (180.0 / 3.14159265358979323846264338327950288);
			const double x = clamp01(value);
			if(name == "ease")return x * x * (3.0 - (2.0 * x));
			return x * x * x * (x * ((x * 6.0) - 15.0) + 10.0);
		}

		if(name == "min" || name == "max"){
			if(!arity_is(2))return fail_arity("2");
			return name == "min" ? std::min(arguments[0], arguments[1])
			                     : std::max(arguments[0], arguments[1]);
		}

		if(name == "clamp" || name == "wrap" || name == "lerp"){
			if(!arity_is(3))return fail_arity("3");
			if(name == "lerp"){
				return arguments[0] + ((arguments[1] - arguments[0]) * arguments[2]);
			}
			if(!validate_bounds(arguments[1], arguments[2], name == "wrap"))return 0.0;
			if(name == "clamp")return std::clamp(arguments[0], arguments[1], arguments[2]);
			const double width = arguments[2] - arguments[1];
			const double offset = arguments[0] - arguments[1];
			double wrapped = std::fmod(offset, width);
			if(wrapped < 0.0)wrapped += width;
			return arguments[1] + wrapped;
		}

		if(name == "cycle" || name == "pingpong"){
			if(!arity_between(2, 3))return fail_arity("2 or 3");
			if(!validate_period(arguments[1]))return 0.0;
			const double phase = arguments.size() == 3 ? arguments[2] : 0.0;
			const double cycle = cycle_phase(arguments[0], arguments[1], phase);
			if(name == "cycle")return cycle;
			return 1.0 - std::fabs((2.0 * cycle) - 1.0);
		}

		if(name == "pulse"){
			if(!arity_between(3, 4))return fail_arity("3 or 4");
			if(!validate_period(arguments[1]))return 0.0;
			if(arguments[2] < 0.0 || arguments[2] > 1.0){
				fail("function 'pulse' requires duty in [0, 1]");
				return 0.0;
			}
			const double phase = arguments.size() == 4 ? arguments[3] : 0.0;
			return cycle_phase(arguments[0], arguments[1], phase) < arguments[2] ? 1.0 : 0.0;
		}

		if(name == "accelerate" || name == "decelerate"){
			if(!arity_between(1, 2))return fail_arity("1 or 2");
			const double power = arguments.size() == 2 ? arguments[1] : 2.0;
			if(!(power > 0.0) || !std::isfinite(power)){
				fail("function '" + name + "' requires exponent > 0");
				return 0.0;
			}
			const double x = clamp01(arguments[0]);
			if(name == "accelerate")return std::pow(x, power);
			return 1.0 - std::pow(1.0 - x, power);
		}

		if(name == "swing" || name == "oscillate"){
			if(!arity_between(4, 5))return fail_arity("4 or 5");
			if(!validate_bounds(arguments[1], arguments[2], false))return 0.0;
			if(!validate_period(arguments[3]))return 0.0;
			const double phase = arguments.size() == 5 ? arguments[4] : 0.0;
			const double cycle = cycle_phase(arguments[0], arguments[3], phase);
			if(name == "swing"){
				const double triangle = 1.0 - std::fabs((2.0 * cycle) - 1.0);
				return arguments[1] + ((arguments[2] - arguments[1]) * triangle);
			}
			const double midpoint = (arguments[1] + arguments[2]) * 0.5;
			const double amplitude = (arguments[2] - arguments[1]) * 0.5;
			return midpoint +
			       (amplitude * std::sin(2.0 * 3.14159265358979323846264338327950288 * cycle));
		}

		if(name == "spin"){
			if(!arity_between(2, 3))return fail_arity("2 or 3");
			const double offset = arguments.size() == 3 ? arguments[2] : 0.0;
			return offset + (arguments[0] * arguments[1]);
		}

		fail("unsupported function '" + name + "'");
		return 0.0;
	}

	double parseNumber()
	{
		skipWhitespace();
		const char *start = input_.c_str() + position_;
		char *end = NULL;
		errno = 0;
		const double value = std::strtod(start, &end);
		if(end == start){
			fail("expected a finite number");
			return 0.0;
		}
		if(errno == ERANGE || !std::isfinite(value)){
			fail("number is outside the finite floating-point range");
			return 0.0;
		}
		position_ = static_cast<std::size_t>(end - input_.c_str());
		return value;
	}

	std::string parseIdentifier()
	{
		const std::size_t start = position_;
		while(!atEnd() && is_identifier_char(input_[position_])){
			++position_;
		}
		return input_.substr(start, position_ - start);
	}

	void skipWhitespace()
	{
		while(!atEnd() && std::isspace(static_cast<unsigned char>(input_[position_])) != 0){
			++position_;
		}
	}

	bool match(char character)
	{
		skipWhitespace();
		if(atEnd() || input_[position_] != character)return false;
		++position_;
		return true;
	}

	char peek() const
	{
		return atEnd() ? '\0' : input_[position_];
	}

	bool atEnd() const
	{
		return position_ >= input_.size();
	}

	void fail(const std::string &message)
	{
		if(!valid_)return;
		valid_ = false;
		error_offset_ = position_;
		error_message_ = message;
	}

	Grammar *grammar_ = NULL;
	std::string input_;
	const Token *token_ = NULL;
	std::size_t position_ = 0;
	bool valid_ = true;
	std::size_t error_offset_ = 0;
	std::string error_message_;
};

static bool evaluate_expression_checked(Grammar *grammar,
                                        const Token *token,
                                        const std::string &expression,
                                        const std::string &purpose,
                                        float *out_value)
{
	if(out_value == NULL)return false;
	CheckedGrammarExpressionParser parser(grammar, trim_copy(expression), token);
	const ExpressionEvaluationResult result = parser.parse();
	if(result.ok){
		*out_value = result.value;
		return true;
	}

	std::ostringstream message;
	message << "Invalid " << purpose << " expression '" << expression << "': "
	        << result.message;
	if(active_expansion_budget != NULL){
		abort_expansion(grammar, token, message.str());
	}
	else{
		report_grammar_issue(grammar,
		                     make_action_token_diagnostic(token, message.str()));
	}
	*out_value = std::numeric_limits<float>::quiet_NaN();
	return false;
}

static float MathF2(std::string input){
	Grammar *grammar = active_grammar_runtime == NULL ? NULL : active_grammar_runtime->grammar;
	float value = std::numeric_limits<float>::quiet_NaN();
	if(!evaluate_expression_checked(grammar, NULL, input, "runtime", &value)){
		errorout("Grammar execution error: unresolved expression '" + input + "'.");
	}
	return value;
}

float GrammarDocument::MathF(std::string input){
	float value = std::numeric_limits<float>::quiet_NaN();
	evaluate_expression_checked(this, NULL, input, "arithmetic", &value);
	return value;
}

std::string GrammarDocument::MathS(std::string input){
	const float value = MathF(std::move(input));
	return std::isfinite(value) ? std::to_string(value) : std::string();
}

static glm::vec3 resolve_vec3_arguments(const Token *token)
{
	glm::vec3 result(0.0f);
	if(token == NULL)return result;
	for(std::size_t i=0;i<3;i++){
		if(token->var_names[i]!=""){
			result[i]=MathF2(token->var_names[i]);
		}
		else if(i<token->arguments.size()){
			result[i]=token->arguments[i];
		}
	}
	return result;
}

static bool parse_collision_layer_name(const std::string &name,
	                                   CollisionLayer *layer)
{
	if (layer == nullptr) return false;
	if (name == "Structure") *layer = CollisionLayer::Structure;
	else if (name == "Envelope") *layer = CollisionLayer::Envelope;
	else if (name == "Interior") *layer = CollisionLayer::Interior;
	else if (name == "Furniture") *layer = CollisionLayer::Furniture;
	else if (name == "Plumbing") *layer = CollisionLayer::Plumbing;
	else if (name == "Hvac" || name == "HVAC") *layer = CollisionLayer::Hvac;
	else if (name == "Electrical") *layer = CollisionLayer::Electrical;
	else if (name == "Equipment") *layer = CollisionLayer::Equipment;
	else if (name == "Terrain") *layer = CollisionLayer::Terrain;
	else if (name == "Temporary") *layer = CollisionLayer::Temporary;
	else return false;
	return true;
}

static bool build_collision_layer_mask(const std::vector<std::string> &names,
	                                   CollisionLayerMask *mask,
	                                   std::string *diagnostic)
{
	if (mask == nullptr) return false;
	if (names.empty()) {
		*mask = CollisionLayerMask::all();
		return true;
	}
	CollisionLayerMask parsed_mask;
	for (const std::string &name : names) {
		CollisionLayer layer = CollisionLayer::Temporary;
		if (!parse_collision_layer_name(name, &layer)) {
			if (diagnostic != nullptr) {
				*diagnostic = "Unsupported collision layer '" + name + "'.";
			}
			return false;
		}
		parsed_mask.add(layer);
	}
	*mask = parsed_mask;
	return true;
}

static bool parse_interface_type_name(const std::string &name,
	                                  SpatialInterfaceType *type)
{
	if (type == nullptr) return false;
	if (name == "Support") *type = SpatialInterfaceType::Support;
	else if (name == "Bearing") *type = SpatialInterfaceType::Bearing;
	else if (name == "Mate") *type = SpatialInterfaceType::Mate;
	else if (name == "Seat") *type = SpatialInterfaceType::Seat;
	else if (name == "Insert") *type = SpatialInterfaceType::Insert;
	else if (name == "Socket") *type = SpatialInterfaceType::Socket;
	else if (name == "Shaft") *type = SpatialInterfaceType::Shaft;
	else if (name == "Seal") *type = SpatialInterfaceType::Seal;
	else if (name == "Fastener") *type = SpatialInterfaceType::Fastener;
	else if (name == "Anchor") *type = SpatialInterfaceType::Anchor;
	else if (name == "Hinge") *type = SpatialInterfaceType::Hinge;
	else if (name == "Slide") *type = SpatialInterfaceType::Slide;
	else if (name == "PipePort") *type = SpatialInterfaceType::PipePort;
	else if (name == "DuctPort") *type = SpatialInterfaceType::DuctPort;
	else if (name == "ElectricalPort") *type = SpatialInterfaceType::ElectricalPort;
	else if (name == "DataPort") *type = SpatialInterfaceType::DataPort;
	else if (name == "ControlPort") *type = SpatialInterfaceType::ControlPort;
	else if (name == "DrainPort") *type = SpatialInterfaceType::DrainPort;
	else if (name == "ThermalInterface") *type = SpatialInterfaceType::ThermalInterface;
	else if (name == "InspectionInterface") *type = SpatialInterfaceType::InspectionInterface;
	else return false;
	return true;
}

static bool parse_connection_type_name(const std::string &name,
	                                   SpatialConnectionType *type)
{
	if (type == nullptr) return false;
	if (name == "Contains") *type = SpatialConnectionType::Contains;
	else if (name == "ConnectedTo") *type = SpatialConnectionType::ConnectedTo;
	else if (name == "DrainsTo") *type = SpatialConnectionType::DrainsTo;
	else if (name == "Powers") *type = SpatialConnectionType::Powers;
	else if (name == "ServedBy") *type = SpatialConnectionType::ServedBy;
	else if (name == "Monitors") *type = SpatialConnectionType::Monitors;
	else if (name == "SupportedBy") *type = SpatialConnectionType::SupportedBy;
	else if (name == "SeatedIn") *type = SpatialConnectionType::SeatedIn;
	else if (name == "InsertedIn") *type = SpatialConnectionType::InsertedIn;
	else if (name == "SealedTo") *type = SpatialConnectionType::SealedTo;
	else if (name == "FixedTo") *type = SpatialConnectionType::FixedTo;
	else if (name == "AlignedWith") *type = SpatialConnectionType::AlignedWith;
	else return false;
	return true;
}

static bool parse_direction_frame_name(const std::string &name,
	                                   SpatialDirectionFrame *frame)
{
	if (frame == nullptr) return false;
	if (name == "World") *frame = SpatialDirectionFrame::World;
	else if (name == "Parent") *frame = SpatialDirectionFrame::Parent;
	else if (name == "MovingObject") *frame = SpatialDirectionFrame::MovingObject;
	else if (name == "MovingInterface") *frame = SpatialDirectionFrame::MovingInterface;
	else if (name == "TargetObject") *frame = SpatialDirectionFrame::TargetObject;
	else if (name == "TargetInterface") *frame = SpatialDirectionFrame::TargetInterface;
	else return false;
	return true;
}

static bool parse_collision_position_mode_name(const std::string &name,
	                                           CollisionPositionMode *mode)
{
	if (mode == nullptr) return false;
	if (name == "Touch") *mode = CollisionPositionMode::Touch;
	else if (name == "Gap") *mode = CollisionPositionMode::Gap;
	else if (name == "Drop") *mode = CollisionPositionMode::Drop;
	else if (name == "Seat") *mode = CollisionPositionMode::Seat;
	else if (name == "Insert") *mode = CollisionPositionMode::Insert;
	else if (name == "Tangent") *mode = CollisionPositionMode::Tangent;
	else if (name == "Between") *mode = CollisionPositionMode::Between;
	else if (name == "CenterContact") *mode = CollisionPositionMode::CenterContact;
	else return false;
	return true;
}

static bool evaluate_spatial_expression(const Token *token,
	                                    const GeometryExpression &expression,
	                                    const std::string &purpose,
	                                    float *value)
{
	Grammar *grammar = active_grammar_runtime == nullptr
		? nullptr
		: active_grammar_runtime->grammar;
	return evaluate_expression_checked(
		grammar, token, expression.sourceText(), purpose, value);
}

static bool evaluate_spatial_vector(
	const Token *token,
	const std::array<GeometryExpression, 3> &expressions,
	const std::string &purpose,
	glm::vec3 *value)
{
	if (value == nullptr) return false;
	for (std::size_t axis = 0; axis < expressions.size(); ++axis) {
		if (!evaluate_spatial_expression(
				token, expressions[axis], purpose, &(*value)[axis])) {
			return false;
		}
	}
	return true;
}

static bool evaluate_spatial_clearance(
	const Token *token,
	const SpatialClearanceSyntax &syntax,
	SpatialClearanceRequirement *clearance)
{
	if (clearance == nullptr) return false;
	float nominal = 0.0f;
	float minimum = 0.0f;
	float maximum = 0.0f;
	if (!evaluate_spatial_expression(token, syntax.nominal(), "spatial clearance nominal", &nominal) ||
	    !evaluate_spatial_expression(token, syntax.minimum(), "spatial clearance minimum", &minimum) ||
	    !evaluate_spatial_expression(token, syntax.maximum(), "spatial clearance maximum", &maximum)) {
		return false;
	}
	if (!std::isfinite(nominal) || !std::isfinite(minimum) || !std::isfinite(maximum) ||
	    minimum > nominal || nominal > maximum) {
		errorout("Grammar execution error: spatial clearance must satisfy minimum <= nominal <= maximum.");
		return false;
	}
	*clearance = SpatialClearanceRequirement(nominal, minimum, maximum);
	return true;
}

void SpatialObjectScopeToken::performAction(SceneGenerationContext *)
{
	errorout("Grammar execution error: an unexpanded Object scope reached the scene VM.");
}

void SpatialObjectBeginActionToken::performAction(SceneGenerationContext *context)
{
	if (context == nullptr || !descriptor_syntax) {
		errorout("Grammar execution error: Object declaration is incomplete.");
		return;
	}
	CollisionLayer layer = CollisionLayer::Temporary;
	CollisionLayerMask mask;
	std::string diagnostic;
	if (!parse_collision_layer_name(
			descriptor_syntax->collisionParticipation().layerName(), &layer) ||
	    !build_collision_layer_mask(
			descriptor_syntax->collisionParticipation().maskLayerNames(),
			&mask,
			&diagnostic)) {
		if (diagnostic.empty()) {
			diagnostic = "Unsupported collision layer '" +
				descriptor_syntax->collisionParticipation().layerName() + "'.";
		}
		errorout("Grammar execution error: " + diagnostic);
		return;
	}
	const GrammarSourceRange &range = descriptor_syntax->sourceRange();
	SpatialObjectIdentity identity(
		SpatialObjectId(descriptor_syntax->objectId()),
		descriptor_syntax->objectName(),
		SpatialObjectClass(descriptor_syntax->objectClass()),
		SpatialTaxonomyPath(descriptor_syntax->taxonomy().segments()),
		0,
		1,
		SpatialObjectProvenance(
			source_name,
			range.startLine(),
			range.startColumn(),
			range.endLine(),
			range.endColumn()));
	if (!context->beginSpatialObject(
			std::move(identity),
			descriptor_syntax->containerObjectId(),
			CollisionParticipationPolicy(layer, mask),
			&diagnostic)) {
		errorout("Grammar execution error: " + diagnostic);
	}
}

void SpatialObjectEndActionToken::performAction(SceneGenerationContext *context)
{
	std::string diagnostic;
	if (context == nullptr || !context->endSpatialObject(&diagnostic)) {
		errorout("Grammar execution error: " +
		         (diagnostic.empty() ? "Object scope could not be closed." : diagnostic));
	}
}

void SpatialInterfaceDeclarationToken::performAction(SceneGenerationContext *context)
{
	if (context == nullptr || !declaration_syntax || !context->hasActiveSpatialObject()) {
		errorout("Grammar execution error: Interface requires an active Object scope.");
		return;
	}
	SpatialInterfaceType interface_type = SpatialInterfaceType::InspectionInterface;
	if (!parse_interface_type_name(declaration_syntax->interfaceType(), &interface_type)) {
		errorout("Grammar execution error: unsupported interface type '" +
		         declaration_syntax->interfaceType() + "'.");
		return;
	}
	glm::vec3 origin(0.0f);
	glm::vec3 normal(0.0f);
	glm::vec3 tangent(0.0f);
	float tolerance = 0.0f;
	if (!evaluate_spatial_vector(this, declaration_syntax->origin(), "interface origin", &origin) ||
	    !evaluate_spatial_vector(this, declaration_syntax->normal(), "interface normal", &normal) ||
	    !evaluate_spatial_vector(this, declaration_syntax->tangent(), "interface tangent", &tangent) ||
	    !evaluate_spatial_expression(this, declaration_syntax->tolerance(), "interface tolerance", &tolerance) ||
	    tolerance < 0.0f) {
		errorout("Grammar execution error: Interface contains invalid evaluated values.");
		return;
	}

	SpatialInterfaceRegion region = SpatialInterfaceRegion::point();
	const std::string &region_kind = declaration_syntax->regionKind();
	if (region_kind == "PlaneRectangle") {
		if (declaration_syntax->regionExtentExpressions().size() != 2) {
			errorout("Grammar execution error: PlaneRectangle requires width and height.");
			return;
		}
		float width = 0.0f;
		float height = 0.0f;
		if (!evaluate_spatial_expression(
				this, declaration_syntax->regionExtentExpressions()[0], "interface width", &width) ||
		    !evaluate_spatial_expression(
				this, declaration_syntax->regionExtentExpressions()[1], "interface height", &height)) {
			return;
		}
		region = SpatialInterfaceRegion::planeRectangle(width, height);
	}
	else if (region_kind == "AxisSegment") {
		float length = 0.0f;
		if (declaration_syntax->regionExtentExpressions().size() != 1 ||
		    !evaluate_spatial_expression(
				this, declaration_syntax->regionExtentExpressions()[0], "interface length", &length)) {
			return;
		}
		region = SpatialInterfaceRegion::axisSegment(length);
	}
	else if (region_kind == "ObjectBoundaryFace") {
		region = SpatialInterfaceRegion::objectBoundaryFace(
			declaration_syntax->boundaryFaceName());
	}

	SpatialClearanceRequirement clearance(0.0f, 0.0f, 0.0f);
	if (!evaluate_spatial_clearance(this, declaration_syntax->clearance(), &clearance)) {
		return;
	}
	std::string diagnostic;
	if (!context->addSpatialInterface(
			SpatialInterface(
				SpatialInterfaceId(declaration_syntax->interfaceId()),
				SpatialObjectId(context->currentSpatialObjectId()),
				interface_type,
				SpatialInterfaceFrame(origin, normal, tangent),
				std::move(region),
				InterfaceCompatibilityProfile(),
				clearance),
			&diagnostic)) {
		errorout("Grammar execution error: " + diagnostic);
	}
}

void SpatialConnectionDeclarationToken::performAction(SceneGenerationContext *context)
{
	if (context == nullptr || !declaration_syntax) {
		errorout("Grammar execution error: Connect declaration is incomplete.");
		return;
	}
	SpatialConnectionType connection_type = SpatialConnectionType::ConnectedTo;
	if (!parse_connection_type_name(declaration_syntax->connectionType(), &connection_type)) {
		errorout("Grammar execution error: unsupported connection type '" +
		         declaration_syntax->connectionType() + "'.");
		return;
	}
	SpatialClearanceRequirement clearance(0.0f, 0.0f, 0.0f);
	float insertion_depth = 0.0f;
	if (!evaluate_spatial_clearance(this, declaration_syntax->clearance(), &clearance) ||
	    !evaluate_spatial_expression(
			this,
			declaration_syntax->insertionDepth(),
			"connection insertion depth",
			&insertion_depth)) {
		return;
	}
	std::string diagnostic;
	if (!context->addSpatialConnection(
			SpatialConnection(
				SpatialConnectionId(declaration_syntax->connectionId()),
				SpatialInterfaceReference(
					SpatialObjectId(declaration_syntax->sourceObjectId()),
					SpatialInterfaceId(declaration_syntax->sourceInterfaceId())),
				SpatialInterfaceReference(
					SpatialObjectId(declaration_syntax->targetObjectId()),
					SpatialInterfaceId(declaration_syntax->targetInterfaceId())),
				connection_type,
				clearance,
				insertion_depth,
				ConstraintDegreeOfFreedomState(),
				SpatialConnectionState::Proposed),
			&diagnostic)) {
		errorout("Grammar execution error: " + diagnostic);
	}
}

void SpatialConstraintDeclarationToken::performAction(SceneGenerationContext *context)
{
	if (context == nullptr || !declaration_syntax || !context->hasActiveSpatialObject()) {
		errorout("Grammar execution error: Position requires an active Object scope.");
		return;
	}
	CollisionPositionMode mode = CollisionPositionMode::Touch;
	SpatialDirectionFrame direction_frame = SpatialDirectionFrame::TargetInterface;
	if (!parse_collision_position_mode_name(declaration_syntax->mode(), &mode) ||
	    !parse_direction_frame_name(
			declaration_syntax->direction().frameName(), &direction_frame)) {
		errorout("Grammar execution error: Position contains an unsupported mode or direction frame.");
		return;
	}
	glm::vec3 direction_vector(0.0f);
	SpatialClearanceRequirement clearance(0.0f, 0.0f, 0.0f);
	float seating_depth = 0.0f;
	float maximum_distance = 0.0f;
	float tolerance = 0.0f;
	float priority_value = 0.0f;
	if (!evaluate_spatial_vector(
			this,
			declaration_syntax->direction().vectorExpressions(),
			"position direction",
			&direction_vector) ||
	    !evaluate_spatial_clearance(this, declaration_syntax->clearance(), &clearance) ||
	    !evaluate_spatial_expression(
			this, declaration_syntax->seatingDepth(), "position seating depth", &seating_depth) ||
	    !evaluate_spatial_expression(
			this, declaration_syntax->maximumDistance(), "position maximum distance", &maximum_distance) ||
	    !evaluate_spatial_expression(
			this, declaration_syntax->tolerance(), "position tolerance", &tolerance) ||
	    !evaluate_spatial_expression(
			this, declaration_syntax->priority(), "position priority", &priority_value)) {
		return;
	}
	CollisionLayerMask collision_mask;
	std::string diagnostic;
	if (!build_collision_layer_mask(
			declaration_syntax->collisionMaskLayerNames(),
			&collision_mask,
			&diagnostic)) {
		errorout("Grammar execution error: " + diagnostic);
		return;
	}
	if (!context->addSpatialConstraint(
			std::make_shared<const CollisionPositionConstraint>(
				SpatialConstraintId(declaration_syntax->constraintId()),
				SpatialObjectId(context->currentSpatialObjectId()),
				SpatialInterfaceId(declaration_syntax->movingInterfaceId()),
				SpatialObjectId(declaration_syntax->targetObjectId()),
				SpatialInterfaceId(declaration_syntax->targetInterfaceId()),
				SpatialDirection(direction_frame, direction_vector),
				mode,
				clearance,
				seating_depth,
				maximum_distance,
				tolerance,
				collision_mask,
				static_cast<int>(std::lround(priority_value))),
			&diagnostic)) {
		errorout("Grammar execution error: " + diagnostic);
	}
}

static bool parse_vehicle_joint_type(
	const std::string &name,
	VehicleJointType *joint_type)
{
	if (joint_type == nullptr) return false;
	std::string normalized = name;
	std::transform(
		normalized.begin(), normalized.end(), normalized.begin(),
		[](unsigned char character) {
			return static_cast<char>(std::tolower(character));
		});
	if (normalized == "fixed") *joint_type = VehicleJointType::Fixed;
	else if (normalized == "revolute") *joint_type = VehicleJointType::Revolute;
	else if (normalized == "prismatic") *joint_type = VehicleJointType::Prismatic;
	else if (normalized == "ball") *joint_type = VehicleJointType::Ball;
	else return false;
	return true;
}

void VehicleJointDeclarationToken::performAction(SceneGenerationContext *context)
{
	if (context == nullptr || !declaration_syntax) {
		errorout("Grammar execution error: Joint declaration is incomplete.");
		return;
	}
	VehicleJointType joint_type = VehicleJointType::Fixed;
	if (!parse_vehicle_joint_type(declaration_syntax->jointType(), &joint_type)) {
		errorout("Grammar execution error: Joint type must be Fixed, Revolute, Prismatic, or Ball.");
		return;
	}
	glm::vec3 axis(0.0f);
	float minimum_state = 0.0f;
	float maximum_state = 0.0f;
	float current_state = 0.0f;
	if (!evaluate_spatial_vector(this, declaration_syntax->axis(), "joint axis", &axis) ||
	    !evaluate_spatial_expression(
		    this, declaration_syntax->minimumState(), "joint minimum state", &minimum_state) ||
	    !evaluate_spatial_expression(
		    this, declaration_syntax->maximumState(), "joint maximum state", &maximum_state) ||
	    !evaluate_spatial_expression(
		    this, declaration_syntax->currentState(), "joint current state", &current_state)) {
		return;
	}
	std::string diagnostic;
	if (!context->addVehicleJoint(
			VehicleJoint(
				declaration_syntax->jointIdentifier(), joint_type,
				declaration_syntax->sourceObjectIdentifier(),
				declaration_syntax->sourceInterfaceIdentifier(),
				declaration_syntax->targetObjectIdentifier(),
				declaration_syntax->targetInterfaceIdentifier(),
				axis, minimum_state, maximum_state, current_state),
			&diagnostic)) {
		errorout("Grammar execution error: " + diagnostic);
	}
}

static bool parse_on_off(const std::string &value, bool *enabled)
{
	if (enabled == nullptr) return false;
	if (value == "on" || value == "On" || value == "true" || value == "True") {
		*enabled = true;
		return true;
	}
	if (value == "off" || value == "Off" || value == "false" || value == "False") {
		*enabled = false;
		return true;
	}
	return false;
}

static bool parse_scene_light_type(const std::string &name, SceneLightType *type)
{
	if (type == nullptr) return false;
	if (name == "Directional") *type = SceneLightType::Directional;
	else if (name == "Point") *type = SceneLightType::Point;
	else if (name == "Spot") *type = SceneLightType::Spot;
	else if (name == "RectangularArea" || name == "Area") *type = SceneLightType::RectangularArea;
	else return false;
	return true;
}

static bool parse_fixture_type(const std::string &name, LightFixtureType *type)
{
	if (type == nullptr) return false;
	if (name == "RecessedDownlight") *type = LightFixtureType::RecessedDownlight;
	else if (name == "Pendant") *type = LightFixtureType::Pendant;
	else if (name == "LEDStrip" || name == "LedStrip") *type = LightFixtureType::LedStrip;
	else if (name == "ExteriorWallLight") *type = LightFixtureType::ExteriorWallLight;
	else if (name == "SurfaceDownlight") *type = LightFixtureType::SurfaceDownlight;
	else if (name == "LinearPendant") *type = LightFixtureType::LinearPendant;
	else if (name == "WallSconce") *type = LightFixtureType::WallSconce;
	else if (name == "Bollard") *type = LightFixtureType::Bollard;
	else if (name == "StepLight") *type = LightFixtureType::StepLight;
	else if (name == "ExteriorSpot") *type = LightFixtureType::ExteriorSpot;
	else if (name == "AreaPanel") *type = LightFixtureType::AreaPanel;
	else return false;
	return true;
}

static bool parse_switch_type(const std::string &name, SwitchType *type)
{
	if (type == nullptr) return false;
	if (name == "SinglePole") *type = SwitchType::SinglePole;
	else if (name == "TwoWay") *type = SwitchType::TwoWay;
	else if (name == "Intermediate") *type = SwitchType::Intermediate;
	else if (name == "Dimmer") *type = SwitchType::Dimmer;
	else if (name == "Momentary") *type = SwitchType::Momentary;
	else if (name == "Smart") *type = SwitchType::Smart;
	else if (name == "SceneController") *type = SwitchType::SceneController;
	else return false;
	return true;
}

static bool evaluate_lighting_vector2(
	const Token *token,
	const std::array<GeometryExpression, 2> &expressions,
	const std::string &purpose,
	glm::vec2 *value)
{
	return value != nullptr &&
	       evaluate_spatial_expression(token, expressions[0], purpose, &value->x) &&
	       evaluate_spatial_expression(token, expressions[1], purpose, &value->y);
}

static SceneLightData scene_light_data(
	SceneLightType type,
	float range,
	const glm::vec2 &cone_degrees,
	const glm::vec2 &area_size,
	bool two_sided)
{
	if (type == SceneLightType::Directional) return DirectionalLightData{};
	if (type == SceneLightType::Spot) {
		return SpotLightData{
			range, glm::radians(cone_degrees.x), glm::radians(cone_degrees.y)};
	}
	if (type == SceneLightType::RectangularArea) {
		return RectangularAreaLightData{area_size, range, two_sided};
	}
	return PointLightData{range};
}

static glm::vec3 resolve_lighting_scope_position(
	SceneGenerationContext *context,
	const glm::vec3 &local_position)
{
	if (context == nullptr || context->getCurrentScope() == nullptr) return local_position;
	return glm::vec3(
		context->getCurrentScope()->getTransform() * glm::vec4(local_position, 1.0f));
}

static glm::vec3 resolve_lighting_scope_direction(
	SceneGenerationContext *context,
	const glm::vec3 &local_direction)
{
	if (context == nullptr || context->getCurrentScope() == nullptr) return local_direction;
	const glm::vec3 transformed =
		glm::mat3(context->getCurrentScope()->getTransform()) * local_direction;
	return glm::length(transformed) > 0.0001f
		? glm::normalize(transformed)
		: glm::vec3(0.0f, -1.0f, 0.0f);
}

static bool publish_light_fixture(
	SceneGenerationContext *context,
	const std::string &fixture_id,
	const std::string &emitter_id,
	LightFixtureType fixture_type,
	const glm::vec3 &position,
	const glm::vec3 &direction,
	const glm::vec3 &clearance,
	const glm::vec2 &cone_degrees,
	float temperature,
	float lumens,
	float range,
	const std::string &mount_interface,
	bool casts_shadow,
	std::string *diagnostic)
{
	if (context == nullptr) return false;
	const glm::vec3 world_position = resolve_lighting_scope_position(context, position);
	const glm::vec3 world_direction = resolve_lighting_scope_direction(context, direction);
	const bool area_emitter = fixture_type == LightFixtureType::LedStrip ||
	                          fixture_type == LightFixtureType::AreaPanel ||
	                          fixture_type == LightFixtureType::LinearPendant;
	const SceneLightType light_type = area_emitter
		? SceneLightType::RectangularArea
		: SceneLightType::Spot;
	SceneLight light(
		LightId(emitter_id), fixture_id + " Emitter", light_type,
		area_emitter
			? SceneLightData(RectangularAreaLightData{glm::vec2(1.0f, 0.12f), range, false})
			: SceneLightData(SpotLightData{
				range, glm::radians(cone_degrees.x), glm::radians(cone_degrees.y)}));
	light.setPosition(world_position);
	light.setEmissionDirection(world_direction);
	light.setProvenance(LightProvenance::Grammar);
	light.emission().setUsesColorTemperature(true);
	light.emission().setColorTemperatureKelvin(temperature);
	light.emission().setIntensity(lumens);
	light.emission().setIntensityUnit(PhotometricIntensityUnit::Lumens);
	light.shadow().setEnabled(casts_shadow);

	LightFixtureObject fixture(
		ElectricalObjectId(fixture_id), fixture_id, fixture_type);
	fixture.setTransform(glm::translate(glm::mat4(1.0f), world_position));
	fixture.setMountInterface(mount_interface);
	fixture.setClearanceEnvelope(clearance);
	LightEmitterDefinition emitter;
	emitter.scene_light_id = LightId(emitter_id);
	emitter.type = light_type;
	emitter.emission = light.emission();
	emitter.range = range;
	emitter.inner_cone_radians = glm::radians(cone_degrees.x);
	emitter.outer_cone_radians = glm::radians(cone_degrees.y);
	fixture.emitters().push_back(emitter);
	return context->addLightFixtureWithEmitter(
		std::move(light), std::move(fixture), diagnostic);
}

void SceneLightDeclarationToken::performAction(SceneGenerationContext *context)
{
	if (context == nullptr || !declaration_syntax) {
		errorout("Grammar execution error: Light declaration is incomplete.");
		return;
	}
	SceneLightType type = SceneLightType::Point;
	glm::vec3 position(0.0f);
	glm::vec3 direction(0.0f, -1.0f, 0.0f);
	glm::vec3 color(1.0f);
	glm::vec2 cone_degrees(24.0f, 38.0f);
	glm::vec2 area_size(1.0f);
	float temperature = 6500.0f;
	float intensity = 1000.0f;
	float range = 10.0f;
	float exposure = 0.0f;
	bool casts_shadow = false;
	bool two_sided = false;
	if (!parse_scene_light_type(declaration_syntax->type, &type) ||
	    !evaluate_spatial_vector(this, declaration_syntax->position, "light position", &position) ||
	    !evaluate_spatial_vector(this, declaration_syntax->direction, "light direction", &direction) ||
	    !evaluate_spatial_vector(this, declaration_syntax->color, "light color", &color) ||
	    !evaluate_lighting_vector2(this, declaration_syntax->cone, "light cone", &cone_degrees) ||
	    !evaluate_lighting_vector2(this, declaration_syntax->area, "light area", &area_size) ||
	    !evaluate_spatial_expression(this, declaration_syntax->temperature, "light temperature", &temperature) ||
	    !evaluate_spatial_expression(this, declaration_syntax->intensity, "light intensity", &intensity) ||
	    !evaluate_spatial_expression(this, declaration_syntax->range, "light range", &range) ||
	    !evaluate_spatial_expression(this, declaration_syntax->exposure, "light exposure", &exposure) ||
	    !parse_on_off(declaration_syntax->shadow, &casts_shadow) ||
	    !parse_on_off(declaration_syntax->two_sided, &two_sided)) {
		errorout("Grammar execution error: Light contains an invalid type, expression, or on/off value.");
		return;
	}
	SceneLight light(
		LightId(declaration_syntax->light_id),
		declaration_syntax->light_id,
		type,
		scene_light_data(type, range, cone_degrees, area_size, two_sided));
	light.setProvenance(LightProvenance::Grammar);
	light.setReferenceFrame(
		declaration_syntax->reference_frame == "CameraRelative"
			? SceneLightReferenceFrame::CameraRelative
			: SceneLightReferenceFrame::World);
	if (light.referenceFrame() == SceneLightReferenceFrame::CameraRelative) {
		light.setPosition(position);
		light.setEmissionDirection(direction);
	} else {
		light.setPosition(resolve_lighting_scope_position(context, position));
		light.setEmissionDirection(resolve_lighting_scope_direction(context, direction));
	}
	light.emission().setLinearRgb(color);
	light.emission().setUsesColorTemperature(declaration_syntax->uses_temperature);
	light.emission().setColorTemperatureKelvin(temperature);
	light.emission().setIntensity(intensity);
	light.emission().setIntensityUnit(
		declaration_syntax->intensity_unit == "Candela" ? PhotometricIntensityUnit::Candela
		: declaration_syntax->intensity_unit == "Lux" ? PhotometricIntensityUnit::Lux
		: PhotometricIntensityUnit::Lumens);
	light.setExposureCompensation(exposure);
	light.shadow().setEnabled(casts_shadow);
	std::string diagnostic;
	if (!context->addSceneLight(std::move(light), &diagnostic)) {
		errorout("Grammar execution error: " + diagnostic);
	}
}

void LightFixtureDeclarationToken::performAction(SceneGenerationContext *context)
{
	if (context == nullptr || !declaration_syntax) {
		errorout("Grammar execution error: LightFixture declaration is incomplete.");
		return;
	}
	LightFixtureType fixture_type = LightFixtureType::RecessedDownlight;
	glm::vec3 position(0.0f);
	glm::vec3 direction(0.0f, -1.0f, 0.0f);
	glm::vec3 clearance(0.1f);
	glm::vec2 cone_degrees(24.0f, 36.0f);
	float temperature = 3000.0f;
	float lumens = 700.0f;
	float range = 8.0f;
	bool casts_shadow = false;
	if (!parse_fixture_type(declaration_syntax->fixture_type, &fixture_type) ||
	    !evaluate_spatial_vector(this, declaration_syntax->position, "fixture position", &position) ||
	    !evaluate_spatial_vector(this, declaration_syntax->direction, "fixture direction", &direction) ||
	    !evaluate_spatial_vector(this, declaration_syntax->clearance, "fixture clearance", &clearance) ||
	    !evaluate_lighting_vector2(this, declaration_syntax->cone, "fixture cone", &cone_degrees) ||
	    !evaluate_spatial_expression(this, declaration_syntax->temperature, "fixture temperature", &temperature) ||
	    !evaluate_spatial_expression(this, declaration_syntax->lumens, "fixture lumens", &lumens) ||
	    !evaluate_spatial_expression(this, declaration_syntax->range, "fixture range", &range) ||
	    !parse_on_off(declaration_syntax->shadow, &casts_shadow)) {
		errorout("Grammar execution error: LightFixture contains invalid evaluated values.");
		return;
	}
	std::string diagnostic;
	if (!publish_light_fixture(
			context,
			declaration_syntax->fixture_id,
			declaration_syntax->emitter_id,
			fixture_type,
			position,
			direction,
			clearance,
			cone_degrees,
			temperature,
			lumens,
			range,
			declaration_syntax->mount_interface,
			casts_shadow,
			&diagnostic)) {
		errorout("Grammar execution error: " + diagnostic);
	}
}

void LightSwitchDeclarationToken::performAction(SceneGenerationContext *context)
{
	if (context == nullptr || !declaration_syntax) {
		errorout("Grammar execution error: LightSwitch declaration is incomplete.");
		return;
	}
	SwitchType switch_type = SwitchType::SinglePole;
	glm::vec3 position(0.0f);
	float height = 1.10f;
	float dimmer = 1.0f;
	bool on = true;
	if (!parse_switch_type(declaration_syntax->switch_type, &switch_type) ||
	    !evaluate_spatial_vector(this, declaration_syntax->position, "switch position", &position) ||
	    !evaluate_spatial_expression(this, declaration_syntax->height, "switch height", &height) ||
	    !evaluate_spatial_expression(this, declaration_syntax->dimmer, "switch dimmer", &dimmer) ||
	    !parse_on_off(declaration_syntax->state, &on)) {
		errorout("Grammar execution error: LightSwitch contains invalid evaluated values.");
		return;
	}
	LightSwitch light_switch(
		ElectricalObjectId(declaration_syntax->switch_id),
		declaration_syntax->switch_id,
		switch_type);
	light_switch.setMountObjectId(declaration_syntax->mount_object_id);
	light_switch.setMountHeight(height);
	position.y += height;
	light_switch.setTransform(glm::translate(
		glm::mat4(1.0f), resolve_lighting_scope_position(context, position)));
	light_switch.state().on = on;
	light_switch.state().dimmer = dimmer;
	std::string diagnostic;
	if (!context->addLightSwitch(std::move(light_switch), &diagnostic)) {
		errorout("Grammar execution error: " + diagnostic);
	}
}

void LightingArrayDeclarationToken::performAction(SceneGenerationContext *context)
{
	if (context == nullptr || !declaration_syntax) {
		errorout("Grammar execution error: LightingArray declaration is incomplete.");
		return;
	}
	LightFixtureType fixture_type = LightFixtureType::RecessedDownlight;
	glm::vec3 origin(0.0f);
	glm::vec3 direction(0.0f, -1.0f, 0.0f);
	glm::vec3 clearance(0.1f);
	glm::vec2 grid_value(1.0f);
	glm::vec2 spacing(1.0f);
	glm::vec2 cone_degrees(24.0f, 36.0f);
	float temperature = 3000.0f;
	float lumens = 700.0f;
	float range = 8.0f;
	bool casts_shadow = false;
	if (!parse_fixture_type(declaration_syntax->fixture_type, &fixture_type) ||
	    !evaluate_spatial_vector(this, declaration_syntax->origin, "lighting array origin", &origin) ||
	    !evaluate_spatial_vector(this, declaration_syntax->direction, "lighting array direction", &direction) ||
	    !evaluate_spatial_vector(this, declaration_syntax->clearance, "lighting array clearance", &clearance) ||
	    !evaluate_lighting_vector2(this, declaration_syntax->grid, "lighting array grid", &grid_value) ||
	    !evaluate_lighting_vector2(this, declaration_syntax->spacing, "lighting array spacing", &spacing) ||
	    !evaluate_lighting_vector2(this, declaration_syntax->cone, "lighting array cone", &cone_degrees) ||
	    !evaluate_spatial_expression(this, declaration_syntax->temperature, "lighting array temperature", &temperature) ||
	    !evaluate_spatial_expression(this, declaration_syntax->lumens, "lighting array lumens", &lumens) ||
	    !evaluate_spatial_expression(this, declaration_syntax->range, "lighting array range", &range) ||
	    !parse_on_off(declaration_syntax->shadow, &casts_shadow)) {
		errorout("Grammar execution error: LightingArray contains invalid evaluated values.");
		return;
	}
	const int columns = static_cast<int>(std::lround(grid_value.x));
	const int rows = static_cast<int>(std::lround(grid_value.y));
	if (columns <= 0 || rows <= 0 ||
	    std::fabs(grid_value.x - static_cast<float>(columns)) > 0.0001f ||
	    std::fabs(grid_value.y - static_cast<float>(rows)) > 0.0001f ||
	    static_cast<std::size_t>(columns) * static_cast<std::size_t>(rows) > 4096u) {
		errorout("Grammar execution error: LightingArray grid must contain positive integer dimensions with at most 4096 fixtures.");
		return;
	}
	std::vector<ElectricalObjectId> fixture_ids;
	fixture_ids.reserve(static_cast<std::size_t>(columns) * static_cast<std::size_t>(rows));
	for (int row = 0; row < rows; ++row) {
		for (int column = 0; column < columns; ++column) {
			const std::string suffix = "_" + std::to_string(row + 1) + "_" + std::to_string(column + 1);
			const std::string fixture_id = declaration_syntax->array_id + suffix;
			const std::string emitter_id = fixture_id + "Emitter";
			const glm::vec3 position = origin + glm::vec3(
				static_cast<float>(column) * spacing.x,
				0.0f,
				static_cast<float>(row) * spacing.y);
			std::string diagnostic;
			if (!publish_light_fixture(
					context, fixture_id, emitter_id, fixture_type, position, direction,
					clearance, cone_degrees, temperature, lumens, range,
					declaration_syntax->mount_interface, casts_shadow, &diagnostic)) {
				errorout("Grammar execution error: " + diagnostic);
				return;
			}
			fixture_ids.push_back(ElectricalObjectId(fixture_id));
		}
	}
	if (!declaration_syntax->circuit_id.empty()) {
		LightingCircuit circuit(
			LightingCircuitId(declaration_syntax->circuit_id),
			declaration_syntax->circuit_id);
		circuit.fixtureIds() = std::move(fixture_ids);
		for (const std::string &control_id : declaration_syntax->control_ids) {
			circuit.controlIds().push_back(ElectricalObjectId(control_id));
		}
		circuit.setAlwaysOn(circuit.controlIds().empty());
		std::string diagnostic;
		if (!context->addLightingCircuit(std::move(circuit), &diagnostic)) {
			errorout("Grammar execution error: " + diagnostic);
		}
	}
}

void LightingCircuitDeclarationToken::performAction(SceneGenerationContext *context)
{
	if (context == nullptr || !declaration_syntax) {
		errorout("Grammar execution error: LightingCircuit declaration is incomplete.");
		return;
	}
	bool always_on = false;
	if (!parse_on_off(declaration_syntax->always_on, &always_on)) {
		errorout("Grammar execution error: LightingCircuit alwaysOn must be on or off.");
		return;
	}
	LightingCircuit circuit(
		LightingCircuitId(declaration_syntax->circuit_id), declaration_syntax->circuit_id);
	for (const std::string &control_id : declaration_syntax->control_ids) {
		circuit.controlIds().push_back(ElectricalObjectId(control_id));
	}
	for (const std::string &fixture_id : declaration_syntax->fixture_ids) {
		circuit.fixtureIds().push_back(ElectricalObjectId(fixture_id));
	}
	circuit.setAlwaysOn(always_on);
	std::string diagnostic;
	if (!context->addLightingCircuit(std::move(circuit), &diagnostic)) {
		errorout("Grammar execution error: " + diagnostic);
	}
}

void ControlsDeclarationToken::performAction(SceneGenerationContext *context)
{
	if (context == nullptr || !declaration_syntax) {
		errorout("Grammar execution error: Controls declaration is incomplete.");
		return;
	}
	LightingCircuit circuit(
		LightingCircuitId(declaration_syntax->control_id + "Controls"),
		declaration_syntax->control_id + " Controls");
	circuit.controlIds().push_back(ElectricalObjectId(declaration_syntax->control_id));
	std::string diagnostic;
	for (const std::string &light_id : declaration_syntax->light_ids) {
		const std::string fixture_id = light_id + "ControlFixture";
		LightFixtureObject fixture(
			ElectricalObjectId(fixture_id), fixture_id, LightFixtureType::AreaPanel);
		LightEmitterDefinition emitter;
		emitter.scene_light_id = LightId(light_id);
		fixture.emitters().push_back(emitter);
		if (!context->addLightFixture(std::move(fixture), &diagnostic)) {
			errorout("Grammar execution error: " + diagnostic);
			return;
		}
		circuit.fixtureIds().push_back(ElectricalObjectId(fixture_id));
	}
	if (!context->addLightingCircuit(std::move(circuit), &diagnostic)) {
		errorout("Grammar execution error: " + diagnostic);
	}
}



void GrammarActionToken::performAction(SceneGenerationContext *context){
	if(context == NULL){
		errorout("Grammar execution error: attempted to perform an action without a scene context.");
		return;
	}
	
	if(token_name=="R"){
		errorout("Grammar execution error: a random declaration reached the scene VM instead of the expansion VM.");
		return;
	}
	else if(token_name=="S" ){
		
		
		Scope *newScope = context->getCurrentScope();
		glm::vec3 size = resolve_vec3_arguments(this);
		
		//////std::cout<< << "S (" << size.x << ", " << size.y << ", " << size.z << ") ";
		
		//size *= newScope->getSize();
		//size = glm::vec3(abs(size.x), abs(size.y), abs(size.z));
		
		//////std::cout<< << "S* (" << size.x << ", " << size.y << ", " << size.z << ") "<<std::endl;
		newScope->S(size);
		
	}
	else if(token_name=="D" ){
		
		
		Scope *newScope = context->getCurrentScope();
		glm::vec3 size = resolve_vec3_arguments(this);
		
		//////std::cout<< << "S (" << size.x << ", " << size.y << ", " << size.z << ") ";
		
		//size *= newScope->getSize();
		//size = glm::vec3(abs(size.x), abs(size.y), abs(size.z));
		
		//////std::cout<< << "S* (" << size.x << ", " << size.y << ", " << size.z << ") "<<std::endl;
		newScope->D(size);
		//glm::vec3 pos = context->getCurrentScope()->getPosition();
    //glm::vec3 contextSize = context->getCurrentScope()->getSize();
    //////std::cout<< << " -- Current scope -> POS: (" << pos.x << ", " << pos.y << ", " << pos.z << ") SIZE: (" << contextSize.x << ", " << contextSize.y << ", " << contextSize.z << ") " << std::endl;
		
	}
	else if(token_name=="DSX" || token_name=="DSY" || token_name=="DSZ"){
		Scope *newScope = context->getCurrentScope();
		glm::vec3 size = resolve_vec3_arguments(this);
		const int axis = token_name=="DSX" ? 0 : token_name=="DSY" ? 1 : 2;
		newScope->DS(axis, size);
	}
	else if(token_name=="DTX" || token_name=="DTY" || token_name=="DTZ"){
		Scope *newScope = context->getCurrentScope();
		glm::vec3 position = resolve_vec3_arguments(this);
		const int axis = token_name=="DTX" ? 0 : token_name=="DTY" ? 1 : 2;
		newScope->DT(axis, position);
	}
	else if(token_name=="T"){
		Scope *newScope = context->getCurrentScope();
		glm::vec3 position = resolve_vec3_arguments(this);
		newScope->T(position);
		
	}
	else if(token_name=="A" ){
		if(arguments.size() != 2 && arguments.size() != 4){
			errorout("Grammar execution error: A() requires 2 or 4 resolved arguments.");
			return;
		}
		
		Scope *newScope = context->getCurrentScope();
		
		//////std::cout<<<<"***001";
		float angle,axis;
		
		if(var_names[0]!=""){
				
				angle=MathF2(var_names[0]);
				
				
		}
		else angle=arguments[0];
		
		if(var_names[1]!=""){
				axis=MathF2(var_names[1]);
		}
		else axis=arguments[1];
		
		
		
		//////std::cout<< << "S (" << size.x << ", " << size.y << ", " << size.z << ") ";
		
		//size *= newScope->getSize();
		//size = glm::vec3(abs(size.x), abs(size.y), abs(size.z));
		
		//////std::cout<< << "S* (" << size.x << ", " << size.y << ", " << size.z << ") "<<std::endl;
		int int_axis=axis;
		if(int_axis==0){
		newScope->Rx(angle);	
		}
		else if(int_axis==1){
		newScope->Ry(angle);	
		}
		else {
			newScope->Rz(angle);	
		}
		
		
		//glm::vec3 pos = context->getCurrentScope()->getPosition();
    //glm::vec3 contextSize = context->getCurrentScope()->getSize();
    //////std::cout<< << " -- Current scope -> POS: (" << pos.x << ", " << pos.y << ", " << pos.z << ") SIZE: (" << contextSize.x << ", " << contextSize.y << ", " << contextSize.z << ") " << std::endl;
		
	}
	else if(token_name=="I"){
		if(context->primitive_instances.size() >= kMaxGeneratedPrimitiveCount){
			errorout("Grammar execution error: primitive budget of " +
			         std::to_string(kMaxGeneratedPrimitiveCount) +
			         " was exceeded.");
			context->clearPendingPrimitiveState();
			return;
		}

	    glm::mat4 transform = context->getCurrentScope()->getTransform();
		std::string resolved_material_name=this->material_name;
		std::size_t argument_offset=0;
		
		if(resolved_material_name.empty()){
			int legacy_material_index=0;
			if(arguments.size()>0 && var_names[0]!=""){
				legacy_material_index=static_cast<int>(MathF2(var_names[0]));
			}
			else if(arguments.size()>0){
				legacy_material_index=(int)arguments[0];
			}
			resolved_material_name=legacy_material_name_for_index(legacy_material_index);
			argument_offset=1;
		}
		float arg=0.0;
		float val=0.125f;
		if(arguments.size()>argument_offset)arg=arguments[argument_offset];
		if(arguments.size()>argument_offset+1)val=arguments[argument_offset+1];
		const std::size_t primitive_count_before = context->primitive_instances.size();
		context->addPrimitive(instance_type,
		                     context->getCurrentScope(),
		                     resolved_material_name,
		                     arg,
		                     val,
		                     immovable,
		                     source_start_line,
		                     source_start_column,
		                     source_end_line,
		                     source_end_column,
		                     shape_specification);
		if(context->primitive_instances.size() > primitive_count_before){
			const PrimitiveInstance &stored_instance = context->primitive_instances.back();
			Mesh instance;
			if(stored_instance.resolved_geometry &&
			   stored_instance.resolved_geometry->hasTriangleMesh()){
				instance = stored_instance.resolved_geometry->triangleMesh();
			}
			else{
				instance = Mesh::getInstance(instance_type);
			}
			instance.apply(transform);
			context->getScene().add(instance);
		}
	}
	else if(token_name=="V"){
		glm::vec3 velocity(0.0f);
		for(int i=0;i<3;i++){
			if(var_names[i]!=""){
				velocity[i]=MathF2(var_names[i]);
			}
			else if(arguments.size()>static_cast<std::size_t>(i)){
				velocity[i]=arguments[i];
			}
		}
		context->setPendingVelocity(velocity);
	}
	else if(token_name=="VR"){
		glm::vec3 rotational_velocity(0.0f);
		for(int i=0;i<3;i++){
			if(var_names[i]!=""){
				rotational_velocity[i]=MathF2(var_names[i]);
			}
			else if(arguments.size()>static_cast<std::size_t>(i)){
				rotational_velocity[i]=arguments[i];
			}
		}
		context->setPendingRotationalVelocity(rotational_velocity);
	}
	else if(token_name=="M"){
		float mass=1.0f;
		if(var_names[0]!=""){
			mass=MathF2(var_names[0]);
		}
		else if(!arguments.empty()){
			mass=arguments[0];
		}
		context->setPendingMass(mass);
	}
	else if(token_name=="P"){
		float density=0.0f;
		if(var_names[0]!=""){
			density=MathF2(var_names[0]);
		}
		else if(!arguments.empty()){
			density=arguments[0];
		}
		context->setPendingDensity(density);
	}
	else if(token_name=="G"){
		float magnitude=0.0f;
		if(var_name!=""){
			magnitude=MathF2(var_name);
		}
		else if(!arguments.empty()){
			magnitude=arguments[0];
		}

		glm::vec3 direction(0.0f);
		for(int i=0;i<3;i++){
			if(var_names[i]!=""){
				direction[i]=MathF2(var_names[i]);
			}
			else if(arguments.size()>static_cast<std::size_t>(i+1)){
				direction[i]=arguments[static_cast<std::size_t>(i+1)];
			}
		}
		context->setGravity(magnitude, direction);
	}
	else if(token_name=="["){
		 context->pushScope();
		
	}
	else if(token_name=="]"){
		 context->popScope();
		
	}
	else if(token_name=="{"){
		 context->newScope();
		
	}
	else if(token_name=="}"){
		 context->popScope();
		
	}
	
	
	
}








//////////////////////////////////Read Grammar Script from Grammar file/////////////////////////////////////


void GrammarDocument::ReadTokens2(Rule *rule,std::string rule_str,int index_k){
	// Legacy entry point now delegates to the side-effect-free parser.
	ReadTokens(rule, std::move(rule_str), index_k);
}

static void parse_tokens_into(Grammar *grammar,
                              Rule *rule,
                              const std::vector<std::string> &raw_tokens,
                              const std::vector<SourceTokenSpan> *raw_token_spans,
                              std::size_t *index,
                              int section,
                              std::vector<Token *> *target,
                              const std::string &stop_token)
{
	float value = 0.0f;
	const auto append_owned_token = [&](std::unique_ptr<Token> token) {
		if (token != nullptr && grammar != nullptr) token->source_name = grammar->input_name;
		append_rule_token(rule, section, target, token.release());
	};
	const auto source_range_resolver = [&](std::size_t start_index,
	                                      std::size_t end_index) {
		return resolve_grammar_source_range(raw_token_spans, start_index, end_index);
	};
	while(index != NULL && *index < raw_tokens.size()){
		const std::size_t token_start_index = *index;
		std::string token_str = raw_tokens[*index];
		if(!stop_token.empty() && token_str == stop_token){
			return;
		}

		if(token_str=="["){
			Token *scope_token = new Token("[");
			assign_token_source_range(scope_token, raw_token_spans, token_start_index, token_start_index + 1);
			append_rule_token(rule, section, target, scope_token);
			++(*index);
			parse_tokens_into(grammar, rule, raw_tokens, raw_token_spans, index, section, target, "]");
			if(*index >= raw_tokens.size() || raw_tokens[*index] != "]"){
				throw(1);
			}
			Token *close_scope_token = new Token("]");
			assign_token_source_range(close_scope_token, raw_token_spans, *index, *index + 1);
			append_rule_token(rule, section, target, close_scope_token);
			++(*index);
			continue;
		}

		if(token_str=="?"){
			std::unique_ptr<GrammarConditionalActionToken> token(new GrammarConditionalActionToken());
			++(*index);
			token->condition_expression = parse_parenthesized_expression(raw_tokens, index);
			parse_tokens_into(grammar, rule, raw_tokens, raw_token_spans, index, section, &token->conditional_true_tokens, ":");
			if(*index >= raw_tokens.size() || raw_tokens[*index] != ":"){
				throw(1);
			}
			++(*index);
			parse_tokens_into(grammar, rule, raw_tokens, raw_token_spans, index, section, &token->conditional_false_tokens, stop_token);
			assign_token_source_range(token.get(), raw_token_spans, token_start_index, *index);
			append_owned_token(std::move(token));
			continue;
		}

		++(*index);
		std::unique_ptr<Token> token;

		if(token_str=="Object"){
			std::unique_ptr<SpatialObjectScopeToken> object_token(
				new SpatialObjectScopeToken());
			SpatialObjectSpecificationParser object_parser;
			std::string diagnostic;
			object_token->descriptor_syntax = object_parser.parse(
				raw_tokens,
				index,
				token_start_index,
				source_range_resolver,
				&diagnostic);
			if(!object_token->descriptor_syntax){
				assign_token_source_range(
					object_token.get(), raw_token_spans, token_start_index, *index);
				report_grammar_issue(
					grammar,
					make_action_token_diagnostic(object_token.get(), diagnostic));
				throw(1);
			}
			if(*index >= raw_tokens.size() || raw_tokens[*index] != "["){
				assign_token_source_range(
					object_token.get(), raw_token_spans, token_start_index, *index);
				report_grammar_issue(
					grammar,
					make_action_token_diagnostic(
						object_token.get(),
						"Object(...) must own a bracketed body."));
				throw(1);
			}
			++(*index);
			parse_tokens_into(
				grammar,
				rule,
				raw_tokens,
				raw_token_spans,
				index,
				section,
				&object_token->body_tokens,
				"]");
			if(*index >= raw_tokens.size() || raw_tokens[*index] != "]"){
				assign_token_source_range(
					object_token.get(), raw_token_spans, token_start_index, *index);
				report_grammar_issue(
					grammar,
					make_action_token_diagnostic(
						object_token.get(),
						"Object(...) body is missing its closing bracket."));
				throw(1);
			}
			++(*index);
			assign_token_source_range(
				object_token.get(), raw_token_spans, token_start_index, *index);
			append_owned_token(std::move(object_token));
			continue;
		}
		else if(token_str=="Interface"){
			std::unique_ptr<SpatialInterfaceDeclarationToken> interface_token(
				new SpatialInterfaceDeclarationToken());
			SpatialInterfaceDeclarationParser interface_parser;
			std::string diagnostic;
			interface_token->declaration_syntax = interface_parser.parse(
				raw_tokens,
				index,
				token_start_index,
				parse_expression_argument,
				source_range_resolver,
				&diagnostic);
			if(!interface_token->declaration_syntax){
				assign_token_source_range(
					interface_token.get(), raw_token_spans, token_start_index, *index);
				report_grammar_issue(
					grammar,
					make_action_token_diagnostic(interface_token.get(), diagnostic));
				throw(1);
			}
			assign_token_source_range(
				interface_token.get(), raw_token_spans, token_start_index, *index);
			append_owned_token(std::move(interface_token));
			continue;
		}
		else if(token_str=="Connect"){
			std::unique_ptr<SpatialConnectionDeclarationToken> connection_token(
				new SpatialConnectionDeclarationToken());
			SpatialConnectionDeclarationParser connection_parser;
			std::string diagnostic;
			connection_token->declaration_syntax = connection_parser.parse(
				raw_tokens,
				index,
				token_start_index,
				parse_expression_argument,
				source_range_resolver,
				&diagnostic);
			if(!connection_token->declaration_syntax){
				assign_token_source_range(
					connection_token.get(), raw_token_spans, token_start_index, *index);
				report_grammar_issue(
					grammar,
					make_action_token_diagnostic(connection_token.get(), diagnostic));
				throw(1);
			}
			assign_token_source_range(
				connection_token.get(), raw_token_spans, token_start_index, *index);
			append_owned_token(std::move(connection_token));
			continue;
		}
		else if(token_str=="Joint"){
			std::unique_ptr<VehicleJointDeclarationToken> joint_token(
				new VehicleJointDeclarationToken());
			VehicleJointDeclarationParser joint_parser;
			std::string diagnostic;
			joint_token->declaration_syntax = joint_parser.parse(
				raw_tokens,
				index,
				token_start_index,
				parse_expression_argument,
				source_range_resolver,
				&diagnostic);
			if(!joint_token->declaration_syntax){
				assign_token_source_range(
					joint_token.get(), raw_token_spans, token_start_index, *index);
				report_grammar_issue(
					grammar,
					make_action_token_diagnostic(joint_token.get(), diagnostic));
				throw(1);
			}
			assign_token_source_range(
				joint_token.get(), raw_token_spans, token_start_index, *index);
			append_owned_token(std::move(joint_token));
			continue;
		}
		else if(token_str=="Position"){
			std::unique_ptr<SpatialConstraintDeclarationToken> constraint_token(
				new SpatialConstraintDeclarationToken());
			SpatialConstraintDeclarationParser constraint_parser;
			std::string diagnostic;
			constraint_token->declaration_syntax = constraint_parser.parse(
				raw_tokens,
				index,
				token_start_index,
				parse_expression_argument,
				source_range_resolver,
				&diagnostic);
			if(!constraint_token->declaration_syntax){
				assign_token_source_range(
					constraint_token.get(), raw_token_spans, token_start_index, *index);
				report_grammar_issue(
					grammar,
					make_action_token_diagnostic(constraint_token.get(), diagnostic));
				throw(1);
			}
			assign_token_source_range(
				constraint_token.get(), raw_token_spans, token_start_index, *index);
			append_owned_token(std::move(constraint_token));
			continue;
		}
		else if(token_str=="Light"){
			std::unique_ptr<SceneLightDeclarationToken> light_token(
				new SceneLightDeclarationToken());
			LightingDeclarationParser parser;
			std::string diagnostic;
			light_token->declaration_syntax = parser.parseLight(
				raw_tokens, index, token_start_index, parse_expression_argument,
				source_range_resolver, &diagnostic);
			if(!light_token->declaration_syntax){
				assign_token_source_range(light_token.get(), raw_token_spans, token_start_index, *index);
				report_grammar_issue(grammar, make_action_token_diagnostic(light_token.get(), diagnostic));
				throw(1);
			}
			assign_token_source_range(light_token.get(), raw_token_spans, token_start_index, *index);
			append_owned_token(std::move(light_token));
			continue;
		}
		else if(token_str=="LightFixture"){
			std::unique_ptr<LightFixtureDeclarationToken> fixture_token(
				new LightFixtureDeclarationToken());
			LightingDeclarationParser parser;
			std::string diagnostic;
			fixture_token->declaration_syntax = parser.parseFixture(
				raw_tokens, index, token_start_index, parse_expression_argument,
				source_range_resolver, &diagnostic);
			if(!fixture_token->declaration_syntax){
				assign_token_source_range(fixture_token.get(), raw_token_spans, token_start_index, *index);
				report_grammar_issue(grammar, make_action_token_diagnostic(fixture_token.get(), diagnostic));
				throw(1);
			}
			assign_token_source_range(fixture_token.get(), raw_token_spans, token_start_index, *index);
			append_owned_token(std::move(fixture_token));
			continue;
		}
		else if(token_str=="LightSwitch"){
			std::unique_ptr<LightSwitchDeclarationToken> switch_token(
				new LightSwitchDeclarationToken());
			LightingDeclarationParser parser;
			std::string diagnostic;
			switch_token->declaration_syntax = parser.parseSwitch(
				raw_tokens, index, token_start_index, parse_expression_argument,
				source_range_resolver, &diagnostic);
			if(!switch_token->declaration_syntax){
				assign_token_source_range(switch_token.get(), raw_token_spans, token_start_index, *index);
				report_grammar_issue(grammar, make_action_token_diagnostic(switch_token.get(), diagnostic));
				throw(1);
			}
			assign_token_source_range(switch_token.get(), raw_token_spans, token_start_index, *index);
			append_owned_token(std::move(switch_token));
			continue;
		}
		else if(token_str=="LightingArray"){
			std::unique_ptr<LightingArrayDeclarationToken> array_token(
				new LightingArrayDeclarationToken());
			LightingDeclarationParser parser;
			std::string diagnostic;
			array_token->declaration_syntax = parser.parseArray(
				raw_tokens, index, token_start_index, parse_expression_argument,
				source_range_resolver, &diagnostic);
			if(!array_token->declaration_syntax){
				assign_token_source_range(array_token.get(), raw_token_spans, token_start_index, *index);
				report_grammar_issue(grammar, make_action_token_diagnostic(array_token.get(), diagnostic));
				throw(1);
			}
			assign_token_source_range(array_token.get(), raw_token_spans, token_start_index, *index);
			append_owned_token(std::move(array_token));
			continue;
		}
		else if(token_str=="LightingCircuit"){
			std::unique_ptr<LightingCircuitDeclarationToken> circuit_token(
				new LightingCircuitDeclarationToken());
			LightingDeclarationParser parser;
			std::string diagnostic;
			circuit_token->declaration_syntax = parser.parseCircuit(
				raw_tokens, index, token_start_index, source_range_resolver, &diagnostic);
			if(!circuit_token->declaration_syntax){
				assign_token_source_range(circuit_token.get(), raw_token_spans, token_start_index, *index);
				report_grammar_issue(grammar, make_action_token_diagnostic(circuit_token.get(), diagnostic));
				throw(1);
			}
			assign_token_source_range(circuit_token.get(), raw_token_spans, token_start_index, *index);
			append_owned_token(std::move(circuit_token));
			continue;
		}
		else if(token_str=="Controls"){
			std::unique_ptr<ControlsDeclarationToken> controls_token(
				new ControlsDeclarationToken());
			LightingDeclarationParser parser;
			std::string diagnostic;
			controls_token->declaration_syntax = parser.parseControls(
				raw_tokens, index, token_start_index, source_range_resolver, &diagnostic);
			if(!controls_token->declaration_syntax){
				assign_token_source_range(controls_token.get(), raw_token_spans, token_start_index, *index);
				report_grammar_issue(grammar, make_action_token_diagnostic(controls_token.get(), diagnostic));
				throw(1);
			}
			assign_token_source_range(controls_token.get(), raw_token_spans, token_start_index, *index);
			append_owned_token(std::move(controls_token));
			continue;
		}

		if(token_str=="S" || token_str=="T" || token_str=="D" ||
		   token_str=="DSX" || token_str=="DSY" || token_str=="DSZ" ||
		   token_str=="DTX" || token_str=="DTY" || token_str=="DTZ" ||
		   token_str=="V" || token_str=="VR" ){
			token=std::make_unique<Token>(token_str);
			if(*index >= raw_tokens.size() || raw_tokens[*index] != "("){
				throw(1);
			}
			++(*index);
			for(int i=0;i<3;i++){
				const std::string argument_expression =
					parse_expression_argument(raw_tokens, index, ")");
				if(!isnumber(argument_expression)){
					token->var_names[i]=argument_expression;
					value=0.0f;
				}
				else {
					token->var_names[i]="";
					value=atof(argument_expression.c_str());
				}
				token->addArgument(value);
			}
			if(*index >= raw_tokens.size() || raw_tokens[*index] != ")"){
				throw(1);
			}
			++(*index);
			assign_token_source_range(token.get(), raw_token_spans, token_start_index, *index);
			append_owned_token(std::move(token));
			continue;
		}
		else if(token_str=="G"){
			token=std::make_unique<Token>(token_str);
			if(*index >= raw_tokens.size() || raw_tokens[*index] != "("){
				throw(1);
			}
			++(*index);

			const std::string magnitude_expression =
				parse_expression_argument(raw_tokens, index, ")");
			if(!isnumber(magnitude_expression)){
				token->var_name=magnitude_expression;
				value=0.0f;
			}
			else{
				token->var_name="";
				value=atof(magnitude_expression.c_str());
			}
			token->addArgument(value);

			for(int i=0;i<3;i++){
				const std::string argument_expression =
					parse_expression_argument(raw_tokens, index, ")");
				if(!isnumber(argument_expression)){
					token->var_names[i]=argument_expression;
					value=0.0f;
				}
				else{
					token->var_names[i]="";
					value=atof(argument_expression.c_str());
				}
				token->addArgument(value);
			}

			if(*index >= raw_tokens.size() || raw_tokens[*index] != ")"){
				throw(1);
			}
			++(*index);
			assign_token_source_range(token.get(), raw_token_spans, token_start_index, *index);
			append_owned_token(std::move(token));
			continue;
		}
		else if(token_str=="M" || token_str=="P"){
			token=std::make_unique<Token>(token_str);
			if(*index >= raw_tokens.size() || raw_tokens[*index] != "("){
				throw(1);
			}
			++(*index);
			const std::string argument_expression =
				parse_expression_argument(raw_tokens, index, ")");
			if(!isnumber(argument_expression)){
				token->var_names[0]=argument_expression;
				value=0.0f;
			}
			else {
				token->var_names[0]="";
				value=atof(argument_expression.c_str());
			}
			token->addArgument(value);
			if(*index >= raw_tokens.size() || raw_tokens[*index] != ")"){
				throw(1);
			}
			++(*index);
			assign_token_source_range(token.get(), raw_token_spans, token_start_index, *index);
			append_owned_token(std::move(token));
			continue;
		}
		else if(token_str=="R" || token_str=="R*"){
			if(token_str=="R*"){
				token=std::make_unique<Token>("R");
				token->integer=true;
			}
			else {
				token=std::make_unique<Token>(token_str);
			}

			if(*index >= raw_tokens.size())throw(1);
			token->var_name=raw_tokens[*index];
			if(!is_identifier_token_string(token->var_name))throw(1);
			++(*index);

			if(*index >= raw_tokens.size() || raw_tokens[*index] != "("){
				throw(1);
			}
			++(*index);
			for(int i=0;i<2;i++){
				const std::string argument_expression =
					parse_expression_argument(raw_tokens, index, ")");
				token->var_names[i]=argument_expression;
				token->addArgument(0.0f);
			}
			if(*index >= raw_tokens.size() || raw_tokens[*index] != ")"){
				throw(1);
			}
			++(*index);

			assign_token_source_range(token.get(), raw_token_spans, token_start_index, *index);
			append_owned_token(std::move(token));
			continue;
		}
		else if(token_str=="I" || token_str=="!I"){
			token=std::make_unique<Token>(token_str);
			if(token_str=="!I"){
				token->token_name="I";
				token->immovable=true;
			}
			if(*index >= raw_tokens.size() || raw_tokens[*index] != "("){
				throw(1);
			}
			++(*index);
			if(*index >= raw_tokens.size())throw(1);
			token_str = raw_tokens[*index];
			++(*index);
			if(is_supported_instance_type_name(token_str)){
				token->addInstanceType(token_str);
			}
			else {
				throw(1);
			}
			ShapeSpecificationParser shape_parser;
			if(ShapeSpecificationParser::isSupportedShapeName(token_str)){
				std::string shape_diagnostic;
				if(*index < raw_tokens.size() && raw_tokens[*index] == "("){
					token->shape_descriptor_syntax =
						shape_parser.parseNestedDescriptor(
							token_str,
							raw_tokens,
							index,
							parse_expression_argument,
							&shape_diagnostic);
				}
				else{
					token->shape_descriptor_syntax =
						shape_parser.createBareDescriptor(token_str, &shape_diagnostic);
				}
				if(!token->shape_descriptor_syntax){
					assign_token_source_range(token.get(), raw_token_spans,
					                          token_start_index, *index);
					if(!shape_diagnostic.empty()){
						report_grammar_issue(grammar,
						                     make_action_token_diagnostic(
							                     token.get(), shape_diagnostic));
					}
					throw(1);
				}
			}
			else if(*index < raw_tokens.size() && raw_tokens[*index] == "("){
				assign_token_source_range(token.get(), raw_token_spans,
				                          token_start_index, *index + 1);
				report_grammar_issue(
					grammar,
					make_action_token_diagnostic(
						token.get(),
						"Nested shape options are supported only for curved shape families."));
				throw(1);
			}

			std::vector<std::string> raw_arguments;
			while(*index < raw_tokens.size() && raw_tokens[*index] != ")"){
				raw_arguments.push_back(parse_expression_argument(raw_tokens, index, ")"));
			}
			if(*index >= raw_tokens.size() || raw_tokens[*index] != ")"){
				throw(1);
			}
			++(*index);

			std::string appearance_diagnostic;
			if(!parse_instance_appearance_arguments(
					token.get(), raw_arguments, &appearance_diagnostic)){
				assign_token_source_range(token.get(), raw_token_spans,
				                          token_start_index, *index);
				if(!appearance_diagnostic.empty()){
					report_grammar_issue(
						grammar,
						make_action_token_diagnostic(
							token.get(), appearance_diagnostic));
				}
				throw(1);
			}

			assign_token_source_range(token.get(), raw_token_spans, token_start_index, *index);
			append_owned_token(std::move(token));
			continue;
		}
		else if(token_str=="*"){
			continue;
		}
		else if(token_str=="A"){
			token=std::make_unique<Token>(token_str);
			if(*index >= raw_tokens.size() || raw_tokens[*index] != "("){
				throw(1);
			}
			++(*index);
			std::vector<std::string> rotation_arguments;
			while(*index < raw_tokens.size() && raw_tokens[*index] != ")"){
				rotation_arguments.push_back(
					parse_expression_argument(raw_tokens, index, ")"));
			}
			if(*index >= raw_tokens.size() || raw_tokens[*index] != ")"){
				throw(1);
			}
			++(*index);
			if(rotation_arguments.size() != 2 && rotation_arguments.size() != 4){
				throw(1);
			}
			for(std::size_t argument_index = 0;
			    argument_index < rotation_arguments.size();
			    ++argument_index){
				const std::string expression = removeSpaces(rotation_arguments[argument_index]);
				double literal_value = 0.0;
				const bool literal = parse_finite_number(expression, &literal_value);
				value = literal ? static_cast<float>(literal_value) : 0.0f;
				if(!literal){
					if(argument_index < token->var_names.size()){
						token->var_names[argument_index] = expression;
					}
					else{
						token->var_name = expression;
					}
				}
				token->addArgument(value);
			}
			assign_token_source_range(token.get(), raw_token_spans, token_start_index, *index);
			append_owned_token(std::move(token));
			continue;
		}
		else if(token_str=="]"){
			Token *close_token = new Token(token_str);
			assign_token_source_range(close_token, raw_token_spans, token_start_index, token_start_index + 1);
			append_rule_token(rule, section, target, close_token);
			continue;
		}
		else if(token_str=="{" || token_str=="}"){
			continue;
		}
		else if(token_str==":"){
			throw(1);
		}
		else {
			token=std::make_unique<Token>(token_str);
			if(*index < raw_tokens.size() && raw_tokens[*index] == "("){
				++(*index);
				while(*index < raw_tokens.size() && raw_tokens[*index] != ")"){
					token->rule_arguments.push_back(
						parse_expression_argument(raw_tokens, index, ")"));
				}
				if(*index >= raw_tokens.size() || raw_tokens[*index] != ")"){
					throw(1);
				}
				++(*index);
			}
			assign_token_source_range(token.get(), raw_token_spans, token_start_index, *index);
			append_owned_token(std::move(token));
			continue;
		}
	}
}

void GrammarDocument::ReadTokens(Rule *rule,std::string rule_str,int index_k,int line_number){
	std::vector<std::string> raw_tokens;
	std::size_t index = 0;
	try{
		raw_tokens = split_rule_tokens(rule_str);
		parse_tokens_into(this, rule, raw_tokens, NULL, &index, index_k, NULL, "");
	}catch(int e_number){
		const std::string rule_name = rule != NULL ? rule->rule_name : "<unknown>";
		const std::size_t anchor_index =
			raw_tokens.empty() ? 0 : std::min(index, raw_tokens.size() - 1);
		std::string message = "Rule '" + rule_name + "' could not be parsed.";
		if(!raw_tokens.empty()){
			message += " Near '" + raw_tokens[anchor_index] + "'.";
		}
		report_grammar_issue(this,
		                     locate_rule_section_diagnostic(this,
		                                                     line_number,
		                                                     rule_str,
		                                                     anchor_index,
		                                                     message),
		                     rule_str);
		errorout("  Parse error code: " + std::to_string(e_number));
		errorout("  Section index: " + std::to_string(index_k));

		try {
			if (!raw_tokens.empty()) {
				std::stringstream ss;
				ss << "  Token excerpt:";
				for (size_t i = 0; i < raw_tokens.size() && i < 10; ++i) {
					ss << " [" << i << "] " << raw_tokens[i];
				}
				if (raw_tokens.size() > 10) {
					ss << " ... +" << (raw_tokens.size() - 10) << " more";
				}
				errorout(ss.str());
			}
		} catch (...) {
			errorout("  Could not build a token excerpt for this rule.");
		}

		if (e_number == 1) {
			errorout("  Hint: check delimiter matching, token argument counts, and unexpected characters.");
			if (rule_str.find('\\') != std::string::npos) {
				errorout("  Hint: unexpected '\\' found in the production.");
			}
		}
	}
}

void GrammarDocument::ReadTokens(Rule *rule,
                         const std::vector<SourceTokenSpan> &token_spans,
                         int index_k,
                         int line_number)
{
	std::vector<std::string> raw_tokens;
	raw_tokens.reserve(token_spans.size());
	for(const SourceTokenSpan &span : token_spans){
		raw_tokens.push_back(span.text);
	}

	std::size_t index = 0;
	const std::string rule_text = join_token_texts(token_spans, 0, token_spans.size());
	try{
		parse_tokens_into(this, rule, raw_tokens, &token_spans, &index, index_k, NULL, "");
	}catch(int e_number){
		const std::string rule_name = rule != NULL ? rule->rule_name : "<unknown>";
		const std::size_t anchor_index =
			raw_tokens.empty() ? 0 : std::min(index, raw_tokens.size() - 1);
		std::string message = "Rule '" + rule_name + "' could not be parsed.";
		if(!raw_tokens.empty()){
			message += " Near '" + raw_tokens[anchor_index] + "'.";
		}
		report_grammar_issue(this,
		                     locate_rule_section_diagnostic(this,
		                                                     line_number,
		                                                     rule_text,
		                                                     anchor_index,
		                                                     message),
		                     rule_text);
		errorout("  Parse error code: " + std::to_string(e_number));
		errorout("  Section index: " + std::to_string(index_k));
	}
}



std::string GrammarDocument::ruleBody(Rule *rule,
                              std::istringstream &lin,
                              std::string line,
                              int line_number,
                              const std::string &header_text,
                              const std::vector<SourceTokenSpan> *body_token_spans){
		std::string token_str;
		const std::vector<std::string> header_tokens = split_rule_tokens(header_text);
		const auto report_header_issue = [&](std::size_t token_index, const std::string &message) {
			report_grammar_issue(this,
			                     locate_rule_section_diagnostic(this,
			                                                     line_number,
			                                                     header_text,
			                                                     token_index,
			                                                     message),
			                     header_text);
		};
		const auto report_body_issue = [&](std::size_t token_index, const std::string &message) {
			report_grammar_issue(this,
			                     locate_rule_section_diagnostic(this,
			                                                     line_number,
			                                                     line,
			                                                     token_index,
			                                                     message),
			                     line);
		};
		if(rule == NULL){
			report_grammar_issue(this, line_number, "Internal parser error: rule body received a null rule.");
			return "";
		}

		if(trim_copy(line).empty()){
			report_body_issue(0, "Rule '" + rule->rule_name + "' has an empty production body.");
			return "";
		}

		std::size_t header_token_index = header_tokens.empty() ? 0 : 1;
		if(!(lin >> token_str)){
			report_header_issue(header_token_index, "Rule header is incomplete.");
			return "";
		}
		std::size_t current_header_index =
			header_token_index < header_tokens.size() ? header_token_index++ : header_tokens.size();

			if(token_str=="("){
				const std::size_t parameter_list_index = current_header_index;
				while(lin >> token_str){
					current_header_index =
						header_token_index < header_tokens.size() ? header_token_index++ : header_tokens.size();
					if(token_str==")")break;
					if(is_identifier_token_string(token_str)){
						if(token_str == rule->rule_name + "_count"){
							report_header_issue(current_header_index,
							                    "Parameter '" + token_str +
							                        "' uses the reserved iteration-counter name.");
						}
						else if(std::find(rule->parameter_names.begin(),
						                  rule->parameter_names.end(),
						                  token_str) != rule->parameter_names.end()){
							report_header_issue(current_header_index,
							                    "Rule '" + rule->rule_name +
							                        "' has duplicate parameter '" + token_str + "'.");
						}
						else{
							rule->parameter_names.push_back(token_str);
						}
					}
					else{
						report_header_issue(current_header_index,
						                    "Rule '" + rule->rule_name + "' has an invalid parameter name '" +
						                        token_str + "'.");
					}
				}
				if(token_str!=")"){
					report_header_issue(parameter_list_index,
					                    "Rule '" + rule->rule_name + "' has an unterminated parameter list.");
					return "";
				}
				if(!(lin >> token_str)){
					report_header_issue(parameter_list_index,
					                    "Rule '" + rule->rule_name + "' is missing a production arrow.");
					return "";
				}
				current_header_index =
					header_token_index < header_tokens.size() ? header_token_index++ : header_tokens.size();
			}
			
			
			if(token_str==";"){
				if(!(lin >> token_str)){
					report_header_issue(current_header_index,
					                    "Rule '" + rule->rule_name + "' has an invalid probability value.");
					return "";
				}
				current_header_index =
					header_token_index < header_tokens.size() ? header_token_index++ : header_tokens.size();
				double probability_value = 0.0;
				if(!parse_finite_number(token_str, &probability_value) ||
				   probability_value < 0.0 || probability_value > 1.0){
					report_header_issue(current_header_index,
					                    "Rule '" + rule->rule_name +
					                        "' probability must be a finite number between 0 and 1.");
					return "";
				}
				rule->probability = static_cast<float>(probability_value);
		}
			else {
			
				if(token_str!="->"){
					if(!isnumber(token_str)){
						rule->var_name=token_str;
						//repeat=1;
					}
					else {
						double repeat_literal = 0.0;
						if(!parse_finite_number(token_str, &repeat_literal) ||
						   std::fabs(repeat_literal) > static_cast<double>(kMaxRuleRepeatCount)){
							report_header_issue(current_header_index,
							                    "Rule '" + rule->rule_name +
							                        "' repeat count must be finite and within +/-" +
							                        std::to_string(kMaxRuleRepeatCount) + ".");
							return "";
						}
						rule->repeat = static_cast<int>(repeat_literal);
					}

					do
					{
						if(!(lin >> token_str)){
							report_header_issue(current_header_index,
							                    "Rule '" + rule->rule_name + "' header ended unexpectedly.");
							return "";
						}
						current_header_index =
							header_token_index < header_tokens.size() ? header_token_index++ : header_tokens.size();
					
						if(token_str==";" || token_str=="->")break;
							
						if(!is_identifier_token_string(token_str)){
							report_header_issue(current_header_index,
							                    "Rule '" + rule->rule_name +
							                        "' has an invalid reroll-variable name '" + token_str + "'.");
							return "";
						}
						if(token_str == rule->rule_name + "_count"){
							report_header_issue(current_header_index,
							                    "Reroll binding '" + token_str +
							                        "' uses the reserved iteration-counter name.");
							return "";
						}
						if(std::find(rule->parameter_names.begin(),
						             rule->parameter_names.end(),
						             token_str) != rule->parameter_names.end()){
							report_header_issue(current_header_index,
							                    "Reroll binding '" + token_str +
							                        "' collides with a parameter of rule '" +
							                        rule->rule_name + "'.");
							return "";
						}
						for(int existing_index = 0; existing_index < rule->var_counter; ++existing_index){
							if(rule->var_names[existing_index] == token_str){
								report_header_issue(current_header_index,
								                    "Rule '" + rule->rule_name +
								                        "' has duplicate reroll binding '" + token_str + "'.");
								return "";
							}
						}
						const std::size_t reroll_capacity =
							sizeof(rule->var_names) / sizeof(rule->var_names[0]);
						if(static_cast<std::size_t>(rule->var_counter) >= reroll_capacity){
							report_header_issue(current_header_index,
							                    "Rule '" + rule->rule_name +
							                        "' exceeds the reroll-variable capacity of " +
							                        std::to_string(reroll_capacity) + ".");
							return "";
						}
						rule->var_names[rule->var_counter]=token_str;
						rule->var_counter++;
					
						//	////std::cout<<<<token_str;
					}
					while(true);
				}

				if(token_str==";"){
					if(!(lin >> token_str)){
						report_header_issue(current_header_index,
						                    "Rule '" + rule->rule_name + "' has an invalid probability value.");
						return "";
					}
					current_header_index =
						header_token_index < header_tokens.size() ? header_token_index++ : header_tokens.size();
					double probability_value = 0.0;
					if(!parse_finite_number(token_str, &probability_value) ||
					   probability_value < 0.0 || probability_value > 1.0){
						report_header_issue(current_header_index,
						                    "Rule '" + rule->rule_name +
						                        "' probability must be a finite number between 0 and 1.");
						return "";
					}
					rule->probability = static_cast<float>(probability_value);
				}
		
			}
		
		
		//Sections
		
		
		
		

		
		
		
		
		std::vector<std::string> sections;
		std::vector<std::vector<SourceTokenSpan>> section_token_groups;
		const std::vector<std::string> body_tokens =
			body_token_spans != NULL
				? [&]() {
					std::vector<std::string> tokens;
					tokens.reserve(body_token_spans->size());
					for(const SourceTokenSpan &span : *body_token_spans){
						tokens.push_back(span.text);
					}
					return tokens;
				}()
				: split_rule_tokens(line);
		std::vector<std::size_t> pipe_token_indices;
		for(std::size_t token_index = 0; token_index < body_tokens.size(); ++token_index){
			if(body_tokens[token_index] == "|"){
				pipe_token_indices.push_back(token_index);
			}
		}

		if(body_token_spans != NULL){
			std::size_t section_start = 0;
			for(std::size_t token_index = 0; token_index <= body_token_spans->size(); ++token_index){
				if(token_index < body_token_spans->size() &&
				   (*body_token_spans)[token_index].text != "|"){
					continue;
				}
				section_token_groups.push_back(
					slice_token_spans(*body_token_spans, section_start, token_index));
				sections.push_back(
					join_token_texts(*body_token_spans, section_start, token_index));
				section_start = token_index + 1;
			}
		}
		else{
			sections = breakup(line,"|");
		}
		
		int num_sections=sections.size();
		
		if(!line.empty() && line[line.length()-1]=='|'){
			num_sections--;//std::cout<<<<"CORRECTION | trailing"<<std::endl;
		}

		for(int section_index = 0; section_index < num_sections; ++section_index){
			if(trim_copy(sections[static_cast<std::size_t>(section_index)]).empty()){
				const std::size_t pipe_index =
					pipe_token_indices.empty()
						? 0
						: pipe_token_indices[std::min<std::size_t>(static_cast<std::size_t>(section_index),
						                                          pipe_token_indices.size() - 1)];
				report_body_issue(pipe_index,
				                  "Rule '" + rule->rule_name + "' has an empty section around '|'.");
			}
		}
		 
		 if(num_sections>3){
			 const std::size_t pipe_index =
			 	pipe_token_indices.size() >= 3 ? pipe_token_indices[2] : 0;
			 report_body_issue(pipe_index,
			                   "Rule '" + rule->rule_name + "' has too many '|' sections. At most three are supported.");
			 return "";
		 }
		 if(num_sections<=0){
			 report_body_issue(0, "Rule '" + rule->rule_name + "' has no parsable sections.");
			 return "";
		 }
		////std::cout<<<<"sections: "<<num_sections<<"{"<<line<<"}"<<std::endl;
		
		if(num_sections==1){
			if(body_token_spans != NULL){
				ReadTokens(rule,*body_token_spans,1,line_number);
			}
			else{
				ReadTokens(rule,line,1,line_number);
			}
		}
		else {
			for(int i=0;i<num_sections;i++){
				if(body_token_spans != NULL){
					ReadTokens(rule,
					          section_token_groups[static_cast<std::size_t>(i)],
					          i,
					          line_number);
				}
				else{
					ReadTokens(rule,sections[i],i,line_number);
				}
			}

		}
		

				
	
			
				
		
		
		return line;
	
}

std::string GrammarDocument::ruleAlternate(Rule *rule,
                                  std::string line,
                                  int line_number,
                                  const std::vector<SourceTokenSpan> *body_token_spans){
		if(rule == NULL){
			report_grammar_issue(this, line_number, "Internal parser error: alternate rule received a null rule.");
			return "";
		}
		if(trim_copy(line).empty()){
			report_grammar_issue(this,
			                     line_number,
			                     "Rule '" + rule->rule_name + "' has an empty alternate production.");
			return "";
		}

		rule->alternate = new Rule(rule->rule_name + "_Alt", 1);
		rule->alternate->probability = 1.0f - rule->probability;

		const auto report_alternate_issue = [&](std::size_t token_index,
		                                        const std::string &message) {
			report_grammar_issue(this,
			                     locate_rule_section_diagnostic(this,
			                                                     line_number,
			                                                     line,
			                                                     token_index,
			                                                     message),
			                     line);
		};

		std::vector<std::string> sections;
		std::vector<std::vector<SourceTokenSpan>> section_token_groups;
		std::vector<std::size_t> pipe_token_indices;

		if(body_token_spans != NULL){
			std::size_t section_start = 0;
			for(std::size_t token_index = 0; token_index <= body_token_spans->size(); ++token_index){
				if(token_index < body_token_spans->size() &&
				   (*body_token_spans)[token_index].text != "|"){
					continue;
				}
				if(token_index < body_token_spans->size()){
					pipe_token_indices.push_back(token_index);
				}
				section_token_groups.push_back(
					slice_token_spans(*body_token_spans, section_start, token_index));
				sections.push_back(
					join_token_texts(*body_token_spans, section_start, token_index));
				section_start = token_index + 1;
			}
		}
		else{
			const std::vector<std::string> body_tokens = split_rule_tokens(line);
			std::size_t section_start = 0;
			for(std::size_t token_index = 0; token_index <= body_tokens.size(); ++token_index){
				if(token_index < body_tokens.size() && body_tokens[token_index] != "|")continue;
				if(token_index < body_tokens.size())pipe_token_indices.push_back(token_index);
				std::string section_text;
				for(std::size_t item = section_start; item < token_index; ++item){
					if(!section_text.empty())section_text += ' ';
					section_text += body_tokens[item];
				}
				sections.push_back(section_text);
				section_start = token_index + 1;
			}
		}

		if(sections.empty()){
			report_alternate_issue(
				0,
				"Rule '" + rule->rule_name + "' has no parsable alternate sections.");
			return "";
		}
		if(sections.size() > 3){
			const std::size_t pipe_index =
				pipe_token_indices.size() >= 3 ? pipe_token_indices[2] : 0;
			report_alternate_issue(
				pipe_index,
				"Rule '" + rule->rule_name +
				    "' alternate has too many '|' sections. At most three are supported.");
			return "";
		}

		for(std::size_t section_index = 0; section_index < sections.size(); ++section_index){
			if(!trim_copy(sections[section_index]).empty())continue;
			const std::size_t pipe_index =
				pipe_token_indices.empty()
					? 0
					: pipe_token_indices[std::min(section_index, pipe_token_indices.size() - 1)];
			report_alternate_issue(
				pipe_index,
				"Rule '" + rule->rule_name +
				    "' alternate has an empty section around '|'.");
			return "";
		}

		if(sections.size() == 1){
			if(body_token_spans != NULL){
				ReadTokens(rule->alternate, *body_token_spans, 1, line_number);
			}
			else{
				ReadTokens(rule->alternate, sections[0], 1, line_number);
			}
		}
		else{
			for(std::size_t section_index = 0; section_index < sections.size(); ++section_index){
				if(body_token_spans != NULL){
					ReadTokens(rule->alternate,
					           section_token_groups[section_index],
					           static_cast<int>(section_index),
					           line_number);
				}
				else{
					ReadTokens(rule->alternate,
					           sections[section_index],
					           static_cast<int>(section_index),
					           line_number);
				}
			}
		}

		return line;
}


GrammarDocument::GrammarDocument(std::string filePath)
{
	loadFromFile(filePath);
}

GrammarDocument::GrammarDocument(std::istream &input, const std::string &source_name)
{
	loadFromStream(input, source_name);
}

void GrammarDocument::clearParsedState()
{
	error_lines.clear();
	error_details.clear();

	for(Token *token : tokens_new){
		delete token;
	}
	tokens_new.clear();

	for(Rule *rule : rule_list){
		delete rule;
	}
	rule_list.clear();

	clearGrammarRuntimeVariableSnapshots(this);
}

void GrammarDocument::rebuildFromLines(const std::string &success_message)
{
	const bool quiet_diagnostics = grammarEvaluationDiagnosticsQuiet(this);
	set_grammar_uses_time(this, false);
	// P0 gate: malformed delimiters must be rejected before parsing can build an
	// executable token stream, and certainly before recursive expansion begins.
	const bool source_structure_valid = validate_source_structure_before_expansion(this);
	if(source_structure_valid){
		parse_rule_lines_into_grammar(this, build_logical_rules(lines));

		// Ensure "Start" rule is first if it exists.
		ensure_start_rule_is_first();

		if(rule_list.empty()){
			GrammarDiagnostic diagnostic;
			diagnostic.line = lines.empty() ? -1 : 0;
			diagnostic.start_column = 0;
			diagnostic.end_column = 1;
			diagnostic.message = "No valid rules were parsed.";
			report_grammar_issue(this, diagnostic);
		}

		GrammarSemanticTables semantic_tables;
		if(error_details.empty() && !rule_list.empty()){
			build_and_validate_semantic_tables(this, &semantic_tables);
		}

		if(error_details.empty() && !rule_list.empty()){
			ExpansionBudgetState expansion_state;
			GrammarRuntimeState runtime_state;
			runtime_state.grammar = this;
			runtime_state.symbols = std::move(semantic_tables);
			RuntimeVariableValue time_value;
			time_value.name = "t";
			time_value.value = static_cast<float>(getGrammarEvaluationTime(this));
			time_value.min = time_value.value;
			time_value.max = time_value.value;
			time_value.resampleable = false;
			runtime_state.environment.defineCurrent(time_value);
			clearGrammarRuntimeVariableSnapshots(this);
			GrammarRuntimeVariableSnapshot time_snapshot;
			time_snapshot.name = "t";
			time_snapshot.value = time_value.value;
			time_snapshot.min = time_value.value;
			time_snapshot.max = time_value.value;
			time_snapshot.defining_rule = "<builtin-time>";
			store_runtime_variable_snapshot(this, time_snapshot);
			{
				ScopedGrammarRuntimeSession runtime_session(this, &runtime_state);
				ScopedExpansionBudgetSession expansion_session(this, &expansion_state);
				tokens_new = Recurse(rule_list[0]);
			}

			if(expansion_state.aborted || !error_details.empty()){
				for(Token *token : tokens_new){
					delete token;
				}
				tokens_new.clear();
				clearGrammarRuntimeVariableSnapshots(this);
				errorout("Grammar expansion was aborted; no partial action stream or runtime snapshot will be published.");
			}
		}
		else if(!quiet_diagnostics){
			debugout("Grammar expansion skipped because semantic validation failed.");
		}
	}
	else if(!quiet_diagnostics){
		debugout("Grammar expansion skipped because source structure validation failed.");
	}

	if(!success_message.empty() && !quiet_diagnostics){
		if(error_details.empty()){
			errorout(success_message);
		}
		else{
			debugout("Grammar rebuild completed with blocking diagnostics.");
		}
	}
}

void GrammarDocument::loadFromFile(const std::string &filePath)
{
	std::ifstream fin(filePath);
	if(!fin.is_open()){
		clearParsedState();
		lines.clear();
		input_name = filePath;

		std::stringstream ss;
		ss << "E: Could not open file " << filePath << std::endl;
		errorout(ss.str());
		return;
	}

	loadFromStream(fin, filePath);
}

void GrammarDocument::loadFromStream(std::istream &input, const std::string &source_name)
{
	clearParsedState();
	lines.clear();
	input_name = source_name;

	std::string line;
	while(std::getline(input, line)){
		lines.push_back(line);
	}

	std::stringstream ss;
	ss << "Finshed reading Grammar source";
	if(!source_name.empty()){
		ss << " from " << source_name;
	}
	ss << "...";
	rebuildFromLines(ss.str());
}

void GrammarDocument::loadFromSourceText(const std::string &source_text, const std::string &source_name)
{
	std::istringstream input(source_text);
	loadFromStream(input, source_name.empty() ? "<memory>" : source_name);
}


void GrammarDocument::Reread()
{
   try {
	clearParsedState();
	rebuildFromLines("Finshed Re-reading Grammar source...");
		
	}
	
	catch(...){
		errorout("caught exeception reread grammar");
	}
	
	
	
}


void GrammarDocument::update_token(Token *check_token){
	if(check_token == NULL)return;
	const auto resolve_argument = [&](std::size_t argument_index,
	                                  std::size_t expression_index,
	                                  const std::string &purpose) -> bool {
		if(expression_index >= check_token->var_names.size() ||
		   check_token->var_names[expression_index].empty()){
			return true;
		}
		if(argument_index >= check_token->arguments.size()){
			abort_expansion(this,
			                check_token,
			                "Internal grammar error: expression slot exceeds the action argument list.");
			return false;
		}
		float value = 0.0f;
		if(!evaluate_expression_checked(this,
		                                check_token,
		                                check_token->var_names[expression_index],
		                                purpose,
		                                &value)){
			return false;
		}
		check_token->arguments[argument_index] = value;
		check_token->var_names[expression_index].clear();
		return true;
	};

	if(check_token->token_name=="D" || check_token->token_name=="S" || check_token->token_name=="T" ||
	   check_token->token_name=="DSX" || check_token->token_name=="DSY" || check_token->token_name=="DSZ" ||
	   check_token->token_name=="DTX" || check_token->token_name=="DTY" || check_token->token_name=="DTZ" ||
	   check_token->token_name=="V" || check_token->token_name=="VR"){
		for(std::size_t index=0; index<3; ++index){
			if(!resolve_argument(index, index, check_token->token_name))return;
		}
	}
	else if(check_token->token_name=="A"){
		if(check_token->arguments.size() != 2 && check_token->arguments.size() != 4){
			abort_expansion(this,
			                check_token,
			                "Rotation A() expects either 2 arguments or 4 arguments with lower and upper bounds.");
			return;
		}
		const std::size_t indexed_expression_count =
			std::min<std::size_t>(check_token->arguments.size(), check_token->var_names.size());
		for(std::size_t index = 0; index < indexed_expression_count; ++index){
			if(!resolve_argument(index, index, "rotation"))return;
		}
		if(check_token->arguments.size() == 4 && !check_token->var_name.empty()){
			float upper_bound = 0.0f;
			if(!evaluate_expression_checked(this,
			                                check_token,
			                                check_token->var_name,
			                                "rotation upper bound",
			                                &upper_bound)){
				return;
			}
			check_token->arguments[3] = upper_bound;
			check_token->var_name.clear();
		}

		const float axis_value = check_token->arguments[1];
		const float rounded_axis = std::round(axis_value);
		if(!std::isfinite(axis_value) || std::fabs(axis_value - rounded_axis) > 1.0e-5f ||
		   rounded_axis < 0.0f || rounded_axis > 2.0f){
			abort_expansion(this,
			                check_token,
			                "Rotation axis must be exactly 0 (X), 1 (Y), or 2 (Z).");
			return;
		}
		check_token->arguments[1] = rounded_axis;

		if(check_token->arguments.size() == 4){
			const float lower_bound = check_token->arguments[2];
			const float upper_bound = check_token->arguments[3];
			if(!std::isfinite(lower_bound) || !std::isfinite(upper_bound) ||
			   lower_bound > upper_bound){
				abort_expansion(this,
				                check_token,
				                "Rotation bounds require a finite lower bound <= upper bound.");
				return;
			}
			check_token->arguments[0] =
				std::clamp(check_token->arguments[0], lower_bound, upper_bound);
		}
	}
	else if(check_token->token_name=="I" ||
	        check_token->token_name=="M" ||
	        check_token->token_name=="P"){
		if(check_token->token_name=="I" && check_token->shape_descriptor_syntax){
			ShapeSpecificationEvaluator specification_evaluator;
			const ShapeSpecificationEvaluationResult result =
				specification_evaluator.evaluate(
					*check_token->shape_descriptor_syntax,
					[&](const GeometryExpression &expression,
					    const std::string &purpose,
					    float *value) {
						return evaluate_expression_checked(
							this,
							check_token,
							expression.sourceText(),
							purpose,
							value);
					});
			if(!result.succeeded()){
				if(!expansion_is_aborted()){
					abort_expansion(
						this,
						check_token,
						result.diagnostic.empty()
							? "Procedural shape specification could not be evaluated."
							: result.diagnostic);
				}
				return;
			}
			check_token->shape_specification = result.specification;
			switch(result.specification->family()){
			case ShapeFamily::Cylinder:
				check_token->instance_type = "Cylinder";
				break;
			case ShapeFamily::Sphere:
				check_token->instance_type = "Sphere";
				break;
				case ShapeFamily::AxialProfile:
					check_token->instance_type = "AxialProfile";
					break;
				case ShapeFamily::ExtrudeProfile:
					check_token->instance_type = "ExtrudeProfile";
					break;
				case ShapeFamily::Box:
					check_token->instance_type = "Cube";
					break;
				case ShapeFamily::SweepProfile:
					check_token->instance_type = "SweepProfile";
					break;
				case ShapeFamily::VariableSectionSweep:
					check_token->instance_type = "VariableSectionSweep";
					break;
				case ShapeFamily::SweepDisk:
					check_token->instance_type = "SweepDisk";
					break;
				case ShapeFamily::TaperedSweep:
					check_token->instance_type = "TaperedSweep";
					break;
				case ShapeFamily::BranchJunction:
					check_token->instance_type = "BranchJunction";
					break;
				case ShapeFamily::LeafBlade:
					check_token->instance_type = "LeafBlade";
					break;
				case ShapeFamily::PetalBlade:
					check_token->instance_type = "PetalBlade";
					break;
				case ShapeFamily::Plant:
					check_token->instance_type = "Plant";
					break;
				case ShapeFamily::Vine:
					check_token->instance_type = "Vine";
					break;
				case ShapeFamily::ScatterRegion:
					check_token->instance_type = "ScatterRegion";
					break;
				case ShapeFamily::Revolve:
					check_token->instance_type = "Revolve";
					break;
				case ShapeFamily::Loft:
					check_token->instance_type = "Loft";
					break;
				case ShapeFamily::SurfaceLoft:
					check_token->instance_type = "SurfaceLoft";
					break;
				case ShapeFamily::CurveNetworkSurface:
					check_token->instance_type = "CurveNetworkSurface";
					break;
				case ShapeFamily::ShellLoft:
					check_token->instance_type = "ShellLoft";
					break;
				case ShapeFamily::ShellOffset:
					check_token->instance_type = "ShellOffset";
					break;
				case ShapeFamily::MirrorShape:
					check_token->instance_type = "MirrorShape";
					break;
				case ShapeFamily::CurvedPanel:
					check_token->instance_type = "CurvedPanel";
					break;
				case ShapeFamily::FormedPanel:
					check_token->instance_type = "FormedPanel";
					break;
				case ShapeFamily::PanelCut:
					check_token->instance_type = "PanelCut";
					break;
				case ShapeFamily::HostedOpening:
					check_token->instance_type = "HostedOpening";
					break;
				case ShapeFamily::EmbossedBead:
					check_token->instance_type = "EmbossedBead";
					break;
				case ShapeFamily::EdgeFlange:
					check_token->instance_type = "EdgeFlange";
					break;
				case ShapeFamily::CompoundShape:
					check_token->instance_type = "CompoundShape";
					break;
				case ShapeFamily::LayerSet:
					check_token->instance_type = "LayerSet";
					break;
				case ShapeFamily::FoldedProfile:
					check_token->instance_type = "FoldedProfile";
					break;
				case ShapeFamily::PanelBox:
					check_token->instance_type = "PanelBox";
					break;
				case ShapeFamily::PanelArray:
					check_token->instance_type = "PanelArray";
					break;
				case ShapeFamily::GlazingPanel:
					check_token->instance_type = "GlazingPanel";
					break;
				case ShapeFamily::SurfaceTiling:
					check_token->instance_type = "SurfaceTiling";
					break;
				case ShapeFamily::InstanceArray:
					check_token->instance_type = "InstanceArray";
					break;
				case ShapeFamily::GeneratedMeshReference:
					check_token->instance_type = "GeneratedMeshReference";
					break;
				}
		}
		for(std::size_t index=0; index<check_token->arguments.size() && index<3; ++index){
			if(!resolve_argument(index, index, check_token->token_name=="I" ? "instance" : "physics"))return;
		}
		if(check_token->token_name=="I" && check_token->material_name.empty() &&
		   !check_token->arguments.empty()){
			const float material_index = check_token->arguments[0];
			if(!std::isfinite(material_index) ||
			   static_cast<double>(material_index) <
			       static_cast<double>(std::numeric_limits<int>::lowest()) ||
			   static_cast<double>(material_index) >
			       static_cast<double>(std::numeric_limits<int>::max())){
				abort_expansion(this,
				                check_token,
				                "Instance material index is outside the supported int range.");
				return;
			}
		}
	}
	else if(check_token->token_name=="G"){
		if(!check_token->var_name.empty()){
			float magnitude = 0.0f;
			if(!evaluate_expression_checked(this,
			                                check_token,
			                                check_token->var_name,
			                                "gravity magnitude",
			                                &magnitude)){
				return;
			}
			if(check_token->arguments.empty()){
				abort_expansion(this, check_token, "Internal grammar error: gravity magnitude slot is missing.");
				return;
			}
			check_token->arguments[0] = magnitude;
			check_token->var_name.clear();
		}
		for(std::size_t index=0; index<3; ++index){
			if(!resolve_argument(index+1, index, "gravity direction"))return;
		}
	}
}

static bool split_condition_expression(const std::string &expression,
                                       std::string *lhs,
                                       std::string *rhs,
                                       std::string *op)
{
	if(lhs == NULL || rhs == NULL || op == NULL)return false;

	int depth = 0;
	for(std::size_t index = 0; index < expression.size(); ++index){
		const char c = expression[index];
		if(c == '('){
			++depth;
			continue;
		}
		if(c == ')'){
			if(depth > 0)--depth;
			continue;
		}
		if(depth != 0)continue;

		if(index + 1 < expression.size()){
			const std::string two_char_operator = expression.substr(index, 2);
			if(two_char_operator == "<=" || two_char_operator == ">=" ||
			   two_char_operator == "==" || two_char_operator == "!="){
				*lhs = trim_copy(expression.substr(0, index));
				*rhs = trim_copy(expression.substr(index + 2));
				*op = two_char_operator;
				return !lhs->empty() && !rhs->empty();
			}
		}

		if(c == '<' || c == '>'){
			*lhs = trim_copy(expression.substr(0, index));
			*rhs = trim_copy(expression.substr(index + 1));
			*op = std::string(1, c);
			return !lhs->empty() && !rhs->empty();
		}
	}

	return false;
}

static bool evaluate_condition_expression(Grammar *grammar,
                                          const Token *condition_token,
                                          bool *out_result)
{
	if(grammar == NULL || out_result == NULL)return false;
	const std::string expression =
		condition_token == NULL ? std::string() : trim_copy(condition_token->getConditionExpression());
	if(expression.empty()){
		abort_expansion(grammar, condition_token, "Conditional expression is empty.");
		return false;
	}

	std::string lhs;
	std::string rhs;
	std::string op;
	if(split_condition_expression(expression, &lhs, &rhs, &op)){
		float lhs_value = 0.0f;
		float rhs_value = 0.0f;
		if(!evaluate_expression_checked(grammar, condition_token, lhs, "conditional left operand", &lhs_value) ||
		   !evaluate_expression_checked(grammar, condition_token, rhs, "conditional right operand", &rhs_value)){
			return false;
		}
		const double epsilon = 0.000001;
		if(op == "<")*out_result = lhs_value < rhs_value;
		else if(op == ">")*out_result = lhs_value > rhs_value;
		else if(op == "<=")*out_result = lhs_value <= rhs_value;
		else if(op == ">=")*out_result = lhs_value >= rhs_value;
		else if(op == "==")*out_result = std::fabs(lhs_value - rhs_value) <= epsilon;
		else *out_result = std::fabs(lhs_value - rhs_value) > epsilon;
		return true;
	}

	float value = 0.0f;
	if(!evaluate_expression_checked(grammar, condition_token, expression, "conditional", &value)){
		return false;
	}
	*out_result = std::fabs(value) > 0.000001f;
	return true;
}

static void destroy_expanded_tokens(std::vector<Token *> *tokens)
{
	if(tokens == NULL)return;
	for(Token *token : *tokens){
		delete token;
	}
	tokens->clear();
}

static bool random_range_is_valid(Grammar *grammar,
                                  const Token *token,
                                  float minimum,
                                  float maximum,
                                  bool integer)
{
	if(!std::isfinite(minimum) || !std::isfinite(maximum)){
		abort_expansion(grammar, token, "Random-variable range must be finite.");
		return false;
	}
	if(minimum > maximum){
		abort_expansion(grammar,
		                token,
		                "Random-variable minimum " + std::to_string(minimum) +
		                    " exceeds maximum " + std::to_string(maximum) + ".");
		return false;
	}
	if(integer){
		const double integer_min = static_cast<double>(std::numeric_limits<int>::lowest());
		const double integer_max = static_cast<double>(std::numeric_limits<int>::max());
		if(static_cast<double>(minimum) < integer_min ||
		   static_cast<double>(maximum) > integer_max){
			abort_expansion(grammar,
			                token,
			                "Integer random-variable range exceeds the supported int range.");
			return false;
		}
		if(std::ceil(minimum) > std::floor(maximum)){
			abort_expansion(grammar,
			                token,
			                "Integer random-variable range contains no integer value.");
			return false;
		}
	}
	return true;
}

static bool execute_random_declaration(Grammar *grammar, const Token *token)
{
	if(grammar == NULL || token == NULL)return false;
	GrammarRuntimeState *runtime = runtime_for(grammar);
	if(runtime == NULL){
		abort_expansion(grammar, token, "Random declaration has no active grammar environment.");
		return false;
	}
	if(!is_identifier_token_string(token->var_name)){
		abort_expansion(grammar,
		                token,
		                "Random declaration has invalid variable name '" + token->var_name + "'.");
		return false;
	}
	float minimum = 0.0f;
	float maximum = 0.0f;
	if(!evaluate_expression_checked(grammar, token, token->var_names[0], "random minimum", &minimum) ||
	   !evaluate_expression_checked(grammar, token, token->var_names[1], "random maximum", &maximum)){
		return false;
	}
	if(!random_range_is_valid(grammar, token, minimum, maximum, token->integer)){
		return false;
	}

	RuntimeVariableValue declaration;
	declaration.name = token->var_name;
	declaration.min = minimum;
	declaration.max = maximum;
	declaration.integer = token->integer;
	declaration.resampleable = true;
	declaration.value = sample_variable_value(minimum, maximum, token->integer);
	RuntimeVariableValue &stored = runtime->environment.defineCurrent(std::move(declaration));

	GrammarRuntimeVariableSnapshot snapshot;
	snapshot.name = stored.name;
	snapshot.value = stored.value;
	snapshot.min = stored.min;
	snapshot.max = stored.max;
	snapshot.integer = stored.integer;
	snapshot.instance_count = stored.instance_count;
	if(active_expansion_budget != NULL && !active_expansion_budget->call_stack.empty()){
		snapshot.defining_rule = active_expansion_budget->call_stack.back();
	}
	store_runtime_variable_snapshot(grammar, snapshot);
	return true;
}

static Rule *find_rule_symbol(Grammar *grammar, const std::string &name)
{
	GrammarRuntimeState *runtime = runtime_for(grammar);
	if(runtime != NULL){
		const auto found = runtime->symbols.rules.find(name);
		if(found != runtime->symbols.rules.end())return found->second;
	}
	if(grammar == NULL)return NULL;
	const int index = grammar->findRule(name);
	return index < 0 ? NULL : grammar->rule_list[static_cast<std::size_t>(index)];
}

static void expand_rule_tokens(Grammar *grammar,
                               const std::vector<Token *> &source_tokens,
                               std::vector<Token *> *expanded_tokens)
{
	if(grammar == NULL || expanded_tokens == NULL)return;

	for(Token *source_token : source_tokens){
		if(expansion_is_aborted())return;
		if(source_token == NULL)continue;
		if(!consume_expansion_work(grammar, source_token))return;

		if(auto *object_scope = dynamic_cast<SpatialObjectScopeToken *>(source_token)){
			if(!reserve_expanded_action(grammar, source_token))return;
			std::unique_ptr<SpatialObjectBeginActionToken> begin_token(
				new SpatialObjectBeginActionToken());
			begin_token->descriptor_syntax = object_scope->descriptor_syntax;
			begin_token->source_start_line = source_token->source_start_line;
			begin_token->source_start_column = source_token->source_start_column;
			begin_token->source_end_line = source_token->source_end_line;
			begin_token->source_end_column = source_token->source_end_column;
			begin_token->source_name = source_token->source_name;
			expanded_tokens->push_back(begin_token.release());

			if(!reserve_expanded_action(grammar, source_token))return;
			std::unique_ptr<Token> open_scope(new Token("["));
			open_scope->source_start_line = source_token->source_start_line;
			open_scope->source_start_column = source_token->source_start_column;
			open_scope->source_end_line = source_token->source_end_line;
			open_scope->source_end_column = source_token->source_end_column;
			open_scope->source_name = source_token->source_name;
			expanded_tokens->push_back(open_scope.release());

			expand_rule_tokens(grammar, object_scope->body_tokens, expanded_tokens);
			if(expansion_is_aborted())return;

			if(!reserve_expanded_action(grammar, source_token))return;
			std::unique_ptr<Token> close_scope(new Token("]"));
			close_scope->source_start_line = source_token->source_start_line;
			close_scope->source_start_column = source_token->source_start_column;
			close_scope->source_end_line = source_token->source_end_line;
			close_scope->source_end_column = source_token->source_end_column;
			close_scope->source_name = source_token->source_name;
			expanded_tokens->push_back(close_scope.release());

			if(!reserve_expanded_action(grammar, source_token))return;
			std::unique_ptr<SpatialObjectEndActionToken> end_token(
				new SpatialObjectEndActionToken());
			end_token->source_start_line = source_token->source_start_line;
			end_token->source_start_column = source_token->source_start_column;
			end_token->source_end_line = source_token->source_end_line;
			end_token->source_end_column = source_token->source_end_column;
			end_token->source_name = source_token->source_name;
			expanded_tokens->push_back(end_token.release());
			continue;
		}

		if(source_token->isConditionalToken()){
			bool condition_result = false;
			if(!evaluate_condition_expression(grammar, source_token, &condition_result))return;
			const std::vector<Token *> &branch_tokens =
				condition_result ? source_token->getTrueBranchTokens()
				                 : source_token->getFalseBranchTokens();
			expand_rule_tokens(grammar, branch_tokens, expanded_tokens);
			continue;
		}

		if(source_token->token_name == "R"){
			if(!execute_random_declaration(grammar, source_token))return;
			continue;
		}

		if(source_token->isRule()!=""){
			Rule *called_rule = find_rule_symbol(grammar, source_token->token_name);
			if(called_rule == NULL){
				abort_expansion(grammar,
				                source_token,
				                "Undefined rule reference '" + source_token->token_name + "'.",
				                source_token->token_name);
				return;
			}

			std::vector<Token *> more_tokens = grammar->Recurse(called_rule, source_token);
			if(expansion_is_aborted()){
				destroy_expanded_tokens(&more_tokens);
				return;
			}
			for(Token *more_token : more_tokens){
				expanded_tokens->push_back(more_token);
			}
			continue;
		}

		if(!reserve_expanded_action(grammar, source_token))return;
		Token *expanded_token = source_token->clone();
		GrammarRuntimeState *runtime = runtime_for(grammar);
		if(runtime != NULL){
			expanded_token->captured_variables = runtime->environment.visibleValues();
		}
		grammar->update_token(expanded_token);
		if(expansion_is_aborted()){
			delete expanded_token;
			return;
		}
		expanded_tokens->push_back(expanded_token);
	}
}

static void expand_sectioned_rule(Grammar *grammar,
                                  Rule *production,
                                  const std::string &counter_rule_name,
                                  Token *call_token,
                                  int repeat_count,
                                  std::vector<Token *> *expanded_tokens)
{
	if(grammar == NULL || production == NULL || expanded_tokens == NULL)return;
	if(expansion_is_aborted())return;
	GrammarRuntimeState *runtime = runtime_for(grammar);
	if(runtime == NULL){
		abort_expansion(grammar, call_token, "Rule expansion has no active lexical environment.");
		return;
	}

	expand_rule_tokens(grammar, production->section_tokens[0], expanded_tokens);
	if(expansion_is_aborted())return;

	RuntimeVariableValue counter;
	counter.name = counter_rule_name + "_count";
	counter.value = 0.0f;
	counter.min = 0.0f;
	counter.max = repeat_count > 0 ? static_cast<float>(repeat_count - 1) : 0.0f;
	counter.integer = true;
	runtime->environment.defineCurrent(std::move(counter));

	for(int iteration = 0; iteration < repeat_count; ++iteration){
		if(expansion_is_aborted())return;
		if(!consume_expansion_work(grammar, call_token))return;
		if(!runtime->environment.setVisibleValue(counter_rule_name + "_count",
		                                         static_cast<float>(iteration))){
			abort_expansion(grammar, call_token, "Iteration counter disappeared from its lexical frame.");
			return;
		}
		expand_rule_tokens(grammar, production->section_tokens[1], expanded_tokens);
	}

	if(expansion_is_aborted())return;
	expand_rule_tokens(grammar, production->section_tokens[2], expanded_tokens);
}

std::vector<Token *> GrammarDocument::Recurse(Rule *rule, Token *call_token){
	if(rule == NULL)return {};

	ScopedExpansionBudgetSession expansion_session(this);
	ScopedGrammarRuntimeSession runtime_session(this);
	ScopedRuleExpansion rule_expansion(this, rule, call_token);
	if(!rule_expansion.entered() || expansion_is_aborted())return {};

	GrammarRuntimeState *runtime = runtime_for(this);
	if(runtime == NULL){
		abort_expansion(this, call_token, "Rule recursion has no active grammar runtime.", rule->rule_name);
		return {};
	}

	std::vector<Token *> new_tokens;
	try{
		const std::size_t supplied_argument_count =
			call_token == NULL ? 0 : call_token->rule_arguments.size();
		if(supplied_argument_count != rule->parameter_names.size()){
			abort_expansion(this,
			                call_token,
			                "Rule '" + rule->rule_name + "' expects " +
			                    std::to_string(rule->parameter_names.size()) + " argument(s), but received " +
			                    std::to_string(supplied_argument_count) + ".",
			                rule->rule_name);
			return {};
		}

		// Evaluate all call arguments and reroll sources in the caller environment
		// before introducing the callee frame.
		std::vector<RuntimeVariableValue> parameter_values;
		parameter_values.reserve(rule->parameter_names.size());
		for(std::size_t parameter_index = 0;
		    parameter_index < rule->parameter_names.size();
		    ++parameter_index){
			const std::string &parameter_name = rule->parameter_names[parameter_index];
			const std::string argument_expression =
				removeSpaces(call_token->rule_arguments[parameter_index]);
			RuntimeVariableValue bound;
			bound.name = parameter_name;
			const RuntimeVariableValue *source =
				is_identifier_token_string(argument_expression)
					? runtime->environment.lookup(argument_expression)
					: NULL;
			if(source != NULL){
				bound.value = source->value;
				bound.min = source->min;
				bound.max = source->max;
				bound.integer = source->integer;
				bound.resampleable = source->resampleable;
				bound.instance_count = source->instance_count;
			}
			else{
				float value = 0.0f;
				if(!evaluate_expression_checked(this,
				                                call_token,
				                                argument_expression,
				                                "argument for parameter '" + parameter_name + "'",
				                                &value)){
					return {};
				}
				bound.value = value;
				bound.min = value;
				bound.max = value;
			}
			parameter_values.push_back(std::move(bound));
		}

		std::vector<RuntimeVariableValue> rerolled_values;
		rerolled_values.reserve(static_cast<std::size_t>(std::max(0, rule->var_counter)));
		for(int variable_index = 0; variable_index < rule->var_counter; ++variable_index){
			const std::string &variable_name = rule->var_names[variable_index];
			const RuntimeVariableValue *source = runtime->environment.lookup(variable_name);
			if(source == NULL){
				abort_expansion(this,
				                call_token,
				                "Rule '" + rule->rule_name +
				                    "' requests an undefined reroll variable '" + variable_name + "'.",
				                rule->rule_name);
				return {};
			}
			if(!random_range_is_valid(this, call_token, source->min, source->max, source->integer)){
				return {};
			}
			RuntimeVariableValue rerolled = *source;
			rerolled.value = sample_variable_value(source->min, source->max, source->integer);
			rerolled.instance_count = source->instance_count + 1;
			rerolled_values.push_back(std::move(rerolled));
		}

		ScopedLexicalFrame lexical_frame(runtime);
		for(RuntimeVariableValue &parameter : parameter_values){
			runtime->environment.defineCurrent(std::move(parameter));
		}
		for(RuntimeVariableValue &rerolled : rerolled_values){
			RuntimeVariableValue &stored = runtime->environment.defineCurrent(std::move(rerolled));
			GrammarRuntimeVariableSnapshot snapshot;
			snapshot.name = stored.name;
			snapshot.value = stored.value;
			snapshot.min = stored.min;
			snapshot.max = stored.max;
			snapshot.integer = stored.integer;
			snapshot.instance_count = stored.instance_count;
			snapshot.defining_rule = rule->rule_name;
			store_runtime_variable_snapshot(this, snapshot);
		}

		float repeat_value = static_cast<float>(rule->repeat);
		if(!rule->var_name.empty() &&
		   !evaluate_expression_checked(this,
		                                call_token,
		                                rule->var_name,
		                                "repeat count for rule '" + rule->rule_name + "'",
		                                &repeat_value)){
			return {};
		}
		if(std::fabs(repeat_value) > static_cast<float>(kMaxRuleRepeatCount)){
			abort_expansion(this,
			                call_token,
			                "Rule '" + rule->rule_name + "' exceeded the repeat limit of " +
			                    std::to_string(kMaxRuleRepeatCount) + ".",
			                rule->rule_name);
			return {};
		}
		if(repeat_value <= 0.0f)return new_tokens;
		const int repeat_count = static_cast<int>(repeat_value);
		if(repeat_count <= 0)return new_tokens;

		Rule *selected_production = rule;
		if(rule->alternate != NULL){
			std::uniform_real_distribution<double> unif(0.0, 1.0);
			if(rule->probability <= static_cast<float>(unif(grammar_rng))){
				selected_production = rule->alternate;
			}
		}
		expand_sectioned_rule(this,
		                      selected_production,
		                      rule->rule_name,
		                      call_token,
		                      repeat_count,
		                      &new_tokens);

		if(expansion_is_aborted()){
			destroy_expanded_tokens(&new_tokens);
			return {};
		}
		return new_tokens;
	}
	catch(...){
		destroy_expanded_tokens(&new_tokens);
		abort_expansion(this,
		                call_token,
		                "An exception interrupted grammar expansion for rule '" +
		                    rule->rule_name + "'.",
		                rule->rule_name);
		errorout("caught exception recurse grammar");
		return {};
	}
}


int GrammarDocument::findRule(std::string rule_name){
	for(std::size_t i=0;i<rule_list.size();i++){
		if(rule_list[i]->rule_name.compare(rule_name)==0) {
			return static_cast<int>(i);
		}
	}
	return -1;
}


void GrammarDocument::addContext(){
	if(context!=NULL){
		delete context;
		context=NULL;
	}
	this->context=new Context();
}


void GrammarDocument::generateGeometry()
{
	if(context == NULL){
		GrammarDiagnostic diagnostic;
		diagnostic.message = "Scene generation was requested without a context.";
		report_grammar_issue(this, diagnostic);
		return;
	}
	if(!error_details.empty()){
		errorout("Scene generation skipped because the grammar contains blocking errors.");
		return;
	}

	// Complete the entire execution preflight before mutating the scene. This
	// prevents a late unmatched scope from leaving a partially generated model.
	std::size_t action_count = 0;
	std::size_t primitive_count = 0;
	std::vector<Token *> scope_stack;
	std::vector<Token *> spatial_object_stack;
	for(Token *token : tokens_new){
		if(token == NULL)continue;
		if(++action_count > kMaxExpandedActionCount){
			report_grammar_issue(
				this,
				make_action_token_diagnostic(
					token,
					"Scene execution exceeded the action budget of " +
					    std::to_string(kMaxExpandedActionCount) + "."));
			return;
		}
		if(token->token_name == "I" && ++primitive_count > kMaxGeneratedPrimitiveCount){
			report_grammar_issue(
				this,
				make_action_token_diagnostic(
					token,
					"Scene execution exceeded the primitive budget of " +
					    std::to_string(kMaxGeneratedPrimitiveCount) + "."));
			return;
		}
		if(token->token_name == "ObjectBegin"){
			spatial_object_stack.push_back(token);
		}
		else if(token->token_name == "ObjectEnd"){
			if(spatial_object_stack.empty()){
				report_grammar_issue(
					this,
					make_action_token_diagnostic(
						token,
						"Scene execution blocked a spatial object-stack underflow."));
				return;
			}
			spatial_object_stack.pop_back();
		}

		if(token->token_name == "[" || token->token_name == "{"){
			scope_stack.push_back(token);
		}
		else if(token->token_name == "]" || token->token_name == "}"){
			if(scope_stack.empty()){
				report_grammar_issue(
					this,
					make_action_token_diagnostic(
						token,
						"Scene execution blocked a scope-stack underflow."));
				return;
			}
			Token *open_token = scope_stack.back();
			const std::string expected_close =
				open_token->token_name == "[" ? "]" : "}";
			if(token->token_name != expected_close){
				report_grammar_issue(
					this,
					make_action_token_diagnostic(
						token,
						"Scene execution found closing scope '" + token->token_name +
						    "' but expected '" + expected_close + "'."));
				return;
			}
			scope_stack.pop_back();
		}
	}

	if(!scope_stack.empty()){
		Token *open_token = scope_stack.back();
		report_grammar_issue(
			this,
			make_action_token_diagnostic(
				open_token,
				"Scene execution ended with " + std::to_string(scope_stack.size()) +
				    " unclosed scope(s)."));
		return;
	}
	if(!spatial_object_stack.empty()){
		Token *open_token = spatial_object_stack.back();
		report_grammar_issue(
			this,
			make_action_token_diagnostic(
				open_token,
				"Scene execution ended with " +
					std::to_string(spatial_object_stack.size()) +
					" unclosed spatial object scope(s)."));
		return;
	}

	for(Token *token : tokens_new){
		if(token == NULL)continue;
		token->performAction(context);
		if(!error_details.empty())return;
	}
	std::string spatial_diagnostic;
	if(!context->finalizeSpatialBuildingModel(&spatial_diagnostic)){
		GrammarDiagnostic diagnostic;
		diagnostic.message = "Spatial building model failed: " + spatial_diagnostic;
		report_grammar_issue(this, diagnostic);
	}
}










GrammarDocument::~GrammarDocument(){
	clearGrammarRuntimeVariableSnapshots(this);
	clearGrammarEvaluationState(this);
	for(Token *token : tokens_new){
		delete token;
	}
	tokens_new.clear();

	for(Rule *rule : rule_list){
		delete rule;
	}
	rule_list.clear();

	if(context!=NULL){
		delete context;
		context=NULL;
	}
}
