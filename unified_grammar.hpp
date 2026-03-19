#ifndef PROGEN3D_UNIFIED_GRAMMAR_HPP
#define PROGEN3D_UNIFIED_GRAMMAR_HPP

#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <random>
#include <memory>
#include <functional>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace ProGen3D {

// ================= TEXTURE PRESETS =================
const std::vector<std::string> TEXTURE_PRESETS = {
    "roughConcrete", "smoothConcrete", "crackedAsphalt", "redBrickWall", "whiteTiles", "blueCeramicTile",
    "shinySteel", "brushedAluminum", "oxidizedCopper", "agedBronze", "shinyGold", "tarnishedSilver", 
    "rawIron", "polishedTitanium", "clearGlass", "frostedGlass", "tintedGreenGlass", "stainedBlueGlass",
    "glossyWhitePlastic", "matteBlackPlastic", "redABS", "bluePVC", "translucentPoly",
    "freshGrass", "dryGrass", "desertSand", "muddySoil", "grayStone", "darkBasalt", "whiteMarble", "greenMarble",
    "oakPlanks", "darkWalnut", "pineWood", "mahoganyWood", "cherryWood", "mapleWood",
    "iceBlock", "moltenLava", "deepWater", "snowPowder", "fluffyCloud", "mossPatch", "claySoil", "rustyMetal",
    "carbonFiber", "circuitBoard", "glowingPanel", "hazardStripe", "steelGrid", "chromeSurface"
};

// ================= UTILITY STRUCTURES =================
struct Vec3 {
    double x, y, z;
    Vec3(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}
    Vec3 operator+(const Vec3& other) const { return Vec3(x + other.x, y + other.y, z + other.z); }
    Vec3 operator-(const Vec3& other) const { return Vec3(x - other.x, y - other.y, z - other.z); }
    Vec3 operator*(double scalar) const { return Vec3(x * scalar, y * scalar, z * scalar); }
};

struct Command {
    enum Type { LINE, MOVE, START_POLYGON, END_POLYGON, VERTEX };
    Type type;
    Vec3 from, to, position;
    
    Command(Type t) : type(t) {}
    Command(Type t, const Vec3& f, const Vec3& to) : type(t), from(f), to(to) {}
    Command(Type t, const Vec3& pos) : type(t), position(pos) {}
};

struct Context {
    char left, right;
    size_t position, length;
    Context(char l = '\0', char r = '\0', size_t pos = 0, size_t len = 0) 
        : left(l), right(r), position(pos), length(len) {}
};

struct StochasticRule {
    std::string production;
    double probability;
    StochasticRule(const std::string& prod, double prob) : production(prod), probability(prob) {}
};

// ================= CORE L-SYSTEM GRAMMAR ENGINE =================
class ProGen3DGrammar {
private:
    std::string axiom;
    std::unordered_map<char, std::string> simpleRules;
    std::unordered_map<char, std::vector<StochasticRule>> stochasticRules;
    std::unordered_map<std::string, double> constants;
    std::vector<std::string> history;
    
    int iterations;
    double angle;
    bool stochastic;
    bool contextSensitive;
    bool parametric;
    
    std::mt19937 rng;
    std::uniform_real_distribution<double> dist;

public:
    ProGen3DGrammar(const std::string& ax = "A", int iter = 3, double ang = 25.0, uint32_t seed = 0)
        : axiom(ax), iterations(iter), angle(ang), stochastic(false), 
          contextSensitive(false), parametric(false), rng(seed ? seed : std::random_device{}()), dist(0.0, 1.0) {}

    // RNG utilities
    double chance() { return dist(rng); }
    
    template<typename T>
    const T& pick(const std::vector<T>& arr) {
        return arr[static_cast<size_t>(chance() * arr.size())];
    }
    
    double range(double a, double b) { return a + (b - a) * chance(); }
    int irange(int a, int b) { return static_cast<int>(std::floor(range(a, b + 1))); }

    // Rule management
    void addRule(char symbol, const std::string& production) {
        simpleRules[symbol] = production;
    }
    
    void addStochasticRule(char symbol, const std::vector<StochasticRule>& rules) {
        stochasticRules[symbol] = rules;
        stochastic = true;
    }
    
    void setConstant(const std::string& name, double value) {
        constants[name] = value;
    }

    // Grammar generation
    std::string generate(int iter = -1) {
        if (iter == -1) iter = iterations;
        
        std::string current = axiom;
        history.clear();
        history.push_back(current);

        for (int i = 0; i < iter; ++i) {
            current = applyRules(current);
            history.push_back(current);
        }

        return current;
    }

private:
    std::string applyRules(const std::string& input) {
        std::string output;
        output.reserve(input.length() * 2); // Reserve space for efficiency

        for (size_t i = 0; i < input.length(); ++i) {
            char symbol = input[i];
            
            if (stochastic && stochasticRules.find(symbol) != stochasticRules.end()) {
                output += applyStochasticRule(symbol);
            } else {
                output += applySimpleRule(symbol);
            }
        }

        return output;
    }

    std::string applySimpleRule(char symbol) {
        auto it = simpleRules.find(symbol);
        return (it != simpleRules.end()) ? it->second : std::string(1, symbol);
    }

