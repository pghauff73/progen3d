#include "editor/service/PreviewMaterialLibrary.h"
#include "editor/service/RenderRowExecutor.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <utility>

namespace preview_materials {
namespace {

struct ParsedMaterialSpec {
	std::string name;
	std::string family = "blend";
	std::string description;
	std::vector<std::string> color_names;
	std::vector<std::string> descriptors;
	std::vector<MaterialSwatch> palette;
	std::string wood_species;
	float opacity = 1.0f;
	float reflectance = 0.28f;
	float smoothness = 0.45f;
	float height_scale = 0.04f;
	float emission_strength = 0.0f;
	float uv_rotation_speed = 0.0f;
	float uv_activity = 0.0f;
	float pulse_strength = 0.0f;
	float normal_strength = 1.0f;
	float transmission = 0.0f;
	float index_of_refraction = 1.5f;
	float thickness = 0.01f;
	float anisotropic = 0.0f;
	float sheen = 0.0f;
	float clearcoat = 0.0f;
	float clearcoat_roughness = 0.0f;
	MaterialSwatch attenuation_color{1.0f, 1.0f, 1.0f};
	bool two_tone = false;
	bool vertical_pattern = false;
	bool emphasize_veins = false;
	uint32_t seed = 0;
};

struct MaterialTokenMatch {
	std::size_t position = 0;
	const MaterialLexeme *lexeme = nullptr;
};

float clamp01(float value)
{
	return std::clamp(value, 0.0f, 1.0f);
}

float lerp_scalar(float a, float b, float t)
{
	return a + (b - a) * t;
}

float smooth_curve(float value)
{
	value = clamp01(value);
	return value * value * (3.0f - 2.0f * value);
}

MaterialSwatch make_swatch(float r, float g, float b)
{
	return MaterialSwatch{r, g, b};
}

MaterialSwatch mix_swatch(const MaterialSwatch &a, const MaterialSwatch &b, float t)
{
	return make_swatch(lerp_scalar(a.r, b.r, t),
	                   lerp_scalar(a.g, b.g, t),
	                   lerp_scalar(a.b, b.b, t));
}

MaterialSwatch scale_swatch(const MaterialSwatch &color, float scale)
{
	return make_swatch(clamp01(color.r * scale),
	                   clamp01(color.g * scale),
	                   clamp01(color.b * scale));
}

MaterialSwatch lighten_swatch(const MaterialSwatch &color, float amount)
{
	return mix_swatch(color, make_swatch(1.0f, 1.0f, 1.0f), clamp01(amount));
}

MaterialSwatch darken_swatch(const MaterialSwatch &color, float amount)
{
	return mix_swatch(color, make_swatch(0.0f, 0.0f, 0.0f), clamp01(amount));
}

uint32_t stable_hash(const std::string &text)
{
	uint32_t hash = 2166136261u;
	for (unsigned char character : text) {
		hash ^= static_cast<uint32_t>(character);
		hash *= 16777619u;
	}
	return hash;
}

float lattice_noise(int x, int y, uint32_t seed)
{
	uint32_t value = seed;
	value ^= static_cast<uint32_t>(x) * 374761393u;
	value ^= static_cast<uint32_t>(y) * 668265263u;
	value = (value ^ (value >> 13u)) * 1274126177u;
	value ^= value >> 16u;
	return static_cast<float>(value & 0x00ffffffu) / static_cast<float>(0x01000000u);
}

float value_noise(float x, float y, uint32_t seed)
{
	const int x0 = static_cast<int>(std::floor(x));
	const int y0 = static_cast<int>(std::floor(y));
	const int x1 = x0 + 1;
	const int y1 = y0 + 1;
	const float tx = smooth_curve(x - static_cast<float>(x0));
	const float ty = smooth_curve(y - static_cast<float>(y0));

	const float n00 = lattice_noise(x0, y0, seed);
	const float n10 = lattice_noise(x1, y0, seed);
	const float n01 = lattice_noise(x0, y1, seed);
	const float n11 = lattice_noise(x1, y1, seed);
	const float nx0 = lerp_scalar(n00, n10, tx);
	const float nx1 = lerp_scalar(n01, n11, tx);
	return lerp_scalar(nx0, nx1, ty);
}

float fractal_noise(float x, float y, uint32_t seed, int octaves = 5)
{
	float amplitude = 0.5f;
	float frequency = 1.0f;
	float total = 0.0f;
	float normalization = 0.0f;
	for (int octave = 0; octave < octaves; ++octave) {
		total += value_noise(x * frequency, y * frequency, seed + static_cast<uint32_t>(octave * 977u)) * amplitude;
		normalization += amplitude;
		amplitude *= 0.5f;
		frequency *= 2.0f;
	}
	return normalization > 0.0f ? total / normalization : 0.0f;
}

std::string join_strings(const std::vector<std::string> &parts)
{
	std::ostringstream stream;
	for (std::size_t index = 0; index < parts.size(); ++index) {
		if (index > 0) {
			stream << ", ";
		}
		stream << parts[index];
	}
	return stream.str();
}

template <std::size_t N>
std::vector<MaterialTokenMatch> scan_material_lexemes(const std::string &name,
                                                      const std::array<MaterialLexeme, N> &lexemes)
{
	std::vector<MaterialTokenMatch> matches;
	for (std::size_t position = 0; position < name.size();) {
		const MaterialLexeme *best_match = nullptr;
		for (const MaterialLexeme &lexeme : lexemes) {
			if (lexeme.token.empty() ||
			    position + lexeme.token.size() > name.size() ||
			    name.compare(position, lexeme.token.size(), lexeme.token) != 0) {
				continue;
			}
			if (best_match == nullptr || lexeme.token.size() > best_match->token.size()) {
				best_match = &lexeme;
			}
		}
		if (best_match != nullptr) {
			matches.push_back({position, best_match});
			position += best_match->token.size();
		} else {
			++position;
		}
	}
	return matches;
}

bool material_matches_token(const std::vector<MaterialTokenMatch> &matches, std::string_view token)
{
	for (const MaterialTokenMatch &match : matches) {
		if (match.lexeme != nullptr && match.lexeme->token == token) {
			return true;
		}
	}
	return false;
}

void sort_and_unique_strings(std::vector<std::string> *values)
{
	if (values == nullptr) {
		return;
	}
	std::sort(values->begin(), values->end());
	values->erase(std::unique(values->begin(), values->end()), values->end());
}

ParsedMaterialSpec parse_material_name(const std::string &material_name)
{
	ParsedMaterialSpec spec;
	spec.name = material_name.empty() ? "softwhitematteplaster" : material_name;
	spec.seed = stable_hash(spec.name);
	const auto &lexemes = material_lexemes();
	const std::vector<MaterialTokenMatch> matches = scan_material_lexemes(spec.name, lexemes);
	for (const MaterialTokenMatch &match : matches) {
		if (spec.color_names.size() >= 3) {
			break;
		}
		if (match.lexeme == nullptr || match.lexeme->kind != MaterialLexemeKind::Color) {
			continue;
		}
		const std::string color_name(match.lexeme->token);
		if (std::find(spec.color_names.begin(), spec.color_names.end(), color_name) == spec.color_names.end()) {
			spec.color_names.push_back(color_name);
			spec.palette.push_back(match.lexeme->swatch);
		}
	}
	const bool has_explicit_colors = !spec.palette.empty();
	const bool wants_boards = material_matches_token(matches, "boards");
	const bool wants_floral = material_matches_token(matches, "floral");
	const bool wants_horizontal = material_matches_token(matches, "horizontal");
	spec.vertical_pattern = material_matches_token(matches, "vertical");
	if (wants_horizontal) {
		spec.vertical_pattern = false;
	}
	spec.two_tone = material_matches_token(matches, "twotone") ||
	                material_matches_token(matches, "duotone");
	spec.emphasize_veins = material_matches_token(matches, "veins") ||
	                       material_matches_token(matches, "veined");

	auto set_palette = [&](const std::vector<std::string> &labels,
	                      const MaterialSwatch &first,
	                      const MaterialSwatch &second,
	                      const MaterialSwatch &third) {
		spec.color_names = labels;
		spec.palette = {first, second, third};
	};

	if (material_matches_token(matches, "oak")) {
		spec.wood_species = "oak";
	} else if (material_matches_token(matches, "walnut")) {
		spec.wood_species = "walnut";
	} else if (material_matches_token(matches, "pine")) {
		spec.wood_species = "pine";
	} else if (material_matches_token(matches, "cedar")) {
		spec.wood_species = "cedar";
	} else if (material_matches_token(matches, "birch")) {
		spec.wood_species = "birch";
	}

	if (!has_explicit_colors) {
		if (spec.wood_species == "oak") {
			set_palette({"oak", "natural"},
			            make_swatch(0.74f, 0.56f, 0.34f),
			            make_swatch(0.48f, 0.35f, 0.19f),
			            make_swatch(0.88f, 0.74f, 0.50f));
		} else if (spec.wood_species == "walnut") {
			set_palette({"walnut", "natural"},
			            make_swatch(0.42f, 0.28f, 0.18f),
			            make_swatch(0.24f, 0.16f, 0.10f),
			            make_swatch(0.58f, 0.40f, 0.24f));
		} else if (spec.wood_species == "pine") {
			set_palette({"pine", "natural"},
			            make_swatch(0.84f, 0.72f, 0.46f),
			            make_swatch(0.64f, 0.50f, 0.27f),
			            make_swatch(0.94f, 0.85f, 0.64f));
		} else if (spec.wood_species == "cedar") {
			set_palette({"cedar", "natural"},
			            make_swatch(0.72f, 0.40f, 0.24f),
			            make_swatch(0.46f, 0.23f, 0.14f),
			            make_swatch(0.84f, 0.58f, 0.38f));
		} else if (spec.wood_species == "birch") {
			set_palette({"birch", "natural"},
			            make_swatch(0.88f, 0.80f, 0.62f),
			            make_swatch(0.68f, 0.58f, 0.38f),
			            make_swatch(0.96f, 0.91f, 0.74f));
		} else if (wants_floral) {
			set_palette({"rose", "sage"},
			            make_swatch(0.86f, 0.60f, 0.70f),
			            make_swatch(0.92f, 0.89f, 0.82f),
			            make_swatch(0.48f, 0.66f, 0.46f));
		}
	}

	if (spec.palette.empty()) {
		spec.color_names = {"white", "grey"};
		spec.palette = {make_swatch(0.90f, 0.91f, 0.92f),
		                make_swatch(0.60f, 0.62f, 0.66f)};
	}
	if (spec.palette.size() == 1) {
		spec.palette.push_back(lighten_swatch(spec.palette.front(), 0.22f));
	}

	if (wants_floral) {
		spec.family = "floral";
		spec.reflectance = 0.20f;
		spec.smoothness = 0.40f;
		spec.height_scale = 0.050f;
	} else if (material_matches_token(matches, "metal")) {
		spec.family = "metal";
		spec.reflectance = 0.88f;
		spec.smoothness = 0.78f;
		spec.height_scale = 0.030f;
		} else if (material_matches_token(matches, "glass")) {
			spec.family = "glass";
			spec.opacity = 0.38f;
			spec.reflectance = 0.82f;
			spec.smoothness = 0.95f;
			spec.height_scale = 0.010f;
			spec.transmission = 0.94f;
			spec.index_of_refraction = 1.50f;
			spec.thickness = 0.018f;
			spec.clearcoat = 0.32f;
			spec.clearcoat_roughness = 0.05f;
	} else if (material_matches_token(matches, "neon")) {
		spec.family = "neon";
		spec.reflectance = 0.58f;
		spec.smoothness = 0.94f;
		spec.height_scale = 0.025f;
		spec.emission_strength = 1.15f;
	} else if (material_matches_token(matches, "grass")) {
		spec.family = "grass";
		spec.reflectance = 0.10f;
		spec.smoothness = 0.18f;
		spec.height_scale = 0.075f;
	} else if (material_matches_token(matches, "brick")) {
		spec.family = "brick";
		spec.reflectance = 0.12f;
		spec.smoothness = 0.26f;
		spec.height_scale = 0.085f;
	} else if (wants_boards) {
		spec.family = "boards";
		spec.reflectance = 0.22f;
		spec.smoothness = 0.40f;
		spec.height_scale = 0.060f;
	} else if (material_matches_token(matches, "wood") || !spec.wood_species.empty()) {
		spec.family = "wood";
		spec.reflectance = 0.20f;
		spec.smoothness = 0.38f;
		spec.height_scale = 0.055f;
	} else if (material_matches_token(matches, "marble")) {
		spec.family = "marble";
		spec.reflectance = 0.30f;
		spec.smoothness = 0.72f;
		spec.height_scale = 0.032f;
	} else if (material_matches_token(matches, "stone") || material_matches_token(matches, "rock")) {
		spec.family = "stone";
		spec.reflectance = 0.22f;
		spec.smoothness = 0.44f;
		spec.height_scale = 0.065f;
	} else if (material_matches_token(matches, "concrete")) {
		spec.family = "concrete";
		spec.reflectance = 0.16f;
		spec.smoothness = 0.32f;
		spec.height_scale = 0.070f;
	} else if (material_matches_token(matches, "ceramic") || material_matches_token(matches, "tile")) {
		spec.family = "ceramic";
		spec.reflectance = 0.46f;
		spec.smoothness = 0.84f;
		spec.height_scale = 0.040f;
	} else if (material_matches_token(matches, "plastic")) {
		spec.family = "plastic";
		spec.reflectance = 0.40f;
		spec.smoothness = 0.76f;
		spec.height_scale = 0.024f;
	} else if (material_matches_token(matches, "paint") || material_matches_token(matches, "plaster")) {
		spec.family = "paint";
		spec.reflectance = 0.26f;
		spec.smoothness = 0.58f;
		spec.height_scale = 0.028f;
	}

	if (material_matches_token(matches, "transparent")) {
		spec.opacity = 0.24f;
	}
	if (material_matches_token(matches, "translucent")) {
		spec.opacity = 0.45f;
	}
	if (material_matches_token(matches, "smoky")) {
		spec.opacity = std::min(spec.opacity, 0.68f);
		spec.attenuation_color = make_swatch(0.54f, 0.60f, 0.66f);
		spec.thickness = std::max(spec.thickness, 0.045f);
	}
	if (material_matches_token(matches, "opaque")) {
		spec.opacity = 1.0f;
	}

	if (material_matches_token(matches, "shiny") ||
	    material_matches_token(matches, "glossy") ||
	    material_matches_token(matches, "polished") ||
	    material_matches_token(matches, "reflective")) {
		spec.reflectance = std::max(spec.reflectance, 0.78f);
		spec.smoothness = std::max(spec.smoothness, 0.86f);
	}
	if (material_matches_token(matches, "mirror")) {
		spec.reflectance = std::max(spec.reflectance, 0.96f);
		spec.smoothness = std::max(spec.smoothness, 0.98f);
		spec.height_scale = std::min(spec.height_scale, 0.014f);
	}
	if (material_matches_token(matches, "frosted")) {
		spec.smoothness = std::min(spec.smoothness, 0.42f);
		spec.transmission = std::max(spec.transmission, 0.78f);
		spec.opacity = std::max(spec.opacity, 0.52f);
		spec.normal_strength = std::max(spec.normal_strength, 1.35f);
	}
	if (material_matches_token(matches, "crystal")) {
		spec.family = "glass";
		spec.smoothness = 0.99f;
		spec.reflectance = 0.90f;
		spec.transmission = 0.98f;
		spec.index_of_refraction = 1.52f;
		spec.opacity = 0.30f;
	}
	if (material_matches_token(matches, "brushed")) {
		spec.anisotropic = 0.72f;
		spec.smoothness = std::min(spec.smoothness, 0.66f);
	}
	if (material_matches_token(matches, "velvet")) {
		spec.sheen = 0.82f;
		spec.smoothness = std::min(spec.smoothness, 0.38f);
	}
	if (material_matches_token(matches, "clearcoat")) {
		spec.clearcoat = 1.0f;
		spec.clearcoat_roughness = 0.08f;
	}
	if (material_matches_token(matches, "smooth") || material_matches_token(matches, "satin")) {
		spec.smoothness = std::max(spec.smoothness, 0.78f);
	}
	if (material_matches_token(matches, "rough") ||
	    material_matches_token(matches, "aged") ||
	    material_matches_token(matches, "weathered")) {
		spec.smoothness = std::min(spec.smoothness, 0.28f);
	}
	if (material_matches_token(matches, "matte") || material_matches_token(matches, "dull")) {
		spec.reflectance = std::min(spec.reflectance, 0.22f);
		spec.smoothness = std::min(spec.smoothness, 0.42f);
	}
	if (material_matches_token(matches, "glowing") ||
	    material_matches_token(matches, "emissive")) {
		spec.emission_strength = std::max(spec.emission_strength, 0.95f);
		spec.smoothness = std::max(spec.smoothness, 0.72f);
	}
	if (material_matches_token(matches, "rotating")) {
		spec.uv_rotation_speed = std::max(spec.uv_rotation_speed, 0.70f);
		spec.pulse_strength = std::max(spec.pulse_strength, 0.12f);
	}
	if (material_matches_token(matches, "animated") ||
	    material_matches_token(matches, "active")) {
		spec.uv_activity = std::max(spec.uv_activity, 1.0f);
		spec.pulse_strength = std::max(spec.pulse_strength, 0.34f);
	}
	if (spec.family == "neon") {
		spec.emission_strength = std::max(spec.emission_strength,
		                                  material_matches_token(matches, "glowing") ? 1.85f : 1.25f);
	}
	if (spec.family == "glass") {
		spec.reflectance = std::max(spec.reflectance, 0.82f);
	}
	if (spec.family == "boards" && spec.wood_species.empty()) {
		spec.wood_species = "oak";
	}
	if (spec.two_tone) {
		spec.smoothness = std::max(spec.smoothness, 0.44f);
	}
	if (spec.emphasize_veins) {
		spec.height_scale = std::max(spec.height_scale, 0.050f);
	}
	if (!spec.wood_species.empty()) {
		spec.descriptors.push_back(spec.wood_species);
	}
	if (wants_horizontal || spec.vertical_pattern) {
		spec.descriptors.push_back(spec.vertical_pattern ? "vertical" : "horizontal");
	}
	if (spec.two_tone) {
		spec.descriptors.push_back("two tone");
	}
	if (spec.emphasize_veins) {
		spec.descriptors.push_back("veins");
	}
	if (spec.uv_rotation_speed > 0.0f) {
		spec.descriptors.push_back("rotating");
	}
	if (spec.uv_activity > 0.0f) {
		spec.descriptors.push_back("active");
	}
	sort_and_unique_strings(&spec.descriptors);

	spec.opacity = clamp01(spec.opacity);
	spec.reflectance = clamp01(spec.reflectance);
	spec.smoothness = clamp01(spec.smoothness);
	spec.height_scale = std::clamp(spec.height_scale, 0.0f, 0.16f);
	spec.emission_strength = std::clamp(spec.emission_strength, 0.0f, 2.5f);
	spec.pulse_strength = std::clamp(spec.pulse_strength, 0.0f, 1.0f);
	spec.normal_strength = std::clamp(spec.normal_strength, 0.0f, 2.0f);
	spec.transmission = clamp01(spec.transmission);
	spec.index_of_refraction = std::clamp(spec.index_of_refraction, 1.0f, 2.5f);
	spec.thickness = std::clamp(spec.thickness, 0.0f, 1.0f);
	spec.anisotropic = clamp01(spec.anisotropic);
	spec.sheen = clamp01(spec.sheen);
	spec.clearcoat = clamp01(spec.clearcoat);
	spec.clearcoat_roughness = clamp01(spec.clearcoat_roughness);

	std::ostringstream description;
	description << spec.family;
	if (!spec.descriptors.empty()) {
		description << " • " << join_strings(spec.descriptors);
	}
	description << " • " << join_strings(spec.color_names)
	            << " • opacity " << std::fixed << std::setprecision(2) << spec.opacity
	            << " • reflectance " << spec.reflectance
	            << " • smoothness " << spec.smoothness
		            << " • height " << spec.height_scale
		            << " • glow " << spec.emission_strength
		            << " • transmission " << spec.transmission
		            << " • ior " << spec.index_of_refraction;
	spec.description = description.str();
	return spec;
}

}

const std::array<MaterialLexeme, 78> &material_lexemes()
{
	static const std::array<MaterialLexeme, 78> lexemes = {{
		{"transparent", MaterialLexemeKind::Opacity, {}},
		{"translucent", MaterialLexemeKind::Opacity, {}},
		{"emissive", MaterialLexemeKind::Finish, {}},
		{"glowing", MaterialLexemeKind::Finish, {}},
		{"active", MaterialLexemeKind::Finish, {}},
		{"animated", MaterialLexemeKind::Finish, {}},
		{"rotating", MaterialLexemeKind::Finish, {}},
		{"reflective", MaterialLexemeKind::Finish, {}},
		{"weathered", MaterialLexemeKind::Finish, {}},
		{"polished", MaterialLexemeKind::Finish, {}},
		{"mirror", MaterialLexemeKind::Finish, {}},
		{"concrete", MaterialLexemeKind::Family, {}},
		{"ceramic", MaterialLexemeKind::Family, {}},
		{"plastic", MaterialLexemeKind::Family, {}},
		{"plaster", MaterialLexemeKind::Family, {}},
		{"grass", MaterialLexemeKind::Family, {}},
		{"brick", MaterialLexemeKind::Family, {}},
		{"neon", MaterialLexemeKind::Family, {}},
		{"floral", MaterialLexemeKind::Family, {}},
		{"boards", MaterialLexemeKind::Family, {}},
		{"orange", MaterialLexemeKind::Color, make_swatch(0.84f, 0.48f, 0.24f)},
		{"purple", MaterialLexemeKind::Color, make_swatch(0.58f, 0.42f, 0.76f)},
		{"silver", MaterialLexemeKind::Color, make_swatch(0.76f, 0.78f, 0.81f)},
		{"yellow", MaterialLexemeKind::Color, make_swatch(0.88f, 0.78f, 0.32f)},
		{"magenta", MaterialLexemeKind::Color, make_swatch(0.88f, 0.26f, 0.86f)},
		{"lime", MaterialLexemeKind::Color, make_swatch(0.62f, 0.94f, 0.24f)},
		{"violet", MaterialLexemeKind::Color, make_swatch(0.50f, 0.38f, 0.90f)},
		{"twotone", MaterialLexemeKind::Finish, {}},
		{"duotone", MaterialLexemeKind::Finish, {}},
		{"horizontal", MaterialLexemeKind::Finish, {}},
		{"vertical", MaterialLexemeKind::Finish, {}},
		{"veins", MaterialLexemeKind::Finish, {}},
		{"veined", MaterialLexemeKind::Finish, {}},
		{"oak", MaterialLexemeKind::Finish, {}},
		{"walnut", MaterialLexemeKind::Finish, {}},
		{"pine", MaterialLexemeKind::Finish, {}},
		{"cedar", MaterialLexemeKind::Finish, {}},
		{"birch", MaterialLexemeKind::Finish, {}},
		{"smooth", MaterialLexemeKind::Finish, {}},
		{"glossy", MaterialLexemeKind::Finish, {}},
		{"opaque", MaterialLexemeKind::Opacity, {}},
		{"shiny", MaterialLexemeKind::Finish, {}},
		{"rough", MaterialLexemeKind::Finish, {}},
		{"smoky", MaterialLexemeKind::Opacity, {}},
		{"white", MaterialLexemeKind::Color, make_swatch(0.94f, 0.95f, 0.96f)},
		{"green", MaterialLexemeKind::Color, make_swatch(0.35f, 0.62f, 0.39f)},
		{"black", MaterialLexemeKind::Color, make_swatch(0.16f, 0.18f, 0.20f)},
		{"chrome", MaterialLexemeKind::Color, make_swatch(0.84f, 0.87f, 0.90f)},
		{"copper", MaterialLexemeKind::Color, make_swatch(0.74f, 0.47f, 0.33f)},
		{"beige", MaterialLexemeKind::Color, make_swatch(0.81f, 0.75f, 0.63f)},
		{"glass", MaterialLexemeKind::Family, {}},
		{"marble", MaterialLexemeKind::Family, {}},
		{"stone", MaterialLexemeKind::Family, {}},
		{"metal", MaterialLexemeKind::Family, {}},
		{"paint", MaterialLexemeKind::Family, {}},
		{"brown", MaterialLexemeKind::Color, make_swatch(0.48f, 0.34f, 0.22f)},
		{"blue", MaterialLexemeKind::Color, make_swatch(0.28f, 0.50f, 0.84f)},
		{"grey", MaterialLexemeKind::Color, make_swatch(0.58f, 0.60f, 0.63f)},
		{"gray", MaterialLexemeKind::Color, make_swatch(0.58f, 0.60f, 0.63f)},
		{"gold", MaterialLexemeKind::Color, make_swatch(0.82f, 0.70f, 0.33f)},
		{"wood", MaterialLexemeKind::Family, {}},
		{"rock", MaterialLexemeKind::Family, {}},
		{"tile", MaterialLexemeKind::Family, {}},
		{"amber", MaterialLexemeKind::Color, make_swatch(0.92f, 0.66f, 0.22f)},
		{"teal", MaterialLexemeKind::Color, make_swatch(0.24f, 0.63f, 0.67f)},
		{"cyan", MaterialLexemeKind::Color, make_swatch(0.33f, 0.71f, 0.81f)},
		{"pink", MaterialLexemeKind::Color, make_swatch(0.87f, 0.63f, 0.73f)},
		{"red", MaterialLexemeKind::Color, make_swatch(0.78f, 0.28f, 0.24f)},
			{"aged", MaterialLexemeKind::Finish, {}},
			{"matte", MaterialLexemeKind::Finish, {}},
			{"satin", MaterialLexemeKind::Finish, {}},
			{"frosted", MaterialLexemeKind::Finish, {}},
			{"crystal", MaterialLexemeKind::Finish, {}},
			{"brushed", MaterialLexemeKind::Finish, {}},
			{"velvet", MaterialLexemeKind::Finish, {}},
			{"clearcoat", MaterialLexemeKind::Finish, {}},
			{"dull", MaterialLexemeKind::Finish, {}}
	}};
	return lexemes;
}

GLuint upload_rgba_texture(const unsigned char *pixels,
	                       int width,
	                       int height,
	                       bool color_texture)
{
	if (pixels == nullptr || width <= 0 || height <= 0) {
		return 0;
	}

	auto configure_texture_antialiasing = []() {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		if (!progen3d_texture_anisotropy_supported()) {
			return;
		}
		GLfloat max_anisotropy = 1.0f;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max_anisotropy);
		const GLfloat anisotropy = std::min<GLfloat>(max_anisotropy, 8.0f);
		if (anisotropy > 1.0f) {
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, anisotropy);
		}
	};

	GLuint texture = 0;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	configure_texture_antialiasing();
	const GLint internal_format = color_texture ? GL_SRGB8_ALPHA8 : GL_RGBA8;
	glTexImage2D(
		GL_TEXTURE_2D, 0, internal_format, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	return texture;
}

