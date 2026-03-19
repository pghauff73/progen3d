
class Variable{
	public:
	std::string var_name;
	float value;
	int instance_count=0;
	Variable(std::string name,float min,float max){
		this->var_name=name;
		this->min=min;
		this->max=max;
		this->value=rand()/(float)RAND_MAX*(max-min)+min;
	}
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
	float getRandom(){
		
		return rand()/(float)RAND_MAX*(max-min)+min;
	}
	float min,max;
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
	void print(){
		std::cout<<"Token: "<<token_name<<" -> ";
		if(var_name!="")std::cout<<var_name<<":";
		if(arguments.size()>0){
			std::cout<<" ( ";
			for(int i=0;i<arguments.size();i++){
			std::cout<<arguments[i]<<" ";
			}
			std::cout<<" ) ";
		}
		if(var_names[0]!=""){
		
		}
		if(var_names[1]!=""){
	            	
		}
		if(var_names[2]!=""){
		
		}
		
		std::cout<<"["<<var_names[0]<<" "<<var_names[1]<<" "<<var_names[2]<<"]";
		if(instance_type!=""){
			std::cout<<instance_type<<":"<<instance_type;
		}
		std::cout<<std::endl;
		
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
	
	int repeat=1,count=0;
	std::string rule_name;
	std::vector<Token *> tokens,section_tokens[3];
	std::string var_name;
	std::string var_names[10];
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
    
	int findRule(std::string rule_name);
	std::string MathS(std::string input);
	std::vector<Token *> Recurse(Rule *rule);
	void generateGeometry();
	void ReadTokens(Rule *rule,std::string token_str,int i);
	void addContext();

    std::vector<Rule *> rule_list;
	std::vector<Token *> tokens_new;
	Context *context;
};