    std::string applyStochasticRule(char symbol) {
        auto it = stochasticRules.find(symbol);
        if (it == stochasticRules.end()) {
            return std::string(1, symbol);
        }

        const auto& rules = it->second;
        double rand = chance();
        double cumulative = 0.0;

        for (const auto& rule : rules) {
            cumulative += rule.probability;
            if (rand <= cumulative) {
                return rule.production;
            }
        }

        return rules.empty() ? std::string(1, symbol) : rules.back().production;
    }

public:
    // 3D Interpretation
    std::vector<Command> interpret(const std::string& lstring, double interpretAngle = -1) {
        if (interpretAngle == -1) interpretAngle = angle;
        
        std::vector<Command> commands;
        std::vector<std::pair<Vec3, Vec3>> stack; // position, direction pairs
        
        Vec3 position(0, 0, 0);
        Vec3 direction(0, 1, 0);

        for (char symbol : lstring) {
            switch (symbol) {
                case 'F': // Move forward and draw
                {
                    Vec3 newPos = position + direction;
                    commands.emplace_back(Command::LINE, position, newPos);
                    position = newPos;
                    break;
                }
                case 'f': // Move forward without drawing
                    position = position + direction;
                    break;
                    
                case '+': // Rotate right
                    direction = rotate2D(direction, interpretAngle);
                    break;
                    
                case '-': // Rotate left
                    direction = rotate2D(direction, -interpretAngle);
                    break;
                    
                case '&': // Pitch down
                    direction = pitchDown(direction, interpretAngle);
                    break;
                    
                case '^': // Pitch up
                    direction = pitchUp(direction, interpretAngle);
                    break;
                    
                case '\\': // Roll left
                    direction = rollLeft(direction, interpretAngle);
                    break;
                    
                case '/': // Roll right
                    direction = rollRight(direction, interpretAngle);
                    break;
                    
                case '|': // Turn around
                    direction = Vec3(-direction.x, -direction.y, -direction.z);
                    break;
                    
                case '[': // Push state
                    stack.emplace_back(position, direction);
                    break;
                    
                case ']': // Pop state
                    if (!stack.empty()) {
                        auto state = stack.back();
                        stack.pop_back();
                        position = state.first;
                        direction = state.second;
                    }
                    break;
                    
                case '{': // Start polygon
                    commands.emplace_back(Command::START_POLYGON);
                    break;
                    
                case '}': // End polygon
                    commands.emplace_back(Command::END_POLYGON);
                    break;
                    
                case '.': // Add vertex
                    commands.emplace_back(Command::VERTEX, position);
                    break;
            }
        }

        return commands;
    }

private:
    // 3D Rotation utilities
    Vec3 rotate2D(const Vec3& dir, double angle) const {
        double rad = angle * M_PI / 180.0;
        double cosA = std::cos(rad);
        double sinA = std::sin(rad);
        
        return Vec3(
            dir.x * cosA - dir.y * sinA,
            dir.x * sinA + dir.y * cosA,
            dir.z
        );
    }

    Vec3 pitchDown(const Vec3& dir, double angle) const {
        double rad = angle * M_PI / 180.0;
        double cosA = std::cos(rad);
        double sinA = std::sin(rad);
        
        return Vec3(
            dir.x,
            dir.y * cosA - dir.z * sinA,
            dir.y * sinA + dir.z * cosA
        );
    }

    Vec3 pitchUp(const Vec3& dir, double angle) const {
        return pitchDown(dir, -angle);
    }

    Vec3 rollLeft(const Vec3& dir, double angle) const {
        double rad = angle * M_PI / 180.0;
        double cosA = std::cos(rad);
        double sinA = std::sin(rad);
        
        return Vec3(
            dir.x * cosA + dir.z * sinA,
            dir.y,
            -dir.x * sinA + dir.z * cosA
        );
    }

    Vec3 rollRight(const Vec3& dir, double angle) const {
        return rollLeft(dir, -angle);
    }

public:
    // Getters
    const std::string& getAxiom() const { return axiom; }
    int getIterations() const { return iterations; }
    double getAngle() const { return angle; }
    const std::vector<std::string>& getHistory() const { return history; }
};

// ================= GRAMMAR LIBRARY =================
class GrammarLibrary {
public:
    static std::unique_ptr<ProGen3DGrammar> createTreeSimple() {
        auto grammar = std::make_unique<ProGen3DGrammar>("F", 4, 25.7);
        grammar->addRule('F', "F[+F]F[-F]F");
        return grammar;
    }

    static std::unique_ptr<ProGen3DGrammar> createTreeComplex() {
        auto grammar = std::make_unique<ProGen3DGrammar>("X", 5, 25.0);
        grammar->addRule('X', "F[+X][-X]FX");
        grammar->addRule('F', "FF");
        return grammar;
    }

    static std::unique_ptr<ProGen3DGrammar> createBush() {
        auto grammar = std::make_unique<ProGen3DGrammar>("F", 4, 22.5);
        grammar->addRule('F', "FF+[+F-F-F]-[-F+F+F]");
        return grammar;
    }

    static std::unique_ptr<ProGen3DGrammar> createPlant3D() {
        auto grammar = std::make_unique<ProGen3DGrammar>("A", 8, 22.5);
        grammar->addRule('A', "[&FL!A]/////[&FL!A]///////[&FL!A]");
        grammar->addRule('F', "S/////F");
        grammar->addRule('S', "FL");
        grammar->addRule('L', "[^^^{-f+f+f-|-f+f+f}]");
        return grammar;
    }

    static std::unique_ptr<ProGen3DGrammar> createStochasticTree() {
        auto grammar = std::make_unique<ProGen3DGrammar>("F", 4, 25.0);
        std::vector<StochasticRule> rules = {
            {"F[+F]F[-F]F", 0.33},
            {"F[+F]F", 0.33},
            {"F[-F]F", 0.34}
        };
        grammar->addStochasticRule('F', rules);
        return grammar;
    }

    static std::unique_ptr<ProGen3DGrammar> createDragonCurve() {
        auto grammar = std::make_unique<ProGen3DGrammar>("FX", 10, 90.0);
        grammar->addRule('X', "X+YF+");
        grammar->addRule('Y', "-FX-Y");
        return grammar;
    }

