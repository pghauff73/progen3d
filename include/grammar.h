#pragma once

#include "grammar/model/ShapeDescriptorSyntax.h"
#include "grammar/model/LightingDeclarationSyntax.h"
#include "grammar/model/SpatialConnectionDeclarationSyntax.h"
#include "grammar/model/SpatialConstraintDeclarationSyntax.h"
#include "grammar/model/SpatialInterfaceDeclarationSyntax.h"
#include "grammar/model/SpatialObjectDescriptorSyntax.h"
#include "grammar/model/VehicleJointDeclarationSyntax.h"

#include <array>
#include <istream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

class SceneGenerationContext; // forward declaration to avoid include-order problems
class ShapeSpecification;

struct SourceTokenSpan {
	std::string text;
	int line = -1;
	int start_column = 0;
	int end_column = 0;
};

class GrammarModelElement {
public:
	virtual ~GrammarModelElement() = default;
};

class GrammarVariableDefinition : public GrammarModelElement {
	public:
	std::string var_name;
	float value;
	int instance_count=0;
	GrammarVariableDefinition(std::string name,float min,float max,bool i);
	GrammarVariableDefinition(std::string name,float value){
		this->var_name=name;
		this->min=0;
		this->max=0;
		this->value=value;
	}
	float getValue(){
		//this->value=rand()/(float)RAND_MAX*(max-min)+min;
		return this->value;
	}
	float getRandom();
	float min,max;
	bool integer=false;
	bool scoped=false;
};
class GrammarActionToken : public GrammarModelElement {
	public:
	GrammarActionToken(GrammarActionToken *t){
		if(t==NULL)return;
		this->token_name=t->token_name;
		for(int i=0;i<t->arguments.size();i++){
			this->arguments.push_back(t->arguments[i]);
		}
		this->var_names[0]=t->var_names[0];
		this->var_names[1]=t->var_names[1];
		this->var_names[2]=t->var_names[2];
		this->var_name=t->var_name;
		this->modify[0]=t->modify[0];
		this->modify[1]=t->modify[1];
		this->modify[2]=t->modify[2];
		this->divideby2[0]=t->divideby2[0];
		this->divideby2[1]=t->divideby2[1];
		this->divideby2[2]=t->divideby2[2];
		
			this->instance_type=t->instance_type;
			this->material_name=t->material_name;
			this->shape_descriptor_syntax=t->shape_descriptor_syntax;
			this->shape_specification=t->shape_specification;
			this->instance_count=t->instance_count;
			this->integer=t->integer;
			this->immovable=t->immovable;
			this->rule_arguments=t->rule_arguments;
			this->source_start_line=t->source_start_line;
			this->source_start_column=t->source_start_column;
			this->source_end_line=t->source_end_line;
			this->source_end_column=t->source_end_column;
			this->source_name=t->source_name;
			this->captured_variables=t->captured_variables;
			
		}
	GrammarActionToken(std::string token_name){
		this->token_name=token_name;
	}
	GrammarActionToken(std::string token_name,int instance_count){
		this->token_name=token_name;
		this->instance_count=instance_count;
	}
	virtual ~GrammarActionToken() = default;
	virtual GrammarActionToken *clone() const {
		return new GrammarActionToken(const_cast<GrammarActionToken *>(this));
	}
	virtual bool isConditionalToken() const {
		return false;
	}
	virtual const std::string &getConditionExpression() const {
		static const std::string empty_expression;
		return empty_expression;
	}
	virtual const std::vector<GrammarActionToken *> &getTrueBranchTokens() const {
		static const std::vector<GrammarActionToken *> empty_tokens;
		return empty_tokens;
	}
	virtual const std::vector<GrammarActionToken *> &getFalseBranchTokens() const {
		static const std::vector<GrammarActionToken *> empty_tokens;
		return empty_tokens;
	}
	void addArgument(float value){
		
		arguments.push_back(value);
	}
	void addInstanceType(std::string instance){
		
			this->instance_type=instance;
	}
	int getArgCount(){
		return arguments.size();
	}
	virtual std::string isRule(){
		
		if(token_name!="{" && token_name!="}" && token_name!="+" && token_name!="*" &&
		   token_name!="[" && token_name!="]" && token_name!="?" &&
		   instance_type=="" && arguments.size()==0)return token_name;
		else return "";
	}
	virtual std::string print(){
		std::stringstream ss;
		
		ss.precision(5);
			if(token_name=="R" && integer==true)ss<<token_name<<"* ";
			else if(token_name=="I" && immovable)ss<<"!I ";
			else ss<<token_name<<" ";
			if(token_name=="G"){
				ss<<"( ";
				if(var_name!="")ss<<var_name<<" ";
				else if(!arguments.empty())ss<<arguments[0]<<" ";
				for(int i=0;i<3;i++){
					if(var_names[i]!="")ss<<var_names[i]<<" ";
					else if(arguments.size()>static_cast<std::size_t>(i+1))ss<<arguments[static_cast<std::size_t>(i+1)]<<" ";
				}
				ss<<") ";
				return ss.str();
			}
			if(isRule()!=""){
				if(!rule_arguments.empty()){
					ss<<"( ";
					for(const std::string &argument : rule_arguments){
						ss<<argument<<" ";
					}
					ss<<") ";
				}
				return ss.str();
			}
			if(token_name=="[" || token_name=="]")return ss.str();
			if(token_name=="I"){
				ss<<"( ";
				if(shape_descriptor_syntax){
					ss<<shape_descriptor_syntax->canonicalText()<<" ";
				}
				else if(instance_type!=""){
					ss<<instance_type<<" ";
				}
				if(material_name!="")ss<<material_name<<" ";
				for(std::size_t i=0;i<arguments.size();i++){
					if(i<var_names.size() && var_names[i]!="")ss<<var_names[i]<<" ";
					else ss<<arguments[i]<<" ";
				}
				ss<<") ";
				return ss.str();
			}
			if(var_name!="")ss<<var_name<<" ";
			ss<<"( ";
		if(instance_type!=""){
			ss<<instance_type<<" ";
		}
		if(material_name!=""){
			ss<<material_name<<" ";
		}
		for(std::size_t i=0;i<arguments.size();i++){
			if(var_names[i]!=""){
				ss<<var_names[i]<<" ";
				continue;
			}
			std::stringstream ss2;	
			ss2<<std::fixed<<arguments[i]<<" ";
			std::string str1=ss2.str();
			int pos=str1.find(".00000");
			if(pos!=-1){
				str1.erase(str1.begin()+pos,str1.begin()+pos+6+1);
			}
			ss<<str1+" ";
		}
		ss<<") ";
		return ss.str();
	}
	