PreviewMaterial generate_procedural_material(const std::string &material_name, int resolution,
                                             RenderRowExecutor *rows, PreviewRenderMode mode)
{
	const ParsedMaterialSpec spec = parse_material_name(material_name);
	const int texture_size = std::clamp(resolution, 256, 2048);
	const bool full_material = mode == PreviewRenderMode::Full;
	std::vector<unsigned char> pixels(static_cast<std::size_t>(texture_size * texture_size * 4), 255);
	std::vector<unsigned char> height_pixels(full_material ? pixels.size() : 0, 255);
	std::vector<unsigned char> emissive_pixels(full_material ? pixels.size() : 0, 255);
	const MaterialSwatch primary = spec.palette.front();
	const MaterialSwatch secondary = spec.palette.size() > 1
	                                     ? spec.palette[1]
	                                     : darken_swatch(primary, 0.28f);
	const MaterialSwatch accent = spec.palette.size() > 2
	                                  ? spec.palette[2]
	                                  : lighten_swatch(primary, 0.35f);
	const float roughness = 1.0f - spec.smoothness;
	auto to_byte = [](float value) {
		return static_cast<unsigned char>(std::round(clamp01(value) * 255.0f));
	};
	auto write_swatch_pixel = [&](std::vector<unsigned char> *target,
	                              std::size_t pixel_index,
	                              const MaterialSwatch &color) {
		if (target == nullptr || pixel_index + 3 >= target->size()) {
			return;
		}
		(*target)[pixel_index + 0] = to_byte(color.r);
		(*target)[pixel_index + 1] = to_byte(color.g);
		(*target)[pixel_index + 2] = to_byte(color.b);
		(*target)[pixel_index + 3] = 255;
	};
	auto write_scalar_pixel = [&](std::vector<unsigned char> *target,
	                              std::size_t pixel_index,
	                              float value) {
		const MaterialSwatch gray = make_swatch(value, value, value);
		write_swatch_pixel(target, pixel_index, gray);
	};
	auto smooth_pulse = [](float x, float width, float feather) {
		const float inner = std::max(0.0f, width - feather);
		return glm::smoothstep(0.0f, feather, x) * (1.0f - glm::smoothstep(inner, width, x));
	};

	const auto generate_row = [&](int y) {
		for (int x = 0; x < texture_size; ++x) {
			const float u = static_cast<float>(x) / static_cast<float>(texture_size - 1);
			const float v = static_cast<float>(y) / static_cast<float>(texture_size - 1);
			const float cloud = fractal_noise(u * 4.0f + 1.7f, v * 4.0f + 2.4f, spec.seed);
			const float detail = fractal_noise(u * 14.0f + 9.1f, v * 14.0f + 7.6f, spec.seed ^ 0x9e3779b9u, 4);
			const float cross = fractal_noise((u - v) * 8.0f + 3.2f, (u + v) * 8.0f + 4.8f, spec.seed ^ 0x7f4a7c15u, 3);
			const float line_noise = fractal_noise(u * 40.0f + 6.1f, v * 2.6f + 5.4f, spec.seed ^ 0x6c8e9cf5u, 3);
			const float micro = fractal_noise(u * 48.0f + 5.8f, v * 48.0f + 3.7f, spec.seed ^ 0x51ed270bu, 3);
			const float primary_axis = spec.vertical_pattern ? u : v;
			const float secondary_axis = spec.vertical_pattern ? v : u;

			MaterialSwatch color = mix_swatch(primary, secondary, cloud);
			MaterialSwatch emissive = make_swatch(0.0f, 0.0f, 0.0f);
			float height_value = 0.50f;
			if (spec.family == "metal") {
				const float brushed = 0.35f + 0.65f * line_noise;
				color = mix_swatch(primary, secondary, brushed);
				color = mix_swatch(color, accent, std::pow(detail, 3.0f) * 0.22f);
				color = scale_swatch(color, 0.82f + 0.24f * detail);
				height_value = 0.46f + 0.12f * brushed + 0.06f * micro;
			} else if (spec.family == "glass") {
				const float ripple = fractal_noise(u * 18.0f + 3.4f, v * 18.0f + 1.2f, spec.seed ^ 0x2ed13a49u, 4);
				color = mix_swatch(primary, secondary, 0.40f + 0.22f * cloud);
				color = lighten_swatch(color, 0.20f + 0.10f * detail);
				color = mix_swatch(color, make_swatch(0.94f, 0.97f, 1.0f), 0.18f * ripple);
				height_value = 0.49f + 0.04f * ripple;
			} else if (spec.family == "grass") {
				const float blade_field = fractal_noise(u * 22.0f + 6.0f, v * 10.0f + 1.1f, spec.seed ^ 0x3ad1d91fu, 4);
				const float blade_phase = std::fabs(std::sin((u * 36.0f + blade_field * 2.5f) * 3.1415926f));
				const float blade_mask = std::pow(blade_phase, 7.0f);
				const float tip_fade = glm::smoothstep(0.02f, 0.88f, v);
				color = mix_swatch(primary, secondary, blade_field * 0.60f + detail * 0.40f);
				color = mix_swatch(color, accent, blade_mask * 0.16f);
				color = scale_swatch(color, 0.72f + 0.34f * tip_fade);
				height_value = 0.18f + blade_mask * (0.56f + 0.18f * tip_fade) + 0.10f * micro;
			} else if (spec.family == "brick") {
				const float brick_u = u * 6.0f;
				const float brick_v = v * 10.0f;
				const float row = std::floor(brick_v);
				const float stagger = std::fmod(row, 2.0f) * 0.5f;
				const float local_u = std::fabs(std::fmod(brick_u + stagger, 1.0f));
				const float local_v = std::fabs(std::fmod(brick_v, 1.0f));
				const float mortar_x = std::min(local_u, 1.0f - local_u);
				const float mortar_y = std::min(local_v, 1.0f - local_v);
				const float mortar = std::min(mortar_x, mortar_y);
				const float brick_mask = glm::smoothstep(0.06f, 0.13f, mortar);
				const float brick_noise =
					fractal_noise((brick_u + stagger) * 1.7f, brick_v * 1.2f, spec.seed ^ 0x7ac41261u, 4);
				const MaterialSwatch mortar_color = make_swatch(0.78f, 0.75f, 0.72f);
				color = mix_swatch(primary, secondary, brick_noise);
				color = mix_swatch(color, accent, std::pow(detail, 2.0f) * 0.12f);
				color = scale_swatch(color, 0.74f + 0.22f * brick_noise);
				color = mix_swatch(mortar_color, color, brick_mask);
				height_value = lerp_scalar(0.24f + 0.04f * micro, 0.72f + 0.10f * brick_noise, brick_mask);
			} else if (spec.family == "floral") {
				const float floral_u = u * 4.0f;
				const float floral_v = v * 4.0f;
				const float local_u = std::fmod(floral_u, 1.0f) - 0.5f;
				const float local_v = std::fmod(floral_v, 1.0f) - 0.5f;
				const float angle = std::atan2(local_v, local_u);
				const float radius = std::sqrt(local_u * local_u + local_v * local_v);
				const float petal_radius = 0.16f + 0.07f * (0.5f + 0.5f * std::sin(angle * 5.0f + cloud * 2.6f));
				const float petal = 1.0f - glm::smoothstep(petal_radius, petal_radius + 0.05f, radius);
				const float core = 1.0f - glm::smoothstep(0.04f, 0.10f, radius);
				const float leaf_radius =
					std::sqrt(local_u * local_u + std::pow(local_v + 0.22f, 2.0f));
				const float leaf = 1.0f - glm::smoothstep(0.10f, 0.22f, leaf_radius);
				const MaterialSwatch background = lighten_swatch(secondary, 0.08f);
				const MaterialSwatch petals = mix_swatch(primary, accent, 0.44f + 0.20f * detail);
				const MaterialSwatch leaf_color =
					mix_swatch(accent, make_swatch(0.28f, 0.56f, 0.30f), 0.55f);
				color = mix_swatch(background, leaf_color, leaf * 0.55f);
				color = mix_swatch(color, petals, petal);
				color = mix_swatch(color, make_swatch(0.95f, 0.84f, 0.36f), core);
				height_value = 0.22f + petal * 0.42f + core * 0.22f + leaf * 0.10f;
			} else if (spec.family == "boards") {
				const float board_coord = primary_axis * 6.0f;
				const float board_index = std::floor(board_coord);
				const float board_local = std::fmod(board_coord, 1.0f);
				const float seam_distance = std::min(board_local, 1.0f - board_local);
				const float seam_mask = glm::smoothstep(0.03f, 0.10f, seam_distance);
				const float board_noise =
					fractal_noise(board_index * 0.78f + 2.4f,
					             secondary_axis * 4.0f + 3.2f,
					             spec.seed ^ 0x12a4bb91u,
					             4);
				float species_frequency = 24.0f;
				if (spec.wood_species == "walnut") {
					species_frequency = 34.0f;
				} else if (spec.wood_species == "oak") {
					species_frequency = 28.0f;
				} else if (spec.wood_species == "pine") {
					species_frequency = 18.0f;
				} else if (spec.wood_species == "cedar") {
					species_frequency = 22.0f;
				} else if (spec.wood_species == "birch") {
					species_frequency = 26.0f;
				}
				const float grain =
					0.5f + 0.5f * std::sin((secondary_axis * species_frequency + board_noise * 2.8f) * 3.1415926f);
				const float alternating = std::fmod(board_index, 2.0f);
				const MaterialSwatch plank_a =
					mix_swatch(primary, secondary, spec.two_tone ? alternating : board_noise);
				const MaterialSwatch plank_b =
					spec.two_tone
					    ? mix_swatch(secondary, accent, 0.35f)
					    : mix_swatch(primary, accent, 0.18f + 0.42f * detail);
				color = mix_swatch(plank_a, plank_b, grain * 0.58f + detail * 0.28f);
				color = scale_swatch(color, 0.74f + 0.24f * grain);
				color = mix_swatch(make_swatch(0.16f, 0.11f, 0.08f), color, seam_mask);
				height_value = lerp_scalar(0.16f + 0.03f * micro, 0.70f + 0.08f * grain, seam_mask);
			} else if (spec.family == "neon") {
				const float tile_u = u * 4.5f;
				const float tile_v = v * 4.5f;
				const float straight_band =
					smooth_pulse(std::fabs(std::fmod(tile_v, 1.0f) - 0.5f), 0.18f, 0.06f);
				const float vertical_band =
					smooth_pulse(std::fabs(std::fmod(tile_u, 1.0f) - 0.5f), 0.12f, 0.05f);
				const float diagonal_band =
					smooth_pulse(std::fabs(std::fmod((u + v) * 6.0f + cloud * 0.4f, 1.0f) - 0.5f), 0.10f, 0.04f);
				const float glow_mask = std::max(straight_band, std::max(vertical_band * 0.72f, diagonal_band * 0.92f));
				const MaterialSwatch dark_back = darken_swatch(secondary, 0.82f);
				const MaterialSwatch neon_core = lighten_swatch(accent, 0.28f);
				color = mix_swatch(dark_back, neon_core, glow_mask * 0.38f + 0.10f * micro);
				emissive = scale_swatch(neon_core, glow_mask);
				height_value = 0.38f + glow_mask * 0.28f;
			} else if (spec.family == "wood") {
				float species_frequency = 26.0f;
				float knot_frequency = 10.0f;
				if (spec.wood_species == "walnut") {
					species_frequency = 34.0f;
					knot_frequency = 8.0f;
				} else if (spec.wood_species == "oak") {
					species_frequency = 28.0f;
					knot_frequency = 11.0f;
				} else if (spec.wood_species == "pine") {
					species_frequency = 18.0f;
					knot_frequency = 7.0f;
				} else if (spec.wood_species == "cedar") {
					species_frequency = 22.0f;
					knot_frequency = 9.0f;
				} else if (spec.wood_species == "birch") {
					species_frequency = 24.0f;
					knot_frequency = 6.0f;
				}
				const float grain =
					0.5f + 0.5f * std::sin((primary_axis * species_frequency + cloud * 2.8f + line_noise * 1.2f) * 3.1415926f);
				const float secondary_grain =
					0.5f + 0.5f * std::sin((secondary_axis * species_frequency * 0.32f + detail * 3.6f) * 3.1415926f);
				const float knot =
					std::pow(fractal_noise(u * knot_frequency + 4.2f,
					                      v * knot_frequency + 2.6f,
					                      spec.seed ^ 0x5b8d0f21u,
					                      3),
					        6.0f);
				color = mix_swatch(primary, secondary, grain * 0.72f + secondary_grain * 0.28f);
				color = mix_swatch(color, accent, knot * 0.22f);
				color = scale_swatch(color, 0.72f + 0.30f * detail);
				height_value = 0.26f + 0.42f * grain + 0.14f * knot + 0.08f * micro;
			} else if (spec.family == "marble") {
				const float veins = std::pow(std::fabs(std::sin((u + v) * 18.0f + detail * 7.0f)), 3.0f);
				color = mix_swatch(primary, secondary, cloud * 0.55f + veins * 0.45f);
				color = mix_swatch(color, accent, veins * 0.20f);
				height_value = 0.44f + veins * 0.14f + 0.06f * micro;
			} else if (spec.family == "stone" || spec.family == "concrete") {
				const float speckle = std::pow(detail, 6.0f);
				color = mix_swatch(primary, secondary, cloud * 0.70f + cross * 0.30f);
				color = mix_swatch(color, accent, speckle * 0.18f);
				color = scale_swatch(color, 0.82f + 0.18f * cross);
				height_value = 0.42f + 0.20f * cross + 0.16f * detail;
			} else {
				color = mix_swatch(primary, secondary, cloud * 0.65f + detail * 0.35f);
				color = mix_swatch(color, accent, std::pow(cross, 2.0f) * 0.14f);
				height_value = 0.40f + 0.16f * cross + 0.08f * micro;
			}

			if (spec.two_tone && spec.family != "boards" && spec.family != "floral") {
				const float duo_band =
					0.5f + 0.5f * std::sin((primary_axis * 10.0f + cross * 4.0f) * 3.1415926f);
				const float duo_mix = glm::smoothstep(0.28f, 0.72f, duo_band);
				const MaterialSwatch tone_a =
					mix_swatch(primary, secondary, 0.30f + 0.18f * detail);
				const MaterialSwatch tone_b =
					mix_swatch(secondary, accent, 0.45f + 0.12f * cloud);
				color = mix_swatch(tone_a, tone_b, duo_mix * 0.82f);
				height_value = std::max(height_value, 0.34f + 0.12f * duo_mix);
			}
			if (spec.emphasize_veins) {
				const float veins =
					std::pow(std::fabs(std::sin((u * 1.8f + v * 0.6f + detail * 0.35f) * 14.0f + cloud * 4.0f)), 6.0f);
				color = mix_swatch(color, accent, veins * 0.26f);
				height_value = std::max(height_value, 0.38f + veins * 0.24f);
			}
			color = mix_swatch(color, darken_swatch(color, 0.35f), roughness * detail * 0.12f);
			if (spec.emission_strength > 0.0f && spec.family != "neon") {
				const float glow_mask =
					std::pow(std::max(detail, fractal_noise(u * 7.0f + 4.0f, v * 7.0f + 8.0f, spec.seed ^ 0x4f3a8c59u, 3)), 3.4f);
				emissive = mix_swatch(emissive, lighten_swatch(accent, 0.18f), glow_mask);
				height_value = std::max(height_value, 0.40f + glow_mask * 0.12f);
			}
			if (spec.uv_activity > 0.0f && spec.emission_strength <= 0.0f) {
				const float active_mask =
					0.5f + 0.5f * std::sin((u + v + detail) * 12.0f + cloud * 3.4f);
				emissive = mix_swatch(emissive,
				                     scale_swatch(accent, 0.16f + 0.10f * active_mask),
				                     spec.pulse_strength * 0.22f);
			}
			if (spec.family == "glass") {
				emissive = mix_swatch(emissive, make_swatch(0.02f, 0.04f, 0.05f), 0.35f * std::pow(detail, 2.5f));
			}
			if (spec.family == "metal" && spec.reflectance > 0.9f) {
				emissive = mix_swatch(emissive, make_swatch(0.03f, 0.03f, 0.03f), 0.25f * micro);
			}
			height_value = clamp01(height_value);

			const std::size_t pixel_index = static_cast<std::size_t>((y * texture_size + x) * 4);
			write_swatch_pixel(&pixels, pixel_index, color);
			if (full_material) {
				write_scalar_pixel(&height_pixels, pixel_index, height_value);
				write_swatch_pixel(&emissive_pixels, pixel_index, emissive);
			}
		}
	};
	const auto generate_rows = [&](int first_row, int end_row) {
		for (int row = first_row; row < end_row; ++row) generate_row(row);
	};
	if (rows) {
		rows->run(texture_size, texture_size, generate_rows);
	} else {
		generate_rows(0, texture_size);
	}

	PreviewMaterial material;
	float default_mapping_scale = 0.82f;
	if (spec.family == "grass") {
		default_mapping_scale = 1.22f;
	} else if (spec.family == "brick") {
		default_mapping_scale = 0.94f;
	} else if (spec.family == "boards") {
		default_mapping_scale = spec.vertical_pattern ? 0.70f : 0.76f;
	} else if (spec.family == "floral") {
		default_mapping_scale = 1.04f;
	} else if (spec.family == "wood") {
		default_mapping_scale = 0.72f;
	} else if (spec.family == "glass") {
		default_mapping_scale = 0.58f;
	} else if (spec.family == "metal") {
		default_mapping_scale = 0.68f;
	} else if (spec.family == "neon") {
		default_mapping_scale = 1.08f;
	} else if (spec.family == "stone" || spec.family == "concrete") {
		default_mapping_scale = 0.88f;
	}
	material.texture = upload_rgba_texture(pixels.data(), texture_size, texture_size, true);
	material.opaque_base_texture = material.texture;
	if (full_material) {
		material.height_texture = upload_rgba_texture(height_pixels.data(), texture_size, texture_size);
		material.emissive_texture = upload_rgba_texture(emissive_pixels.data(), texture_size, texture_size, true);
	}
	material.name = spec.name;
	material.family = spec.family;
	material.description = spec.description;
	material.opacity = spec.opacity;
	material.reflectance = spec.reflectance;
	material.smoothness = spec.smoothness;
	material.height_scale = spec.height_scale;
	material.emission_strength = spec.emission_strength;
	material.uv_rotation_speed = spec.uv_rotation_speed;
	material.uv_activity = spec.uv_activity;
	material.pulse_strength = spec.pulse_strength;
	material.normal_strength = spec.normal_strength;
	material.transmission = spec.transmission;
	material.index_of_refraction = spec.index_of_refraction;
	material.thickness = spec.thickness;
	material.attenuation_color = glm::vec3(
		spec.attenuation_color.r,
		spec.attenuation_color.g,
		spec.attenuation_color.b);
	material.texture_resolution = texture_size;

	// Set PBR material properties based on material type
	if (spec.family == "metal") {
		material.metallic = 1.0f;
		material.roughness = 1.0f - spec.smoothness;
	} else if (spec.family == "glass") {
		material.metallic = 0.0f;
		material.roughness = 0.1f;
		material.subsurface = 0.2f;
	} else if (spec.family == "wood" || spec.family == "boards") {
		material.metallic = 0.0f;
		material.roughness = 0.8f - spec.smoothness * 0.6f;
	} else if (spec.family == "stone" || spec.family == "concrete" || spec.family == "brick") {
		material.metallic = 0.0f;
		material.roughness = 0.9f;
	} else if (spec.family == "grass" || spec.family == "floral") {
		material.metallic = 0.0f;
		material.roughness = 0.7f;
		material.subsurface = 0.3f;
	} else if (spec.family == "neon") {
		material.metallic = 0.1f;
		material.roughness = 0.3f;
	} else {
		material.metallic = 0.0f;
		material.roughness = 1.0f - spec.smoothness;
	}
	material.ao = 1.0f;
	material.subsurface = std::max(0.0f, material.subsurface);
	material.anisotropic = spec.anisotropic;
	material.sheen = std::max(spec.sheen,
	                          (spec.family == "cloth" || spec.family == "floral") ? 0.3f : 0.0f);
	material.clearcoat = std::max(spec.clearcoat,
	                              (spec.family == "glass" || spec.family == "metal") ? 0.1f : 0.0f);
	material.clearcoat_roughness = spec.clearcoat > 0.0f
		? spec.clearcoat_roughness
		: (1.0f - spec.smoothness) * 0.5f;

	material.mapping_mode = static_cast<int>(PreviewMappingMode::UV);
	material.mapping_scale = default_mapping_scale;
	material.projection_axes = {
		glm::vec3(1.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f)};
	material.swatches = {
		glm::vec3(primary.r, primary.g, primary.b),
		glm::vec3(secondary.r, secondary.g, secondary.b),
		glm::vec3(accent.r, accent.g, accent.b)};
	// Auto-generate any missing texture maps
	auto_generate_missing_texture_maps(&material, mode);

	return material;
}