    static std::unique_ptr<ProGen3DGrammar> createHilbertCurve() {
        auto grammar = std::make_unique<ProGen3DGrammar>("A", 5, 90.0);
        grammar->addRule('A', "-BF+AFA+FB-");
        grammar->addRule('B', "+AF-BFB-FA+");
        return grammar;
    }
};

// ================= SIMPLE TOKENIZER (C++ replacement) =================
// Lightweight tokenizer used for editor-like parsing of grammar files.
// This mirrors the previous JS EnhancedTokenizer behavior but is
// implemented in portable C++ and returns a vector of tokens.

struct Token {
    enum class Type { Whitespace, Comment, Operator, Number, Identifier, Keyword, Primitive, Texture, Unknown };
    Type type;
    std::string value;
    size_t start;
    size_t end;
    int line;

    Token(Type t = Type::Unknown, std::string v = "", size_t s = 0, size_t e = 0, int ln = 0)
        : type(t), value(std::move(v)), start(s), end(e), line(ln) {}
};

class Tokenizer {
public:
    Tokenizer() = default;

    std::vector<Token> tokenize(const std::string& text) {
        std::vector<Token> out;
        std::vector<std::string> lines = splitLines(text);
        size_t globalPos = 0;
        for (size_t ln = 0; ln < lines.size(); ++ln) {
            const std::string& line = lines[ln];
            tokenizeLine(line, globalPos, static_cast<int>(ln), out);
            globalPos += line.size() + 1; // include newline
        }
        return out;
    }

private:
    static std::vector<std::string> splitLines(const std::string& s) {
        std::vector<std::string> res;
        size_t pos = 0, start = 0;
        while ((pos = s.find('\n', start)) != std::string::npos) {
            res.emplace_back(s.substr(start, pos - start));
            start = pos + 1;
        }
        res.emplace_back(s.substr(start));
        return res;
    }