	virtual void performAction(SceneGenerationContext *context);
	
	
	std::string token_name;
	std::vector<float> arguments;
	std::string instance_type;
	std::string material_name;
	std::shared_ptr<const ShapeDescriptorSyntax> shape_descriptor_syntax;
	std::shared_ptr<const ShapeSpecification> shape_specification;
	int instance_count=0;
	std::string var_name;
	std::array<std::string, 3> var_names{};
		bool modify[3]{false, false, false};
		bool divideby2[3]{false, false, false};
		bool integer=false;
		bool immovable=false;
		std::vector<std::string> rule_arguments;
		int source_start_line=-1;
		int source_start_column=0;
			int source_end_line=-1;
			int source_end_column=0;
	std::string source_name;
	std::unordered_map<std::string, float> captured_variables;
};

class GrammarConditionalActionToken : public GrammarActionToken {
	public:
	GrammarConditionalActionToken()
		: GrammarActionToken("?")
	{
	}
	GrammarConditionalActionToken(GrammarConditionalActionToken *t)
		: GrammarActionToken(t)
	{
		if(t == NULL)return;
		condition_expression = t->condition_expression;
		for(GrammarActionToken *conditional_token : t->conditional_true_tokens){
			conditional_true_tokens.push_back(
				conditional_token != NULL ? conditional_token->clone() : NULL);
		}
		for(GrammarActionToken *conditional_token : t->conditional_false_tokens){
			conditional_false_tokens.push_back(
				conditional_token != NULL ? conditional_token->clone() : NULL);
		}
	}
	~GrammarConditionalActionToken() override {
		for(GrammarActionToken *conditional_token : conditional_true_tokens){
			delete conditional_token;
		}
		for(GrammarActionToken *conditional_token : conditional_false_tokens){
			delete conditional_token;
		}
	}
	GrammarActionToken *clone() const override {
		return new GrammarConditionalActionToken(
			const_cast<GrammarConditionalActionToken *>(this));
	}
	bool isConditionalToken() const override {
		return true;
	}
	const std::string &getConditionExpression() const override {
		return condition_expression;
	}
	const std::vector<GrammarActionToken *> &getTrueBranchTokens() const override {
		return conditional_true_tokens;
	}
	const std::vector<GrammarActionToken *> &getFalseBranchTokens() const override {
		return conditional_false_tokens;
	}
	std::string print() override {
		std::stringstream ss;
		ss<<"? ( "<<condition_expression<<" ) ";
		for(GrammarActionToken *conditional_token : conditional_true_tokens){
			if(conditional_token!=NULL)ss<<conditional_token->print();
		}
		ss<<": ";
		for(GrammarActionToken *conditional_token : conditional_false_tokens){
			if(conditional_token!=NULL)ss<<conditional_token->print();
		}
		return ss.str();
	}

