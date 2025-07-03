

#include "Solution.h"
#include <iostream>
#include <string>
#include <stack>
#include <algorithm>
#include <cmath>
#include <sstream>

/*class nodeBSP{
	
	public:
	nodeBSP(float Xpos,float Xneg,float Ypos,float Yneg,float Zpos,float Zneg,int index);
	float Xpos,Xneg,Ypos,Yneg,Zpos,Zneg;
	int index;
}

class BSP{
	public:
	BSP(int order,float Xpos,float Xneg,float Ypos,float Yneg,float Zpos,float Zneg);
	add(glm::vec3 center);
	find(glm::vec3 center);
	private:
	
	
	static int order;
	
	float Xpos,Xneg,Ypos,Yneg,Zpos,Zneg;
	static std::vector<BSP *> nodes;
	static int index_counter;
	int index;
	float Xpos,Xneg,Ypos,Yneg,Zpos,Zneg;
};




BSP::BSP(int order,float Xpos,float Xneg,float Ypos,float Yneg,float Zpos,float Zneg){
	this->order=order;
	this->index_counter=0;
	this->nodes.push_back(new BSP(Xpos,0,Ypos,0,Zpos,0,this->index_counter++));
	
	
}
BSP::BSP(float Xpos,float Xneg,float Ypos,float Yneg,float Zpos,float Zneg,int index){
	this->index=index;
	
}


BSP::add(gml::vec3 center){
	
	this->centers.push_back(center);
	
}



BSP::find(glm::vec3 find_center){
	
	
}
*/






Solution::Solution(){
	
	   ops[0]='+';
	    ops[1]='*';
	    ops[2]='/';
	    ops[3]='^';
	    ops[4]='(';
	    ops[5]=')';
	    
	
	
}
float Solution::Process(std::string input){
	//std::cout<<"Process:"<<input<<std::endl;
		std::string A="";
		int pos1=input.find("-");
		if(pos1==0){
			A="-";
			input.erase(0,  1);
			}
		while((pos1=input.find("-"))!=-1){
			std::string B;
				B=input.substr(0, pos1);
					input.erase(0, pos1 + 1);
				A+=B+"+-";
				
			
		}
		A+=input;
		
		std::string start;
	
		pos1=A.find("cos(");
		if(pos1!=-1){
			
			start=A.substr(0,pos1);
			A.erase(0,pos1+4);
			int pos2=A.find(")");
			std::string B=A.substr(0,pos2);
			A.erase(0,pos2+1);
			float bfloat=cos(process(B));
					
			//std::cout<<start<<"cos("<<B<<")"<<A<<std::endl;
			A=start+std::to_string(bfloat)+A;	
		}
		pos1=A.find("sin(");
		if(pos1!=-1){
			
			start=A.substr(0,pos1);
			A.erase(0,pos1+4);
			int pos2=A.find(")");
			std::string B=A.substr(0,pos2);
			A.erase(0,pos2+1);
			float bfloat=sin(process(B));
					
			//std::cout<<start<<"cos("<<B<<")"<<A<<std::endl;
			A=start+std::to_string(bfloat)+A;	
		}
	
	
	
	  float afloat=process(A);
	  if(afloat<0.000001 && afloat >-0.000001)afloat=0.0f;
	  //std::cout<<std::fixed<<"ret"<<afloat<<std::endl;
		return afloat;
}
float Solution::process(std::string input){
		
		std::cout.precision(8);
		
		if(input[0]=='(' && input[input.size()-1]==')'){
			input=input.substr(1,input.size()-2);
		}
	 	
		
		//std::cout<<"process::"<<input<<std::endl;
	 	int pos[10];
	 	
	 	
	 	for(int i=0;i<6;i++)pos[i]=-1;
		std::string A,B;
		bool bracket=false;
		bool isnumeric=true;
		
		for(int j=0;j<4;j++){
			if(input[0]==ops[j])return 0.0f;
		}
		int J=0;
	while(isnumeric && J<4){
		for(int i=0;i<input.length();i++){
			////std::cout<<input[i]<<std::endl;
			for(int j=0;j<2;j++){
				int m;
				////std::cout<<input[i]<<","<<ops[j]<<std::endl;
				if(j==0)m=4;
				else m=J;
				if(input[i]==ops[m]){
					
					if(m==4){//bracket first
						pos[4]=i;
						int bracket_counter=0;
						bracket_counter++;
						while(bracket_counter!=0){
							i++;
							if(input[i]==ops[4])bracket_counter++;
							if(input[i]==ops[5])bracket_counter--;
							
						}
						pos[5]=i;
						A=input.substr(pos[4]+1, pos[5]-1);
						
						//std::cout<<A<<std::endl;  
						
					   bracket=true;
						
					}
					else {
						isnumeric=false;
						pos[m]=i;
						////std::cout<<i<<","<<ops[m]<<std::endl;
						if(bracket){
							input.erase(pos[4],pos[5]+2);
						}
						else {
							
						A=input.substr(0, pos[m]);
						input.erase(0, pos[m]+1);
						}
						
						if(pos[0]!=-1){
								
								float afloat=process(A)+process(input);
								//std::cout<<A<<"+"<<input<<"="<<afloat<<std::endl;  
							return afloat;
							
						}
						else if(pos[1]!=-1){
							float afloat=process(A)*process(input);
									////std::cout<<A<<"*"<<input<<"="<<afloat<<std::endl;  
									return afloat;
								}
						else if(pos[2]!=-1){
									
									float ret=process(input);
								    float afloat;
								    if(ret!=0.0f)
								    afloat=process(A)/ret;
									else afloat=0.0f;
								
								//std::cout<<A<<"/"<<input<<"="<<afloat<<std::endl;  
								
									return afloat;
								
							}
						else if(pos[3]!=-1){
								  
								float afloat=pow(process(A),process(input));
							  //std::cout<<A<<"^"<<input<<"="<<afloat<<std::endl;  
								return afloat;
						}
						else
							return 0.0f;
					
						
						
					}
				}
			}


		}
		J++;
		bracket=false;
	}
		if(isnumeric==true)return atof(input.c_str());
		
							
							
						
			
		return 0.0f;	
}