    static bool isWhitespace(char c) noexcept { return c == ' ' || c == '\t' || c == '\r' || c == '\f' || c == '\v'; }
    static bool isDigitChar(char c) noexcept { return (c >= '0' && c <= '9') || c == '.' || c == '-'; }
    static bool isIdentStart(char c) noexcept { return (c == '_' || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')); }

    void tokenizeLine(const std::string& line, size_t lineStart, int lineNum, std::vector<Token>& out) {
        size_t pos = 0;
        while (pos < line.size()) {
            char c = line[pos];
            if (isWhitespace(c)) {
                size_t s = pos;
                while (pos < line.size() && isWhitespace(line[pos])) ++pos;
                out.emplace_back(Token::Type::Whitespace, line.substr(s, pos - s), lineStart + s, lineStart + pos, lineNum);
                continue;
            }

            // comments
            if (c == '/' && pos + 1 < line.size() && line[pos + 1] == '/') {
                out.emplace_back(Token::Type::Comment, line.substr(pos), lineStart + pos, lineStart + line.size(), lineNum);
                break;
            }

            // arrow operator '->'
            if (c == '-' && pos + 1 < line.size() && line[pos + 1] == '>') {
                out.emplace_back(Token::Type::Operator, "->", lineStart + pos, lineStart + pos + 2, lineNum);
                pos += 2;
                continue;
            }

            // single-char operators
            if (std::string("+-=*/%()[],{}<>:|&^!~\\.").find(c) != std::string::npos) {
                out.emplace_back(Token::Type::Operator, std::string(1, c), lineStart + pos, lineStart + pos + 1, lineNum);
                ++pos;
                continue;
            }

            // numbers
            if (isDigitChar(c)) {
                size_t s = pos;
                if (c == '-') ++pos; // allow leading minus
                while (pos < line.size() && (line[pos] == '.' || (line[pos] >= '0' && line[pos] <= '9'))) ++pos;
                std::string val = line.substr(s, pos - s);
                out.emplace_back(Token::Type::Number, val, lineStart + s, lineStart + pos, lineNum);
                continue;
            }

            // identifiers / keywords / primitives / textures
            if (isIdentStart(c)) {
                size_t s = pos;
                while (pos < line.size() && (isIdentStart(line[pos]) || (line[pos] >= '0' && line[pos] <= '9'))) ++pos;
                std::string val = line.substr(s, pos - s);
                // classify lightly: keywords/primitives/textures are project-specific;
                // leave as Identifier here — higher-level code can reclassify if needed.
                out.emplace_back(Token::Type::Identifier, val, lineStart + s, lineStart + pos, lineNum);
                continue;
            }

            // unknown single char
            out.emplace_back(Token::Type::Unknown, std::string(1, c), lineStart + pos, lineStart + pos + 1, lineNum);
            ++pos;
        }
    }
};


// ================= RVGG CONFIGURATION =================
struct RVGGConfig {
    // Motif likelihoods
    double pRadial = 0.7, pStack = 0.7, pBand = 0.4, pMirror = 0.4, pDihedral = 0.45;
    double pSpiral = 0.45, pGrid = 0.45, pRandomWalk = 0.35, pFork = 0.35, pRingStack = 0.45;
    double pLSystem = 0.3;
    
    // Ranges
    int spokesMin = 10, spokesMax = 15;
    int stacksMin = 4, stacksMax = 12;
    int bandSegMin = 8, bandSegMax = 16;
    int spiralSegMin = 12, spiralSegMax = 24;
    int gridNMin = 3, gridNMax = 7;
    int forkDepthMin = 3, forkDepthMax = 5;
    int ringNMin = 8, ringNMax = 16;
    int ringLayersMin = 2, ringLayersMax = 6;
    
    // Transforms
    double scaleMin = 0.12, scaleMax = 3.5;
    
    // Textures
    std::string texStrategy = "perMotif";
    std::string texId = "roughConcrete";
    std::vector<std::string> texPool = TEXTURE_PRESETS;
    
    // Attempts
    int tries = 10;
};

// ================= RVGG GENERATOR =================
class RVGG {
private:
    RVGGConfig config;
    std::mt19937 rng;
    std::uniform_real_distribution<double> dist;
    int gid;

public:
    RVGG(const RVGGConfig& cfg = RVGGConfig{}) : config(cfg), rng(std::random_device{}()), dist(0.0, 1.0), gid(0) {}

    // Utility functions
    bool chance(double p) { return dist(rng) < p; }
    
    template<typename T>
    const T& pick(const std::vector<T>& arr) {
        return arr[static_cast<size_t>(dist(rng) * arr.size())];
    }
    
    double range(double a, double b) { return a + (b - a) * dist(rng); }
    int irange(int a, int b) { return static_cast<int>(std::floor(range(a, b + 1))); }
    
    std::string fmt(double n) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(5) << n;
        std::string s = oss.str();
        
        // Remove trailing zeros
        s.erase(s.find_last_not_of('0') + 1, std::string::npos);
        if (s.back() == '.') s += '0';
        
        return s;
    }
    
    std::string pickTexture() {
        const auto& pool = config.texPool.empty() ? TEXTURE_PRESETS : config.texPool;
        return pick(pool);
    }
    
    int uniqueId() { return ++gid; }

    struct MotifResult {
        std::vector<std::string> rules;
        std::vector<std::string> main;
        std::unordered_map<std::string, std::string> metadata;
    };

    // Motif generators
    MotifResult motifSpokes(const std::string& tex) {
        int u = uniqueId();
        std::string prim = "Cube";
        std::string R = fmt(range(1.2, 3.2));
        std::string s = fmt(range(config.scaleMin, config.scaleMax));
        int axis = 1;
        
        std::ostringstream rule;
        rule << "Spokes" << u << " -> R* N" << u << " ( " << config.spokesMin << " " << config.spokesMax << " ) Spokes" << u << "Init\n\n";
        rule << "Spokes" << u << "Init 1 N" << u << " -> R* k" << u << " ( 0 N" << u << "/2 ) Spokes" << u << "Core\n\n";
        rule << "Spokes" << u << "Core 1 N" << u << " k" << u << " ->\n";
        rule << "  A ( 360/N" << u << "*k" << u << " " << axis << " )\n";
        rule << "  [ T ( " << R << " 0 0 ) Element" << u << " ]\n\n";
        rule << "Element" << u << " 1 -> [\n";
        rule << "  DS ( k" << u << "/N" << u << " 1 1 )\n";
        rule << "  A ( 90 0 )\n";
        rule << "  A ( 180 2 )\n";
        rule << "  I ( " << prim << " " << tex << " " << s << " )\n";
        rule << "]";

        return {
            {rule.str()},
            {"Spokes" + std::to_string(u)},
            {{"type", "radial"}}
        };
    }

    MotifResult motifStack(const std::string& tex) {
        int u = uniqueId();
        std::string prim = "Cube";
        std::string h = fmt(range(0.35, 1.8));
        std::string s = fmt(range(config.scaleMin, config.scaleMax));
        
        std::ostringstream rule;
        rule << "Stack" << u << " -> R* H" << u << " ( " << config.stacksMin << " " << config.stacksMax << " ) Stack" << u << "Init\n\n";
        rule << "Stack" << u << "Init 1 H" << u << " -> R* i" << u << " ( 0 H" << u << "/2 ) Stack" << u << "Core\n\n";
        rule << "Stack" << u << "Core 1 H" << u << " i" << u << " ->\n";
        rule << "  A ( 0 1 )\n";
        rule << "  [ T ( 0 " << h << "*i" << u << " 0 ) Element" << u << " ]\n\n";
        rule << "Element" << u << " 1 -> [\n";
        rule << "  DS ( 1 i" << u << "/H" << u << " 1 )\n";
        rule << "  A ( 90 0 )\n";
        rule << "  A ( 180 2 )\n";
        rule << "  I ( " << prim << " " << tex << " " << s << " )\n";
        rule << "]";

        return {
            {rule.str()},
            {"Stack" + std::to_string(u)},
            {{"type", "stack"}}
        };
    }

    // Add more motif methods here (motifBand, motifMirror, etc.)

    struct GenerationResult {
        uint32_t seed;
        std::string program;
        std::unordered_map<std::string, std::vector<std::string>> manifest;
        double score;
    };

    GenerationResult generateGrammar(uint32_t seed = 0) {
        if (seed == 0) seed = std::random_device{}();
        rng.seed(seed);
        
        std::vector<std::string> chosen, rules, main;
        std::unordered_set<std::string> usedTextures;
        
        std::string globalTex = (config.texStrategy == "global") ? 
            (config.texId.empty() ? pickTexture() : config.texId) : "";

        // Generate motifs based on probabilities
        if (chance(config.pRadial)) {
            std::string tex = globalTex.empty() ? pickTexture() : globalTex;
            usedTextures.insert(tex);
            auto result = motifSpokes(tex);
            rules.insert(rules.end(), result.rules.begin(), result.rules.end());
            main.insert(main.end(), result.main.begin(), result.main.end());
            chosen.push_back(result.main[0] + ":" + tex);
        }

        if (chance(config.pStack)) {
            std::string tex = globalTex.empty() ? pickTexture() : globalTex;
            usedTextures.insert(tex);
            auto result = motifStack(tex);
            rules.insert(rules.end(), result.rules.begin(), result.rules.end());
            main.insert(main.end(), result.main.begin(), result.main.end());
            chosen.push_back(result.main[0] + ":" + tex);
        }

        // Ensure at least one motif
        if (main.empty()) {
            std::string tex = globalTex.empty() ? pickTexture() : globalTex;
            usedTextures.insert(tex);
            auto result = motifSpokes(tex);
            rules.insert(rules.end(), result.rules.begin(), result.rules.end());
            main.insert(main.end(), result.main.begin(), result.main.end());
            chosen.push_back(result.main[0] + ":" + tex);
        }

        // Create start rule
        std::string pre = "S ( " + fmt(range(0.9, 1.2)) + " " + fmt(range(0.9, 1.2)) + " " + fmt(range(0.9, 1.2)) + " )";
        std::string post = "T ( " + fmt(range(-0.8, 0.8)) + " " + fmt(range(-0.4, 0.4)) + " " + fmt(range(-0.8, 0.8)) + " )";
        
        std::ostringstream mainStr;
        for (size_t i = 0; i < main.size(); ++i) {
            if (i > 0) mainStr << " ";
            mainStr << main[i];
        }
        
        std::string start = "Start 1 -> " + pre + " | " + mainStr.str() + " | " + post;

        // Combine program
        std::ostringstream program;
        program << start << "\n\n";
        for (const auto& rule : rules) {
            program << rule << "\n\n";
        }

        std::unordered_map<std::string, std::vector<std::string>> manifest;
        manifest["motifs"] = chosen;
        manifest["textures"] = std::vector<std::string>(usedTextures.begin(), usedTextures.end());

        return {
            seed,
            program.str(),
            manifest,
            computeFitness(program.str())
        };
    }

    double computeFitness(const std::string& program) {
        double score = 50.0;
        
        // Count instances
        size_t instCount = 0;
        size_t pos = 0;
        while ((pos = program.find("I (", pos)) != std::string::npos) {
            instCount++;
            pos += 3;
        }
        score += std::min(instCount * 2.5, 25.0);
        
        // Count rules
        size_t ruleCount = 0;
        pos = 0;
        while ((pos = program.find("->", pos)) != std::string::npos) {
            ruleCount++;
            pos += 2;
        }
        score += std::min(ruleCount * 2.0, 16.0);
        
        // Symmetry bonus
        if (program.find("Spokes") != std::string::npos || 
            program.find("Dihedral") != std::string::npos ||
            program.find("Ring") != std::string::npos) {
            score += 6.0;
        }
        
        // Start rule validation
        if (program.find("Start") == std::string::npos) {
            score -= 30.0;
        }
        
        return std::max(0.0, score);
    }

    GenerationResult generateValid(uint32_t seed = 0) {
        GenerationResult best;
        best.score = -1.0;
        
        for (int k = 0; k < config.tries; ++k) {
            uint32_t currentSeed = (seed != 0) ? (seed + k) : (std::random_device{}() + k);
            auto result = generateGrammar(currentSeed);
            
            if (best.score < 0 || result.score > best.score) {
                best = result;
            }
        }
        
        return best;
    }
};

// ================= SHAPE GRAMMAR =================
struct Shape {
    std::string type;
    double x, y, z;
    double width, height, depth;
    int level;
    