GLuint generate_default_alpha_map(int texture_size = 64)
{
	std::vector<unsigned char> alpha_pixels(static_cast<std::size_t>(texture_size * texture_size * 4), 255);
	return upload_rgba_texture(alpha_pixels.data(), texture_size, texture_size);
}

// Generate a default glow/emissive map (mostly black with subtle variation)
GLuint generate_default_glow_map(int texture_size = 64)
{
	std::vector<unsigned char> glow_pixels(static_cast<std::size_t>(texture_size * texture_size * 4), 0);

	for (int y = 0; y < texture_size; ++y) {
		for (int x = 0; x < texture_size; ++x) {
			const float u = static_cast<float>(x) / static_cast<float>(texture_size - 1);
			const float v = static_cast<float>(y) / static_cast<float>(texture_size - 1);

			// Very subtle glow variation
			const float noise = fractal_noise(u * 8.0f + 1.0f, v * 8.0f + 2.0f, 0x12345678u, 2);
			const unsigned char glow = static_cast<unsigned char>(std::max(0.0f, noise * 10.0f));

			const std::size_t pixel_index = static_cast<std::size_t>((y * texture_size + x) * 4);
			glow_pixels[pixel_index + 0] = glow;
			glow_pixels[pixel_index + 1] = glow;
			glow_pixels[pixel_index + 2] = glow;
			glow_pixels[pixel_index + 3] = 255;
		}
	}

	return upload_rgba_texture(glow_pixels.data(), texture_size, texture_size);
}

