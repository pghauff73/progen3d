#include <fstream>
#include <iostream>
#include <sstream>
#include <iostream>
#include <list>
#include <string>
#include <unordered_map>
#include <stack>
#include <vector>
#include <random>
#include <chrono>

#include "PLYWriter.h"
#include "Context.h"
#include "Scope.h"
#include "grammar.h"
#include "Solution.h"





extern void errorout(std::string error_str);
std::vector<Variable *> variable_list;
std::vector<Variable *> full_variable_list;


extern std::mt19937_64 rng;

std::string removeSpaces(std::string word) {
    std::string newWord;
    for (int i = 0; i < word.length(); i++) {
        if (word[i] != ' ') {
            newWord += word[i];
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
    for(int i = 0; i < str.size(); i++)
        if( !isalpha(str[i]) || !isspace(str[i]) )
            return true;
    return false;
}
bool isnumber(const std::string& s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && ( (*it)=='-' || (*it)=='.' || std::isdigit(*it)) ) ++it;
    return !s.empty() && it == s.end();
}





Variable::Variable(std::string name,float min,float max,bool i){
		this->var_name=name;
		this->min=min;
		this->max=max;
		std::uniform_real_distribution<double> unif(0, 1);
		if(i)
			this->value=floorf(unif(rng)*(max-min)+min);
		else
			this->value=unif(rng)*(max-min)+min;
		this->integer=i;
	}
float Variable::getRandom(){
	std::uniform_real_distribution<double> unif(0, 1);
		if(integer)
		return floorf(unif(rng)*(max-min)+min);
		else return unif(rng)*(max-min)+min;
	}

    
    
int findVariable(std::string var_name){
	//////std::cout<<<<"Searching Back for "<<var_name<<std::endl;
	for(int i=variable_list.size()-1;i>=0;i--){
	
		if(variable_list[i]->var_name.compare(var_name)==0) {
	
			//////std::cout<<<<"Found "<<var_name<<"="<<variable_list[i]->value<<std::endl;
		return i;
		}
	
	}
	return -1;
}
int findVariableForward(std::string var_name){
	//////std::cout<<<<"Searching Forward for "<<var_name<<std::endl;
	for(int i=0;i<variable_list.size();i++){
	
		if(variable_list[i]->var_name.compare(var_name)==0) {
	
			//////std::cout<<<<"Found "<<var_name<<"="<<variable_list[i]->value<<std::endl;
		return i;
		}
	
	}
	
	return -1;
}

void removeVariable(std::string var_name){
	
	
	for(int i=variable_list.size()-1;i>=0;i--){
		if(variable_list[i]->var_name.compare(var_name)==0) {
			if(variable_list[i]->max==variable_list[i]->min){
				variable_list.erase (variable_list.begin()+i);
				return;
			}
		}
		
	}
	
}


    
void addVariable(std::string var_name,float min,float max,bool integer){
	
	
	int index=findVariableForward(var_name);
	if(index==-1){
		Variable *var=new Variable(var_name,min,max,integer);
		var->value=var->getRandom();
		variable_list.push_back(var);
		full_variable_list.push_back(var);
	}
	
	
}

void addVariable(std::string var_name){
	
	//////std::cout<<<<"Adding variable "<<var_name;
	int index=findVariableForward(var_name);
	if(index!=-1){
	
		Variable *var=new Variable(var_name,variable_list[index]->getRandom());
		var->instance_count=variable_list[index]->instance_count+1;
		variable_list.push_back(var);
		full_variable_list.push_back(var);
		//////std::cout<<<<" SUCCESS"<<var->instance_count<<std::endl;
	}
	//else 
		//////std::cout<<<<" FAILED"<<std::endl;
}
    
    




std::string MathS2(std::string input){
	Solution X;
	//return std::to_string(X.Solve(input));
	
	
		//mathS::Assembler assembler;
				////std::cout<<<<"IN:"<<input<<";";
			bool remove_var=false;
			//replace variables with floats
		for(int i=variable_list.size()-1;i>=0;i--){
			std::string str2 = variable_list[i]->var_name;
					int pos=0;
		      while((pos=input.find(str2))!=-1){
				//  //std::cout<<<<"===="<<str2;
				  remove_var=true;
				  input.replace(pos,str2.length(),std::to_string(variable_list[i]->value));	
				
				 
			  }
			   if(remove_var){
				   removeVariable(str2);
				   remove_var=false;
			   }
		}
		
	return std::to_string(X.Process(input));	
		/*
		 * 
std::string replacevars(std::string input,bool remove_var){
	
		std::string output="";
		
	    std::string ops[7];
		ops[0]="+";
	    ops[1]="*";
	    ops[2]="/";
	    ops[3]="(";
	    ops[4]=")";
	    ops[5]="^";
	    ops[6]="-";
	    std::vector<std::pair<int,int>> pos_ops_list;
	    
		for(int i=0;i<7;i++){
			int pos;
			
			while((pos=input.find(ops[i]))!=-1){
				input.replace(pos,1," ");
				//std::cout<<<<input<<":"<<pos<<":"<<ops[i]<<std::endl;
				pos_ops_list.push_back(std::make_pair(pos,i));
			}
		}
		  if(pos_ops_list.size()==0){
		//std::cout<<<<":"<<input<<":"<<std::endl;
			  for(int i=variable_list.size()-1;i>=0;i--){
					if(variable_list[i]->max==variable_list[i]->min){
						std::string str2 = variable_list[i]->var_name;
						std::string str1=removeSpaces(input);
						if(str1==str2){
							output=std::to_string(variable_list[i]->value);
							if(remove_var)removeVariable(str2);
						}
					}
			  }
		  }
		  else {
			  std::sort(pos_ops_list.begin(), pos_ops_list.end());
		
			std::string varstr;
			std::vector<std::string> lines=breakup(input," ");
			int counter=0;
			if(pos_ops_list[counter].first==0){
					output+=ops[pos_ops_list[counter++].second];
			}
			
			for(int j=0;j<lines.size();j++){
			
					bool matched=false;
					for(int i=variable_list.size()-1;i>=0;i--){
						if(variable_list[i]->max==variable_list[i]->min){
							std::string str2 = variable_list[i]->var_name;
							std::string str1=removeSpaces(lines[j]);
							if(str1==str2){
								output+=std::to_string(variable_list[i]->value);
								if(counter<pos_ops_list.size())output+=ops[pos_ops_list[counter++].second];
								matched=true;
								if(remove_var)removeVariable(str2);
								break;
							}
						}
					}
					if(matched==false){
						output+=removeSpaces(lines[j]);
						if(counter<pos_ops_list.size())output+=ops[pos_ops_list[counter++].second];
				}
				
			}
		
	}
		//std::cout<<<<output<<std::endl;
	return output;
}


std::string MathS2(std::string input){
	Solution X;

	return std::to_string(X.Process(replacevars(input,true)));	

}
std::string Grammar::MathS(std::string input){
	Solution X;
		
		return std::to_string(X.Process(replacevars(input,false)));	
		
	
}

		std::vector<std::string> mul_div;
		std::vector<int> sign_addition;
		std::vector<std::string> addition=breakup(input,"+");
		for(int i=0;i<addition.size();i++){
			std::vector<std::string> minus=breakup(addition[i],"-");
			for(int j=0;j<minus.size();j++){
				
				if(j>0){
					mul_div.push_back(minus[j]);
					sign_addition.push_back(-1);
				}
				else {
					mul_div.push_back(minus[j]);
					sign_addition.push_back(1);
				}
			}
		}
		float sum=0.0f;
		for(int i=0;i<mul_div.size();i++){
			std::vector<std::string> mul=breakup(mul_div[i],"*");
			float out=1.0f*(float)sign_addition[i];
			for(int j=0;j<mul.size();j++){
				std::vector<std::string> div=breakup(mul[j],"/");
				if(div.size()==2)out*=atof(div[0].c_str())/atof(div[1].c_str());
				else out*=atof(div[0].c_str());
						 //std::cout<<<<out<<"+";
				}
			sum+=out;
			}
		
		
		////std::cout<<<<"OUT="<<sum<<std::endl;
		
		
	
	std::string out_str=std::to_string(sum);
	if(isnumber(out_str))
		return out_str;
	else
	return input;*/
}

std::string replacevars(std::string input){
	for(int i=variable_list.size()-1;i>=0;i--){
			if(variable_list[i]->max==variable_list[i]->min){
			std::string str2 = variable_list[i]->var_name;
					
		      if(input.find(str2)!=-1){
				 
				  input.replace(input.find(str2),str2.length(),std::to_string(variable_list[i]->value));	
				 break;
			  }
		  }
		}
	return input;
}

std::string replacevars_ampersand(std::string input){
	for(int i=0;i<variable_list.size();i++){
			
			std::string str2 = variable_list[i]->var_name;
					
			if(input.find("&"+str2)!=-1){
				 
				  input.replace(input.find("&"+str2),str2.length()+1,std::to_string(variable_list[i]->getRandom()));	
				 break;
			  }
		
		}
	return input;
}



std::string Grammar::MathS(std::string input){
	Solution X;
		
		return std::to_string(X.Process(replacevars_ampersand(replacevars(replacevars(replacevars(replacevars(input)))))));	
		
	
}



void Token::performAction(Context *context){
	
	if(token_name=="R"){
		
		addVariable(var_name,arguments[0],arguments[1],integer);
		
	}
	else if(token_name=="S" ){
		
		
		Scope *newScope = context->getCurrentScope();
		
		//////std::cout<<<<"***001";
		glm::vec3 size;
		
		if(var_names[0]!=""){
				
				size.x=atof(MathS2(var_names[0]).c_str());
				
				
		}
		else size.x=arguments[0];
		
		if(var_names[1]!=""){
				size.y=atof(MathS2(var_names[1]).c_str());
		}
		else size.y=arguments[1];
		
		if(var_names[2]!=""){
				
				size.z=atof(MathS2(var_names[2]).c_str());
		}
		else size.z=arguments[2];
		
		//////std::cout<< << "S (" << size.x << ", " << size.y << ", " << size.z << ") ";
		
		//size *= newScope->getSize();
		//size = glm::vec3(abs(size.x), abs(size.y), abs(size.z));
		
		//////std::cout<< << "S* (" << size.x << ", " << size.y << ", " << size.z << ") "<<std::endl;
		newScope->S(size);
		glm::vec3 pos = context->getCurrentScope()->getPosition();
    glm::vec3 contextSize = context->getCurrentScope()->getSize();
    //////std::cout<< << " -- Current scope -> POS: (" << pos.x << ", " << pos.y << ", " << pos.z << ") SIZE: (" << contextSize.x << ", " << contextSize.y << ", " << contextSize.z << ") " << std::endl;
		
	}
	else if(token_name=="D" ){
		
		
		Scope *newScope = context->getCurrentScope();
		
		//////std::cout<<<<"***001";
		glm::vec3 size;
		
		if(var_names[0]!=""){
				
				size.x=atof(MathS2(var_names[0]).c_str());
				
				
		}
		else size.x=arguments[0];
		
		if(var_names[1]!=""){
				size.y=atof(MathS2(var_names[1]).c_str());
		}
		else size.y=arguments[1];
		
		if(var_names[2]!=""){
				
				size.z=atof(MathS2(var_names[2]).c_str());
		}
		else size.z=arguments[2];
		
		//////std::cout<< << "S (" << size.x << ", " << size.y << ", " << size.z << ") ";
		
		//size *= newScope->getSize();
		//size = glm::vec3(abs(size.x), abs(size.y), abs(size.z));
		
		//////std::cout<< << "S* (" << size.x << ", " << size.y << ", " << size.z << ") "<<std::endl;
		newScope->D(size);
		//glm::vec3 pos = context->getCurrentScope()->getPosition();
    //glm::vec3 contextSize = context->getCurrentScope()->getSize();
    //////std::cout<< << " -- Current scope -> POS: (" << pos.x << ", " << pos.y << ", " << pos.z << ") SIZE: (" << contextSize.x << ", " << contextSize.y << ", " << contextSize.z << ") " << std::endl;
		
	}
	else if(token_name=="T"){
		Scope *newScope = context->getCurrentScope();
		glm::vec3 position;
		
		if(var_names[0]!=""){
			position.x=atof(MathS2(var_names[0]).c_str());
				
		}
		else position.x=arguments[0];
		
		if(var_names[1]!=""){
				position.y=atof(MathS2(var_names[1]).c_str());
		}
		else position.y=arguments[1];
		
		if(var_names[2]!=""){
				position.z=atof(MathS2(var_names[2]).c_str());
		}
		else position.z=arguments[2];
		
		
				
		newScope->T(position);
		
	}
	else if(token_name=="A" ){
		
		
		Scope *newScope = context->getCurrentScope();
		
		//////std::cout<<<<"***001";
		float angle,axis;
		
		if(var_names[0]!=""){
				
				angle=atof(MathS2(var_names[0]).c_str());
				
				
		}
		else angle=arguments[0];
		
		if(var_names[1]!=""){
				axis=atof(MathS2(var_names[1]).c_str());
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
	    glm::mat4 transform = context->getCurrentScope()->getTransform();
		Mesh instance = Mesh::getInstance(instance_type);
		instance.apply(transform);
		int texindex=0;
		
		if(var_names[0]!=""){
			texindex=(int)atof(MathS2(var_name).c_str());
			////std::cout<<<<"MathS2: "<<texindex<<" "<<std::endl;
			}
			else texindex=(int)arguments[0];
		int arg=0;
		float val=0.125f;
		if(arguments.size()>1)arg=(int)arguments[1];
		if(arguments.size()>2)val=arguments[2];
		context->addPrimitive(instance_type,context->getCurrentScope(),texindex,arg,val);
		context->getScene().add(instance);
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


void Grammar::ReadTokens2(Rule *rule,std::string rule_str,int index_k){
			float value;
			for(char& c : rule_str) {
					if(c=='['){
							int pos1=-1,pos2=-1;
							if((pos1 = rule_str.find("["))!=-1){
								if((pos2 = rule_str.find("]"))!=-1){
									rule->addToken(new Token("["),index_k);
									ReadTokens(rule,rule_str.substr(pos1+1,pos2-1),index_k);
									rule->addToken(new Token("]"),index_k);
									ReadTokens(rule,rule_str.substr(pos2+1,rule_str.length()),index_k);
									return;
								}
								else return;
							}
							else return;
					}
					else if(c=='S' || c=='T'){
						int pos1=-1,pos2=-1;
							if((pos1 = rule_str.find("("))!=-1){
								if((pos2 = rule_str.find(")"))!=-1){
									std::vector<std::string> var_list=breakup(rule_str.substr(pos1+1,pos2-1)," ");
									if(var_list.size()!=3)return;
									Token *token;
										if(c=='S')token=new Token("S");
										else token=new Token("T");
									for(int i=0;i<3;i++){
										if(!isnumber(var_list[i])){
											token->var_names[i]=var_list[i];
											 value=0.0f;
										}
										else {
											token->var_names[i]="";
											value=atof(var_list[i].c_str());
										}
										token->addArgument(value);
									}
									rule->addToken(token,index_k);
									ReadTokens(rule,rule_str.substr(pos2+1,rule_str.length()),index_k);
									return;
								}
								else return;
							}
							else return;
						
					}
					else if(c=='R'){
						int pos1=-1,pos2=-1,pos=-1;
							Token *token;
							token=new Token("R");
							if((pos=rule_str.find("R*"))!=-1)
									token->integer=true;
							else if((pos=rule_str.find("R "))==-1)return;
							
							if((pos1 = rule_str.find("("))!=-1){
								if((pos2 = rule_str.find(")"))!=-1){
									std::vector<std::string> var_list=breakup(rule_str.substr(pos1+1,pos2-1)," ");
									if(var_list.size()!=2)return;
									token->var_name=removeSpaces(rule_str.substr(pos+2,pos1-1));
									for(int i=0;i<2;i++){
											value=atof(var_list[i].c_str());
											token->addArgument(value);
									}
									addVariable(token->var_name,token->arguments[0],token->arguments[1],token->integer);
									rule->addToken(token,index_k);
									ReadTokens(rule,rule_str.substr(pos2+1,rule_str.length()),index_k);
									return;
								}
								else return;
							}
							else return;
						
					}					
					else if(c=='I'){
						int pos1=-1,pos2=-1;
						Token *token;
						if((pos1 = rule_str.find("("))!=-1){
								if((pos2 = rule_str.find(")"))!=-1){
									std::vector<std::string> var_list=breakup(rule_str.substr(pos1+1,pos2-1)," ");
									if(!(var_list.size()>=2 && var_list.size()<=3))return;
									token->addInstanceType(removeSpaces(var_list[0]));
									for(int i=1;i<var_list.size();i++){
											value=atof(var_list[i].c_str());
											token->addArgument(value);
									}
									rule->addToken(token,index_k);
									ReadTokens(rule,rule_str.substr(pos2+1,rule_str.length()),index_k);
									return;
								}
								else return;
						}
						else return;
					}
			}
			
			
			
}



void Grammar::ReadTokens(Rule *rule,std::string rule_str,int index_k){
	try{
	std::istringstream lin(rule_str);
			
			////std::cout<<<<"("<<index_k<<"): ";
			
			std::string token_str;
			float value;
			
			while(lin>>token_str){
				////std::cout<<<<token_str;
				Token *token;
				if(token_str=="S" || token_str=="T" || token_str=="D" ){
					token=new Token(token_str);
					
					lin>>token_str;
					if(token_str=="("){
						
						
						
						for(int i=0;i<3;i++){
							lin>>token_str;
							
							////std::cout<<<<" "<<token_str;
							
							if(!isnumber(token_str)){
								
								token->var_names[i]=token_str;
							     value=0.0f;
							}
							else {
								token->var_names[i]="";
								value=atof(token_str.c_str());
							}
							token->addArgument(value);
						}
						lin>>token_str;
						if(token_str!=")"){
							////std::cout<<<<"Error-expected ')' 001 "<<std::endl;
							throw(1);
							}
					rule->addToken(token,index_k);
					}
					else {
						////std::cout<<<<"Error-expected '(' 001"<<std::endl;
						throw(1);
						
					}
				}
				else if(token_str=="R" || token_str=="R*"){
					
					if(token_str=="R*"){
					
					token=new Token("R");
					token->integer=true;
					
					}
					else {
					token=new Token(token_str);
					}
					
					lin>>token_str;
				
					token->var_name=token_str;
					
					////std::cout<<<<" "<<token_str;
					
					
					lin>>token_str;
					
					if(token_str=="("){
						
						for(int i=0;i<2;i++){
							lin>>token_str;
							
							value=atof(token_str.c_str());
							token->addArgument(value);
						}
						
						lin>>token_str;
						if(token_str!=")"){
							////std::cout<<<<"Error-expected ')' 002 "<<std::endl;
							throw(1);
						}
					
					
		
					addVariable(token->var_name,token->arguments[0],token->arguments[1],token->integer);
					
					rule->addToken(token,index_k);
					}
					else {
						////std::cout<<<<"Error-expected '(' 002"<<std::endl;
						throw(1);
					}
				}
				else if(token_str=="I"){
					token=new Token(token_str);
					lin>>token_str;
					if(token_str=="("){
						lin>>token_str;
						if(token_str=="Cube" || token_str=="Sphere" || token_str=="Cylinder" || token_str=="CubeX" ||token_str=="CubeY" ){
							
							token->addInstanceType(token_str);
						}
						else {
							////std::cout<<<<"Error-expected ')' 003 "<<std::endl;
							throw(1);
						}
						
						
						
						lin>>token_str;
						if(token_str!=")"){
						//	//std::cout<<<<"token_str: "<<token_str<<" ";
							if(!isnumber(token_str)){
								
								token->var_names[0]=token_str;
							     value=0.0f;
							}
							else {
								token->var_names[0]="";
								value=atof(token_str.c_str());
							}
							token->addArgument(value);
							     lin>>token_str;
						}
						
				
						if(token_str!=")"){
							//std::cout<<<<"token_str: "<<token_str<<" ";
							//token->var_name=token_str;
							     value=atof(token_str.c_str());
							     token->addArgument(value);
							     lin>>token_str;
						}
						
						if(token_str!=")"){
							//std::cout<<<<"token_str: "<<token_str<<" ";
							//token->var_name=token_str;
							     value=atof(token_str.c_str());
							     token->addArgument(value);
							     lin>>token_str;
						}	
								
						
						if(token_str==")"){
							rule->addToken(token,index_k);
						
						}
						else {
							////std::cout<<<<"Error-expected ')' 003 "<<std::endl;
							throw(1);
						}
						
			
					}
					else {
						////std::cout<<<<"Missing ("<<std::endl;exit(1);
						throw(1);
						}
					
				}
				else if(token_str=="*"){
					//token=new Token(token_str);
					//lin>>token->var_name;
					
					
					
					//rule->addToken(token,index_k);					
				}
				else if(token_str=="A"){
					
					token=new Token(token_str);
					
					lin>>token_str;
					if(token_str=="("){
						
						
						
						for(int i=0;i<2;i++){
							lin>>token_str;
							
							////std::cout<<<<" "<<token_str;
							
							if(!isnumber(token_str)){
								
								token->var_names[i]=token_str;
							     value=0.0f;
							}
							else {
								token->var_names[i]="";
								value=atof(token_str.c_str());
							}
							token->addArgument(value);
						}
						lin>>token_str;
						if(token_str!=")"){
							////std::cout<<<<"Error-expected ')' 001 "<<std::endl;
							throw(1);
							}
					rule->addToken(token,index_k);
					}
					else {
						////std::cout<<<<"Error-expected '(' 001"<<std::endl;
						throw(1);
						
					}			
				}
				else if(token_str=="["){
					token=new Token(token_str);
					rule->addToken(token,index_k);
					
				}
				else if(token_str=="]"){
					token=new Token(token_str);
					rule->addToken(token,index_k);
				}
				else if(token_str=="{"){
					//token=new Token(token_str);
					//rule->addToken(token,index_k);
					
				}
				else if(token_str=="}"){
					//token=new Token(token_str);
					//rule->addToken(token,index_k);
				}
				else {
					
					token=new Token(token_str);
					rule->addToken(token,index_k);
					
						
					
					
				}
			
			//////std::cout<<<<"////std::cout<<<<" "<<token_str;section("<<index_k<<"):"<<token->token_name<<std::endl;	
				
			
			}
}catch(int e_number){
	
	std::string error_str="Token Error "+std::to_string(e_number);
	errorout(error_str);
	
}
}



std::string Grammar::ruleBody(Rule *rule,std::istringstream &lin,std::string line){
		int pos = 0;
		std::string token_str;
		float value;
			
		
				
		lin >> token_str;
		
		
		if(token_str==";"){
				lin >> token_str;
				
				if(isnumber(token_str)){
					rule->probability=atof(token_str.c_str());
				////std::cout<<<<"PROBABILITY:"<<rule->probability<<std::endl;
				}
				//else {////std::cout<<<<"not number must be"<<std::endl;exit(1);}
		}
		else {
		
			if(token_str!="->"){
				if(!isnumber(token_str)){
						rule->var_name=token_str;
						//repeat=1;
				}
				else {
						rule->repeat=atoi(token_str.c_str());
				}
				
			 }
			
			 do
			 {
				lin >> token_str;
			
				 if(token_str==";" || token_str=="->")break;
					
				  if(findVariableForward(token_str)!=-1){
						rule->var_names[rule->var_counter]=token_str;
						rule->var_counter++;
					}
				
					//	////std::cout<<<<token_str;
			 }
			while(true);

			if(token_str==";"){
					lin >> token_str;
					
					if(isnumber(token_str)){
						rule->probability=atof(token_str.c_str());
					////std::cout<<<<"PROBABILITY:"<<rule->probability<<std::endl;
					}
					//else {////std::cout<<<<"not number must be"<<std::endl;exit(1);}
			}
		
		}
		
		
		//Sections
		
		
		
		

		
		
		
		
		rule_list.push_back(rule);
		std::string delimiter = "|";
		std::vector<std::string> sections;
		
		
		sections=breakup(line,delimiter);
		
		int num_sections=sections.size();
		
		if( line[line.length()-1]=='|'){
			num_sections--;//std::cout<<<<"CORRECTION | trailing"<<std::endl;
		}
		 
		 if(num_sections>3){
			 //std::cout<<<<"Too many sections "<<num_sections<<std::endl;
			 return "";
		 }
		////std::cout<<<<"sections: "<<num_sections<<"{"<<line<<"}"<<std::endl;
		
		if(num_sections==1){
			ReadTokens(rule,line,1);
		}
		else {
			for(int i=0;i<num_sections;i++){
							ReadTokens(rule,sections[i],i);
			}
		
		}
		

				
	
			
				
		
		
		return line;
	
}

std::string Grammar::ruleAlternate(Rule *rule,std::string line){
			
			
			char token_char;
			std::string token_str;
			float value;
			
			rule->alternate=new Rule(rule->rule_name+"_Alt",1);
			rule->alternate->probability=1.0f-rule->probability;
			
			

			
			ReadTokens(rule->alternate,line,1);
			
			
		
		return "";
	
}




Grammar::Grammar(std::string filePath)
{

    std::ifstream fin(filePath);
    if (!fin.is_open()) {
		std::stringstream ss;
		
        ss << "E: Could not open file " << filePath << std::endl;
        errorout(ss.str());
        return;
    }

    std::string line;
    while(std::getline(fin, line) ) {
        lines.push_back(line);
        if(line!="" && line[0]!='#'){
			
			std::vector<std::string> rule_sections;

			std::string delimiter = "->";
			
			rule_sections=breakup(line,delimiter);

			


			
			
			std::istringstream lin(rule_sections[0]+" ->");
			std::string rulename;
			lin >> rulename;
			////std::cout<<<<"Rule name:"<<rulename<<" "<<line<<std::endl;
			
				Rule *rule=new Rule(rulename,1);
			
				line=ruleBody(rule,lin,rule_sections[1]);
			
				if(rule_sections.size()>2)ruleAlternate(rule,rule_sections[2]);
			
			
		}
		
	}
	
	
	

		
		
		tokens_new=Recurse(rule_list[0]);
		
	std::stringstream ss;
	ss<<"Finshed reading Grammar file..."<<std::endl;
	errorout(ss.str());
		
	
	
	
	
}


void Grammar::Reread()
{
   try {
   
   for(int i=0;i<tokens_new.size();i++){
   delete tokens_new[i];
}
for(int i=0;i<rule_list.size();i++){
   delete rule_list[i];
   
}

tokens_new.clear();
rule_list.clear();
variable_list.clear();
full_variable_list.clear();
    
    std::string line;
    for(int i=0;i<lines.size();i++){
        line=lines[i];
        if(line!="" && line[0]!='#'){
			
			std::vector<std::string> rule_sections;

			std::string delimiter = "->";
			
			rule_sections=breakup(line,delimiter);

			


			
			
			std::istringstream lin(rule_sections[0]+" ->");
			std::string rulename;
			lin >> rulename;
			////std::cout<<<<"Rule name:"<<rulename<<" "<<line<<std::endl;
			
				Rule *rule=new Rule(rulename,1);
			
				line=ruleBody(rule,lin,rule_sections[1]);
			
				if(rule_sections.size()>2)ruleAlternate(rule,rule_sections[2]);
			
			
		}
		
	}
	
	
	

		
		
		tokens_new=Recurse(rule_list[0]);
		
	
	errorout("Finshed Re-reading Grammar file...");
		
	}
	
	catch(...){
		errorout("caught exeception reread grammar");
	}
	
	
	
}


void Grammar::update_token(Token *check_token){

						if( check_token->token_name=="D" || check_token->token_name=="S" || check_token->token_name=="T"){ //replace variable with value
										
						for(int i=0;i<3;i++){
								if(check_token->var_names[i]!=""){
													
										check_token->arguments[i]=atof(MathS(check_token->var_names[i]).c_str());
													
										check_token->var_names[i]="";
									}
						}
						}
						if( check_token->token_name=="A" ){ //replace variable with value
										
						for(int i=0;i<2;i++){
								if(check_token->var_names[i]!=""){
													
										check_token->arguments[i]=atof(MathS(check_token->var_names[i]).c_str());
													
										check_token->var_names[i]="";
									}
						}
						}
						if(check_token->token_name=="I" ){
							if(check_token->var_names[0]!=""){
													
										check_token->arguments[0]=atof(MathS(check_token->var_names[0]).c_str());
													
										check_token->var_names[0]="";
									}
							
						}

}



std::vector<Token *> Grammar::Recurse(Rule *rule){
	
		try{
		//////std::cout<<<<"Rule Name: "<<rule->rule_name;
		
		std::vector<Token *> new_tokens;
		for(int i=0;i<rule->var_counter;i++){
			
			int index=findVariableForward(rule->var_names[i]);
		
			addVariable(variable_list[index]->var_name);
		}
		
		//////std::cout<<<<"Rule Var Name: "<<rule->var_name;
		
		
		//int index=findVariable
		if(rule->var_name!=""){
		
		

				//float val=variable_list[index]->getValue();
				rule->repeat=atof(MathS(rule->var_name).c_str());//floorf(val);	
			
			
			
		}
		
		float roll=1.1f;
		if(rule->alternate!=NULL){
			std::uniform_real_distribution<double> unif(0, 1);
			roll=unif(rng);//rand()/(float)RAND_MAX;
		
		////std::cout<<<<"Roll<<roll<<std::endl;
		if(rule->probability>roll){
			
			std::vector<Token *> more_tokens=Recurse(rule->alternate);
		
				for(int k=0;k<more_tokens.size();k++){
					update_token(more_tokens[k]);		
					new_tokens.push_back(more_tokens[k]);
		
				}
				
		}
		else {
	    Variable *var=new Variable(rule->rule_name+"_count",0,0,true);
		variable_list.push_back(var);
		for(int l=0;l<rule->repeat;l++){
						var->value=l;
		                ////std::cout<<<<"l_"<<l<<";"<<std::endl;
						for(int j=0;j<rule->section_tokens[1].size();j++){
							////std::cout<<<<"j_"<<j<<";";	
							if(rule->section_tokens[1][j]->isRule()!=""){
								   // ////std::cout<<<<"RULE;";
								int index=findRule(rule->section_tokens[1][j]->token_name);
								if(index!=-1){
									Rule *fromtoken = rule_list[index];
									
									//////std::cout<<<<"PROB"<<prob<<">"<<fromtoken->probability<<std::endl;
									
										//////std::cout<<<<"HERE009";
										std::vector<Token *> more_tokens=Recurse(fromtoken);
										//////std::cout<<<<"HERE007";
										for(int k=0;k<more_tokens.size();k++){
											update_token(more_tokens[k]);
											new_tokens.push_back(more_tokens[k]);
										}
									}
									////std::cout<<<<"HERE008";
							  
						////std::cout<<<<"HERE0010";
							}
							else {
								    ////std::cout<<<<"notRULE";
								Token *check_token=new Token(rule->section_tokens[1][j]);
								update_token(check_token);
								new_tokens.push_back(check_token);
							}
							
						////std::cout<<<<"HERE0011";	
						}
					
				}
		removeVariable(rule->rule_name+"_count");	
		}
		
		return new_tokens;
	}
	
	
	
	
	
	
	
	
	
	
		//////std::cout<<<<"REPEAT="<<rule->repeat<<std::endl;
			
		for(int m=0;m<3;m++){
		
				////std::cout<<<<"m_"<<m<<std::endl<<std::endl;
				if(m==1){
					Variable *var=new Variable(rule->rule_name+"_count",0,0,true);
					variable_list.push_back(var);
					for(int l=0;l<rule->repeat;l++){
						var->value=l;
		                ////std::cout<<<<"l_"<<l<<";"<<std::endl;
						for(int j=0;j<rule->section_tokens[m].size();j++){
							////std::cout<<<<"j_"<<j<<";";	
							if(rule->section_tokens[m][j]->isRule()!=""){
								   // ////std::cout<<<<"RULE;";
								int index=findRule(rule->section_tokens[m][j]->token_name);
								if(index!=-1){
									Rule *fromtoken = rule_list[index];
									
									//////std::cout<<<<"PROB"<<prob<<">"<<fromtoken->probability<<std::endl;
									
										std::vector<Token *> more_tokens=Recurse(fromtoken);
										//////std::cout<<<<"HERE007";
										for(int k=0;k<more_tokens.size();k++){
											update_token(more_tokens[k]);
											new_tokens.push_back(more_tokens[k]);
										}
									
									////std::cout<<<<"HERE008";
							  }
						////std::cout<<<<"HERE0010";
							}
							else {
								    ////std::cout<<<<"notRULE";
								Token *check_token=new Token(rule->section_tokens[m][j]);
								update_token(check_token);
								new_tokens.push_back(check_token);
							}
							
						////std::cout<<<<"HERE0011";	
						}
					////std::cout<<<<"HERE0012";
					}
					removeVariable(rule->rule_name+"_count");	
					////std::cout<<<<"HERE013";
					////std::cout<<<<std::endl;
				}
				else {
					////std::cout<<<<"HERE001";
						for(int j=0;j<rule->section_tokens[m].size();j++){
						
							if(rule->section_tokens[m][j]->isRule()!=""){
								int index=findRule(rule->section_tokens[m][j]->token_name);
								if(index!=-1){
									Rule *fromtoken = rule_list[index];
									std::vector<Token *> more_tokens=Recurse(fromtoken);
									////std::cout<<<<"HERE006";
									for(int k=0;k<more_tokens.size();k++){
										update_token(more_tokens[k]);
										new_tokens.push_back(more_tokens[k]);
									}
									////std::cout<<<<"HERE007";
								}
						
							}
							else {
								
								
								
								Token *check_token=new Token(rule->section_tokens[m][j]);
								update_token(check_token);
								new_tokens.push_back(check_token);
							}
							
							
						  }
					
				} 
		
		
		
		
		
		////std::cout<<<<"HERE004";
		
	}
	
	
	for(int i=0;i<rule->var_counter;i++){
			
			removeVariable(rule->var_names[i]);
		}
	return new_tokens;
	}
	catch(...){
		
		errorout("caught exception recurse grammar");
		std::vector<Token *> empty;
		return empty;
	}
	////std::cout<<<<"HERE005";
		
		
}
	
	
	
	



int Grammar::findRule(std::string rule_name){
	////std::cout<<<<"Searching for rule:  "<<rule_name<<std::endl;
	for(int i=0;i<rule_list.size();i++){
	
		if(rule_list[i]->rule_name.compare(rule_name)==0) {
			
			////std::cout<<<<"Found "<<rule_name<<std::endl;
		return i;
		}
	
	}
	return -1;
}


void Grammar::addContext(){
//std::cout<<<<"Adding Context..."<<std::endl;
//if(context!=NULL)delete context;
    this->context=new Context();
}


void Grammar::generateGeometry()
{
	
    
    
    
    
    for(int k=0;k<tokens_new.size();k++){
		//tokens_new[k]->print();
			tokens_new[k]->performAction(context);
			
	}		
     
}










Grammar::~Grammar(){
	
	
	
    for(int k=0;k<tokens_new.size();k++){
		//tokens_new[k]->print();
			
			////std::cout<<<<k<<",";
			delete tokens_new[k];
			
	}		
	
	
	//for(int i=0;i<rule_list.size();i++){
	//	//std::cout<<<<i<<",";
	 //delete rule_list[i];	
	//}
	
	//delete context;
	
	
	
}