	std::string condition_expression;
	std::vector<GrammarActionToken *> conditional_true_tokens;
	std::vector<GrammarActionToken *> conditional_false_tokens;
};

class SpatialObjectScopeToken : public GrammarActionToken {
public:
	SpatialObjectScopeToken()
		: GrammarActionToken("Object")
	{
	}

	SpatialObjectScopeToken(const SpatialObjectScopeToken *token)
		: GrammarActionToken(const_cast<SpatialObjectScopeToken *>(token))
	{
		if (token == nullptr) return;
		descriptor_syntax = token->descriptor_syntax;
		for (GrammarActionToken *body_token : token->body_tokens) {
			body_tokens.push_back(body_token != nullptr ? body_token->clone() : nullptr);
		}
	}

	~SpatialObjectScopeToken() override
	{
		for (GrammarActionToken *body_token : body_tokens) delete body_token;
		body_tokens.clear();
	}

	GrammarActionToken *clone() const override
	{
		return new SpatialObjectScopeToken(this);
	}

	std::string isRule() override { return {}; }
	std::string print() override
	{
		std::stringstream text;
		text << (descriptor_syntax ? descriptor_syntax->canonicalText() : "Object()") << " [ ";
		for (GrammarActionToken *body_token : body_tokens) {
			if (body_token != nullptr) text << body_token->print();
		}
		text << "] ";
		return text.str();
	}

	void performAction(SceneGenerationContext *context) override;

	std::shared_ptr<const SpatialObjectDescriptorSyntax> descriptor_syntax;
	std::vector<GrammarActionToken *> body_tokens;
};

class SpatialObjectBeginActionToken : public GrammarActionToken {
public:
	SpatialObjectBeginActionToken()
		: GrammarActionToken("ObjectBegin")
	{
	}

	SpatialObjectBeginActionToken(const SpatialObjectBeginActionToken *token)
		: GrammarActionToken(const_cast<SpatialObjectBeginActionToken *>(token))
	{
		if (token != nullptr) descriptor_syntax = token->descriptor_syntax;
	}

	GrammarActionToken *clone() const override
	{
		return new SpatialObjectBeginActionToken(this);
	}
	std::string isRule() override { return {}; }
	std::string print() override
	{
		return descriptor_syntax ? descriptor_syntax->canonicalText() + " " : "Object() ";
	}
	void performAction(SceneGenerationContext *context) override;

	std::shared_ptr<const SpatialObjectDescriptorSyntax> descriptor_syntax;
};

class SpatialObjectEndActionToken : public GrammarActionToken {
public:
	SpatialObjectEndActionToken()
		: GrammarActionToken("ObjectEnd")
	{
	}

	GrammarActionToken *clone() const override
	{
		return new SpatialObjectEndActionToken(*this);
	}
	std::string isRule() override { return {}; }
	std::string print() override { return "ObjectEnd "; }
	void performAction(SceneGenerationContext *context) override;
};

class SpatialInterfaceDeclarationToken : public GrammarActionToken {
public:
	SpatialInterfaceDeclarationToken()
		: GrammarActionToken("Interface")
	{
	}

	SpatialInterfaceDeclarationToken(const SpatialInterfaceDeclarationToken *token)
		: GrammarActionToken(const_cast<SpatialInterfaceDeclarationToken *>(token))
	{
		if (token != nullptr) declaration_syntax = token->declaration_syntax;
	}