// Generate a default normal map (mostly flat with slight variation)
GLuint generate_default_normal_map(int texture_size = 64)
{
	std::vector<unsigned char> normal_pixels(static_cast<std::size_t>(texture_size * texture_size * 4), 128);

	for (int y = 0; y < texture_size; ++y) {
		for (int x = 0; x < texture_size; ++x) {
			const float u = static_cast<float>(x) / static_cast<float>(texture_size - 1);
			const float v = static_cast<float>(y) / static_cast<float>(texture_size - 1);

			// Generate slight normal variation
			const float noise_x = fractal_noise(u * 12.0f + 3.0f, v * 12.0f + 4.0f, 0xABCDEF01u, 3) - 0.5f;
			const float noise_y = fractal_noise(u * 12.0f + 5.0f, v * 12.0f + 6.0f, 0xFEDCBA12u, 3) - 0.5f;

			// Convert to normal map colors (default flat is 128, 128, 255)
			const unsigned char normal_x = static_cast<unsigned char>(std::clamp(128 + noise_x * 30.0f, 0.0f, 255.0f));
			const unsigned char normal_y = static_cast<unsigned char>(std::clamp(128 + noise_y * 30.0f, 0.0f, 255.0f));

			const std::size_t pixel_index = static_cast<std::size_t>((y * texture_size + x) * 4);
			normal_pixels[pixel_index + 0] = normal_x;
			normal_pixels[pixel_index + 1] = normal_y;
			normal_pixels[pixel_index + 2] = 255;  // Z is always up
			normal_pixels[pixel_index + 3] = 255;
		}
	}

	return upload_rgba_texture(normal_pixels.data(), texture_size, texture_size);
}

