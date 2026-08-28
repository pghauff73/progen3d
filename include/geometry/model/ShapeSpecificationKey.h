#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <utility>

class ShapeSpecificationKey
{
public:
	explicit ShapeSpecificationKey(std::string canonical_value = {})
		: canonical_value_(std::move(canonical_value))
	{
	}

	const std::string &canonicalValue() const
	{
		return canonical_value_;
	}

	bool operator==(const ShapeSpecificationKey &other) const
	{
		return canonical_value_ == other.canonical_value_;
	}

private:
	std::string canonical_value_;
};

struct ShapeSpecificationKeyHash
{
	std::size_t operator()(const ShapeSpecificationKey &key) const
	{
		return std::hash<std::string>{}(key.canonicalValue());
	}
};
