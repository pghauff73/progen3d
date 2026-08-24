#pragma once

#include <glm/glm.hpp>

class CollisionContact
{
public:
	static CollisionContact createSeparated()
	{
		return CollisionContact(false, glm::vec3(0.0f), 0.0f, glm::vec3(0.0f));
	}

	static CollisionContact createIntersecting(const glm::vec3 &normal,
	                                           float penetration,
	                                           const glm::vec3 &point)
	{
		return CollisionContact(true, normal, penetration, point);
	}

	bool intersects() const
	{
		return intersects_;
	}

	const glm::vec3 &normal() const
	{
		return normal_;
	}

	float penetration() const
	{
		return penetration_;
	}

	const glm::vec3 &point() const
	{
		return point_;
	}

private:
	CollisionContact(bool intersects,
	                 const glm::vec3 &normal,
	                 float penetration,
	                 const glm::vec3 &point)
		: intersects_(intersects),
		  normal_(normal),
		  penetration_(penetration),
		  point_(point)
	{
	}

	bool intersects_ = false;
	glm::vec3 normal_{0.0f};
	float penetration_ = 0.0f;
	glm::vec3 point_{0.0f};
};