GLuint generate_default_roughness_map(float smoothness, int texture_size = 64)
{
	std::vector<unsigned char> roughness_pixels(static_cast<std::size_t>(texture_size * texture_size * 4), 128);
	(void)smoothness;

	for (int y = 0; y < texture_size; ++y) {
		for (int x = 0; x < texture_size; ++x) {
			const float u = static_cast<float>(x) / static_cast<float>(texture_size - 1);
			const float v = static_cast<float>(y) / static_cast<float>(texture_size - 1);

			// Add slight variation
			const float noise = fractal_noise(u * 10.0f + 7.0f, v * 10.0f + 8.0f, 0x23456789u, 2);
			const unsigned char roughness = static_cast<unsigned char>(std::clamp(
				228.0f + noise * 27.0f, 0.0f, 255.0f));

			const std::size_t pixel_index = static_cast<std::size_t>((y * texture_size + x) * 4);
			roughness_pixels[pixel_index + 0] = roughness;
			roughness_pixels[pixel_index + 1] = roughness;
			roughness_pixels[pixel_index + 2] = roughness;
			roughness_pixels[pixel_index + 3] = 255;
		}
	}

	return upload_rgba_texture(roughness_pixels.data(), texture_size, texture_size);
}

GLuint generate_default_metallic_map(int texture_size = 64)
{
	std::vector<unsigned char> metallic_pixels(static_cast<std::size_t>(texture_size * texture_size * 4), 255);

	for (int y = 0; y < texture_size; ++y) {
		for (int x = 0; x < texture_size; ++x) {
			const unsigned char metallic = 255;

			const std::size_t pixel_index = static_cast<std::size_t>((y * texture_size + x) * 4);
			metallic_pixels[pixel_index + 0] = metallic;
			metallic_pixels[pixel_index + 1] = metallic;
			metallic_pixels[pixel_index + 2] = metallic;
			metallic_pixels[pixel_index + 3] = 255;
		}
	}

	return upload_rgba_texture(metallic_pixels.data(), texture_size, texture_size);
}

