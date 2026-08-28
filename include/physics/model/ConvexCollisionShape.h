#pragma once

#include <glm/glm.hpp>

#include <vector>

class ConvexCollisionShape
{
public:
	ConvexCollisionShape(const std::vector<glm::vec3> &vertices, const glm::vec3 &center)
		: vertices_(&vertices), center_(center)
	{
	}

	bool isValid() const
	{
		return vertices_ != nullptr && vertices_->size() >= 4u;
	}

	const std::vector<glm::vec3> &vertices() const
	{
		return *vertices_;
	}

	const glm::vec3 &center() const
	{
		return center_;
	}

	glm::vec3 supportPoint(const glm::vec3 &direction) const
	{
		glm::vec3 support = vertices_->front();
		float maximum_projection = glm::dot(support, direction);
		for (std::size_t index = 1; index < vertices_->size(); ++index) {
			const float projection = glm::dot((*vertices_)[index], direction);
			if (projection > maximum_projection) {
				maximum_projection = projection;
				support = (*vertices_)[index];
			}
		}
		return support;
	}

private:
	const std::vector<glm::vec3> *vertices_ = nullptr;
	glm::vec3 center_{0.0f};
};