    Shape(const std::string& t = "", double x = 0, double y = 0, double z = 0,
          double w = 1, double h = 1, double d = 1, int lvl = 0)
        : type(t), x(x), y(y), z(z), width(w), height(h), depth(d), level(lvl) {}
};

class ShapeGrammar {
private:
    std::vector<Shape> shapes;
    std::vector<std::function<bool(const Shape&)>> patterns;
    std::vector<std::function<std::vector<Shape>(const Shape&)>> replacements;
    std::mt19937 rng;
    std::uniform_real_distribution<double> dist;

public:
    ShapeGrammar(uint32_t seed = 0) : rng(seed ? seed : std::random_device{}()), dist(0.0, 1.0) {}

    void addShape(const Shape& shape) {
        shapes.push_back(shape);
    }

    void addRule(std::function<bool(const Shape&)> pattern, 
                 std::function<std::vector<Shape>(const Shape&)> replacement) {
        patterns.push_back(pattern);
        replacements.push_back(replacement);
    }

    std::vector<Shape> generate(int iterations = 1) {
        std::vector<Shape> currentShapes = shapes;

        for (int i = 0; i < iterations; ++i) {
            std::vector<Shape> newShapes;

            for (const auto& shape : currentShapes) {
                bool replaced = false;

                for (size_t j = 0; j < patterns.size(); ++j) {
                    if (patterns[j](shape)) {
                        auto replacementShapes = replacements[j](shape);
                        newShapes.insert(newShapes.end(), replacementShapes.begin(), replacementShapes.end());
                        replaced = true;
                        break;
                    }
                }

                if (!replaced) {
                    newShapes.push_back(shape);
                }
            }

            currentShapes = newShapes;
        }

        return currentShapes;
    }

    std::string toRVGG(const std::vector<std::string>& textures = TEXTURE_PRESETS) {
        std::ostringstream program;
        std::vector<std::string> mainRules;
        int gid = 0;

        for (const auto& shape : shapes) {
            ++gid;
            std::string tex = textures[static_cast<size_t>(dist(rng) * textures.size())];
            double scale = 0.5 + dist(rng) * 1.5;

            program << "Shape" << gid << " 1 -> [\n";
            program << "  S ( " << shape.width << " " << shape.height << " " << shape.depth << " )\n";
            program << "  T ( " << shape.x << " " << shape.y << " " << shape.z << " )\n";
            program << "  I ( Cube " << tex << " " << std::fixed << std::setprecision(3) << scale << " )\n";
            program << "]\n\n";

            mainRules.push_back("Shape" + std::to_string(gid));
        }

        std::ostringstream start;
        start << "Start 1 -> ";
        for (size_t i = 0; i < mainRules.size(); ++i) {
            if (i > 0) start << " ";
            start << mainRules[i];
        }

        return start.str() + "\n\n" + program.str();
    }


