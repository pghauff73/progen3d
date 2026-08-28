#pragma once

#include <array>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct StlCatalogEntry {
	struct NumericBounds {
		std::array<float, 3> size{0.0f, 0.0f, 0.0f};
	};

	struct BoundingBox {
		std::array<float, 3> min{0.0f, 0.0f, 0.0f};
		std::array<float, 3> max{0.0f, 0.0f, 0.0f};
		std::array<float, 3> size{0.0f, 0.0f, 0.0f};
		std::array<float, 3> center{0.0f, 0.0f, 0.0f};
	};

	struct StlGeometry {
		BoundingBox bounding_box;
		std::array<float, 3> center_of_mass{0.0f, 0.0f, 0.0f};
	};

	struct ConnectionPoint {
		std::string name;
		std::string type;
		std::array<int, 3> axis_vector{0, 0, 0};
		std::string movement_type;
		std::array<float, 3> center{0.0f, 0.0f, 0.0f};
		NumericBounds extents_numeric;
		float diameter_numeric = 0.0f;
		float width_numeric = 0.0f;
		std::string notes;
	};

	struct ConnectionSurface {
		std::string name;
		std::string source_connection_point;
		std::string shape;
		std::string plane;
		std::array<int, 3> axis_vector{0, 0, 0};
		std::array<float, 3> center{0.0f, 0.0f, 0.0f};
		NumericBounds bounds_numeric;
		float diameter_numeric = 0.0f;
		std::string thread_type;
		float thread_pitch_numeric = 0.0f;
		float thread_height_numeric = 0.0f;
		float thread_depth_numeric = 0.0f;
		std::string notes;
	};

	struct DetectedConnection {
		std::string name;
		std::string primitive;
		std::string feature_role;
		std::array<int, 3> axis_vector{0, 0, 0};
		std::string plane;
		std::array<float, 3> center{0.0f, 0.0f, 0.0f};
		NumericBounds bounds_numeric;
		float diameter_numeric = 0.0f;
		float length_numeric = 0.0f;
		float outer_diameter_numeric = 0.0f;
		float inner_diameter_numeric = 0.0f;
		std::string metric_size;
		std::string thread_type;
		float thread_pitch_numeric = 0.0f;
		std::string source_file;
		std::string source_module;
		std::string evidence;
	};

	std::string name;
	std::string category;
	std::string category_path;
	int category_depth = 0;
	std::string qualified_name;
	std::string description;
	std::string short_description;
	std::string tooltip;
	std::string source_file;
	std::string source_module;
	std::string implementation_file;
	std::string implementation_module;
	StlGeometry stl_geometry;
	std::vector<ConnectionPoint> connection_points;
	std::vector<ConnectionSurface> connection_surfaces;
	std::vector<DetectedConnection> scad_detected_connections;
};

struct StlCategoryTreeNode {
	std::string name;
	std::vector<std::string> path;
	std::string path_string;
	int count = 0;
	std::vector<std::string> categories;
	std::vector<std::string> qualified_names;
	std::vector<std::string> names;
	std::vector<StlCategoryTreeNode> children;
};

using StartupProgressCallback = std::function<bool(float, const std::string &)>;

extern std::vector<std::string> part_class_names;
extern std::unordered_set<std::string> part_class_name_set;
extern std::vector<std::string> stl_part_names;
extern std::unordered_set<std::string> stl_part_name_set;
extern std::vector<std::string> stl_category_names;
extern std::vector<StlCatalogEntry> stl_catalog_entries;
extern std::unordered_map<std::string, StlCatalogEntry> stl_catalog_lookup;
extern std::unordered_map<std::string, std::vector<std::string>> stl_part_names_by_category;
extern StlCategoryTreeNode stl_category_tree_root;
extern bool has_stl_category_tree;

std::string sanitize_stl_category(const std::string &category);
std::string infer_stl_category(const std::string &name);
std::vector<StlCatalogEntry> collect_stl_catalog_entries(const std::string &stls_dir);
std::vector<StlCatalogEntry> parse_stl_catalog_entries(const std::string &json_text);
bool load_stl_category_tree(const std::string &path);
void set_part_class_catalog(const std::vector<std::string> &names);
bool load_part_class_catalog(const std::string &path);
bool write_stl_catalog_file(const std::string &path, const std::vector<StlCatalogEntry> &entries);
void generate_stl_part_catalog(const std::string &stls_dir,
                               const std::string &catalog_path,
                               const std::string &tree_path = "",
                               const StartupProgressCallback &progress_callback = StartupProgressCallback());
void append_stl_part_catalog(const std::string &path, const std::string &tree_path = "");
