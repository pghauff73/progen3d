#pragma once

#include <string>

class MathematicalEvaluator
{
public:
	virtual ~MathematicalEvaluator() = default;
	virtual float evaluate(std::string expression) = 0;
};

class ArithmeticExpressionEvaluator : public MathematicalEvaluator {
private:
    char ops[10];
	
public:
	ArithmeticExpressionEvaluator();
	float evaluate(std::string expression) override;
	float Process(std::string expression);
	float process(std::string expression);
};

class ProceduralNoiseField
{
public:
	virtual ~ProceduralNoiseField() = default;
	virtual double sample(double x, double y) = 0;
};

static int maxPrimeIndex=10;

class ValueNoiseField2d : public ProceduralNoiseField {
private:

int numOctaves = 7;
double persistence = 0.5;


int primeIndex = 0;

int primes[10][3] = {
  { 995615039, 600173719, 701464987 },
  { 831731269, 162318869, 136250887 },
  { 174329291, 946737083, 245679977 },
  { 362489573, 795918041, 350777237 },
  { 457025711, 880830799, 909678923 },
  { 787070341, 177340217, 593320781 },
  { 405493717, 291031019, 391950901 },
  { 458904767, 676625681, 424452397 },
  { 531736441, 939683957, 810651871 },
  { 997169939, 842027887, 423882827 }
};
public:
ValueNoiseField2d(int,double);
double Noise(int,int,int);
double SmoothedNoise(int,int,int);
double Interpolate(double,double,double);
double InterpolatedNoise(int,double,double);
double ValueNoise_2D(double,double);
double sample(double x, double y) override;


int numX = 128;
int numY = 128;

};

using Solution = ArithmeticExpressionEvaluator;
using Noise2d = ValueNoiseField2d;