    static ShapeGrammar createBuildingGrammar(uint32_t seed = 0) {
        ShapeGrammar grammar(seed);

        // Initial ground floor
        grammar.addShape(Shape("floor", 0, 0, 0, 20, 4, 20, 0));

        // Rule: Add floors
        grammar.addRule(
            [](const Shape& shape) { return shape.type == "floor" && shape.level < 10; },
            [&grammar](const Shape& shape) {
                std::vector<Shape> result;
                result.push_back(shape); // Keep original
                
                // Add new floor above
                Shape newFloor("floor", 0, shape.y + shape.height, 0,
                              shape.width * 0.95, 3, shape.depth * 0.95, shape.level + 1);
                result.push_back(newFloor);
                
                return result;
            }
        );

        // Rule: Add windows
        grammar.addRule(
            [](const Shape& shape) { return shape.type == "floor"; },
            [](const Shape& shape) {
                std::vector<Shape> result;
                result.push_back(shape); // Keep original
                
                // Add windows
                int windowCount = static_cast<int>(shape.width / 2);
                for (int i = 0; i < windowCount; ++i) {
                    Shape window("window", shape.x + i * 2, shape.y, shape.z,
                               1, 2, 0.1, shape.level);
                    result.push_back(window);
                }
                
                return result;
            }
        );

        return grammar;
    }

    static ShapeGrammar createCityGrammar(uint32_t seed = 0) {
        ShapeGrammar grammar(seed);
        std::mt19937 rng(seed);
        std::uniform_real_distribution<double> dist(0.0, 1.0);

        const int gridSize = 5;
        const double blockSize = 30.0;

        for (int i = 0; i < gridSize; ++i) {
            for (int j = 0; j < gridSize; ++j) {
                if (dist(rng) > 0.3) { // 70% chance of building
                    double height = 5 + dist(rng) * 20;
                    Shape building("building", 
                                 i * blockSize, 0, j * blockSize,
                                 blockSize * 0.8, height, blockSize * 0.8, 0);
                    grammar.addShape(building);
                }
            }
        }

        return grammar;
    }
};

// ================= GRAMMAR ANALYZER =================
struct ComplexityAnalysis {
    size_t length;
    size_t uniqueSymbols;
    int depth;
    double branchingFactor;
    double growthRate;
};

struct RVGGAnalysis {
    size_t ruleCount;
    size_t instanceCount;
    size_t transformCount;
    std::vector<std::string> motifs;
    std::unordered_set<std::string> textures;
    double complexity;
};

struct Suggestion {
    enum Type { WARNING, INFO, ERROR };
    Type type;
    std::string message;
    std::string severity;
    
    Suggestion(Type t, const std::string& msg, const std::string& sev)
        : type(t), message(msg), severity(sev) {}
};

class GrammarAnalyzer {
public:
    static ComplexityAnalysis analyzeComplexity(ProGen3DGrammar& grammar) {
        std::string generated = grammar.generate();
        
        std::unordered_set<char> uniqueChars;
        for (char c : generated) {
            uniqueChars.insert(c);
        }
        
        return {
            generated.length(),
            uniqueChars.size(),
            grammar.getIterations(),
            calculateBranchingFactor(generated),
            static_cast<double>(generated.length()) / grammar.getAxiom().length()
        };
    }

private:
    static double calculateBranchingFactor(const std::string& generated) {
        size_t branches = 0;
        for (char c : generated) {
            if (c == '[') branches++;
        }
        return static_cast<double>(branches) / generated.length();
    }

public:
    static bool detectInfiniteLoops(const ProGen3DGrammar& grammar) {
        // Simplified cycle detection - would need more sophisticated implementation
        // for full cycle detection in grammar rules
        return false; // Placeholder
    }

    static std::vector<Suggestion> suggestOptimizations(ProGen3DGrammar& grammar) {
        std::vector<Suggestion> suggestions;
        ComplexityAnalysis analysis = analyzeComplexity(grammar);

        if (analysis.length > 10000) {
            suggestions.emplace_back(Suggestion::WARNING,
                "Generated string is very long. Consider reducing iterations.",
                "high");
        }

        if (analysis.branchingFactor > 0.05) { // 5% branching factor threshold
            suggestions.emplace_back(Suggestion::INFO,
                "High branching factor detected. This may cause exponential growth.",
                "medium");
        }

        if (detectInfiniteLoops(grammar)) {
            suggestions.emplace_back(Suggestion::ERROR,
                "Potential infinite loop detected in grammar rules.",
                "critical");
        }

        return suggestions;
    }

