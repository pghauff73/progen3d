#pragma once

#include "audio/model/AudioEventDefinition.h"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

struct AudioEventDeclarationResult {
	std::optional<AudioEventDefinition> definition;
	std::size_t error_column = 0;
	std::string error;
};

class AudioEventDeclarationParser {
public:
	static bool recognizes(std::string_view line)
	{
		const std::size_t start = line.find_first_not_of(" \t\r");
		if (start == std::string_view::npos || line.substr(start, keyword.size()) != keyword)
			return false;
		const auto suffix = line.substr(start + keyword.size());
		const std::size_t next = suffix.find_first_not_of(" \t\r");
		return next == std::string_view::npos || suffix[next] == '(';
	}

	AudioEventDeclarationResult parse(std::string_view line) const;

private:
	static constexpr std::string_view keyword = "audioEvent";
};
