#pragma once

#include "geometry/model/CurveArcLengthTable.h"

#include <cstddef>
#include <memory>
#include <string>

class Curve3DEvaluator;

class CurveArcLengthService
{
public:
	std::shared_ptr<const CurveArcLengthTable> build(
		const Curve3DEvaluator &curve,
		std::size_t sample_count,
		std::size_t maximum_sample_count,
		std::string *diagnostic = nullptr) const;
};