Noise2d::Noise2d(int numOctaves,double persistence){
	this->numOctaves=numOctaves;
	this->persistence=persistence;
}

double Noise2d::Noise(int i, int x, int y) {
  int n = x + y * 57;
  n = (n << 13) ^ n;
  int a = primes[i][0], b = primes[i][1], c = primes[i][2];
  int t = (n * (n * n * a + b) + c) & 0x7fffffff;
  return 1.0 - (double)(t)/1073741824.0;
}

double Noise2d::SmoothedNoise(int i, int x, int y) {
  double corners = (Noise(i, x-1, y-1) + Noise(i, x+1, y-1) +
                    Noise(i, x-1, y+1) + Noise(i, x+1, y+1)) / 16,
         sides = (Noise(i, x-1, y) + Noise(i, x+1, y) + Noise(i, x, y-1) +
                  Noise(i, x, y+1)) / 8,
         center = Noise(i, x, y) / 4;
  return corners + sides + center;
}

double Noise2d::Interpolate(double a, double b, double x) {  // cosine interpolation
  double ft = x * 3.1415927,
         f = (1 - cos(ft)) * 0.5;
  return  a*(1-f) + b*f;
}

double Noise2d::InterpolatedNoise(int i, double x, double y) {
  int integer_X = x;
  double fractional_X = x - integer_X;
  int integer_Y = y;
  double fractional_Y = y - integer_Y;

  double v1 = SmoothedNoise(i, integer_X, integer_Y),
         v2 = SmoothedNoise(i, integer_X + 1, integer_Y),
         v3 = SmoothedNoise(i, integer_X, integer_Y + 1),
         v4 = SmoothedNoise(i, integer_X + 1, integer_Y + 1),
         i1 = Interpolate(v1, v2, fractional_X),
         i2 = Interpolate(v3, v4, fractional_X);
  return Interpolate(i1, i2, fractional_Y);
}

double Noise2d::ValueNoise_2D(double x, double y) {
  double total = 0,
         frequency = pow(2, this->numOctaves),
         amplitude = 1;
  for (int i = 0; i < numOctaves; ++i) {
    frequency /= 2;
    amplitude *= persistence;
    total += InterpolatedNoise((primeIndex + i) % maxPrimeIndex,
        x / frequency, y / frequency) * amplitude;
  }
  return total / frequency;
}