    static RVGGAnalysis analyzeRVGG(const std::string& program) {
        RVGGAnalysis analysis;
        analysis.ruleCount = 0;
        analysis.instanceCount = 0;
        analysis.transformCount = 0;
        analysis.complexity = 0;

        // Count rules (simplified pattern matching)
        size_t pos = 0;
        while ((pos = program.find("->", pos)) != std::string::npos) {
            analysis.ruleCount++;
            pos += 2;
        }

        // Count instances
        pos = 0;
        while ((pos = program.find("I (", pos)) != std::string::npos) {
            analysis.instanceCount++;
            pos += 3;
        }

        // Count transforms
        pos = 0;
        std::vector<std::string> transforms = {"T (", "S (", "A (", "R ("};
        for (const auto& transform : transforms) {
            pos = 0;
            while ((pos = program.find(transform, pos)) != std::string::npos) {
                analysis.transformCount++;
                pos += transform.length();
            }
        }

        // Detect motifs
        std::vector<std::pair<std::string, std::string>> motifPatterns = {
            {"Radial", "Spokes"},
            {"Dihedral", "Dihedral"},
            {"Stack", "Stack"},
            {"Band", "Band"},
            {"Mirror", "Mirror"},
            {"Spiral", "Spiral"},
            {"Grid", "Grid"},
            {"Walk", "Walk"},
            {"Fork", "Fork"},
            {"Ring", "Ring"},
            {"L-System", "LSystem"}
        };

        for (const auto& motif : motifPatterns) {
            if (program.find(motif.second) != std::string::npos) {
                analysis.motifs.push_back(motif.first);
            }
        }

        // Calculate complexity score
        analysis.complexity = 
            analysis.ruleCount * 2.0 +
            analysis.instanceCount * 1.5 +
            analysis.transformCount * 0.5 +
            analysis.motifs.size() * 5.0;

        return analysis;
    }
};

// ================= GRAMMAR VISUALIZER =================
struct Canvas {
    int width, height;
    std::vector<std::vector<char>> buffer;
    
    Canvas(int w, int h) : width(w), height(h), buffer(h, std::vector<char>(w, ' ')) {}
    
    void setPixel(int x, int y, char c = '*') {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            buffer[y][x] = c;
        }
    }
    
    void clear() {
        for (auto& row : buffer) {
            std::fill(row.begin(), row.end(), ' ');
        }
    }
    
    std::string toString() const {
        std::ostringstream oss;
        for (const auto& row : buffer) {
            for (char c : row) {
                oss << c;
            }
            oss << '\n';
        }
        return oss.str();
    }
};

struct VisualizationOptions {
    int lineWidth = 1;
    char strokeChar = '*';
    char fillChar = '#';
    double scale = 10.0;
    int centerX = 40;
    int centerY = 20;
    double rotationX = 0.0;
    double rotationY = 0.0;
    double rotationZ = 0.0;
};

class GrammarVisualizer {
private:
    std::unique_ptr<Canvas> canvas;
    VisualizationOptions options;

public:
    GrammarVisualizer(int width, int height) 
        : canvas(std::make_unique<Canvas>(width, height)) {
        options.centerX = width / 2;
        options.centerY = height / 2;
    }

    void draw(const std::vector<Command>& commands, const VisualizationOptions& opts = {}) {
        options = opts;
        canvas->clear();

        std::vector<Vec3> polygonVertices;
        bool inPolygon = false;

        for (const auto& command : commands) {
            switch (command.type) {
                case Command::LINE:
                    drawLine(command.from, command.to);
                    break;

                case Command::START_POLYGON:
                    inPolygon = true;
                    polygonVertices.clear();
                    break;

                case Command::VERTEX:
                    if (inPolygon) {
                        polygonVertices.push_back(command.position);
                    }
                    break;

                case Command::END_POLYGON:
                    if (!polygonVertices.empty()) {
                        drawPolygon(polygonVertices);
                    }
                    inPolygon = false;
                    polygonVertices.clear();
                    break;
            }
        }
    }

    void draw3D(const std::vector<Command>& commands, const VisualizationOptions& opts = {}) {
        options = opts;
        
        // Project 3D commands to 2D
        std::vector<Command> projected;
        for (const auto& cmd : commands) {
            if (cmd.type == Command::LINE) {
                Vec3 from2D = project3D(cmd.from);
                Vec3 to2D = project3D(cmd.to);
                projected.emplace_back(Command::LINE, from2D, to2D);
            } else {
                projected.push_back(cmd);
            }
        }

        draw(projected, opts);
    }

private:
    Vec3 project3D(const Vec3& point) {
        Vec3 p = point;

        // Apply rotations
        if (options.rotationX != 0.0) {
            double cosX = std::cos(options.rotationX);
            double sinX = std::sin(options.rotationX);
            double y1 = p.y * cosX - p.z * sinX;
            double z1 = p.y * sinX + p.z * cosX;
            p.y = y1;
            p.z = z1;
        }

        if (options.rotationY != 0.0) {
            double cosY = std::cos(options.rotationY);
            double sinY = std::sin(options.rotationY);
            double x1 = p.x * cosY + p.z * sinY;
            double z1 = -p.x * sinY + p.z * cosY;
            p.x = x1;
            p.z = z1;
        }

        if (options.rotationZ != 0.0) {
            double cosZ = std::cos(options.rotationZ);
            double sinZ = std::sin(options.rotationZ);
            double x1 = p.x * cosZ - p.y * sinZ;
            double y1 = p.x * sinZ + p.y * cosZ;
            p.x = x1;
            p.y = y1;
        }

        // Perspective projection
        double distance = 5.0;
        double factor = distance / (distance + p.z);

        return Vec3(p.x * factor, p.y * factor, p.z);
    }

    void drawLine(const Vec3& from, const Vec3& to) {
        int x1 = static_cast<int>(options.centerX + from.x * options.scale);
        int y1 = static_cast<int>(options.centerY - from.y * options.scale);
        int x2 = static_cast<int>(options.centerX + to.x * options.scale);
        int y2 = static_cast<int>(options.centerY - to.y * options.scale);

        // Bresenham's line algorithm (simplified)
        int dx = std::abs(x2 - x1);
        int dy = std::abs(y2 - y1);
        int sx = (x1 < x2) ? 1 : -1;
        int sy = (y1 < y2) ? 1 : -1;
        int err = dx - dy;

        int x = x1, y = y1;
        while (true) {
            canvas->setPixel(x, y, options.strokeChar);
            
            if (x == x2 && y == y2) break;
            
            int e2 = 2 * err;
            if (e2 > -dy) {
                err -= dy;
                x += sx;
            }
            if (e2 < dx) {
                err += dx;
                y += sy;
            }
        }
    }

