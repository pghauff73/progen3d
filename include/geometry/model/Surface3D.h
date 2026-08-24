#pragma once

#include <glm/glm.hpp>

#include <string>

class Surface3D
{
public:
	virtual ~Surface3D() = default;

	virtual bool isValid(std::string *diagnostic = nullptr) const = 0;
	virtual glm::dvec3 evaluatePosition(double u, double v) const = 0;

	virtual glm::dvec3 evaluatePartialDerivativeU(double u, double v) const;
	virtual glm::dvec3 evaluatePartialDerivativeV(double u, double v) const;
	virtual glm::dvec3 evaluateNormal(double u, double v) const;
};