// Generate a default ambient occlusion map (mostly white with some darkening)
GLuint generate_default_ao_map(int texture_size = 64)
{
	std::vector<unsigned char> ao_pixels(static_cast<std::size_t>(texture_size * texture_size * 4), 255);

	for (int y = 0; y < texture_size; ++y) {
		for (int x = 0; x < texture_size; ++x) {
			const float u = static_cast<float>(x) / static_cast<float>(texture_size - 1);
			const float v = static_cast<float>(y) / static_cast<float>(texture_size - 1);

			// Generate some subtle AO patterns
			const float noise = fractal_noise(u * 6.0f + 11.0f, v * 6.0f + 12.0f, 0x456789ABu, 2);
			const float ao = 1.0f - (noise * 0.15f);  // Slight darkening

			const unsigned char ao_value = static_cast<unsigned char>(std::clamp(ao * 255.0f, 0.0f, 255.0f));

			const std::size_t pixel_index = static_cast<std::size_t>((y * texture_size + x) * 4);
			ao_pixels[pixel_index + 0] = ao_value;
			ao_pixels[pixel_index + 1] = ao_value;
			ao_pixels[pixel_index + 2] = ao_value;
			ao_pixels[pixel_index + 3] = 255;
		}
	}

	return upload_rgba_texture(ao_pixels.data(), texture_size, texture_size);
}

