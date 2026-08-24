#pragma once

#include <glm/glm.hpp>

#include <string>

class Curve3DEvaluator
{
public:
	virtual ~Curve3DEvaluator() = default;

	virtual bool isValid(std::string *diagnostic = nullptr) const = 0;
	virtual glm::dvec3 evaluatePosition(double parameter) const = 0;
	virtual glm::dvec3 evaluateFirstDerivative(double parameter) const = 0;
	virtual glm::dvec3 evaluateSecondDerivative(double parameter) const = 0;
};
