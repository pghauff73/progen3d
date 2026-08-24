#pragma once

#include <cstddef>
#include <string>
#include <utility>

class McsMv22SurfaceOwnerDefinition
{
public:
	McsMv22SurfaceOwnerDefinition(
		std::string identifier,
		std::string kind,
		bool closure,
		std::size_t face_count)
		: identifier_(std::move(identifier)),
		  kind_(std::move(kind)),
		  closure_(closure),
		  face_count_(face_count)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &kind() const { return kind_; }
	bool isClosure() const { return closure_; }
	std::size_t faceCount() const { return face_count_; }

private:
	std::string identifier_;
	std::string kind_;
	bool closure_ = false;
	std::size_t face_count_ = 0u;
};
