#pragma once
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

struct GrammarReuseListing {
    std::string reference;
    std::string title;
    std::string category;
    std::string searchText;
};

struct GrammarImportError : std::runtime_error {
    GrammarImportError(int line, const std::string& message) : std::runtime_error(message), line(line) {}
    int line;
};

struct ExpandedGrammarSource {
    std::vector<std::string> lines;
    // Indexed by generated line minus original line count.
    std::vector<int> declarationLines;
};

class GrammarReuseCatalogue {
public:
    static std::vector<GrammarReuseListing> listEntries();
    static std::string readStandalone(const std::string& reference);
    static bool isImportLine(const std::string& line);
    static std::optional<ExpandedGrammarSource> expandImports(const std::vector<std::string>& lines);
    static std::optional<int> runCommand(int argc, char** argv);
};
