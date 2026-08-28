#pragma once

#include <cstddef>
#include <cstdint>

class MeshTopologyReport
{
public:
	std::size_t boundary_edge_count = 0;
	std::size_t nonmanifold_edge_count = 0;
	std::size_t degenerate_triangle_count = 0;
	float signed_volume = 0.0f;
	std::uint64_t topology_hash = 0;

	bool isWatertight() const
	{
		return boundary_edge_count == 0 &&
		       nonmanifold_edge_count == 0 &&
		       degenerate_triangle_count == 0;
	}
};
