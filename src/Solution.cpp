

#include "Solution.h"
#include <cctype>
#include <cstdlib>
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






namespace {

class ExpressionParser {
public:
	explicit ExpressionParser(const std::string &input)
		: input_(input)
	{
	}

	float parse()
	{
		const float value = parse_expression();
		skip_whitespace();
		if(!valid_ || !at_end()){
			return 0.0f;
		}
		if(!std::isfinite(value)){
			return 0.0f;
		}
		return std::fabs(value) < 0.000001f ? 0.0f : value;
	}

private:
	float parse_expression()
	{
		float value = parse_term();
		while(valid_){
			skip_whitespace();
			if(match('+')){
				value += parse_term();
				continue;
			}
			if(match('-')){
				value -= parse_term();
				continue;
			}
			break;
		}
		return value;
	}

	float parse_term()
	{
		float value = parse_unary();
		while(valid_){
			skip_whitespace();
			if(match('*')){
				value *= parse_unary();
				continue;
			}
			if(match('/')){
				const float divisor = parse_unary();
				if(std::fabs(divisor) < 0.000001f){
					return 0.0f;
				}
				value /= divisor;
				continue;
			}
			break;
		}
		return value;
	}

	float parse_unary()
	{
		skip_whitespace();
		if(match('+')){
			return parse_unary();
		}
		if(match('-')){
			return -parse_unary();
		}
		return parse_power();
	}

	float parse_power()
	{
		float value = parse_primary();
		skip_whitespace();
		if(match('^')){
			const float exponent = parse_unary();
			value = std::pow(value, exponent);
		}
		return value;
	}

	float parse_primary()
	{
		skip_whitespace();
		if(at_end()){
			valid_ = false;
			return 0.0f;
		}

		if(match('(')){
			const float value = parse_expression();
			skip_whitespace();
			if(!match(')')){
				valid_ = false;
				return 0.0f;
			}
			return value;
		}

		if(std::isalpha(static_cast<unsigned char>(peek())) != 0){
			const std::string identifier = parse_identifier();
			skip_whitespace();
			if(identifier == "sin" || identifier == "cos"){
				if(!match('(')){
					valid_ = false;
					return 0.0f;
				}
				const float argument = parse_expression();
				skip_whitespace();
				if(!match(')')){
					valid_ = false;
					return 0.0f;
				}
				return identifier == "sin" ? std::sin(argument) : std::cos(argument);
			}

			valid_ = false;
			return 0.0f;
		}

		return parse_number();
	}

	float parse_number()
	{
		skip_whitespace();
		const char *start = input_.c_str() + position_;
		char *end = NULL;
		const float value = std::strtof(start, &end);
		if(end == start){
			valid_ = false;
			return 0.0f;
		}
		position_ = static_cast<std::size_t>(end - input_.c_str());
		return value;
	}

	std::string parse_identifier()
	{
		const std::size_t start = position_;
		while(!at_end() && std::isalpha(static_cast<unsigned char>(peek())) != 0){
			++position_;
		}
		return input_.substr(start, position_ - start);
	}

	void skip_whitespace()
	{
		while(!at_end() && std::isspace(static_cast<unsigned char>(peek())) != 0){
			++position_;
		}
	}

	bool match(char c)
	{
		skip_whitespace();
		if(at_end() || input_[position_] != c){
			return false;
		}
		++position_;
		return true;
	}

	char peek() const
	{
		return at_end() ? '\0' : input_[position_];
	}

	bool at_end() const
	{
		return position_ >= input_.size();
	}

	const std::string &input_;
	std::size_t position_ = 0;
	bool valid_ = true;
};

}




ArithmeticExpressionEvaluator::ArithmeticExpressionEvaluator(){
	
	   ops[0]='+';
	    ops[1]='*';
	    ops[2]='/';
	    ops[3]='^';
	    ops[4]='(';
	    ops[5]=')';
	    
	
	
}
float ArithmeticExpressionEvaluator::evaluate(std::string input){
	ExpressionParser parser(input);
	return parser.parse();
}
float ArithmeticExpressionEvaluator::Process(std::string input){
	return evaluate(std::move(input));
}
float ArithmeticExpressionEvaluator::process(std::string input){
	return evaluate(std::move(input));
}




ValueNoiseField2d::ValueNoiseField2d(int numOctaves,double persistence){
	this->numOctaves=numOctaves;
	this->persistence=persistence;
}

double ValueNoiseField2d::Noise(int i, int x, int y) {
  int n = x + y * 57;
  n = (n << 13) ^ n;
  int a = primes[i][0], b = primes[i][1], c = primes[i][2];
  int t = (n * (n * n * a + b) + c) & 0x7fffffff;
  return 1.0 - (double)(t)/1073741824.0;
}

double ValueNoiseField2d::SmoothedNoise(int i, int x, int y) {
  double corners = (Noise(i, x-1, y-1) + Noise(i, x+1, y-1) +
                    Noise(i, x-1, y+1) + Noise(i, x+1, y+1)) / 16,
         sides = (Noise(i, x-1, y) + Noise(i, x+1, y) + Noise(i, x, y-1) +
                  Noise(i, x, y+1)) / 8,
         center = Noise(i, x, y) / 4;
  return corners + sides + center;
}

double ValueNoiseField2d::Interpolate(double a, double b, double x) {  // cosine interpolation
  double ft = x * 3.1415927,
         f = (1 - cos(ft)) * 0.5;
  return  a*(1-f) + b*f;
}

double ValueNoiseField2d::InterpolatedNoise(int i, double x, double y) {
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

double ValueNoiseField2d::ValueNoise_2D(double x, double y) {
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

double ValueNoiseField2d::sample(double x, double y)
{
	return ValueNoise_2D(x, y);
}

