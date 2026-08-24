#pragma once

#include <glm/glm.hpp>

#include <string>
#include <utility>
#include <vector>

class CrownAttractionPointSamplingResult
{
public:
	static CrownAttractionPointSamplingResult succeeded(
		std::vector<glm::vec3> points)
	{
		return CrownAttractionPointSamplingResult(
			true, std::move(points), {});
	}

	static CrownAttractionPointSamplingResult failed(std::string diagnostic)
	{
		return CrownAttractionPointSamplingResult(
			false, {}, std::move(diagnostic));
	}

	bool succeeded() const { return succeeded_; }
	const std::vector<glm::vec3> &points() const { return points_; }
	const std::string &diagnostic() const { return diagnostic_; }

private:
	CrownAttractionPointSamplingResult(
		bool succeeded,
		std::vector<glm::vec3> points,
		std::string diagnostic)
		: succeeded_(succeeded),
		  points_(std::move(points)),
		  diagnostic_(std::move(diagnostic))
	{
	}

	bool succeeded_ = false;
	std::vector<glm::vec3> points_;
	std::string diagnostic_;
};
