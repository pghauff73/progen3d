#pragma once

#include "geometry/model/ProfileLoop2D.h"

#include <utility>
#include <vector>

class Profile2D
{
public:
	Profile2D(ProfileLoop2D outer_loop,
	          std::vector<ProfileLoop2D> inner_loops)
		: outer_loop_(std::move(outer_loop)),
		  inner_loops_(std::move(inner_loops))
	{
	}

	const ProfileLoop2D &outerLoop() const { return outer_loop_; }
	const std::vector<ProfileLoop2D> &innerLoops() const { return inner_loops_; }
	std::size_t loopCount() const { return 1u + inner_loops_.size(); }

	std::size_t pointCount() const
	{
		std::size_t count = outer_loop_.points().size();
		for (const ProfileLoop2D &inner_loop : inner_loops_) {
			count += inner_loop.points().size();
		}
		return count;
	}

	std::size_t windingCorrectionCount() const
	{
		std::size_t count =
			outer_loop_.windingCorrection() == ProfileWindingCorrection::None ? 0u : 1u;
		for (const ProfileLoop2D &inner_loop : inner_loops_) {
			if (inner_loop.windingCorrection() != ProfileWindingCorrection::None) ++count;
		}
		return count;
	}

private:
	ProfileLoop2D outer_loop_{{}, ProfileWindingCorrection::None};
	std::vector<ProfileLoop2D> inner_loops_;
};