    void drawPolygon(const std::vector<Vec3>& vertices) {
        // Simple polygon drawing - connect vertices
        for (size_t i = 0; i < vertices.size(); ++i) {
            size_t next = (i + 1) % vertices.size();
            drawLine(vertices[i], vertices[next]);
        }
    }

public:
    std::string render() const {
        return canvas->toString();
    }

    void clear() {
        canvas->clear();
    }

    void setRotation(double x, double y, double z) {
        options.rotationX = x;
        options.rotationY = y;
        options.rotationZ = z;
    }
};

// ================= UNIFIED GRAMMAR SYSTEM =================
class ProGen3DUnifiedGrammar {
public:
    enum Mode { RVGG_MODE, LSYSTEM_MODE, SHAPE_MODE, HYBRID_MODE };

private:
    std::unique_ptr<RVGG> rvgg;
    std::unique_ptr<ProGen3DGrammar> lsystem;
    std::unique_ptr<ShapeGrammar> shapeGrammar;
    Mode mode;

public:
    ProGen3DUnifiedGrammar(Mode m = RVGG_MODE, const RVGGConfig& rvggConfig = {})
        : mode(m), rvgg(std::make_unique<RVGG>(rvggConfig)) {}

    struct UnifiedResult {
        uint32_t seed;
        std::string program;
        std::vector<Command> commands;
        std::unordered_map<std::string, std::vector<std::string>> manifest;
        double score;
        std::string type;
    };

    UnifiedResult generate(uint32_t seed = 0) {
        if (seed == 0) seed = std::random_device{}();

        switch (mode) {
            case RVGG_MODE:
                return generateRVGG(seed);
            case LSYSTEM_MODE:
                return generateLSystem(seed);
            case SHAPE_MODE:
                return generateShapeGrammar(seed);
            case HYBRID_MODE:
                return generateHybrid(seed);
            default:
                return generateRVGG(seed);
        }
    }

private:
    UnifiedResult generateRVGG(uint32_t seed) {
        auto result = rvgg->generateValid(seed);
        
        UnifiedResult unified;
        unified.seed = result.seed;
        unified.program = result.program;
        unified.manifest = result.manifest;
        unified.score = result.score;
        unified.type = "rvgg";
        
        return unified;
    }

    UnifiedResult generateLSystem(uint32_t seed) {
        // Create a random L-System from library
        std::vector<std::function<std::unique_ptr<ProGen3DGrammar>()>> templates = {
            GrammarLibrary::createTreeSimple,
            GrammarLibrary::createTreeComplex,
            GrammarLibrary::createBush,
            GrammarLibrary::createPlant3D,
            GrammarLibrary::createStochasticTree
        };

        std::mt19937 rng(seed);
        auto templateFunc = templates[rng() % templates.size()];
        lsystem = templateFunc();

        std::string lstring = lsystem->generate();
        std::vector<Command> commands = lsystem->interpret(lstring);

        UnifiedResult unified;
        unified.seed = seed;
        unified.program = lstring;
        unified.commands = commands;
        unified.type = "lsystem";
        unified.score = static_cast<double>(lstring.length()); // Simple scoring
        
        unified.manifest["type"] = {"lsystem"};
        unified.manifest["length"] = {std::to_string(lstring.length())};
        unified.manifest["commands"] = {std::to_string(commands.size())};
        
        return unified;
    }

    UnifiedResult generateShapeGrammar(uint32_t seed) {
        shapeGrammar = std::make_unique<ShapeGrammar>(ShapeGrammar::createBuildingGrammar(seed));
        std::vector<Shape> shapes = shapeGrammar->generate(10);
        std::string program = shapeGrammar->toRVGG();

        UnifiedResult unified;
        unified.seed = seed;
        unified.program = program;
        unified.type = "shape";
        unified.score = static_cast<double>(shapes.size() * 10); // Simple scoring
        
        unified.manifest["type"] = {"shape"};
        unified.manifest["shapes"] = {std::to_string(shapes.size())};
        
        return unified;
    }

    UnifiedResult generateHybrid(uint32_t seed) {
        auto rvggResult = generateRVGG(seed);
        auto lsystemResult = generateLSystem(seed + 1);

        // Merge programs
        std::string hybridProgram = rvggResult.program + "\n\n// L-System Integration\n" + lsystemResult.program;

        UnifiedResult unified;
        unified.seed = seed;
        unified.program = hybridProgram;
        unified.commands = lsystemResult.commands;
        unified.score = (rvggResult.score + lsystemResult.score) / 2.0;
        unified.type = "hybrid";
        
        // Merge manifests
        unified.manifest = rvggResult.manifest;
        for (const auto& pair : lsystemResult.manifest) {
            unified.manifest[pair.first] = pair.second;
        }
        
        return unified;
    }

public:
    // Analysis
    RVGGAnalysis analyze(const std::string& program) {
        return GrammarAnalyzer::analyzeRVGG(program);
    }

    // Visualization
    std::unique_ptr<GrammarVisualizer> visualize(int width, int height, 
                                                const std::string& program, 
                                                const VisualizationOptions& options = {}) {
        auto visualizer = std::make_unique<GrammarVisualizer>(width, height);
        
        if (mode == LSYSTEM_MODE && lsystem) {
            std::vector<Command> commands = lsystem->interpret(program);
            visualizer->draw3D(commands, options);
        } else {
            // For RVGG, would need additional parsing
            // This is a placeholder - full implementation would require RVGG parser
        }

        return visualizer;
    }

    // Getters/Setters
    Mode getMode() const { return mode; }
    void setMode(Mode m) { mode = m; }
};

} // namespace ProGen3D

#endif // PROGEN3D_UNIFIED_GRAMMAR_HPP