	GrammarActionToken *clone() const override
	{
		return new SpatialInterfaceDeclarationToken(this);
	}
	std::string isRule() override { return {}; }
	std::string print() override
	{
		return declaration_syntax ? declaration_syntax->canonicalText() + " " : "Interface() ";
	}
	void performAction(SceneGenerationContext *context) override;

	std::shared_ptr<const SpatialInterfaceDeclarationSyntax> declaration_syntax;
};

class SpatialConnectionDeclarationToken : public GrammarActionToken {
public:
	SpatialConnectionDeclarationToken()
		: GrammarActionToken("Connect")
	{
	}

	SpatialConnectionDeclarationToken(const SpatialConnectionDeclarationToken *token)
		: GrammarActionToken(const_cast<SpatialConnectionDeclarationToken *>(token))
	{
		if (token != nullptr) declaration_syntax = token->declaration_syntax;
	}

	GrammarActionToken *clone() const override
	{
		return new SpatialConnectionDeclarationToken(this);
	}
	std::string isRule() override { return {}; }
	std::string print() override
	{
		return declaration_syntax ? declaration_syntax->canonicalText() + " " : "Connect() ";
	}
	void performAction(SceneGenerationContext *context) override;

	std::shared_ptr<const SpatialConnectionDeclarationSyntax> declaration_syntax;
};

class SpatialConstraintDeclarationToken : public GrammarActionToken {
public:
	SpatialConstraintDeclarationToken()
		: GrammarActionToken("Position")
	{
	}

	SpatialConstraintDeclarationToken(const SpatialConstraintDeclarationToken *token)
		: GrammarActionToken(const_cast<SpatialConstraintDeclarationToken *>(token))
	{
		if (token != nullptr) declaration_syntax = token->declaration_syntax;
	}

	GrammarActionToken *clone() const override
	{
		return new SpatialConstraintDeclarationToken(this);
	}
	std::string isRule() override { return {}; }
	std::string print() override
	{
		return declaration_syntax ? declaration_syntax->canonicalText() + " " : "Position() ";
	}
	void performAction(SceneGenerationContext *context) override;

	std::shared_ptr<const SpatialConstraintDeclarationSyntax> declaration_syntax;
};

class VehicleJointDeclarationToken : public GrammarActionToken {
public:
	VehicleJointDeclarationToken()
		: GrammarActionToken("Joint")
	{
	}

	VehicleJointDeclarationToken(const VehicleJointDeclarationToken *token)
		: GrammarActionToken(const_cast<VehicleJointDeclarationToken *>(token))
	{
		if (token != nullptr) declaration_syntax = token->declaration_syntax;
	}

	GrammarActionToken *clone() const override
	{
		return new VehicleJointDeclarationToken(this);
	}
	std::string isRule() override { return {}; }
	std::string print() override
	{
		return declaration_syntax ? declaration_syntax->canonicalText() + " " : "Joint() ";
	}
	void performAction(SceneGenerationContext *context) override;

	std::shared_ptr<const VehicleJointDeclarationSyntax> declaration_syntax;
};

class SceneLightDeclarationToken : public GrammarActionToken {
public:
	SceneLightDeclarationToken() : GrammarActionToken("Light") {}
	SceneLightDeclarationToken(const SceneLightDeclarationToken *token)
		: GrammarActionToken(const_cast<SceneLightDeclarationToken *>(token))
	{
		if (token != nullptr) declaration_syntax = token->declaration_syntax;
	}
	GrammarActionToken *clone() const override { return new SceneLightDeclarationToken(this); }
	std::string isRule() override { return {}; }
	std::string print() override
	{
		return declaration_syntax ? declaration_syntax->canonicalText() + " " : "Light() ";
	}
	void performAction(SceneGenerationContext *context) override;
	std::shared_ptr<const SceneLightDeclarationSyntax> declaration_syntax;
};

class LightFixtureDeclarationToken : public GrammarActionToken {
public:
	LightFixtureDeclarationToken() : GrammarActionToken("LightFixture") {}
	LightFixtureDeclarationToken(const LightFixtureDeclarationToken *token)
		: GrammarActionToken(const_cast<LightFixtureDeclarationToken *>(token))
	{
		if (token != nullptr) declaration_syntax = token->declaration_syntax;
	}
	GrammarActionToken *clone() const override { return new LightFixtureDeclarationToken(this); }
	std::string isRule() override { return {}; }
	std::string print() override
	{
		return declaration_syntax ? declaration_syntax->canonicalText() + " " : "LightFixture() ";
	}
	void performAction(SceneGenerationContext *context) override;
	std::shared_ptr<const LightFixtureDeclarationSyntax> declaration_syntax;
};