// Auto-generate all missing texture maps for a material
void auto_generate_missing_texture_maps(PreviewMaterial *material, PreviewRenderMode mode)
{
	if (material == nullptr) {
		return;
	}
	const int auxiliary_texture_resolution =
		std::clamp(material->texture_resolution, 64, 512);

	// Generate alpha map if missing
	if (material->alpha_texture == 0) {
		material->alpha_texture = generate_default_alpha_map(auxiliary_texture_resolution);
		material->opaque_alpha_texture = material->alpha_texture;
	}
	if (mode == PreviewRenderMode::Basic) return;

	// Generate glow map if missing
	if (material->glow_texture == 0) {
		material->glow_texture = generate_default_glow_map(auxiliary_texture_resolution);
	}

	// Generate normal map if missing
	if (material->normal_texture == 0) {
		material->normal_texture = generate_default_normal_map(auxiliary_texture_resolution);
	}

	// Generate roughness map if missing
	if (material->roughness_texture == 0) {
		material->roughness_texture = generate_default_roughness_map(
			material->smoothness, auxiliary_texture_resolution);
	}

	// Generate metallic map if missing
	if (material->metallic_texture == 0) {
		material->metallic_texture = generate_default_metallic_map(auxiliary_texture_resolution);
	}

	// Generate ambient occlusion map if missing
	if (material->ao_texture == 0) {
		material->ao_texture = generate_default_ao_map(auxiliary_texture_resolution);
	}
}

