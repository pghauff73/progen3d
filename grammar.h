
class Context; // forward declaration to avoid include-order problems

class Variable{
	public:
	std::string var_name;
	float value;
	int instance_count=0;
	Variable(std::string name,float min,float max,bool i);
	Variable(std::string name,float value){
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
};
class Token{
	public:
	Token(Token *t){
		this->token_name=t->token_name;
		for(int i=0;i<t->arguments.size();i++){
			this->arguments.push_back(t->arguments[i]);
		}
		this->var_names[0]=t->var_names[0];
		this->var_names[1]=t->var_names[1];
		this->var_names[2]=t->var_names[2];
		this->var_name=t->var_name;
		
		this->instance_type=t->instance_type;
		
	}
	Token(std::string token_name){
		this->token_name=token_name;
	}
	Token(std::string token_name,int instance_count){
		this->token_name=token_name;
		this->instance_count=instance_count;
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
	std::string isRule(){
		
		if(token_name!="{" && token_name!="}" && token_name!="+" && token_name!="*" && token_name!="[" && token_name!="]" && instance_type=="" && arguments.size()==0)return token_name;
		else return "";
	}
	std::string print(){
		std::stringstream ss;
		
		ss.precision(5);
		if(token_name=="R" && integer==true)ss<<token_name<<"* ";
		else ss<<token_name<<" ";
		if(token_name=="[" || token_name=="]" || isRule()!="")return ss.str();
		if(var_name!="")ss<<var_name<<" ";
		ss<<"( ";
		if(instance_type!=""){
			ss<<instance_type<<" ";
		}
		if(var_names[0]!=""){
		   ss<<var_names[0]<<" ";
		}
		else {
		std::stringstream ss2;	
			ss2<<std::fixed<<arguments[0]<<" ";
			std::string str1=ss2.str();
			int pos=str1.find(".00000");
			if(pos!=-1){
				str1.erase(str1.begin()+pos,str1.begin()+pos+6+1);
			}
			ss<<str1+" ";
		}
		
		if(var_names[1]!=""){
	         ss<<var_names[1]<<" "; 	
		}
		else {
		std::stringstream ss2;	
			ss2<<std::fixed<<arguments[1]<<" ";
			std::string str1=ss2.str();
			int pos=str1.find(".00000");
			if(pos!=-1){
				str1.erase(str1.begin()+pos,str1.begin()+pos+6+1);
			}
			ss<<str1+" ";
		}
		if(arguments.size()>2){
			if(var_names[2]!=""){
				ss<<var_names[2]<<" "; 	
			}
			else {
			std::stringstream ss2;	
			ss2<<std::fixed<<arguments[2]<<" ";
			std::string str1=ss2.str();
			int pos=str1.find(".00000");
			if(pos!=-1){
				str1.erase(str1.begin()+pos,str1.begin()+pos+6+1);
			}
			ss<<str1+" ";
				
				
			}
		}
		ss<<") ";
		return ss.str();
	}
	
	void performAction(Context *context);
	
	
	std::string token_name;
	std::vector<float> arguments;
	std::string instance_type;
	int instance_count=0;
	std::string var_name;
	std::string var_names[3];
	bool modify[3];
	bool divideby2[3];
	bool integer=false;
};

class Rule {
	public:
	Rule(std::string rule_name,int repeat){
		this->rule_name=rule_name;
		this->repeat=repeat;
		this->count=0;
	}

	void addToken(Token *token,int section){
		
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
	std::vector<Token *> tokens,section_tokens[3];
	std::string var_name;
	std::string var_names[20];
	bool modify;
	bool divideby2;
	int var_counter=0;
	float probability=1.0f;
	Rule *alternate=NULL;
};

class Grammar
{
public:
    Grammar(std::string filePath);
    ~Grammar();
	std::string ruleAlternate(Rule *rule,std::string line);
	std::string ruleBody(Rule *,std::istringstream &lin,std::string line);
	void update_token(Token *check_token);
    void Reread();
	int findRule(std::string rule_name);
	std::string MathS(std::string input);
	std::vector<Token *> Recurse(Rule *rule);
	void generateGeometry();
	void ReadTokens(Rule *rule,std::string token_str,int i);
	void ReadTokens2(Rule *rule,std::string rule_str,int index_k);
	void addContext();

    std::vector<Rule *> rule_list;
	std::vector<Token *> tokens_new;
	Context *context=NULL;
	std::vector<std::string> lines;
};