class LightSwitchDeclarationToken : public GrammarActionToken {
public:
	LightSwitchDeclarationToken() : GrammarActionToken("LightSwitch") {}
	LightSwitchDeclarationToken(const LightSwitchDeclarationToken *token)
		: GrammarActionToken(const_cast<LightSwitchDeclarationToken *>(token))
	{
		if (token != nullptr) declaration_syntax = token->declaration_syntax;
	}
	GrammarActionToken *clone() const override { return new LightSwitchDeclarationToken(this); }
	std::string isRule() override { return {}; }
	std::string print() override
	{
		return declaration_syntax ? declaration_syntax->canonicalText() + " " : "LightSwitch() ";
	}
	void performAction(SceneGenerationContext *context) override;
	std::shared_ptr<const LightSwitchDeclarationSyntax> declaration_syntax;
};

class LightingArrayDeclarationToken : public GrammarActionToken {
public:
	LightingArrayDeclarationToken() : GrammarActionToken("LightingArray") {}
	LightingArrayDeclarationToken(const LightingArrayDeclarationToken *token)
		: GrammarActionToken(const_cast<LightingArrayDeclarationToken *>(token))
	{
		if (token != nullptr) declaration_syntax = token->declaration_syntax;
	}
	GrammarActionToken *clone() const override { return new LightingArrayDeclarationToken(this); }
	std::string isRule() override { return {}; }
	std::string print() override
	{
		return declaration_syntax ? declaration_syntax->canonicalText() + " " : "LightingArray() ";
	}
	void performAction(SceneGenerationContext *context) override;
	std::shared_ptr<const LightingArrayDeclarationSyntax> declaration_syntax;
};

class LightingCircuitDeclarationToken : public GrammarActionToken {
public:
	LightingCircuitDeclarationToken() : GrammarActionToken("LightingCircuit") {}
	LightingCircuitDeclarationToken(const LightingCircuitDeclarationToken *token)
		: GrammarActionToken(const_cast<LightingCircuitDeclarationToken *>(token))
	{
		if (token != nullptr) declaration_syntax = token->declaration_syntax;
	}
	GrammarActionToken *clone() const override { return new LightingCircuitDeclarationToken(this); }
	std::string isRule() override { return {}; }
	std::string print() override
	{
		return declaration_syntax ? declaration_syntax->canonicalText() + " " : "LightingCircuit() ";
	}
	void performAction(SceneGenerationContext *context) override;
	std::shared_ptr<const LightingCircuitDeclarationSyntax> declaration_syntax;
};

class ControlsDeclarationToken : public GrammarActionToken {
public:
	ControlsDeclarationToken() : GrammarActionToken("Controls") {}
	ControlsDeclarationToken(const ControlsDeclarationToken *token)
		: GrammarActionToken(const_cast<ControlsDeclarationToken *>(token))
	{
		if (token != nullptr) declaration_syntax = token->declaration_syntax;
	}
	GrammarActionToken *clone() const override { return new ControlsDeclarationToken(this); }
	std::string isRule() override { return {}; }
	std::string print() override
	{
		return declaration_syntax ? declaration_syntax->canonicalText() + " " : "Controls() ";
	}
	void performAction(SceneGenerationContext *context) override;
	std::shared_ptr<const ControlsDeclarationSyntax> declaration_syntax;
};

class GrammarRuleDefinition : public GrammarModelElement {
	public:
	GrammarRuleDefinition(std::string rule_name,int repeat){
		this->rule_name=rule_name;
		this->repeat=repeat;
		this->count=0;
	}
	~GrammarRuleDefinition(){
		std::vector<GrammarActionToken *> unique_tokens;
		for(int section_index=0;section_index<3;section_index++){
			for(GrammarActionToken *token : section_tokens[section_index]){
				bool seen=false;
				for(GrammarActionToken *existing : unique_tokens){
					if(existing==token){
						seen=true;
						break;
					}
				}
				if(!seen){
					unique_tokens.push_back(token);
				}
			}
			section_tokens[section_index].clear();
		}
		tokens.clear();
		for(GrammarActionToken *token : unique_tokens){
			delete token;
		}
		if(alternate!=NULL){
			delete alternate;
			alternate=NULL;
		}
	}