bool is_user_texture_slot_name(std::string_view material_name)
{
	constexpr std::string_view kPrefix = "usertexture";
	if (material_name.size() <= kPrefix.size() || material_name.rfind(kPrefix, 0) != 0) {
		return false;
	}

	for (char ch : material_name.substr(kPrefix.size())) {
		if (std::isdigit(static_cast<unsigned char>(ch)) == 0) {
			return false;
		}
	}
	return true;
}

void destroy_preview_material_textures(PreviewMaterial *material)
{
	if (material == nullptr) {
		return;
	}
	if (material->texture != 0) {
		glDeleteTextures(1, &material->texture);
		material->texture = 0;
	}
	if (material->height_texture != 0) {
		glDeleteTextures(1, &material->height_texture);
		material->height_texture = 0;
	}
	if (material->emissive_texture != 0) {
		glDeleteTextures(1, &material->emissive_texture);
		material->emissive_texture = 0;
	}
	if (material->alpha_texture != 0) {
		glDeleteTextures(1, &material->alpha_texture);
		material->alpha_texture = 0;
	}
	if (material->glow_texture != 0) {
		glDeleteTextures(1, &material->glow_texture);
		material->glow_texture = 0;
	}
	if (material->normal_texture != 0) {
		glDeleteTextures(1, &material->normal_texture);
		material->normal_texture = 0;
	}
	if (material->roughness_texture != 0) {
		glDeleteTextures(1, &material->roughness_texture);
		material->roughness_texture = 0;
	}
	if (material->metallic_texture != 0) {
		glDeleteTextures(1, &material->metallic_texture);
		material->metallic_texture = 0;
	}
	if (material->ao_texture != 0) {
		glDeleteTextures(1, &material->ao_texture);
		material->ao_texture = 0;
	}
}

}
