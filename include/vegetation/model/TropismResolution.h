#pragma once

#include <glm/glm.hpp>

#include <string>
#include <utility>

class TropismResolution
{
public:
	static TropismResolution succeeded(
		glm::vec3 direction,
		glm::vec3 environmental_vector)
	{
		return TropismResolution(
			true, direction, environmental_vector, {});
	}

	static TropismResolution failed(std::string diagnostic)
	{
		return TropismResolution(
			false, glm::vec3(0.0f), glm::vec3(0.0f),
			std::move(diagnostic));
	}

	bool succeeded() const { return succeeded_; }
	const glm::vec3 &direction() const { return direction_; }
	const glm::vec3 &environmentalVector() const
	{
		return environmental_vector_;
	}
	const std::string &diagnostic() const { return diagnostic_; }

private:
	TropismResolution(
		bool succeeded,
		glm::vec3 direction,
		glm::vec3 environmental_vector,
		std::string diagnostic)
		: succeeded_(succeeded),
		  direction_(direction),
		  environmental_vector_(environmental_vector),
		  diagnostic_(std::move(diagnostic))
	{
	}

	bool succeeded_ = false;
	glm::vec3 direction_{0.0f};
	glm::vec3 environmental_vector_{0.0f};
	std::string diagnostic_;
};