	void addToken(GrammarActionToken *token,int section){
		
		section_tokens[section].push_back(token);
		
		if(section==1){
		  tokens.push_back(token);	
		}
		
	}
	bool increment(){
		if(repeat==0)return false;
		count++;
		if(count==repeat)return false;
		else return true;
	}
	std::string print(){
			std::stringstream ss;
			ss<<rule_name<<" ";
			if(!parameter_names.empty()){
				ss<<"( ";
				for(const std::string &parameter_name : parameter_names){
					ss<<parameter_name<<" ";
				}
				ss<<") ";
			}
			if(var_name!="")ss<<var_name<<" ";
			else ss<<repeat<<" ";
		for(int i=0;i<var_counter;i++){
			if(var_names[i]!="")ss<<var_names[i]<<" ";
		}
		if(probability<1.0f)ss<<"; "<<probability<<" ";
		ss<<"-> ";
		for(int k=0;k<3;k++){
			for(int j=0;j<section_tokens[k].size();j++){
				ss<<section_tokens[k][j]->print();
			}
			if(k==0 && section_tokens[0].size()!=0)ss<<"| ";
			if(k==1 && section_tokens[2].size()!=0)ss<<"| ";
		}
		if(alternate!=NULL){
			ss<<"-> ";
			for(int j=0;j<alternate->section_tokens[1].size();j++){
				ss<<alternate->section_tokens[1][j]->print();
			}
		}
		return ss.str();
	}
	int repeat=1,count=0;
	std::string rule_name;
	std::vector<GrammarActionToken *> tokens,section_tokens[3];
		std::string var_name;
		std::string var_names[20];
		std::vector<std::string> parameter_names;
	bool modify;
	bool divideby2;
	int var_counter=0;
	float probability=1.0f;
	GrammarRuleDefinition *alternate=NULL;
};

struct GrammarDiagnostic {
	int line = -1;
	int start_column = 0;
	int end_column = 0;
	std::string message;
};

class GrammarDocument
{
public:
	GrammarDocument() = default;
    GrammarDocument(std::string filePath);
    GrammarDocument(std::istream &input, const std::string &source_name = "");
    ~GrammarDocument();
		void loadFromFile(const std::string &filePath);
		void loadFromStream(std::istream &input, const std::string &source_name = "");
		void loadFromSourceText(const std::string &source_text, const std::string &source_name = "");
		std::string ruleAlternate(GrammarRuleDefinition *rule,std::string line, int line_number = -1, const std::vector<SourceTokenSpan> *body_token_spans = nullptr);
		std::string ruleBody(GrammarRuleDefinition *,std::istringstream &lin,std::string line, int line_number = -1, const std::string &header_text = "", const std::vector<SourceTokenSpan> *body_token_spans = nullptr);
		void update_token(GrammarActionToken *check_token);
	    void Reread();
		int findRule(std::string rule_name);
		float MathF(std::string input);
		std::string MathS(std::string input);
		std::vector<GrammarActionToken *> Recurse(GrammarRuleDefinition *rule, GrammarActionToken *call_token=NULL);
	void generateGeometry();
	void ReadTokens(GrammarRuleDefinition *rule,std::string token_str,int i, int line_number = -1);
	void ReadTokens(GrammarRuleDefinition *rule,const std::vector<SourceTokenSpan> &token_spans,int i, int line_number = -1);
	void ReadTokens2(GrammarRuleDefinition *rule,std::string rule_str,int index_k);
	void addContext();
	void ensure_start_rule_is_first();

    std::vector<GrammarRuleDefinition *> rule_list;
	std::vector<GrammarActionToken *> tokens_new;
	SceneGenerationContext *context=NULL;
	std::vector<std::string> lines;
	std::string input_name;

	// Error tracking
	std::vector<int> error_lines;
	std::vector<GrammarDiagnostic> error_details;

private:
	void clearParsedState();
	void rebuildFromLines(const std::string &success_message);
};

using Variable = GrammarVariableDefinition;
using Token = GrammarActionToken;
using Rule = GrammarRuleDefinition;
using Grammar = GrammarDocument;
