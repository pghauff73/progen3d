#pragma once

#include <json/json.h>

#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <vector>

struct ParameterAxis {
	std::string name;
	std::vector<Json::Value> values;
};

class ParameterGrid {
public:
	static ParameterGrid parse(const Json::Value &specification, const std::string &label);
	const std::vector<ParameterAxis> &axes() const { return axes_; }
	std::size_t size() const { return size_; }
	void forEach(const std::function<void(const Json::Value &)> &evaluate) const;

private:
	std::vector<ParameterAxis> axes_;
	std::size_t size_ = 1;
};

class ParameterizedGrammar {
public:
	explicit ParameterizedGrammar(const std::filesystem::path &source);
	void checkBindings(const ParameterGrid &grid) const;
	std::string apply(const Json::Value &values) const;
	const std::string &source() const { return source_; }

private:
	struct Binding {
		std::size_t literal;
		std::size_t annotation_value;
		std::string original;
		bool numeric;
	};
	std::string source_;
	std::map<std::string, Binding> bindings_;
};

bool experiment_identifier(std::string_view text);
