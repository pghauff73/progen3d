#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

struct GeneratedMcsMv2SourceReleaseRecord
{
    std::string_view version;
    std::string_view model_schema;
    std::string_view manifest_schema;
    std::string_view release_manifest_sha256;
    std::size_t signed_artifact_count;
};

struct GeneratedMcsMv2ReferenceFrameRecord
{
    std::string_view schema;
    std::string_view name;
    std::array<double, 3> origin;
    std::array<double, 3> source_x_axis;
    std::array<double, 3> source_y_axis;
    std::array<double, 3> source_z_axis;
    std::string_view handedness;
    std::string_view length_unit;
    std::string_view angle_unit;
    std::array<std::array<double, 4>, 4> source_to_progen3d_matrix;
};

struct GeneratedMcsMv2FieldCalibrationRecord
{
    std::string_view schema;
    std::string_view method;
    int calibration_iterations;
    std::array<double, 3> field_prewarp_scale;
    std::array<double, 3> field_prewarp_translation;
    double maximum_field_prewarp_fraction;
    bool post_mesh_affine_correction_applied;
    double maximum_post_mesh_correction_fraction;
    std::array<double, 3> target_bounds_minimum;
    std::array<double, 3> target_bounds_maximum;
    std::array<double, 3> accepted_final_bounds_minimum;
    std::array<double, 3> accepted_final_bounds_maximum;
    double package_tolerance;
};

struct GeneratedMcsMv2AssuranceLevelRecord
{
    std::string_view identifier;
    bool passed;
    bool partial_static_envelopes;
    std::string_view claim;
    std::string_view note;
};

struct GeneratedMcsMv2SourceAssuranceRecord
{
    std::string_view schema;
    std::string_view achieved_level;
    std::string_view achieved_label;
    bool release_gate_passed;
    std::array<GeneratedMcsMv2AssuranceLevelRecord, 6> levels;
    std::array<std::string_view, 4> known_limitations;
};

struct GeneratedMcsMv2EvidenceRecord
{
    std::string_view status;
    std::string_view source;
    double confidence;
    bool has_uncertainty;
    double uncertainty;
    std::string_view note;
};

struct GeneratedMcsMv2ParameterDependencyRecord
{
    std::string_view identifier;
    std::string_view serialized_value;
    std::string_view unit;
    std::string_view value_type;
    std::string_view formula;
    std::array<std::string_view, 4> dependencies;
    std::size_t dependency_count;
    GeneratedMcsMv2EvidenceRecord evidence;
};

struct GeneratedMcsMv2PackageRecord
{
    double length;
    double width;
    double height;
    double wheelbase;
    double track_front;
    double track_rear;
    double ground_clearance;
    double front_overhang;
    double rear_overhang;
};

struct GeneratedMcsMv2PlatformRecord
{
    double floor_height;
    double front_hpoint_x;
    double rear_hpoint_x;
    double hpoint_z;
    double eye_z;
    double head_clearance;
    double seat_lateral;
};

struct GeneratedMcsMv2StyleRecord
{
    double roof_scale;
    double greenhouse_front_factor;
    double greenhouse_rear_factor;
    double nose_taper;
    double tail_taper;
    double hood_wedge;
    double roof_crown;
    double tumblehome;
    double belt_rise;
    double shoulder_strength;
    double front_fender_amplitude;
    double rear_haunch_amplitude;
    double rocker_tuck;
    double door_scallop;
    double spoiler_scale;
    double splitter_scale;
    double grille_scale;
};

struct GeneratedMcsMv2WheelRecord
{
    double radius;
    double width;
    double front_track;
    double rear_track;
    double steer_minimum_degrees;
    double steer_maximum_degrees;
    double travel_minimum;
    double travel_maximum;
    double wheelhouse_clearance;
};

struct GeneratedMcsMv2PowertrainRecord
{
    std::string_view architecture;
    std::string_view drive_layout;
    bool has_battery_pack;
    double battery_thickness;
    double engine_envelope_length;
    double engine_envelope_width;
    double engine_envelope_height;
    int exhaust_count;
};

struct GeneratedMcsMv2ClosureRecord
{
    double front_door_maximum_degrees;
    double rear_door_maximum_degrees;
    double bonnet_maximum_degrees;
    double hatch_maximum_degrees;
    double side_glass_travel;
    double nominal_panel_gap;
};

struct GeneratedMcsMv2ColorRecord
{
    int red;
    int green;
    int blue;
};

struct GeneratedMcsMv2StationRecord
{
    std::string_view role;
    int index;
    double normalized_station;
    double source_x;
    double underbody_z;
    double underbody_halfwidth;
    double rocker_z;
    double rocker_halfwidth;
    double lower_z;
    double lower_halfwidth;
    double shoulder_z;
    double shoulder_halfwidth;
    double belt_z;
    double belt_halfwidth;
    double glass_shoulder_z;
    double glass_shoulder_halfwidth;
    double roof_rail_z;
    double roof_rail_halfwidth;
    double roof_crown_z;
    double confidence;
};

struct GeneratedMcsMv2SectionSampleRecord
{
    double source_x;
    std::array<double, 15> values;
};

struct GeneratedMcsMv2FieldSampleRecord
{
    std::array<double, 3> source_point;
    double body_field_value;
    double final_field_value;
};

struct GeneratedMcsMv2CurveSampleRecord
{
    std::string_view identifier;
    std::array<std::array<double, 3>, 7> source_points;
};

struct GeneratedMcsMv2PanelPatchRecord
{
    std::string_view identifier;
    std::uint64_t source_face_count;
    bool closure;
};

struct GeneratedMcsMv2PanelRelationshipRecord
{
    std::string_view first_panel_identifier;
    std::string_view second_panel_identifier;
    std::string_view relationship;
};

struct GeneratedMcsMv2BodyInWhiteMemberRecord
{
    std::string_view identifier;
    std::string_view role;
};

struct GeneratedMcsMv2BodyInWhiteJointRecord
{
    std::string_view first_member_identifier;
    std::string_view second_member_identifier;
    std::string_view joining_method;
};

struct GeneratedMcsMv2VariantRecord
{
    std::string_view key;
    std::string_view label;
    std::string_view description;
    std::string_view package_basis;
    GeneratedMcsMv2ReferenceFrameRecord reference_frame;
    GeneratedMcsMv2FieldCalibrationRecord field_calibration;
    GeneratedMcsMv2SourceAssuranceRecord source_assurance;
    GeneratedMcsMv2PackageRecord package;
    GeneratedMcsMv2PlatformRecord platform;
    GeneratedMcsMv2StyleRecord style;
    GeneratedMcsMv2WheelRecord wheels;
    GeneratedMcsMv2PowertrainRecord powertrain;
    GeneratedMcsMv2ClosureRecord closures;
    GeneratedMcsMv2ColorRecord body_color;
    bool has_crossover_cladding;
    std::array<GeneratedMcsMv2ParameterDependencyRecord, 74> parameter_dependencies;
    std::array<GeneratedMcsMv2StationRecord, 11> stations;
    std::array<GeneratedMcsMv2SectionSampleRecord, 7> section_samples;
    std::array<GeneratedMcsMv2FieldSampleRecord, 6> field_samples;
    std::array<GeneratedMcsMv2CurveSampleRecord, 14> curve_samples;
    std::array<GeneratedMcsMv2PanelPatchRecord, 11> panel_patches;
    std::array<GeneratedMcsMv2PanelRelationshipRecord, 6> panel_relationships;
    std::array<GeneratedMcsMv2BodyInWhiteMemberRecord, 22> body_in_white_members;
    std::array<GeneratedMcsMv2BodyInWhiteJointRecord, 25> body_in_white_joints;
    std::array<std::string_view, 8> occupant_envelope_identifiers;
    std::array<std::string_view, 3> functional_envelope_identifiers;
    std::size_t functional_envelope_count;
};

inline const GeneratedMcsMv2SourceReleaseRecord &generatedMcsMv2SourceReleaseRecord()
{
    static const GeneratedMcsMv2SourceReleaseRecord record{
        "2.0.1",
        "MCSMv2.0.1-hybrid-semantic-implicit-integrity",
        "MCSMv2.0.1.ReleaseManifest.v1",
        "c49934afea36ad14f9f6b848324b652334ef3db9421ee340d454abc32421a6d1",
        86u};
    return record;
}

inline const std::array<GeneratedMcsMv2VariantRecord, 4> &generatedMcsMv2VariantRecords()
{
    static const std::array<GeneratedMcsMv2VariantRecord, 4> records{{
GeneratedMcsMv2VariantRecord{
    "reference", "Research Median AWD Hot Hatch", "Median package of Golf R, S3 Sportback and GRMN Corolla with a balanced fastback hatch envelope.", "median of three official hot-hatch packages",
    GeneratedMcsMv2ReferenceFrameRecord{"MCSMv2.0.1.VehicleReferenceFrame.v1", "MCSMv2VehicleFrame", {{0, 0, 0}}, {{1, 0, 0}}, {{0, 1, 0}}, {{0, 0, 1}}, "right", "m", "deg", {{{{0, 1, 0, 0}}, {{0, 0, 1, 0}}, {{1, 0, 0, 0}}, {{0, 0, 0, 1}}}}},
    GeneratedMcsMv2FieldCalibrationRecord{"MCSMv2.0.1.FieldCalibration.v1", "iterative inverse affine field prewarp", 3, {{1.0000440260000001, 1.0065456813, 1.0061670852}}, {{-8.0182499999999993e-05, 6.5000000000000003e-09, -0.0062182966000000001}}, 0.0065456813109459056, false, 0, {{-2.1764999999999999, -0.90800000000000003, 0.10846212}}, {{2.1764999999999999, 0.90800000000000003, 1.4681200000000001}}, {{-2.1764994099999999, -0.90791438999999996, 0.10922835}}, {{2.1765001700000002, 0.90791443000000005, 1.4678377}}, 0.0025000000000000001},
    GeneratedMcsMv2SourceAssuranceRecord{"MCSMv2.0.1.Assurance.v1", "V2", "Package and semantic geometry consistency", true, {{GeneratedMcsMv2AssuranceLevelRecord{"V0", true, false, "finite, framed, acyclic parameter model", ""}, GeneratedMcsMv2AssuranceLevelRecord{"V1", true, false, "watertight, wound, non-self-intersecting final body mesh", ""}, GeneratedMcsMv2AssuranceLevelRecord{"V2", true, false, "package, continuous sections, curve network, datums and BIW graph consistent", ""}, GeneratedMcsMv2AssuranceLevelRecord{"V3", false, true, "independent closure, glass and suspension kinematics", "Static occupant and sampled tyre checks exist; full closure/glass/suspension kinematics remain deferred."}, GeneratedMcsMv2AssuranceLevelRecord{"V4", false, false, "manufacturing and structural analysis", ""}, GeneratedMcsMv2AssuranceLevelRecord{"V5", false, false, "CFD, crash, thermal, ergonomic and physical validation", ""}}}, {{"panel patches remain semantic mesh masks rather than exclusive surface-domain partitions", "BIW validation is graph connectivity rather than geometric joint/load-path analysis", "semantic-surface to implicit-field agreement is diagnostic, not yet constrained", "inverse fitting remains a synthetic semantic metric demonstration"}}},
    GeneratedMcsMv2PackageRecord{4.3529999999999998, 1.8160000000000001, 1.4681199999999999, 2.6288999999999998, 1.5682499999999999, 1.56799, 0.13, 0.86204999999999998, 0.86204999999999998},
    GeneratedMcsMv2PlatformRecord{0.26000000000000001, 0.15773399999999999, -0.76238099999999986, 0.56000000000000005, 1.1800000000000002, 0.085000000000000006, 0.25424000000000002},
    GeneratedMcsMv2StyleRecord{1, 1, 1, 1, 1, 0.52000000000000002, 0.5, 0.29999999999999999, 0.044999999999999998, 0.032000000000000001, 0.029999999999999999, 0.040000000000000001, 0.050000000000000003, 0.024, 1, 1, 1},
    GeneratedMcsMv2WheelRecord{0.34000000000000002, 0.23499999999999999, 1.5682499999999999, 1.56799, -32, 32, -0.070000000000000007, 0.059999999999999998, 0.028000000000000001},
    GeneratedMcsMv2PowertrainRecord{"ICE_AWD", "AWD", false, 0, 0.78000000000000003, 1.1799999999999999, 0.56000000000000005, 4},
    GeneratedMcsMv2ClosureRecord{68, 68, 62, 72, 0.55000000000000004, 0.0040000000000000001},
    GeneratedMcsMv2ColorRecord{190, 12, 8}, false,
    {{
        GeneratedMcsMv2ParameterDependencyRecord{"battery_pack", "false", "bool", "bool", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"ArchitectureReference", "median of three official hot-hatch packages", 0.71999999999999997, false, 0, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"battery_thickness", "0.0", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"ArchitectureReference", "median of three official hot-hatch packages", 0.71999999999999997, false, 0, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"belt_rise", "0.045", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"bonnet_max_deg", "62.0", "deg", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MVPv2.5 closure design ranges", 0.55000000000000004, true, 2, "Angle uncertainty is expressed in degrees."}},
        GeneratedMcsMv2ParameterDependencyRecord{"crossover_cladding", "false", "bool", "bool", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"door_scallop", "0.024", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"drive_layout", "\"AWD\"", "enum", "enum", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"ArchitectureReference", "median of three official hot-hatch packages", 0.71999999999999997, false, 0, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"engine_envelope_height", "0.56", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"ArchitectureReference", "median of three official hot-hatch packages", 0.71999999999999997, false, 0, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"engine_envelope_length", "0.78", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"ArchitectureReference", "median of three official hot-hatch packages", 0.71999999999999997, false, 0, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"engine_envelope_width", "1.18", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"ArchitectureReference", "median of three official hot-hatch packages", 0.71999999999999997, false, 0, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"exhaust_count", "4", "count", "int", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"ArchitectureReference", "median of three official hot-hatch packages", 0.71999999999999997, false, 0, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"eye_z", "1.1800000000000002", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2 platform derivation", 0.81999999999999995, true, 0.01, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"floor_height", "0.26", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2 platform derivation", 0.81999999999999995, true, 0.01, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"wheelbase", "2.6289", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PublishedPackage", "MCSMv1 official manufacturer package research", 0.94999999999999996, true, 0.002, "Manufacturer-derived package reference; not proprietary CAD."}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_axle_x", "1.31445", "m", "float", "wheelbase / 2", {{"wheelbase", "", "", ""}}, 1, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_door_max_deg", "68.0", "deg", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MVPv2.5 closure design ranges", 0.55000000000000004, true, 2, "Angle uncertainty is expressed in degrees."}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_overhang", "0.86205", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PublishedPackage", "MCSMv1 official manufacturer package research", 0.94999999999999996, true, 0.002, "Manufacturer-derived package reference; not proprietary CAD."}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_extent_x", "2.1765", "m", "float", "front_axle_x + front_overhang", {{"front_axle_x", "front_overhang", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_fender_amplitude", "0.03", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_hpoint_x", "0.15773399999999999", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2 platform derivation", 0.81999999999999995, true, 0.01, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"width", "1.816", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PublishedPackage", "MCSMv1 official manufacturer package research", 0.94999999999999996, true, 0.002, "Manufacturer-derived package reference; not proprietary CAD."}},
        GeneratedMcsMv2ParameterDependencyRecord{"track_front", "1.56825", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PublishedPackage", "MCSMv1 official manufacturer package research", 0.94999999999999996, true, 0.002, "Manufacturer-derived package reference; not proprietary CAD."}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_track_body_margin", "0.24775000000000014", "m", "float", "width - track_front", {{"width", "track_front", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"wheel_radius", "0.34", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PackageReference", "MCSMv1 tyre and wheel package research", 0.81999999999999995, true, 0.0060000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"wheel_width", "0.235", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PackageReference", "MCSMv1 tyre and wheel package research", 0.81999999999999995, true, 0.0060000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"steer_min_deg", "-32.0", "deg", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"EngineeringAssumption", "MCSMv2 concept-level envelope policy", 0.57999999999999996, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"steer_max_deg", "32.0", "deg", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"EngineeringAssumption", "MCSMv2 concept-level envelope policy", 0.57999999999999996, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"steer_max_abs_deg", "32.0", "deg", "float", "max(abs(steer_min_deg), abs(steer_max_deg))", {{"steer_min_deg", "steer_max_deg", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"wheelhouse_clearance", "0.028", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"EngineeringAssumption", "MCSMv2 concept-level envelope policy", 0.57999999999999996, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_wheelhouse_rx", "0.4302655135474016", "m", "float", "r + 0.5*w*sin(delta_max) + clearance", {{"wheel_radius", "wheel_width", "steer_max_abs_deg", "wheelhouse_clearance"}}, 4, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_wheelhouse_ry", "0.3256725498392897", "m", "float", "0.5*w + r*sin(delta_max) + clearance", {{"wheel_radius", "wheel_width", "steer_max_abs_deg", "wheelhouse_clearance"}}, 4, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"travel_min", "-0.07", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"EngineeringAssumption", "MCSMv2 concept-level envelope policy", 0.57999999999999996, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"travel_down", "0.07", "m", "float", "abs(travel_min)", {{"travel_min", "", "", ""}}, 1, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"travel_max", "0.06", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"EngineeringAssumption", "MCSMv2 concept-level envelope policy", 0.57999999999999996, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"travel_up", "0.06", "m", "float", "abs(travel_max)", {{"travel_max", "", "", ""}}, 1, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_wheelhouse_rz", "0.43800000000000006", "m", "float", "r + max(jounce,rebound) + clearance", {{"wheel_radius", "travel_down", "travel_up", "wheelhouse_clearance"}}, 4, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"greenhouse_front_factor", "1.0", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"greenhouse_rear_factor", "1.0", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"grille_scale", "1.0", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"ground_clearance", "0.13", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PublishedPackage", "MCSMv1 official manufacturer package research", 0.94999999999999996, true, 0.002, "Manufacturer-derived package reference; not proprietary CAD."}},
        GeneratedMcsMv2ParameterDependencyRecord{"hatch_max_deg", "72.0", "deg", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MVPv2.5 closure design ranges", 0.55000000000000004, true, 2, "Angle uncertainty is expressed in degrees."}},
        GeneratedMcsMv2ParameterDependencyRecord{"head_clearance", "0.085", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2 platform derivation", 0.81999999999999995, true, 0.01, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"height", "1.4681199999999999", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PublishedPackage", "MCSMv1 official manufacturer package research", 0.94999999999999996, true, 0.002, "Manufacturer-derived package reference; not proprietary CAD."}},
        GeneratedMcsMv2ParameterDependencyRecord{"hood_wedge", "0.52", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"hpoint_z", "0.56", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2 platform derivation", 0.81999999999999995, true, 0.01, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"length", "4.353", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PublishedPackage", "MCSMv1 official manufacturer package research", 0.94999999999999996, true, 0.002, "Manufacturer-derived package reference; not proprietary CAD."}},
        GeneratedMcsMv2ParameterDependencyRecord{"minimum_roof_from_occupant", "1.365", "m", "float", "H-point + seated-head envelope + clearance", {{"hpoint_z", "head_clearance", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"nominal_panel_gap", "0.004", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MVPv2.5 closure design ranges", 0.55000000000000004, true, 2, "Angle uncertainty is expressed in degrees."}},
        GeneratedMcsMv2ParameterDependencyRecord{"nose_taper", "1.0", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"occupant_roof_margin", "0.10311999999999988", "m", "float", "height - minimum_roof_from_occupant", {{"height", "minimum_roof_from_occupant", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_axle_x", "-1.31445", "m", "float", "-wheelbase / 2", {{"wheelbase", "", "", ""}}, 1, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_overhang", "0.86205", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PublishedPackage", "MCSMv1 official manufacturer package research", 0.94999999999999996, true, 0.002, "Manufacturer-derived package reference; not proprietary CAD."}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_extent_x", "-2.1765", "m", "float", "rear_axle_x - rear_overhang", {{"rear_axle_x", "rear_overhang", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"resolved_length", "4.353", "m", "float", "front_extent_x - rear_extent_x", {{"front_extent_x", "rear_extent_x", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"package_length_residual", "0.0", "m", "float", "resolved_length - length", {{"resolved_length", "length", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"powertrain_architecture", "\"ICE_AWD\"", "enum", "enum", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"ArchitectureReference", "median of three official hot-hatch packages", 0.71999999999999997, false, 0, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_door_max_deg", "68.0", "deg", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MVPv2.5 closure design ranges", 0.55000000000000004, true, 2, "Angle uncertainty is expressed in degrees."}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_haunch_amplitude", "0.04", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_hpoint_x", "-0.7623809999999999", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2 platform derivation", 0.81999999999999995, true, 0.01, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"track_rear", "1.56799", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PublishedPackage", "MCSMv1 official manufacturer package research", 0.94999999999999996, true, 0.002, "Manufacturer-derived package reference; not proprietary CAD."}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_track_body_margin", "0.24801000000000006", "m", "float", "width - track_rear", {{"width", "track_rear", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_wheelhouse_rx", "0.36800000000000005", "m", "float", "r + clearance", {{"wheel_radius", "wheelhouse_clearance", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_wheelhouse_ry", "0.1455", "m", "float", "0.5*w + clearance", {{"wheel_width", "wheelhouse_clearance", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_wheelhouse_rz", "0.43800000000000006", "m", "float", "r + max(jounce,rebound) + clearance", {{"wheel_radius", "travel_down", "travel_up", "wheelhouse_clearance"}}, 4, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"rocker_tuck", "0.05", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"roof_crown", "0.5", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"roof_scale", "1.0", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"seat_lateral", "0.25424", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2 platform derivation", 0.81999999999999995, true, 0.01, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"shoulder_strength", "0.032", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"side_glass_travel", "0.55", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MVPv2.5 closure design ranges", 0.55000000000000004, true, 2, "Angle uncertainty is expressed in degrees."}},
        GeneratedMcsMv2ParameterDependencyRecord{"splitter_scale", "1.0", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"spoiler_scale", "1.0", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"tail_taper", "1.0", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"tumblehome", "0.3", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
    }},
    {{
        GeneratedMcsMv2StationRecord{"tail_face", 0, 0, -2.1764999999999999, 0.12818251039709991, 0.28506772685416043, 0.1627030464663497, 0.39592739840855617, 0.28438768960562744, 0.42357068561870564, 0.43841376613740418, 0.43626748828546508, 0.48013224317510655, 0.40572876410548259, 0.50183452820464625, 0.043626748828546509, 0.52156477461269213, 0.028357386738555233, 0.56210002847749818, 0.68000000000000005},
        GeneratedMcsMv2StationRecord{"rear_bumper", 1, 0.059999999999999998, -1.9153199999999999, 0.1084621241821615, 0.48835030946001629, 0.15284285335888051, 0.67826431869446713, 0.3427600328018453, 0.72712186524232203, 0.57379421750295667, 0.7496018682733212, 0.62261203357803696, 0.69712973749418872, 0.64431431860757671, 0.074960186827332131, 0.71492316145016366, 0.048724121437765888, 0.75930389062688275, 0.68000000000000005},
        GeneratedMcsMv2StationRecord{"rear_axle", 2, 0.19803583735354929, -1.3144499999999999, 0.11843250747438826, 0.57674658089604947, 0.15295304354363803, 0.80103691791117981, 0.4434587825480758, 0.87402796650995518, 0.76396121750244084, 0.90800000000000003, 0.94482193215839161, 0.84444000000000008, 0.96652421718793136, 0.59020000000000006, 1.2870703889550597, 0.45858369692463219, 1.3314511181317785, 0.80000000000000004},
        GeneratedMcsMv2StationRecord{"rear_door", 3, 0.33000000000000002, -0.74000999999999983, 0.11832231728963068, 0.53362232838342161, 0.15284285335888051, 0.74114212275475222, 0.46687904599842239, 0.81621430399770667, 0.81058512273184857, 0.85130614282960748, 0.92481739468268631, 0.79171471283153494, 0.94651967971222606, 0.55334899283924499, 1.4046611292890891, 0.54805369601360243, 1.4490418584658082, 0.68000000000000005},
        GeneratedMcsMv2StationRecord{"b_pillar", 4, 0.45500000000000002, -0.19588499999999986, 0.11832231728963057, 0.51424392558470877, 0.15284285335888051, 0.71422767442320678, 0.47277304430272726, 0.78841722704097117, 0.82229125825289873, 0.82312974912000991, 0.89440934198702693, 0.76551066668160928, 0.91611162701656668, 0.5350343369280065, 1.423739270823281, 0.56749129314274382, 1.4681199999999999, 0.80000000000000004},
        GeneratedMcsMv2StationRecord{"front_door", 5, 0.58499999999999996, 0.37000499999999992, 0.11832231728963073, 0.51840561392725226, 0.15284285335888051, 0.72000779712118368, 0.47128045409794184, 0.79255700362623083, 0.81932680826283855, 0.8264626041499824, 0.85000495646368268, 0.76861022185948369, 0.87170724149322243, 0.53720069269748849, 1.3112986736014303, 0.50730903303811214, 1.3556794027781491, 0.68000000000000005},
        GeneratedMcsMv2StationRecord{"a_pillar", 6, 0.68999999999999995, 0.82706999999999953, 0.11832231728963062, 0.54798743260581517, 0.15284285335888051, 0.76109365639696558, 0.46561359440210182, 0.82581953159448096, 0.80807179525582318, 0.85585196300355448, 0.80807179525582318, 0.79594232559330558, 0.82946678387736272, 0.17767945244076896, 1.0572604048028691, 0.09320815165625447, 1.1016411339795882, 0.68000000000000005},
        GeneratedMcsMv2StationRecord{"hood_rear", 7, 0.77000000000000002, 1.1753100000000001, 0.11832231728963062, 0.58642589293601843, 0.15284285335888051, 0.81448040685558132, 0.45890634496168098, 0.86740746026183746, 0.79475045261720934, 0.89161421377100669, 0.82535071659547121, 0.82920121880703634, 0.84705300162501096, 0.089161421377100661, 1.0334275536688904, 0.057954923895115439, 1.0778082828456093, 0.68000000000000005},
        GeneratedMcsMv2StationRecord{"front_axle", 8, 0.80196416264645076, 1.3144499999999999, 0.11832231728963062, 0.59736029396956458, 0.15284285335888051, 0.82966707495772862, 0.45480561452183571, 0.87714576526321342, 0.78660594632696124, 0.89867968299840006, 0.81527534132728596, 0.83577210518851219, 0.83697762635682571, 0.08986796829984002, 1.0250144996331798, 0.058414179394896008, 1.0693952288098989, 0.80000000000000004},
        GeneratedMcsMv2StationRecord{"nose", 9, 0.94499999999999995, 1.9370849999999997, 0.1190897059328016, 0.51152630036096758, 0.15361024200205139, 0.7104531949457884, 0.39951262737089521, 0.73757798655628337, 0.67603119971238312, 0.74944830086819836, 0.69134642333926832, 0.69698691980742455, 0.71304870836880807, 0.074944830086819847, 0.87151013851883175, 0.048714139556432903, 0.91589086769555084, 0.68000000000000005},
        GeneratedMcsMv2StationRecord{"front_face", 10, 1, 2.1764999999999999, 0.14790289661203845, 0.298200986044661, 0.18242343268128822, 0.41416803617314024, 0.3609027881195887, 0.42943236846774974, 0.57093476150179068, 0.43608607530853211, 0.57538170859325932, 0.4055600500369349, 0.59708399362279907, 0.043608607530853216, 0.71096698639722822, 0.028345594895054591, 0.75534771557394731, 0.68000000000000005},
    }},
    {{
        GeneratedMcsMv2SectionSampleRecord{-2.1764999999999999, {{0.12818251039709991, 0.28506772685416043, 0.1627030464663497, 0.39592739840855617, 0.28438768960562744, 0.42357068561870564, 0.43841376613740418, 0.43626748828546508, 0.48013224317510655, 0.40572876410548259, 0.50183452820464625, 0.043626748828546509, 0.52156477461269213, 0.028357386738555233, 0.56210002847749818}}},
        GeneratedMcsMv2SectionSampleRecord{-1.4510000000000001, {{0.11712179926322369, 0.57164376137681772, 0.15280157015295226, 0.79394966857891347, 0.42941991532109725, 0.86531116528344265, 0.73780781730194467, 0.89849286461974653, 0.89330201364615303, 0.83559836409636423, 0.91500429867569277, 0.527058771169024, 1.1783191739123204, 0.38716121317295826, 1.2226999030890393}}},
        GeneratedMcsMv2SectionSampleRecord{-0.72550000000000003, {{0.11832231728963068, 0.53291976702327237, 0.15284285335888051, 0.74016634308787821, 0.46712576400131867, 0.81522718186656917, 0.81107512197572529, 0.85031597907345657, 0.92396082223833509, 0.79079386053831446, 0.94566310726787484, 0.55270538639774691, 1.4057202033237397, 0.54888881638707587, 1.4501009325004588}}},
        GeneratedMcsMv2SectionSampleRecord{0, {{0.11832231728963061, 0.514792119270016, 0.15284285335888051, 0.71498905454168904, 0.4725576991423831, 0.78895492261668432, 0.8218635602805392, 0.82355837285301481, 0.88115763322248908, 0.76590928675330372, 0.90285991825202883, 0.5356333633867959, 1.4003647370799306, 0.55953266256401823, 1.4447454662566495}}},
        GeneratedMcsMv2SectionSampleRecord{0.72549999999999981, {{0.11832231728963064, 0.53941956688716941, 0.15284285335888051, 0.74919384289884638, 0.46718479694932047, 0.81624392087689446, 0.81119255465995432, 0.84742818492435035, 0.81119302226989765, 0.78810821197964587, 0.83262655058563406, 0.2461269063653973, 1.0906958857666622, 0.15283089168537445, 1.1350766149433813}}},
        GeneratedMcsMv2SectionSampleRecord{1.4509999999999996, {{0.11835012234209412, 0.5930474883324941, 0.15287065841134398, 0.82367706712846411, 0.44800023877255113, 0.86991695454266416, 0.77306334746076499, 0.89084715767364864, 0.79945273249090665, 0.82848785663649338, 0.8211550175204464, 0.089084715767364875, 1.0074720425642427, 0.057905065248787163, 1.0518527717409618}}},
        GeneratedMcsMv2SectionSampleRecord{2.1764999999999999, {{0.14790289661203845, 0.29820098604466105, 0.18242343268128822, 0.41416803617314019, 0.3609027881195887, 0.42943236846774974, 0.57093476150179068, 0.43608607530853216, 0.57538170859325932, 0.4055600500369349, 0.59708399362279907, 0.043608607530853216, 0.71096698639722811, 0.028345594895054595, 0.7553477155739472}}},
    }},
    {{
        GeneratedMcsMv2FieldSampleRecord{{{0, 0, 0.80746600000000002}}, -0.62960537040553288, -0.62960537040553288},
        GeneratedMcsMv2FieldSampleRecord{{{0, 0.82355837285301481, 0.8218635602805392}}, 4.022934020702034e-09, 4.022934020702034e-09},
        GeneratedMcsMv2FieldSampleRecord{{{0, 1.12592, 0.8218635602805392}}, 0.30236163011911044, 0.30236163011911044},
        GeneratedMcsMv2FieldSampleRecord{{{2.1964999999999999, 0, 0.58724799999999999}}, 0.024272695141552423, 0.024272695141552423},
        GeneratedMcsMv2FieldSampleRecord{{{0, 0, 1.4881199999999999}}, 0.052658380804416698, 0.052658380804416698},
        GeneratedMcsMv2FieldSampleRecord{{{1.3144499999999999, -0.78412499999999996, 0.34000000000000002}}, -0.074386974082719631, 0.32567254983928973},
    }},
    {{
        GeneratedMcsMv2CurveSampleRecord{"centre_spine", {{{{-2.1764999999999999, 0, 0.34514126943729906}}, {{-1.4891842105263158, 0, 0.64868948005364191}}, {{-0.75604736842105269, 0, 0.78309198215500941}}, {{-0.022910526315789603, 0, 0.78335262275116535}}, {{0.71022631578947371, 0, 0.63014543850378524}}, {{1.4433631578947366, 0, 0.58570592554910317}}, {{2.1764999999999999, 0, 0.45162530609299284}}}}},
        GeneratedMcsMv2CurveSampleRecord{"roof_centre", {{{{-2.1764999999999999, 0, 0.56210002847749818}}, {{-1.4891842105263158, 0, 1.1809855318779545}}, {{-0.75604736842105269, 0, 1.4478613941594782}}, {{-0.022910526315789603, 0, 1.4483829282127001}}, {{0.71022631578947371, 0, 1.1419685597179399}}, {{1.4433631578947366, 0, 1.0530648793763744}}, {{2.1764999999999999, 0, 0.7553477155739472}}}}},
        GeneratedMcsMv2CurveSampleRecord{"roof_rail_left", {{{{-2.1764999999999999, -0.028357386738555233, 0.52156477461269213}}, {{-1.4891842105263158, -0.35714583576429776, 1.1366048027012357}}, {{-0.75604736842105269, -0.54708248624810318, 1.4034806649827591}}, {{-0.022910526315789603, -0.56124937271594955, 1.4040021990359812}}, {{0.71022631578947371, -0.16758436805556082, 1.0975878305412208}}, {{1.4433631578947366, -0.057959823213073844, 1.0086841501996553}}, {{2.1764999999999999, -0.028345594895054595, 0.71096698639722811}}}}},
        GeneratedMcsMv2CurveSampleRecord{"roof_rail_right", {{{{-2.1764999999999999, 0.028357386738555233, 0.52156477461269213}}, {{-1.4891842105263158, 0.35714583576429776, 1.1366048027012357}}, {{-0.75604736842105269, 0.54708248624810318, 1.4034806649827591}}, {{-0.022910526315789603, 0.56124937271594955, 1.4040021990359812}}, {{0.71022631578947371, 0.16758436805556082, 1.0975878305412208}}, {{1.4433631578947366, 0.057959823213073844, 1.0086841501996553}}, {{2.1764999999999999, 0.028345594895054595, 0.71096698639722811}}}}},
        GeneratedMcsMv2CurveSampleRecord{"glass_shoulder_left", {{{{-2.1764999999999999, -0.043626748828546509, 0.50183452820464625}}, {{-1.4891842105263158, -0.4917277473518093, 0.89231862486112112}}, {{-0.75604736842105269, -0.55410077626989851, 0.94751048447703667}}, {{-0.022910526315789603, -0.53551782396580827, 0.90435538771459922}}, {{0.71022631578947371, -0.26046622359136795, 0.83310599485047176}}, {{1.4433631578947366, -0.089168958789344388, 0.82218339839153387}}, {{2.1764999999999999, -0.043608607530853216, 0.59708399362279907}}}}},
        GeneratedMcsMv2CurveSampleRecord{"glass_shoulder_right", {{{{-2.1764999999999999, 0.043626748828546509, 0.50183452820464625}}, {{-1.4891842105263158, 0.4917277473518093, 0.89231862486112112}}, {{-0.75604736842105269, 0.55410077626989851, 0.94751048447703667}}, {{-0.022910526315789603, 0.53551782396580827, 0.90435538771459922}}, {{0.71022631578947371, 0.26046622359136795, 0.83310599485047176}}, {{1.4433631578947366, 0.089168958789344388, 0.82218339839153387}}, {{2.1764999999999999, 0.043608607530853216, 0.59708399362279907}}}}},
        GeneratedMcsMv2CurveSampleRecord{"belt_left", {{{{-2.1764999999999999, -0.40572876410548259, 0.48013224317510655}}, {{-1.4891842105263158, -0.8301282729072571, 0.87061633983158138}}, {{-0.75604736842105269, -0.79279034143231608, 0.92580819944749693}}, {{-0.022910526315789603, -0.76582280960944904, 0.88265310268505948}}, {{0.71022631578947371, -0.78691560345030453, 0.81166132506567668}}, {{1.4433631578947366, -0.82927131674090282, 0.80048111336199412}}, {{2.1764999999999999, -0.4055600500369349, 0.57538170859325932}}}}},
        GeneratedMcsMv2CurveSampleRecord{"belt_right", {{{{-2.1764999999999999, 0.40572876410548259, 0.48013224317510655}}, {{-1.4891842105263158, 0.8301282729072571, 0.87061633983158138}}, {{-0.75604736842105269, 0.79279034143231608, 0.92580819944749693}}, {{-0.022910526315789603, 0.76582280960944904, 0.88265310268505948}}, {{0.71022631578947371, 0.78691560345030453, 0.81166132506567668}}, {{1.4433631578947366, 0.82927131674090282, 0.80048111336199412}}, {{2.1764999999999999, 0.4055600500369349, 0.57538170859325932}}}}},
        GeneratedMcsMv2CurveSampleRecord{"shoulder_left", {{{{-2.1764999999999999, -0.43626748828546508, 0.43841376613740418}}, {{-1.4891842105263158, -0.89261104613683562, 0.72780604455387965}}, {{-0.75604736842105269, -0.8524627327229205, 0.81002174095413637}}, {{-0.022910526315789603, -0.82346538667682689, 0.821954265704535}}, {{0.71022631578947371, -0.84614581016161794, 0.81166059423237025}}, {{1.4433631578947366, -0.89168958789344366, 0.77396267771952987}}, {{2.1764999999999999, -0.43608607530853216, 0.57093476150179068}}}}},
        GeneratedMcsMv2CurveSampleRecord{"shoulder_right", {{{{-2.1764999999999999, 0.43626748828546508, 0.43841376613740418}}, {{-1.4891842105263158, 0.89261104613683562, 0.72780604455387965}}, {{-0.75604736842105269, 0.8524627327229205, 0.81002174095413637}}, {{-0.022910526315789603, 0.82346538667682689, 0.821954265704535}}, {{0.71022631578947371, 0.84614581016161794, 0.81166059423237025}}, {{1.4433631578947366, 0.89168958789344366, 0.77396267771952987}}, {{2.1764999999999999, 0.43608607530853216, 0.57093476150179068}}}}},
        GeneratedMcsMv2CurveSampleRecord{"rocker_left", {{{{-2.1764999999999999, -0.39592739840855617, 0.1627030464663497}}, {{-1.4891842105263158, -0.78953233989865912, 0.15273413962075205}}, {{-0.75604736842105269, -0.74229311583899349, 0.15284310621979039}}, {{-0.022910526315789603, -0.71482469058835352, 0.15284285335888051}}, {{0.71022631578947371, -0.74740693119434243, 0.15284285335888051}}, {{1.4433631578947366, -0.82432475109951098, 0.15286750779108188}}, {{2.1764999999999999, -0.41416803617314019, 0.18242343268128822}}}}},
        GeneratedMcsMv2CurveSampleRecord{"rocker_right", {{{{-2.1764999999999999, 0.39592739840855617, 0.1627030464663497}}, {{-1.4891842105263158, 0.78953233989865912, 0.15273413962075205}}, {{-0.75604736842105269, 0.74229311583899349, 0.15284310621979039}}, {{-0.022910526315789603, 0.71482469058835352, 0.15284285335888051}}, {{0.71022631578947371, 0.74740693119434243, 0.15284285335888051}}, {{1.4433631578947366, 0.82432475109951098, 0.15286750779108188}}, {{2.1764999999999999, 0.41416803617314019, 0.18242343268128822}}}}},
        GeneratedMcsMv2CurveSampleRecord{"underbody_edge_left", {{{{-2.1764999999999999, -0.28506772685416043, 0.12818251039709991}}, {{-1.4891842105263158, -0.56846328472703456, 0.11639342822932934}}, {{-0.75604736842105269, -0.53445104340407523, 0.11832257015054057}}, {{-0.022910526315789603, -0.51467377722361451, 0.11832231728963059}}, {{0.71022631578947371, -0.53813299045992657, 0.11832231728963065}}, {{1.4433631578947366, -0.59351382079164794, 0.11834697172183201}}, {{2.1764999999999999, -0.29820098604466105, 0.14790289661203845}}}}},
        GeneratedMcsMv2CurveSampleRecord{"underbody_edge_right", {{{{-2.1764999999999999, 0.28506772685416043, 0.12818251039709991}}, {{-1.4891842105263158, 0.56846328472703456, 0.11639342822932934}}, {{-0.75604736842105269, 0.53445104340407523, 0.11832257015054057}}, {{-0.022910526315789603, 0.51467377722361451, 0.11832231728963059}}, {{0.71022631578947371, 0.53813299045992657, 0.11832231728963065}}, {{1.4433631578947366, 0.59351382079164794, 0.11834697172183201}}, {{2.1764999999999999, 0.29820098604466105, 0.14790289661203845}}}}},
    }},
    {{
        GeneratedMcsMv2PanelPatchRecord{"hood", 8966, true},
        GeneratedMcsMv2PanelPatchRecord{"roof", 10136, false},
        GeneratedMcsMv2PanelPatchRecord{"front_door_left", 3200, true},
        GeneratedMcsMv2PanelPatchRecord{"front_door_right", 3194, true},
        GeneratedMcsMv2PanelPatchRecord{"rear_door_left", 4442, true},
        GeneratedMcsMv2PanelPatchRecord{"rear_door_right", 4444, true},
        GeneratedMcsMv2PanelPatchRecord{"rear_quarter_left", 4368, false},
        GeneratedMcsMv2PanelPatchRecord{"rear_quarter_right", 4383, false},
        GeneratedMcsMv2PanelPatchRecord{"rear_hatch", 12226, true},
        GeneratedMcsMv2PanelPatchRecord{"front_bumper", 7358, false},
        GeneratedMcsMv2PanelPatchRecord{"rear_bumper", 6296, false},
    }},
    {{
        GeneratedMcsMv2PanelRelationshipRecord{"hood", "front_bumper", "panel_gap"},
        GeneratedMcsMv2PanelRelationshipRecord{"front_door_left", "rear_door_left", "panel_gap"},
        GeneratedMcsMv2PanelRelationshipRecord{"front_door_right", "rear_door_right", "panel_gap"},
        GeneratedMcsMv2PanelRelationshipRecord{"rear_door_left", "rear_quarter_left", "panel_gap"},
        GeneratedMcsMv2PanelRelationshipRecord{"rear_door_right", "rear_quarter_right", "panel_gap"},
        GeneratedMcsMv2PanelRelationshipRecord{"rear_hatch", "rear_bumper", "panel_gap"},
    }},
    {{
        GeneratedMcsMv2BodyInWhiteMemberRecord{"floor_pan", "floor_pan"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"rocker_left", "rocker_left"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"rocker_right", "rocker_right"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"front_rail_left", "front_rail_left"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"rear_rail_left", "rear_rail_left"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"front_rail_right", "front_rail_right"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"rear_rail_right", "rear_rail_right"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"front_crossmember", "front_crossmember"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"centre_crossmember", "centre_crossmember"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"rear_crossmember", "rear_crossmember"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"a_pillar_left", "a_pillar_left"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"b_pillar_left", "b_pillar_left"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"c_pillar_left", "c_pillar_left"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"roof_rail_left", "roof_rail_left"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"a_pillar_right", "a_pillar_right"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"b_pillar_right", "b_pillar_right"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"c_pillar_right", "c_pillar_right"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"roof_rail_right", "roof_rail_right"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"front_tower_left", "front_tower_left"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"front_tower_right", "front_tower_right"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"rear_tower_left", "rear_tower_left"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"rear_tower_right", "rear_tower_right"},
    }},
    {{
        GeneratedMcsMv2BodyInWhiteJointRecord{"front_rail_left", "front_crossmember", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"front_rail_right", "front_crossmember", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"rear_rail_left", "rear_crossmember", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"rear_rail_right", "rear_crossmember", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"rocker_left", "floor_pan", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"rocker_right", "floor_pan", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"front_crossmember", "floor_pan", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"centre_crossmember", "floor_pan", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"rear_crossmember", "floor_pan", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"a_pillar_left", "rocker_left", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"b_pillar_left", "rocker_left", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"c_pillar_left", "rocker_left", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"a_pillar_left", "roof_rail_left", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"b_pillar_left", "roof_rail_left", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"c_pillar_left", "roof_rail_left", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"front_tower_left", "front_rail_left", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"rear_tower_left", "rear_rail_left", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"a_pillar_right", "rocker_right", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"b_pillar_right", "rocker_right", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"c_pillar_right", "rocker_right", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"a_pillar_right", "roof_rail_right", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"b_pillar_right", "roof_rail_right", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"c_pillar_right", "roof_rail_right", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"front_tower_right", "front_rail_right", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"rear_tower_right", "rear_rail_right", "spot_weld_and_adhesive"},
    }},
    {{"occupant_front_left_head", "occupant_front_left_torso", "occupant_front_right_head", "occupant_front_right_torso", "occupant_rear_left_head", "occupant_rear_left_torso", "occupant_rear_right_head", "occupant_rear_right_torso"}},
    {{"engine_envelope", "rear_differential_envelope", ""}},
    2
},
GeneratedMcsMv2VariantRecord{
    "track", "Track Widebody AWD", "GRMN-proportioned wide-track package with stronger fender volumes, splitter and rear wing.", "GRMN Corolla dimensional envelope with original body-surface parameters",
    GeneratedMcsMv2ReferenceFrameRecord{"MCSMv2.0.1.VehicleReferenceFrame.v1", "MCSMv2VehicleFrame", {{0, 0, 0}}, {{1, 0, 0}}, {{0, 1, 0}}, {{0, 0, 1}}, "right", "m", "deg", {{{{0, 1, 0, 0}}, {{0, 0, 1, 0}}, {{1, 0, 0, 0}}, {{0, 0, 0, 1}}}}},
    GeneratedMcsMv2FieldCalibrationRecord{"MCSMv2.0.1.FieldCalibration.v1", "iterative inverse affine field prewarp", 3, {{1.0000409203, 1.0066159281, 1.0062522687}}, {{-7.6044999999999994e-05, -8.9000000000000003e-09, -0.006341579}}, 0.0066159281194781983, false, 0, {{-2.20472, -0.92456000000000005, 0.10955342}}, {{2.20472, 0.92456000000000005, 1.4732000000000001}}, {{-2.2047195799999999, -0.92458848999999999, 0.11033704}}, {{2.2047199700000002, 0.92458848999999999, 1.47291313}}, 0.0025000000000000001},
    GeneratedMcsMv2SourceAssuranceRecord{"MCSMv2.0.1.Assurance.v1", "V2", "Package and semantic geometry consistency", true, {{GeneratedMcsMv2AssuranceLevelRecord{"V0", true, false, "finite, framed, acyclic parameter model", ""}, GeneratedMcsMv2AssuranceLevelRecord{"V1", true, false, "watertight, wound, non-self-intersecting final body mesh", ""}, GeneratedMcsMv2AssuranceLevelRecord{"V2", true, false, "package, continuous sections, curve network, datums and BIW graph consistent", ""}, GeneratedMcsMv2AssuranceLevelRecord{"V3", false, true, "independent closure, glass and suspension kinematics", "Static occupant and sampled tyre checks exist; full closure/glass/suspension kinematics remain deferred."}, GeneratedMcsMv2AssuranceLevelRecord{"V4", false, false, "manufacturing and structural analysis", ""}, GeneratedMcsMv2AssuranceLevelRecord{"V5", false, false, "CFD, crash, thermal, ergonomic and physical validation", ""}}}, {{"panel patches remain semantic mesh masks rather than exclusive surface-domain partitions", "BIW validation is graph connectivity rather than geometric joint/load-path analysis", "semantic-surface to implicit-field agreement is diagnostic, not yet constrained", "inverse fitting remains a synthetic semantic metric demonstration"}}},
    GeneratedMcsMv2PackageRecord{4.40944, 1.8491199999999999, 1.4731999999999998, 2.6390599999999997, 1.5874999999999999, 1.61798, 0.105, 0.88518999999999981, 0.88518999999999981},
    GeneratedMcsMv2PlatformRecord{0.23999999999999999, 0.15834359999999997, -0.76532739999999988, 0.54000000000000004, 1.1600000000000001, 0.085000000000000006, 0.25887680000000002},
    GeneratedMcsMv2StyleRecord{0.97999999999999998, 1, 1, 1, 0.97999999999999998, 0.57999999999999996, 0.44, 0.27000000000000002, 0.055, 0.055, 0.059999999999999998, 0.074999999999999997, 0.055, 0.029999999999999999, 1.45, 1.3500000000000001, 1.1200000000000001},
    GeneratedMcsMv2WheelRecord{0.34200000000000003, 0.245, 1.5874999999999999, 1.61798, -32, 32, -0.070000000000000007, 0.059999999999999998, 0.024},
    GeneratedMcsMv2PowertrainRecord{"ICE_AWD", "AWD", false, 0, 0.78000000000000003, 1.1799999999999999, 0.56000000000000005, 4},
    GeneratedMcsMv2ClosureRecord{68, 68, 62, 72, 0.55000000000000004, 0.0040000000000000001},
    GeneratedMcsMv2ColorRecord{170, 5, 4}, false,
    {{
        GeneratedMcsMv2ParameterDependencyRecord{"battery_pack", "false", "bool", "bool", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"ArchitectureReference", "GRMN Corolla dimensional envelope with original body-surface parameters", 0.71999999999999997, false, 0, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"battery_thickness", "0.0", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"ArchitectureReference", "GRMN Corolla dimensional envelope with original body-surface parameters", 0.71999999999999997, false, 0, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"belt_rise", "0.055", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"bonnet_max_deg", "62.0", "deg", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MVPv2.5 closure design ranges", 0.55000000000000004, true, 2, "Angle uncertainty is expressed in degrees."}},
        GeneratedMcsMv2ParameterDependencyRecord{"crossover_cladding", "false", "bool", "bool", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"door_scallop", "0.03", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"drive_layout", "\"AWD\"", "enum", "enum", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"ArchitectureReference", "GRMN Corolla dimensional envelope with original body-surface parameters", 0.71999999999999997, false, 0, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"engine_envelope_height", "0.56", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"ArchitectureReference", "GRMN Corolla dimensional envelope with original body-surface parameters", 0.71999999999999997, false, 0, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"engine_envelope_length", "0.78", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"ArchitectureReference", "GRMN Corolla dimensional envelope with original body-surface parameters", 0.71999999999999997, false, 0, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"engine_envelope_width", "1.18", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"ArchitectureReference", "GRMN Corolla dimensional envelope with original body-surface parameters", 0.71999999999999997, false, 0, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"exhaust_count", "4", "count", "int", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"ArchitectureReference", "GRMN Corolla dimensional envelope with original body-surface parameters", 0.71999999999999997, false, 0, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"eye_z", "1.1600000000000001", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2 platform derivation", 0.81999999999999995, true, 0.01, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"floor_height", "0.24", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2 platform derivation", 0.81999999999999995, true, 0.01, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"wheelbase", "2.6390599999999997", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PublishedPackage", "MCSMv1 official manufacturer package research", 0.94999999999999996, true, 0.002, "Manufacturer-derived package reference; not proprietary CAD."}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_axle_x", "1.3195299999999999", "m", "float", "wheelbase / 2", {{"wheelbase", "", "", ""}}, 1, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_door_max_deg", "68.0", "deg", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MVPv2.5 closure design ranges", 0.55000000000000004, true, 2, "Angle uncertainty is expressed in degrees."}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_overhang", "0.8851899999999998", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PublishedPackage", "MCSMv1 official manufacturer package research", 0.94999999999999996, true, 0.002, "Manufacturer-derived package reference; not proprietary CAD."}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_extent_x", "2.2047199999999996", "m", "float", "front_axle_x + front_overhang", {{"front_axle_x", "front_overhang", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_fender_amplitude", "0.06", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_hpoint_x", "0.15834359999999997", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2 platform derivation", 0.81999999999999995, true, 0.01, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"width", "1.8491199999999999", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PublishedPackage", "MCSMv1 official manufacturer package research", 0.94999999999999996, true, 0.002, "Manufacturer-derived package reference; not proprietary CAD."}},
        GeneratedMcsMv2ParameterDependencyRecord{"track_front", "1.5875", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PublishedPackage", "MCSMv1 official manufacturer package research", 0.94999999999999996, true, 0.002, "Manufacturer-derived package reference; not proprietary CAD."}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_track_body_margin", "0.26161999999999996", "m", "float", "width - track_front", {{"width", "track_front", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"wheel_radius", "0.342", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PackageReference", "MCSMv1 tyre and wheel package research", 0.81999999999999995, true, 0.0060000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"wheel_width", "0.245", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PackageReference", "MCSMv1 tyre and wheel package research", 0.81999999999999995, true, 0.0060000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"steer_min_deg", "-32.0", "deg", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"EngineeringAssumption", "MCSMv2 concept-level envelope policy", 0.57999999999999996, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"steer_max_deg", "32.0", "deg", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"EngineeringAssumption", "MCSMv2 concept-level envelope policy", 0.57999999999999996, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"steer_max_abs_deg", "32.0", "deg", "float", "max(abs(steer_min_deg), abs(steer_max_deg))", {{"steer_min_deg", "steer_max_deg", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"wheelhouse_clearance", "0.024", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"EngineeringAssumption", "MCSMv2 concept-level envelope policy", 0.57999999999999996, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_wheelhouse_rx", "0.4309151098685676", "m", "float", "r + 0.5*w*sin(delta_max) + clearance", {{"wheel_radius", "wheel_width", "steer_max_abs_deg", "wheelhouse_clearance"}}, 4, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_wheelhouse_ry", "0.3277323883677561", "m", "float", "0.5*w + r*sin(delta_max) + clearance", {{"wheel_radius", "wheel_width", "steer_max_abs_deg", "wheelhouse_clearance"}}, 4, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"travel_min", "-0.07", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"EngineeringAssumption", "MCSMv2 concept-level envelope policy", 0.57999999999999996, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"travel_down", "0.07", "m", "float", "abs(travel_min)", {{"travel_min", "", "", ""}}, 1, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"travel_max", "0.06", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"EngineeringAssumption", "MCSMv2 concept-level envelope policy", 0.57999999999999996, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"travel_up", "0.06", "m", "float", "abs(travel_max)", {{"travel_max", "", "", ""}}, 1, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_wheelhouse_rz", "0.43600000000000005", "m", "float", "r + max(jounce,rebound) + clearance", {{"wheel_radius", "travel_down", "travel_up", "wheelhouse_clearance"}}, 4, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"greenhouse_front_factor", "1.0", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"greenhouse_rear_factor", "1.0", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"grille_scale", "1.12", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"ground_clearance", "0.105", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PublishedPackage", "MCSMv1 official manufacturer package research", 0.94999999999999996, true, 0.002, "Manufacturer-derived package reference; not proprietary CAD."}},
        GeneratedMcsMv2ParameterDependencyRecord{"hatch_max_deg", "72.0", "deg", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MVPv2.5 closure design ranges", 0.55000000000000004, true, 2, "Angle uncertainty is expressed in degrees."}},
        GeneratedMcsMv2ParameterDependencyRecord{"head_clearance", "0.085", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2 platform derivation", 0.81999999999999995, true, 0.01, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"height", "1.4731999999999998", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PublishedPackage", "MCSMv1 official manufacturer package research", 0.94999999999999996, true, 0.002, "Manufacturer-derived package reference; not proprietary CAD."}},
        GeneratedMcsMv2ParameterDependencyRecord{"hood_wedge", "0.58", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"hpoint_z", "0.54", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2 platform derivation", 0.81999999999999995, true, 0.01, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"length", "4.40944", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PublishedPackage", "MCSMv1 official manufacturer package research", 0.94999999999999996, true, 0.002, "Manufacturer-derived package reference; not proprietary CAD."}},
        GeneratedMcsMv2ParameterDependencyRecord{"minimum_roof_from_occupant", "1.345", "m", "float", "H-point + seated-head envelope + clearance", {{"hpoint_z", "head_clearance", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"nominal_panel_gap", "0.004", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MVPv2.5 closure design ranges", 0.55000000000000004, true, 2, "Angle uncertainty is expressed in degrees."}},
        GeneratedMcsMv2ParameterDependencyRecord{"nose_taper", "1.0", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"occupant_roof_margin", "0.12819999999999987", "m", "float", "height - minimum_roof_from_occupant", {{"height", "minimum_roof_from_occupant", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_axle_x", "-1.3195299999999999", "m", "float", "-wheelbase / 2", {{"wheelbase", "", "", ""}}, 1, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_overhang", "0.8851899999999998", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PublishedPackage", "MCSMv1 official manufacturer package research", 0.94999999999999996, true, 0.002, "Manufacturer-derived package reference; not proprietary CAD."}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_extent_x", "-2.2047199999999996", "m", "float", "rear_axle_x - rear_overhang", {{"rear_axle_x", "rear_overhang", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"resolved_length", "4.409439999999999", "m", "float", "front_extent_x - rear_extent_x", {{"front_extent_x", "rear_extent_x", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"package_length_residual", "-8.881784197001252e-16", "m", "float", "resolved_length - length", {{"resolved_length", "length", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"powertrain_architecture", "\"ICE_AWD\"", "enum", "enum", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"ArchitectureReference", "GRMN Corolla dimensional envelope with original body-surface parameters", 0.71999999999999997, false, 0, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_door_max_deg", "68.0", "deg", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MVPv2.5 closure design ranges", 0.55000000000000004, true, 2, "Angle uncertainty is expressed in degrees."}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_haunch_amplitude", "0.075", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_hpoint_x", "-0.7653273999999999", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2 platform derivation", 0.81999999999999995, true, 0.01, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"track_rear", "1.61798", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PublishedPackage", "MCSMv1 official manufacturer package research", 0.94999999999999996, true, 0.002, "Manufacturer-derived package reference; not proprietary CAD."}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_track_body_margin", "0.2311399999999999", "m", "float", "width - track_rear", {{"width", "track_rear", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_wheelhouse_rx", "0.36600000000000005", "m", "float", "r + clearance", {{"wheel_radius", "wheelhouse_clearance", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_wheelhouse_ry", "0.1465", "m", "float", "0.5*w + clearance", {{"wheel_width", "wheelhouse_clearance", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_wheelhouse_rz", "0.43600000000000005", "m", "float", "r + max(jounce,rebound) + clearance", {{"wheel_radius", "travel_down", "travel_up", "wheelhouse_clearance"}}, 4, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"rocker_tuck", "0.055", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"roof_crown", "0.44", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"roof_scale", "0.98", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"seat_lateral", "0.2588768", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2 platform derivation", 0.81999999999999995, true, 0.01, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"shoulder_strength", "0.055", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"side_glass_travel", "0.55", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MVPv2.5 closure design ranges", 0.55000000000000004, true, 2, "Angle uncertainty is expressed in degrees."}},
        GeneratedMcsMv2ParameterDependencyRecord{"splitter_scale", "1.35", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"spoiler_scale", "1.45", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"tail_taper", "0.98", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"tumblehome", "0.27", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
    }},
    {{
        GeneratedMcsMv2StationRecord{"tail_face", 0, 0, -2.2047199999999996, 0.12947222458330851, 0.27344407619906375, 0.16421985658302238, 0.37978343916536633, 0.28724900065537617, 0.40782783390325228, 0.44282478325229274, 0.42166393135610597, 0.48984823665182192, 0.39214745616117858, 0.5116932917878888, 0.042166393135610596, 0.53155333509877922, 0.027408155538146888, 0.56723431759812515, 0.68000000000000005},
        GeneratedMcsMv2StationRecord{"rear_bumper", 1, 0.059999999999999998, -1.9401535999999997, 0.1095534208012611, 0.47302759755734303, 0.14430105280097494, 0.6569827743851987, 0.34620865985023674, 0.7064721925635471, 0.57956737121604862, 0.73094337488055039, 0.63317536297615007, 0.67977733863891188, 0.65502041811221701, 0.07309433748805505, 0.72174966429300358, 0.047511319367235787, 0.76642235541859971, 0.68000000000000005},
        GeneratedMcsMv2StationRecord{"rear_axle", 2, 0.20074884792626727, -1.3195299999999999, 0.11963523049886449, 0.5838992747068622, 0.15438286249857833, 0.81097121487064194, 0.44873043301106097, 0.88644639006693571, 0.77324521945865998, 0.92455999999999994, 0.95687204656595937, 0.85984079999999996, 0.9787171017020263, 0.60373767999999994, 1.2980623237046922, 0.45029825609688801, 1.3427350148302883, 0.80000000000000004},
        GeneratedMcsMv2StationRecord{"rear_door", 3, 0.33000000000000002, -0.74960479999999974, 0.11951282269228476, 0.52954795150320522, 0.15426045469199862, 0.73548326597667391, 0.4715765021702405, 0.81238056202758824, 0.81874075895907439, 0.85158929770033809, 0.9358428994945156, 0.79197804686131434, 0.95768795463058243, 0.55608781139832075, 1.4094035990708207, 0.53347260745330671, 1.4540762901964168, 0.68000000000000005},
        GeneratedMcsMv2StationRecord{"b_pillar", 4, 0.45500000000000002, -0.19842479999999973, 0.11951282269228476, 0.50219564320491861, 0.15426045469199862, 0.69749394889572025, 0.47752980320783572, 0.77311384250742565, 0.83056467629763142, 0.81177828540150943, 0.90421840639211637, 0.75495380542340385, 0.9260634615281832, 0.5300912203671857, 1.4285273088744037, 0.5523974571551622, 1.4731999999999998, 0.80000000000000004},
        GeneratedMcsMv2StationRecord{"front_door", 5, 0.58499999999999996, 0.37480239999999965, 0.11951282269228471, 0.50637750642424628, 0.15426045469199856, 0.70330209225589757, 0.47602219523849432, 0.77731543908312228, 0.82757039935852317, 0.81507278898663937, 0.85861038295898451, 0.75801769375757477, 0.88045543809505133, 0.53224253120827558, 1.320957657986938, 0.49643151362946103, 1.3656303491125341, 0.68000000000000005},
        GeneratedMcsMv2StationRecord{"a_pillar", 6, 0.68999999999999995, 0.83779359999999947, 0.1195128226922846, 0.54641998066892294, 0.15426045469199845, 0.75891663981794855, 0.47029831817485324, 0.82563599791627407, 0.8162021435237915, 0.8591535363273235, 0.8162021435237915, 0.79901278878441095, 0.83718735743701866, 0.21424508975159912, 1.0755416860498275, 0.112515449356619, 1.1202143771754234, 0.68000000000000005},
        GeneratedMcsMv2StationRecord{"hood_rear", 7, 0.77000000000000002, 1.1905487999999997, 0.11951282269228465, 0.59241883538829887, 0.15426045469199851, 0.82280393803930407, 0.46352358343767108, 0.87784648404465715, 0.80274676758744357, 0.90468101881624075, 0.83101684367302608, 0.84135334749910395, 0.85286189880909302, 0.090468101881624075, 1.0445465244614487, 0.058804266223055651, 1.0892192155870448, 0.68000000000000005},
        GeneratedMcsMv2StationRecord{"front_axle", 8, 0.79925115207373276, 1.3195299999999999, 0.11951282269228482, 0.60301099139319725, 0.15426045469199867, 0.83751526582388514, 0.45977414609101774, 0.88738439390315449, 0.79529996841284023, 0.91131447888238593, 0.82150460805401537, 0.84752246536061893, 0.8433496631900822, 0.091131447888238584, 1.0374923008593024, 0.059235441127355085, 1.0821649919848986, 0.80000000000000004},
        GeneratedMcsMv2StationRecord{"nose", 9, 0.94499999999999995, 1.9622007999999993, 0.12028793245135808, 0.50639431591342798, 0.15503556445107192, 0.70332543876865006, 0.40353227324309576, 0.73047841679177616, 0.68283301549413122, 0.74248084774545653, 0.69395441967684612, 0.69050718840327463, 0.71579947481291306, 0.07424808477454567, 0.88192855863775221, 0.048261255103454685, 0.92660124976334846, 0.68000000000000005},
        GeneratedMcsMv2StationRecord{"front_face", 10, 1, 2.2047199999999996, 0.14939102836535603, 0.29399984977069077, 0.1841386603650699, 0.40833312468151495, 0.36453395932972044, 0.42345793213295785, 0.5766791446676518, 0.43007475149268781, 0.5766791446676518, 0.3999695188881997, 0.59813059737243246, 0.043007475149268781, 0.71952969123973887, 0.02795485884702471, 0.764202382365335, 0.68000000000000005},
    }},
    {{
        GeneratedMcsMv2SectionSampleRecord{-2.2047199999999996, {{0.12947222458330851, 0.27344407619906375, 0.16421985658302238, 0.37978343916536633, 0.28724900065537617, 0.40782783390325228, 0.44282478325229274, 0.42166393135610597, 0.48984823665182192, 0.39214745616117858, 0.5116932917878888, 0.042166393135610596, 0.53155333509877922, 0.027408155538146888, 0.56723431759812515}}},
        GeneratedMcsMv2SectionSampleRecord{-1.4698133333333332, {{0.11814805307135033, 0.57623011903463794, 0.15289568507106416, 0.80031960977033034, 0.43373421086196567, 0.87371976810019492, 0.74492367632715117, 0.91072480275540402, 0.90118398215995599, 0.84697406656252572, 0.92302903729602293, 0.53061017807935151, 1.1789284557103696, 0.3738903182828886, 1.2236011468359658}}},
        GeneratedMcsMv2SectionSampleRecord{-0.7349066666666666, {{0.11951282269228476, 0.52858322418533443, 0.15426045469199862, 0.73414336692407567, 0.47182548873011321, 0.81101885244636396, 0.81923526647837197, 0.85022183804471008, 0.9349517519029179, 0.79070630938158026, 0.95679680703898473, 0.55519486024319553, 1.4104565848918327, 0.5342836924917328, 1.4551292760174288}}},
        GeneratedMcsMv2SectionSampleRecord{0, {{0.11951282269228475, 0.50272904392122975, 0.15426045469199862, 0.69823478322393029, 0.47731229134788222, 0.77364093752291085, 0.83013267502813881, 0.8121866344459685, 0.89056527541353592, 0.75533357003475077, 0.91241033054960274, 0.5306860867012605, 1.4057323460408448, 0.54499364073519374, 1.450405037166441}}},
        GeneratedMcsMv2SectionSampleRecord{0.73490666666666637, {{0.1195128226922846, 0.5350808869212702, 0.15426045469199845, 0.74316789850176412, 0.47188533013806611, 0.81211039314452937, 0.81935430328088543, 0.84692255295463659, 0.81935477133107992, 0.78763797424781223, 0.8404466032180451, 0.28287032619063412, 1.1097239601158706, 0.17330039547908282, 1.1543966512414665}}},
        GeneratedMcsMv2SectionSampleRecord{1.4698133333333327, {{0.11954471183556145, 0.59736555971058447, 0.15429234383527529, 0.82967438848692288, 0.45232888197407201, 0.87793024805784281, 0.780482567977133, 0.90098628194356545, 0.80411052414785111, 0.83791724220751584, 0.82595557928391794, 0.090098628194356531, 1.0182975619785279, 0.05856410832633175, 1.0629702531041241}}},
        GeneratedMcsMv2SectionSampleRecord{2.2047199999999996, {{0.14939102836535603, 0.29399984977069071, 0.1841386603650699, 0.40833312468151495, 0.36453395932972044, 0.42345793213295785, 0.5766791446676518, 0.43007475149268781, 0.57667924466765175, 0.39996951888819976, 0.59813069737243241, 0.043007475149268781, 0.71952979123973881, 0.027954858847024706, 0.76420248236533495}}},
    }},
    {{
        GeneratedMcsMv2FieldSampleRecord{{{0, 0, 0.81025999999999998}}, -0.63214503958278179, -0.63214503958278179},
        GeneratedMcsMv2FieldSampleRecord{{{0, 0.8121866344459685, 0.83013267502813881}}, 4.3827875863316861e-09, 4.3827875863316861e-09},
        GeneratedMcsMv2FieldSampleRecord{{{0, 1.1464543999999999, 0.83013267502813881}}, 0.33426776883163239, 0.33426776883163239},
        GeneratedMcsMv2FieldSampleRecord{{{2.2247199999999996, 0, 0.58927999999999991}}, 0.025145572475009582, 0.025145572475009582},
        GeneratedMcsMv2FieldSampleRecord{{{0, 0, 1.4931999999999999}}, 0.052212310862297259, 0.052212310862297259},
        GeneratedMcsMv2FieldSampleRecord{{{1.3195299999999999, -0.79374999999999996, 0.34200000000000003}}, -0.073849311538512444, 0.32773238836775609},
    }},
    {{
        GeneratedMcsMv2CurveSampleRecord{"centre_spine", {{{{-2.2047199999999996, 0, 0.34835327109071684}}, {{-1.5084926315789471, 0, 0.64960649947286231}}, {{-0.76585010526315789, 0, 0.78621142702738767}}, {{-0.023207578947368468, 0, 0.78669740269243871}}, {{0.71943494736842073, 0, 0.64037329589585479}}, {{1.4620774736842099, 0, 0.59186481115209444}}, {{2.2047199999999996, 0, 0.45679675536534547}}}}},
        GeneratedMcsMv2CurveSampleRecord{"roof_centre", {{{{-2.2047199999999996, 0, 0.56723431759812515}}, {{-1.5084926315789471, 0, 1.1818124901971718}}, {{-0.76585010526315789, 0, 1.4529097386655734}}, {{-0.023207578947368468, 0, 1.4538819826925926}}, {{0.71943494736842073, 0, 1.161233769099425}}, {{1.4620774736842099, 0, 1.0641882573239765}}, {{2.2047199999999996, 0, 0.76420248236533495}}}}},
        GeneratedMcsMv2CurveSampleRecord{"roof_rail_left", {{{{-2.2047199999999996, -0.027408155538146888, 0.53155333509877922}}, {{-1.5084926315789471, -0.34435022390440417, 1.1371397990715757}}, {{-0.76585010526315789, -0.53253055245761771, 1.4082370475399772}}, {{-0.023207578947368468, -0.54659055436689474, 1.4092092915669965}}, {{0.71943494736842073, -0.1872721249857634, 1.1165610779738291}}, {{1.4620774736842099, -0.058630436420472908, 1.0195155661983804}}, {{2.2047199999999996, -0.027954858847024706, 0.71952979123973881}}}}},
        GeneratedMcsMv2CurveSampleRecord{"roof_rail_right", {{{{-2.2047199999999996, 0.027408155538146888, 0.53155333509877922}}, {{-1.5084926315789471, 0.34435022390440417, 1.1371397990715757}}, {{-0.76585010526315789, 0.53253055245761771, 1.4082370475399772}}, {{-0.023207578947368468, 0.54659055436689474, 1.4092092915669965}}, {{0.71943494736842073, 0.1872721249857634, 1.1165610779738291}}, {{1.4620774736842099, 0.058630436420472908, 1.0195155661983804}}, {{2.2047199999999996, 0.027954858847024706, 0.71952979123973881}}}}},
        GeneratedMcsMv2CurveSampleRecord{"glass_shoulder_left", {{{{-2.2047199999999996, -0.042166393135610596, 0.5116932917878888}}, {{-1.5084926315789471, -0.49358585340140204, 0.90066485212538905}}, {{-0.76585010526315789, -0.55712499250087877, 0.95871820162789845}}, {{-0.023207578947368468, -0.53057134967955533, 0.91395026254966183}}, {{0.71943494736842073, -0.2959871464237368, 0.84095050298284513}}, {{1.4620774736842099, -0.090200671416112155, 0.82699590219131125}}, {{2.2047199999999996, -0.043007475149268781, 0.59813069737243241}}}}},
        GeneratedMcsMv2CurveSampleRecord{"glass_shoulder_right", {{{{-2.2047199999999996, 0.042166393135610596, 0.5116932917878888}}, {{-1.5084926315789471, 0.49358585340140204, 0.90066485212538905}}, {{-0.76585010526315789, 0.55712499250087877, 0.95871820162789845}}, {{-0.023207578947368468, 0.53057134967955533, 0.91395026254966183}}, {{0.71943494736842073, 0.2959871464237368, 0.84095050298284513}}, {{1.4620774736842099, 0.090200671416112155, 0.82699590219131125}}, {{2.2047199999999996, 0.043007475149268781, 0.59813069737243241}}}}},
        GeneratedMcsMv2CurveSampleRecord{"belt_left", {{{{-2.2047199999999996, -0.39214745616117858, 0.48984823665182192}}, {{-1.5084926315789471, -0.83979871551408369, 0.87881979698932211}}, {{-0.76585010526315789, -0.79345519605791304, 0.93687314649183162}}, {{-0.023207578947368468, -0.75525054154972471, 0.892105207413595}}, {{0.71943494736842073, -0.78585078095821714, 0.81982778387694699}}, {{1.4620774736842099, -0.83886624416984312, 0.80515084705524442}}, {{2.2047199999999996, -0.39996951888819976, 0.57667924466765175}}}}},
        GeneratedMcsMv2CurveSampleRecord{"belt_right", {{{{-2.2047199999999996, 0.39214745616117858, 0.48984823665182192}}, {{-1.5084926315789471, 0.83979871551408369, 0.87881979698932211}}, {{-0.76585010526315789, 0.79345519605791304, 0.93687314649183162}}, {{-0.023207578947368468, 0.75525054154972471, 0.892105207413595}}, {{0.71943494736842073, 0.78585078095821714, 0.81982778387694699}}, {{1.4620774736842099, 0.83886624416984312, 0.80515084705524442}}, {{2.2047199999999996, 0.39996951888819976, 0.57667924466765175}}}}},
        GeneratedMcsMv2CurveSampleRecord{"shoulder_left", {{{{-2.2047199999999996, -0.42166393135610597, 0.44282478325229274}}, {{-1.5084926315789471, -0.90300937152052008, 0.73488547688885908}}, {{-0.76585010526315789, -0.85317763016979908, 0.81817163839969176}}, {{-0.023207578947368468, -0.81209735650508019, 0.83022429308598611}}, {{0.71943494736842073, -0.84500083974001827, 0.81982705214799045}}, {{1.4620774736842099, -0.90200671416112166, 0.78139759032012901}}, {{2.2047199999999996, -0.43007475149268781, 0.5766791446676518}}}}},
        GeneratedMcsMv2CurveSampleRecord{"shoulder_right", {{{{-2.2047199999999996, 0.42166393135610597, 0.44282478325229274}}, {{-1.5084926315789471, 0.90300937152052008, 0.73488547688885908}}, {{-0.76585010526315789, 0.85317763016979908, 0.81817163839969176}}, {{-0.023207578947368468, 0.81209735650508019, 0.83022429308598611}}, {{0.71943494736842073, 0.84500083974001827, 0.81982705214799045}}, {{1.4620774736842099, 0.90200671416112166, 0.78139759032012901}}, {{2.2047199999999996, 0.43007475149268781, 0.5766791446676518}}}}},
        GeneratedMcsMv2CurveSampleRecord{"rocker_left", {{{{-2.2047199999999996, -0.37978343916536633, 0.16421985658302238}}, {{-1.5084926315789471, -0.79434201921166503, 0.15214814074826671}}, {{-0.76585010526315789, -0.73705147387194381, 0.15426074738891588}}, {{-0.023207578947368468, -0.69807379013958404, 0.15426045469199862}}, {{0.71943494736842073, -0.74074580711814486, 0.15426045469199848}}, {{1.4620774736842099, -0.830453722033898, 0.15428899697992618}}, {{2.2047199999999996, -0.40833312468151495, 0.1841386603650699}}}}},
        GeneratedMcsMv2CurveSampleRecord{"rocker_right", {{{{-2.2047199999999996, 0.37978343916536633, 0.16421985658302238}}, {{-1.5084926315789471, 0.79434201921166503, 0.15214814074826671}}, {{-0.76585010526315789, 0.73705147387194381, 0.15426074738891588}}, {{-0.023207578947368468, 0.69807379013958404, 0.15426045469199862}}, {{0.71943494736842073, 0.74074580711814486, 0.15426045469199848}}, {{1.4620774736842099, 0.830453722033898, 0.15428899697992618}}, {{2.2047199999999996, 0.40833312468151495, 0.1841386603650699}}}}},
        GeneratedMcsMv2CurveSampleRecord{"underbody_edge_left", {{{{-2.2047199999999996, -0.27344407619906375, 0.12947222458330851}}, {{-1.5084926315789471, -0.57192625383239881, 0.11740050874855287}}, {{-0.76585010526315789, -0.53067706118779945, 0.11951311538920203}}, {{-0.023207578947368468, -0.50261312890050058, 0.11951282269228475}}, {{0.71943494736842073, -0.53333698112506434, 0.11951282269228462}}, {{1.4620774736842099, -0.59792667986440651, 0.11954136498021233}}, {{2.2047199999999996, -0.29399984977069071, 0.14939102836535603}}}}},
        GeneratedMcsMv2CurveSampleRecord{"underbody_edge_right", {{{{-2.2047199999999996, 0.27344407619906375, 0.12947222458330851}}, {{-1.5084926315789471, 0.57192625383239881, 0.11740050874855287}}, {{-0.76585010526315789, 0.53067706118779945, 0.11951311538920203}}, {{-0.023207578947368468, 0.50261312890050058, 0.11951282269228475}}, {{0.71943494736842073, 0.53333698112506434, 0.11951282269228462}}, {{1.4620774736842099, 0.59792667986440651, 0.11954136498021233}}, {{2.2047199999999996, 0.29399984977069071, 0.14939102836535603}}}}},
    }},
    {{
        GeneratedMcsMv2PanelPatchRecord{"hood", 8841, true},
        GeneratedMcsMv2PanelPatchRecord{"roof", 9896, false},
        GeneratedMcsMv2PanelPatchRecord{"front_door_left", 3341, true},
        GeneratedMcsMv2PanelPatchRecord{"front_door_right", 3336, true},
        GeneratedMcsMv2PanelPatchRecord{"rear_door_left", 4559, true},
        GeneratedMcsMv2PanelPatchRecord{"rear_door_right", 4552, true},
        GeneratedMcsMv2PanelPatchRecord{"rear_quarter_left", 4528, false},
        GeneratedMcsMv2PanelPatchRecord{"rear_quarter_right", 4543, false},
        GeneratedMcsMv2PanelPatchRecord{"rear_hatch", 12071, true},
        GeneratedMcsMv2PanelPatchRecord{"front_bumper", 7136, false},
        GeneratedMcsMv2PanelPatchRecord{"rear_bumper", 6055, false},
    }},
    {{
        GeneratedMcsMv2PanelRelationshipRecord{"hood", "front_bumper", "panel_gap"},
        GeneratedMcsMv2PanelRelationshipRecord{"front_door_left", "rear_door_left", "panel_gap"},
        GeneratedMcsMv2PanelRelationshipRecord{"front_door_right", "rear_door_right", "panel_gap"},
        GeneratedMcsMv2PanelRelationshipRecord{"rear_door_left", "rear_quarter_left", "panel_gap"},
        GeneratedMcsMv2PanelRelationshipRecord{"rear_door_right", "rear_quarter_right", "panel_gap"},
        GeneratedMcsMv2PanelRelationshipRecord{"rear_hatch", "rear_bumper", "panel_gap"},
    }},
    {{
        GeneratedMcsMv2BodyInWhiteMemberRecord{"floor_pan", "floor_pan"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"rocker_left", "rocker_left"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"rocker_right", "rocker_right"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"front_rail_left", "front_rail_left"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"rear_rail_left", "rear_rail_left"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"front_rail_right", "front_rail_right"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"rear_rail_right", "rear_rail_right"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"front_crossmember", "front_crossmember"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"centre_crossmember", "centre_crossmember"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"rear_crossmember", "rear_crossmember"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"a_pillar_left", "a_pillar_left"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"b_pillar_left", "b_pillar_left"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"c_pillar_left", "c_pillar_left"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"roof_rail_left", "roof_rail_left"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"a_pillar_right", "a_pillar_right"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"b_pillar_right", "b_pillar_right"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"c_pillar_right", "c_pillar_right"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"roof_rail_right", "roof_rail_right"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"front_tower_left", "front_tower_left"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"front_tower_right", "front_tower_right"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"rear_tower_left", "rear_tower_left"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"rear_tower_right", "rear_tower_right"},
    }},
    {{
        GeneratedMcsMv2BodyInWhiteJointRecord{"front_rail_left", "front_crossmember", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"front_rail_right", "front_crossmember", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"rear_rail_left", "rear_crossmember", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"rear_rail_right", "rear_crossmember", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"rocker_left", "floor_pan", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"rocker_right", "floor_pan", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"front_crossmember", "floor_pan", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"centre_crossmember", "floor_pan", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"rear_crossmember", "floor_pan", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"a_pillar_left", "rocker_left", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"b_pillar_left", "rocker_left", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"c_pillar_left", "rocker_left", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"a_pillar_left", "roof_rail_left", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"b_pillar_left", "roof_rail_left", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"c_pillar_left", "roof_rail_left", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"front_tower_left", "front_rail_left", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"rear_tower_left", "rear_rail_left", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"a_pillar_right", "rocker_right", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"b_pillar_right", "rocker_right", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"c_pillar_right", "rocker_right", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"a_pillar_right", "roof_rail_right", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"b_pillar_right", "roof_rail_right", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"c_pillar_right", "roof_rail_right", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"front_tower_right", "front_rail_right", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"rear_tower_right", "rear_rail_right", "spot_weld_and_adhesive"},
    }},
    {{"occupant_front_left_head", "occupant_front_left_torso", "occupant_front_right_head", "occupant_front_right_torso", "occupant_rear_left_head", "occupant_rear_left_torso", "occupant_rear_right_head", "occupant_rear_right_torso"}},
    {{"engine_envelope", "rear_differential_envelope", ""}},
    2
},
GeneratedMcsMv2VariantRecord{
    "aero", "Aero Fastback AWD", "S3-sized low-drag-oriented variation with a lower greenhouse, longer taper and reduced frontal openings.", "Audi S3 Sportback package, original aerodynamic variation",
    GeneratedMcsMv2ReferenceFrameRecord{"MCSMv2.0.1.VehicleReferenceFrame.v1", "MCSMv2VehicleFrame", {{0, 0, 0}}, {{1, 0, 0}}, {{0, 1, 0}}, {{0, 0, 1}}, "right", "m", "deg", {{{{0, 1, 0, 0}}, {{0, 0, 1, 0}}, {{1, 0, 0, 0}}, {{0, 0, 0, 1}}}}},
    GeneratedMcsMv2FieldCalibrationRecord{"MCSMv2.0.1.FieldCalibration.v1", "iterative inverse affine field prewarp", 3, {{1.0000459628, 1.0054918983000001, 1.0053958404000001}}, {{-8.3354499999999996e-05, -1.7999999999999999e-08, -0.0054337075999999996}}, 0.0054918983221912132, false, 0, {{-2.1764999999999999, -0.90800000000000003, 0.10792077}}, {{2.1764999999999999, 0.90800000000000003, 1.4404999999999999}}, {{-2.1764998800000002, -0.90772220999999997, 0.10858234}}, {{2.1764998100000001, 0.90772211999999997, 1.4403069500000001}}, 0.0025000000000000001},
    GeneratedMcsMv2SourceAssuranceRecord{"MCSMv2.0.1.Assurance.v1", "V2", "Package and semantic geometry consistency", true, {{GeneratedMcsMv2AssuranceLevelRecord{"V0", true, false, "finite, framed, acyclic parameter model", ""}, GeneratedMcsMv2AssuranceLevelRecord{"V1", true, false, "watertight, wound, non-self-intersecting final body mesh", ""}, GeneratedMcsMv2AssuranceLevelRecord{"V2", true, false, "package, continuous sections, curve network, datums and BIW graph consistent", ""}, GeneratedMcsMv2AssuranceLevelRecord{"V3", false, true, "independent closure, glass and suspension kinematics", "Static occupant and sampled tyre checks exist; full closure/glass/suspension kinematics remain deferred."}, GeneratedMcsMv2AssuranceLevelRecord{"V4", false, false, "manufacturing and structural analysis", ""}, GeneratedMcsMv2AssuranceLevelRecord{"V5", false, false, "CFD, crash, thermal, ergonomic and physical validation", ""}}}, {{"panel patches remain semantic mesh masks rather than exclusive surface-domain partitions", "BIW validation is graph connectivity rather than geometric joint/load-path analysis", "semantic-surface to implicit-field agreement is diagnostic, not yet constrained", "inverse fitting remains a synthetic semantic metric demonstration"}}},
    GeneratedMcsMv2PackageRecord{4.3529999999999998, 1.8160000000000001, 1.4404999999999999, 2.6240000000000001, 1.5489999999999999, 1.518, 0.12, 0.86450000000000005, 0.86450000000000005},
    GeneratedMcsMv2PlatformRecord{0.25, 0.15744, -0.76095999999999997, 0.55000000000000004, 1.1699999999999999, 0.085000000000000006, 0.25424000000000002},
    GeneratedMcsMv2StyleRecord{0.95499999999999996, 1.04, 1.1599999999999999, 1, 0.83999999999999997, 0.46000000000000002, 0.40000000000000002, 0.34999999999999998, 0.037999999999999999, 0.024, 0.02, 0.028000000000000001, 0.059999999999999998, 0.017999999999999999, 0.71999999999999997, 0.88, 0.71999999999999997},
    GeneratedMcsMv2WheelRecord{0.33300000000000002, 0.22500000000000001, 1.5489999999999999, 1.518, -32, 32, -0.070000000000000007, 0.059999999999999998, 0.028000000000000001},
    GeneratedMcsMv2PowertrainRecord{"ICE_AWD", "AWD", false, 0, 0.78000000000000003, 1.1799999999999999, 0.56000000000000005, 4},
    GeneratedMcsMv2ClosureRecord{68, 68, 62, 72, 0.55000000000000004, 0.0040000000000000001},
    GeneratedMcsMv2ColorRecord{154, 14, 12}, false,
    {{
        GeneratedMcsMv2ParameterDependencyRecord{"battery_pack", "false", "bool", "bool", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"ArchitectureReference", "Audi S3 Sportback package, original aerodynamic variation", 0.71999999999999997, false, 0, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"battery_thickness", "0.0", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"ArchitectureReference", "Audi S3 Sportback package, original aerodynamic variation", 0.71999999999999997, false, 0, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"belt_rise", "0.038", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"bonnet_max_deg", "62.0", "deg", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MVPv2.5 closure design ranges", 0.55000000000000004, true, 2, "Angle uncertainty is expressed in degrees."}},
        GeneratedMcsMv2ParameterDependencyRecord{"crossover_cladding", "false", "bool", "bool", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"door_scallop", "0.018", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"drive_layout", "\"AWD\"", "enum", "enum", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"ArchitectureReference", "Audi S3 Sportback package, original aerodynamic variation", 0.71999999999999997, false, 0, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"engine_envelope_height", "0.56", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"ArchitectureReference", "Audi S3 Sportback package, original aerodynamic variation", 0.71999999999999997, false, 0, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"engine_envelope_length", "0.78", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"ArchitectureReference", "Audi S3 Sportback package, original aerodynamic variation", 0.71999999999999997, false, 0, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"engine_envelope_width", "1.18", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"ArchitectureReference", "Audi S3 Sportback package, original aerodynamic variation", 0.71999999999999997, false, 0, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"exhaust_count", "4", "count", "int", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"ArchitectureReference", "Audi S3 Sportback package, original aerodynamic variation", 0.71999999999999997, false, 0, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"eye_z", "1.17", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2 platform derivation", 0.81999999999999995, true, 0.01, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"floor_height", "0.25", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2 platform derivation", 0.81999999999999995, true, 0.01, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"wheelbase", "2.624", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PublishedPackage", "MCSMv1 official manufacturer package research", 0.94999999999999996, true, 0.002, "Manufacturer-derived package reference; not proprietary CAD."}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_axle_x", "1.312", "m", "float", "wheelbase / 2", {{"wheelbase", "", "", ""}}, 1, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_door_max_deg", "68.0", "deg", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MVPv2.5 closure design ranges", 0.55000000000000004, true, 2, "Angle uncertainty is expressed in degrees."}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_overhang", "0.8645", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PublishedPackage", "MCSMv1 official manufacturer package research", 0.94999999999999996, true, 0.002, "Manufacturer-derived package reference; not proprietary CAD."}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_extent_x", "2.1765", "m", "float", "front_axle_x + front_overhang", {{"front_axle_x", "front_overhang", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_fender_amplitude", "0.02", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_hpoint_x", "0.15744", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2 platform derivation", 0.81999999999999995, true, 0.01, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"width", "1.816", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PublishedPackage", "MCSMv1 official manufacturer package research", 0.94999999999999996, true, 0.002, "Manufacturer-derived package reference; not proprietary CAD."}},
        GeneratedMcsMv2ParameterDependencyRecord{"track_front", "1.549", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PublishedPackage", "MCSMv1 official manufacturer package research", 0.94999999999999996, true, 0.002, "Manufacturer-derived package reference; not proprietary CAD."}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_track_body_margin", "0.2670000000000001", "m", "float", "width - track_front", {{"width", "track_front", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"wheel_radius", "0.333", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PackageReference", "MCSMv1 tyre and wheel package research", 0.81999999999999995, true, 0.0060000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"wheel_width", "0.225", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PackageReference", "MCSMv1 tyre and wheel package research", 0.81999999999999995, true, 0.0060000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"steer_min_deg", "-32.0", "deg", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"EngineeringAssumption", "MCSMv2 concept-level envelope policy", 0.57999999999999996, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"steer_max_deg", "32.0", "deg", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"EngineeringAssumption", "MCSMv2 concept-level envelope policy", 0.57999999999999996, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"steer_max_abs_deg", "32.0", "deg", "float", "max(abs(steer_min_deg), abs(steer_max_deg))", {{"steer_min_deg", "steer_max_deg", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"wheelhouse_clearance", "0.028", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"EngineeringAssumption", "MCSMv2 concept-level envelope policy", 0.57999999999999996, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_wheelhouse_rx", "0.4206159172262356", "m", "float", "r + 0.5*w*sin(delta_max) + clearance", {{"wheel_radius", "wheel_width", "steer_max_abs_deg", "wheelhouse_clearance"}}, 4, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_wheelhouse_ry", "0.3169631149896573", "m", "float", "0.5*w + r*sin(delta_max) + clearance", {{"wheel_radius", "wheel_width", "steer_max_abs_deg", "wheelhouse_clearance"}}, 4, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"travel_min", "-0.07", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"EngineeringAssumption", "MCSMv2 concept-level envelope policy", 0.57999999999999996, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"travel_down", "0.07", "m", "float", "abs(travel_min)", {{"travel_min", "", "", ""}}, 1, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"travel_max", "0.06", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"EngineeringAssumption", "MCSMv2 concept-level envelope policy", 0.57999999999999996, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"travel_up", "0.06", "m", "float", "abs(travel_max)", {{"travel_max", "", "", ""}}, 1, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_wheelhouse_rz", "0.43100000000000005", "m", "float", "r + max(jounce,rebound) + clearance", {{"wheel_radius", "travel_down", "travel_up", "wheelhouse_clearance"}}, 4, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"greenhouse_front_factor", "1.04", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"greenhouse_rear_factor", "1.16", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"grille_scale", "0.72", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"ground_clearance", "0.12", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PublishedPackage", "MCSMv1 official manufacturer package research", 0.94999999999999996, true, 0.002, "Manufacturer-derived package reference; not proprietary CAD."}},
        GeneratedMcsMv2ParameterDependencyRecord{"hatch_max_deg", "72.0", "deg", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MVPv2.5 closure design ranges", 0.55000000000000004, true, 2, "Angle uncertainty is expressed in degrees."}},
        GeneratedMcsMv2ParameterDependencyRecord{"head_clearance", "0.085", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2 platform derivation", 0.81999999999999995, true, 0.01, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"height", "1.4405", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PublishedPackage", "MCSMv1 official manufacturer package research", 0.94999999999999996, true, 0.002, "Manufacturer-derived package reference; not proprietary CAD."}},
        GeneratedMcsMv2ParameterDependencyRecord{"hood_wedge", "0.46", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"hpoint_z", "0.55", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2 platform derivation", 0.81999999999999995, true, 0.01, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"length", "4.353", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PublishedPackage", "MCSMv1 official manufacturer package research", 0.94999999999999996, true, 0.002, "Manufacturer-derived package reference; not proprietary CAD."}},
        GeneratedMcsMv2ParameterDependencyRecord{"minimum_roof_from_occupant", "1.355", "m", "float", "H-point + seated-head envelope + clearance", {{"hpoint_z", "head_clearance", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"nominal_panel_gap", "0.004", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MVPv2.5 closure design ranges", 0.55000000000000004, true, 2, "Angle uncertainty is expressed in degrees."}},
        GeneratedMcsMv2ParameterDependencyRecord{"nose_taper", "1.0", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"occupant_roof_margin", "0.08549999999999991", "m", "float", "height - minimum_roof_from_occupant", {{"height", "minimum_roof_from_occupant", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_axle_x", "-1.312", "m", "float", "-wheelbase / 2", {{"wheelbase", "", "", ""}}, 1, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_overhang", "0.8645", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PublishedPackage", "MCSMv1 official manufacturer package research", 0.94999999999999996, true, 0.002, "Manufacturer-derived package reference; not proprietary CAD."}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_extent_x", "-2.1765", "m", "float", "rear_axle_x - rear_overhang", {{"rear_axle_x", "rear_overhang", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"resolved_length", "4.353", "m", "float", "front_extent_x - rear_extent_x", {{"front_extent_x", "rear_extent_x", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"package_length_residual", "0.0", "m", "float", "resolved_length - length", {{"resolved_length", "length", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"powertrain_architecture", "\"ICE_AWD\"", "enum", "enum", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"ArchitectureReference", "Audi S3 Sportback package, original aerodynamic variation", 0.71999999999999997, false, 0, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_door_max_deg", "68.0", "deg", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MVPv2.5 closure design ranges", 0.55000000000000004, true, 2, "Angle uncertainty is expressed in degrees."}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_haunch_amplitude", "0.028", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_hpoint_x", "-0.76096", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2 platform derivation", 0.81999999999999995, true, 0.01, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"track_rear", "1.518", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PublishedPackage", "MCSMv1 official manufacturer package research", 0.94999999999999996, true, 0.002, "Manufacturer-derived package reference; not proprietary CAD."}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_track_body_margin", "0.29800000000000004", "m", "float", "width - track_rear", {{"width", "track_rear", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_wheelhouse_rx", "0.36100000000000004", "m", "float", "r + clearance", {{"wheel_radius", "wheelhouse_clearance", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_wheelhouse_ry", "0.1405", "m", "float", "0.5*w + clearance", {{"wheel_width", "wheelhouse_clearance", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_wheelhouse_rz", "0.43100000000000005", "m", "float", "r + max(jounce,rebound) + clearance", {{"wheel_radius", "travel_down", "travel_up", "wheelhouse_clearance"}}, 4, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"rocker_tuck", "0.06", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"roof_crown", "0.4", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"roof_scale", "0.955", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"seat_lateral", "0.25424", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2 platform derivation", 0.81999999999999995, true, 0.01, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"shoulder_strength", "0.024", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"side_glass_travel", "0.55", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MVPv2.5 closure design ranges", 0.55000000000000004, true, 2, "Angle uncertainty is expressed in degrees."}},
        GeneratedMcsMv2ParameterDependencyRecord{"splitter_scale", "0.88", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"spoiler_scale", "0.72", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"tail_taper", "0.84", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"tumblehome", "0.35", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
    }},
    {{
        GeneratedMcsMv2StationRecord{"tail_face", 0, 0, -2.1764999999999999, 0.12754272418109824, 0.24357841498007729, 0.16254955048664727, 0.33830335413899626, 0.28296862629335323, 0.36204688572187982, 0.43622611397982353, 0.37093243948900007, 0.47466012869337626, 0.34496716872477007, 0.49666813346301736, 0.037093243948900012, 0.5166763195348264, 0.024110608566785006, 0.55842086361432286, 0.68000000000000005},
        GeneratedMcsMv2StationRecord{"rear_bumper", 1, 0.059999999999999998, -1.9153199999999999, 0.1079207666147755, 0.41305266384590017, 0.14499685468630863, 0.573684255341528, 0.34104962068966888, 0.61902702386111674, 0.57093085267262977, 0.63566504636187982, 0.61679794306598379, 0.59116849311654829, 0.63880594783562483, 0.063566504636187987, 0.70963451948284195, 0.041318228013522196, 0.75464043927755109, 0.68000000000000005},
        GeneratedMcsMv2StationRecord{"rear_axle", 2, 0.19859866758557315, -1.3119999999999998, 0.11784376596816604, 0.57454729398052173, 0.15285059227371509, 0.79798235275072482, 0.44141229257193482, 0.87923590990710931, 0.76047709052087009, 0.90800000000000003, 0.93924988353078798, 0.84444000000000008, 0.96125788830042902, 0.58565999999999996, 1.2400975388972653, 0.44299545326817052, 1.2851034586919741, 0.80000000000000004},
        GeneratedMcsMv2StationRecord{"rear_door", 3, 0.33000000000000002, -0.74000999999999983, 0.11773174539793688, 0.53419318620065648, 0.15273857170348593, 0.74193498083424525, 0.46454912945041543, 0.82778987796519243, 0.80653988405016164, 0.85772035876735242, 0.92031507366980636, 0.79767993365363776, 0.9423230784394474, 0.5532296314049423, 1.3748087127049853, 0.54873589109155674, 1.4198146324996943, 0.68000000000000005},
        GeneratedMcsMv2StationRecord{"b_pillar", 4, 0.45500000000000002, -0.19588499999999986, 0.11773174539793677, 0.51731175682247399, 0.15273857170348581, 0.71848855114232502, 0.47041370955359013, 0.8036631260245013, 0.81818759175507827, 0.83327617834101231, 0.89000131620642597, 0.77494684585714146, 0.91200932097606702, 0.53746313502995291, 1.3954940802052909, 0.5700442527837416, 1.4404999999999999, 0.80000000000000004},
        GeneratedMcsMv2StationRecord{"front_door", 5, 0.58499999999999996, 0.37000499999999992, 0.11773174539793688, 0.52191833302082236, 0.15273857170348593, 0.72488657364003117, 0.46892856918448189, 0.80772705735338757, 0.81523793796643274, 0.83664778514231875, 0.84441291424423881, 0.77808244018235651, 0.86642091901387985, 0.53963782141679562, 1.2776629477130821, 0.50397289477900231, 1.3226688675077911, 0.68000000000000005},
        GeneratedMcsMv2StationRecord{"a_pillar", 6, 0.68999999999999995, 0.82706999999999953, 0.11773174539793699, 0.55051271929139267, 0.15273857170348604, 0.76460099901582312, 0.4632899939925903, 0.83579155934784477, 0.80403910112698107, 0.86129230058304451, 0.80403910112698107, 0.80100183954223136, 0.82281525593214733, 0.07743740652565341, 1.0010515478599828, 0.035939002763294751, 1.0460574676546919, 0.68000000000000005},
        GeneratedMcsMv2StationRecord{"hood_rear", 7, 0.77000000000000002, 1.1753100000000001, 0.11773174539793693, 0.58893143724735575, 0.15273857170348598, 0.81796032951021636, 0.45661622186296602, 0.87269877987438405, 0.79078424814731074, 0.89335733786289684, 0.82289323710567819, 0.83082232421249402, 0.84490124187531923, 0.089335733786289687, 1.025236414124123, 0.058068226961088293, 1.0702423339188321, 0.68000000000000005},
        GeneratedMcsMv2StationRecord{"front_axle", 8, 0.8014013324144269, 1.3120000000000003, 0.11773174539793688, 0.60033959318494334, 0.15273857170348593, 0.83380499053464363, 0.45261683824606075, 0.88175918563122258, 0.78284102790817944, 0.90039286569651611, 0.81325695145214871, 0.83736536509776005, 0.83526495622178976, 0.090039286569651628, 1.0167662925241572, 0.058525536270273557, 1.0617722123188662, 0.80000000000000004},
        GeneratedMcsMv2StationRecord{"nose", 9, 0.94499999999999995, 1.9370849999999997, 0.11849530384091378, 0.51704848381857227, 0.15350213014646283, 0.71812289419246145, 0.39751895096926848, 0.74498751259567741, 0.67265754832439273, 0.7565731261262818, 0.69063453644538442, 0.70361300729744214, 0.71264254121502557, 0.075657312612628189, 0.86369338593563605, 0.049177253198208322, 0.90869930573034519, 0.68000000000000005},
        GeneratedMcsMv2StationRecord{"front_face", 10, 1, 2.1764999999999999, 0.14716468174742109, 0.30200862888367697, 0.18217150805297011, 0.41945642900510699, 0.35910182165068577, 0.43477240278280455, 0.56808566882551281, 0.44142343623034969, 0.57558703300413394, 0.41052379569422526, 0.59759503777377498, 0.044142343623034971, 0.70415427234245143, 0.028692523354972729, 0.74916019213716045, 0.68000000000000005},
    }},
    {{
        GeneratedMcsMv2SectionSampleRecord{-2.1764999999999999, {{0.12754272418109824, 0.24357841498007729, 0.16254955048664727, 0.33830335413899626, 0.28296862629335323, 0.36204688572187982, 0.43622611397982353, 0.37093243948900007, 0.47466012869337626, 0.34496716872477007, 0.49666813346301736, 0.037093243948900012, 0.5166763195348264, 0.024110608566785006, 0.55842086361432286}}},
        GeneratedMcsMv2SectionSampleRecord{-1.4510000000000001, {{0.1165063182248194, 0.5626639749218133, 0.15178516459039101, 0.78147774294696337, 0.42757388523131723, 0.85974768245850031, 0.73436153355038314, 0.88750951635883024, 0.88679436644233556, 0.82538385021371197, 0.9088023712119766, 0.51934106064233054, 1.1373897798655501, 0.36781789500084683, 1.1823956996602589}}},
        GeneratedMcsMv2SectionSampleRecord{-0.72550000000000003, {{0.11773174539793688, 0.53356728770838036, 0.15273857170348593, 0.74106567737275053, 0.46479457379041178, 0.82692510833791721, 0.80702735406047987, 0.85685396971770211, 0.91946993684947831, 0.796874191837463, 0.94147794161911935, 0.55267081046791788, 1.3759826663416872, 0.54966419838278757, 1.4209885861363962}}},
        GeneratedMcsMv2SectionSampleRecord{0, {{0.11773174539793679, 0.51792891323884749, 0.15273857170348584, 0.71934571283173254, 0.47019943919628315, 0.80420031161081851, 0.81776202848388535, 0.83371883337301533, 0.87634644598492628, 0.77535851503690423, 0.89835445075456732, 0.53806446501630101, 1.3712061438336638, 0.56133750179275832, 1.4162120636283728}}},
        GeneratedMcsMv2SectionSampleRecord{0.72549999999999981, {{0.11773174539793699, 0.54219636561304196, 0.15273857170348604, 0.75305050779589167, 0.46485335054582516, 0.82764650112670046, 0.80714428036963681, 0.85414737703905352, 0.80714474380832346, 0.79435706064631995, 0.82630102471045275, 0.13576722568887012, 1.0226136507915577, 0.09121839767420048, 1.0676195705862668}}},
        GeneratedMcsMv2SectionSampleRecord{1.4509999999999996, {{0.11776018102336888, 0.59608490500128652, 0.15276700732891793, 0.82789570139067581, 0.44572811141777313, 0.87454048673385387, 0.76913228639445408, 0.89273376099605894, 0.79742777895946404, 0.8302423977263349, 0.81943578372910508, 0.089273376099605922, 0.99910208376915111, 0.058027694464743845, 1.0441080035638601}}},
        GeneratedMcsMv2SectionSampleRecord{2.1764999999999999, {{0.14716468174742109, 0.30200862888367697, 0.18217150805297011, 0.41945642900510705, 0.35910182165068577, 0.43477240278280455, 0.56808566882551281, 0.44142343623034969, 0.57558703300413394, 0.41052379569422531, 0.59759503777377498, 0.044142343623034964, 0.70415427234245143, 0.028692523354972726, 0.74916019213716045}}},
    }},
    {{
        GeneratedMcsMv2FieldSampleRecord{{{0, 0, 0.79227499999999995}}, -0.61609833779906376, -0.61609833779906376},
        GeneratedMcsMv2FieldSampleRecord{{{0, 0.83371883337301533, 0.81776202848388535}}, 4.5436306042919835e-09, 4.5436306042919835e-09},
        GeneratedMcsMv2FieldSampleRecord{{{0, 1.12592, 0.81776202848388535}}, 0.29220116917544564, 0.29220116917544564},
        GeneratedMcsMv2FieldSampleRecord{{{2.1964999999999999, 0, 0.57619999999999993}}, 0.02025709886900166, 0.02025709886900166},
        GeneratedMcsMv2FieldSampleRecord{{{0, 0, 1.4604999999999999}}, 0.053364750309429762, 0.053364750309429762},
        GeneratedMcsMv2FieldSampleRecord{{{1.3120000000000001, -0.77449999999999997, 0.33300000000000002}}, -0.087129337204626484, 0.31696311498965729},
    }},
    {{
        GeneratedMcsMv2CurveSampleRecord{"centre_spine", {{{{-2.1764999999999999, 0, 0.34298179389771055}}, {{-1.4891842105263158, 0, 0.63023259476526416}}, {{-0.75604736842105269, 0, 0.76809877432818818}}, {{-0.022910526315789603, 0, 0.76887883941073154}}, {{0.71022631578947371, 0, 0.59576069910739504}}, {{1.4433631578947366, 0, 0.58153672256281252}}, {{2.1764999999999999, 0, 0.44816243694229074}}}}},
        GeneratedMcsMv2CurveSampleRecord{"roof_centre", {{{{-2.1764999999999999, 0, 0.55842086361432286}}, {{-1.4891842105263158, 0, 1.1446862776416828}}, {{-0.75604736842105269, 0, 1.4184655440114282}}, {{-0.022910526315789603, 0, 1.4200259334235263}}, {{0.71022631578947371, 0, 1.0737896528168531}}, {{1.4433631578947366, 0, 1.0453164336418235}}, {{2.1764999999999999, 0, 0.74916019213716045}}}}},
        GeneratedMcsMv2CurveSampleRecord{"roof_rail_left", {{{{-2.1764999999999999, -0.024110608566785006, 0.5166763195348264}}, {{-1.4891842105263158, -0.33793654482832103, 1.099680357846974}}, {{-0.75604736842105269, -0.54764309297615288, 1.3734596242167192}}, {{-0.022910526315789603, -0.56321692626490205, 1.3750200136288173}}, {{0.71022631578947371, -0.10726488631378556, 1.0287837330221441}}, {{1.4433631578947366, -0.058080396539193231, 1.0003105138471144}}, {{2.1764999999999999, -0.028692523354972726, 0.70415427234245143}}}}},
        GeneratedMcsMv2CurveSampleRecord{"roof_rail_right", {{{{-2.1764999999999999, 0.024110608566785006, 0.5166763195348264}}, {{-1.4891842105263158, 0.33793654482832103, 1.099680357846974}}, {{-0.75604736842105269, 0.54764309297615288, 1.3734596242167192}}, {{-0.022910526315789603, 0.56321692626490205, 1.3750200136288173}}, {{0.71022631578947371, 0.10726488631378556, 1.0287837330221441}}, {{1.4433631578947366, 0.058080396539193231, 1.0003105138471144}}, {{2.1764999999999999, 0.028692523354972726, 0.70415427234245143}}}}},
        GeneratedMcsMv2CurveSampleRecord{"glass_shoulder_left", {{{{-2.1764999999999999, -0.037093243948900012, 0.49666813346301736}}, {{-1.4891842105263158, -0.48305861256457483, 0.88613629612077283}}, {{-0.75604736842105269, -0.55388417755269037, 0.94329996440086117}}, {{-0.022910526315789603, -0.53794848129400352, 0.89991206358535647}}, {{0.71022631578947371, -0.15261029463048481, 0.82687817542676056}}, {{1.4433631578947366, -0.089354456214143438, 0.82045173068616506}}, {{2.1764999999999999, -0.044142343623034964, 0.59759503777377498}}}}},
        GeneratedMcsMv2CurveSampleRecord{"glass_shoulder_right", {{{{-2.1764999999999999, 0.037093243948900012, 0.49666813346301736}}, {{-1.4891842105263158, 0.48305861256457483, 0.88613629612077283}}, {{-0.75604736842105269, 0.55388417755269037, 0.94329996440086117}}, {{-0.022910526315789603, 0.53794848129400352, 0.89991206358535647}}, {{0.71022631578947371, 0.15261029463048481, 0.82687817542676056}}, {{1.4433631578947366, 0.089354456214143438, 0.82045173068616506}}, {{2.1764999999999999, 0.044142343623034964, 0.59759503777377498}}}}},
        GeneratedMcsMv2CurveSampleRecord{"belt_left", {{{{-2.1764999999999999, -0.34496716872477007, 0.47466012869337626}}, {{-1.4891842105263158, -0.8142260596873957, 0.86412829135113178}}, {{-0.75604736842105269, -0.79862369786666987, 0.92129195963122013}}, {{-0.022910526315789603, -0.77526958551902159, 0.87790405881571543}}, {{0.71022631578947371, -0.79335892591511314, 0.80761070582988215}}, {{1.4433631578947366, -0.83099644279153384, 0.79844372591652402}}, {{2.1764999999999999, -0.41052379569422531, 0.57558703300413394}}}}},
        GeneratedMcsMv2CurveSampleRecord{"belt_right", {{{{-2.1764999999999999, 0.34496716872477007, 0.47466012869337626}}, {{-1.4891842105263158, 0.8142260596873957, 0.86412829135113178}}, {{-0.75604736842105269, 0.79862369786666987, 0.92129195963122013}}, {{-0.022910526315789603, 0.77526958551902159, 0.87790405881571543}}, {{0.71022631578947371, 0.79335892591511314, 0.80761070582988215}}, {{1.4433631578947366, 0.83099644279153384, 0.79844372591652402}}, {{2.1764999999999999, 0.41052379569422531, 0.57558703300413394}}}}},
        GeneratedMcsMv2CurveSampleRecord{"shoulder_left", {{{{-2.1764999999999999, -0.37093243948900007, 0.43622611397982353}}, {{-1.4891842105263158, -0.87551189213698444, 0.72452933640497363}}, {{-0.75604736842105269, -0.85873515899641928, 0.80597892904619095}}, {{-0.022910526315789603, -0.83362321023550712, 0.81785228119380338}}, {{0.71022631578947371, -0.85307411388721832, 0.80760998334541101}}, {{1.4433631578947366, -0.89354456214143418, 0.77002854407234367}}, {{2.1764999999999999, -0.44142343623034969, 0.56808566882551281}}}}},
        GeneratedMcsMv2CurveSampleRecord{"shoulder_right", {{{{-2.1764999999999999, 0.37093243948900007, 0.43622611397982353}}, {{-1.4891842105263158, 0.87551189213698444, 0.72452933640497363}}, {{-0.75604736842105269, 0.85873515899641928, 0.80597892904619095}}, {{-0.022910526315789603, 0.83362321023550712, 0.81785228119380338}}, {{0.71022631578947371, 0.85307411388721832, 0.80760998334541101}}, {{1.4433631578947366, 0.89354456214143418, 0.77002854407234367}}, {{2.1764999999999999, 0.44142343623034969, 0.56808566882551281}}}}},
        GeneratedMcsMv2CurveSampleRecord{"rocker_left", {{{{-2.1764999999999999, -0.33830335413899626, 0.16254955048664727}}, {{-1.4891842105263158, -0.77177890583912245, 0.15120658994013386}}, {{-0.75604736842105269, -0.7429686455935568, 0.15273883095049717}}, {{-0.022910526315789603, -0.71916130388768962, 0.15273857170348584}}, {{0.71022631578947371, -0.75132935582840288, 0.15273857170348601}}, {{1.4433631578947366, -0.8285245399784319, 0.15276383778935071}}, {{2.1764999999999999, -0.41945642900510705, 0.18217150805297011}}}}},
        GeneratedMcsMv2CurveSampleRecord{"rocker_right", {{{{-2.1764999999999999, 0.33830335413899626, 0.16254955048664727}}, {{-1.4891842105263158, 0.77177890583912245, 0.15120658994013386}}, {{-0.75604736842105269, 0.7429686455935568, 0.15273883095049717}}, {{-0.022910526315789603, 0.71916130388768962, 0.15273857170348584}}, {{0.71022631578947371, 0.75132935582840288, 0.15273857170348601}}, {{1.4433631578947366, 0.8285245399784319, 0.15276383778935071}}, {{2.1764999999999999, 0.41945642900510705, 0.18217150805297011}}}}},
        GeneratedMcsMv2CurveSampleRecord{"underbody_edge_left", {{{{-2.1764999999999999, -0.24357841498007729, 0.12754272418109824}}, {{-1.4891842105263158, -0.55568081220416821, 0.11577891188884538}}, {{-0.75604736842105269, -0.53493742482736073, 0.11773200464494812}}, {{-0.022910526315789603, -0.51779613879913655, 0.11773174539793678}}, {{0.71022631578947371, -0.54095713619645014, 0.11773174539793696}}, {{1.4433631578947366, -0.5965376687844709, 0.11775701148380165}}, {{2.1764999999999999, -0.30200862888367697, 0.14716468174742109}}}}},
        GeneratedMcsMv2CurveSampleRecord{"underbody_edge_right", {{{{-2.1764999999999999, 0.24357841498007729, 0.12754272418109824}}, {{-1.4891842105263158, 0.55568081220416821, 0.11577891188884538}}, {{-0.75604736842105269, 0.53493742482736073, 0.11773200464494812}}, {{-0.022910526315789603, 0.51779613879913655, 0.11773174539793678}}, {{0.71022631578947371, 0.54095713619645014, 0.11773174539793696}}, {{1.4433631578947366, 0.5965376687844709, 0.11775701148380165}}, {{2.1764999999999999, 0.30200862888367697, 0.14716468174742109}}}}},
    }},
    {{
        GeneratedMcsMv2PanelPatchRecord{"hood", 9155, true},
        GeneratedMcsMv2PanelPatchRecord{"roof", 9942, false},
        GeneratedMcsMv2PanelPatchRecord{"front_door_left", 3011, true},
        GeneratedMcsMv2PanelPatchRecord{"front_door_right", 3001, true},
        GeneratedMcsMv2PanelPatchRecord{"rear_door_left", 4277, true},
        GeneratedMcsMv2PanelPatchRecord{"rear_door_right", 4275, true},
        GeneratedMcsMv2PanelPatchRecord{"rear_quarter_left", 4979, false},
        GeneratedMcsMv2PanelPatchRecord{"rear_quarter_right", 4980, false},
        GeneratedMcsMv2PanelPatchRecord{"rear_hatch", 13089, true},
        GeneratedMcsMv2PanelPatchRecord{"front_bumper", 7506, false},
        GeneratedMcsMv2PanelPatchRecord{"rear_bumper", 5510, false},
    }},
    {{
        GeneratedMcsMv2PanelRelationshipRecord{"hood", "front_bumper", "panel_gap"},
        GeneratedMcsMv2PanelRelationshipRecord{"front_door_left", "rear_door_left", "panel_gap"},
        GeneratedMcsMv2PanelRelationshipRecord{"front_door_right", "rear_door_right", "panel_gap"},
        GeneratedMcsMv2PanelRelationshipRecord{"rear_door_left", "rear_quarter_left", "panel_gap"},
        GeneratedMcsMv2PanelRelationshipRecord{"rear_door_right", "rear_quarter_right", "panel_gap"},
        GeneratedMcsMv2PanelRelationshipRecord{"rear_hatch", "rear_bumper", "panel_gap"},
    }},
    {{
        GeneratedMcsMv2BodyInWhiteMemberRecord{"floor_pan", "floor_pan"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"rocker_left", "rocker_left"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"rocker_right", "rocker_right"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"front_rail_left", "front_rail_left"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"rear_rail_left", "rear_rail_left"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"front_rail_right", "front_rail_right"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"rear_rail_right", "rear_rail_right"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"front_crossmember", "front_crossmember"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"centre_crossmember", "centre_crossmember"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"rear_crossmember", "rear_crossmember"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"a_pillar_left", "a_pillar_left"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"b_pillar_left", "b_pillar_left"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"c_pillar_left", "c_pillar_left"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"roof_rail_left", "roof_rail_left"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"a_pillar_right", "a_pillar_right"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"b_pillar_right", "b_pillar_right"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"c_pillar_right", "c_pillar_right"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"roof_rail_right", "roof_rail_right"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"front_tower_left", "front_tower_left"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"front_tower_right", "front_tower_right"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"rear_tower_left", "rear_tower_left"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"rear_tower_right", "rear_tower_right"},
    }},
    {{
        GeneratedMcsMv2BodyInWhiteJointRecord{"front_rail_left", "front_crossmember", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"front_rail_right", "front_crossmember", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"rear_rail_left", "rear_crossmember", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"rear_rail_right", "rear_crossmember", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"rocker_left", "floor_pan", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"rocker_right", "floor_pan", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"front_crossmember", "floor_pan", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"centre_crossmember", "floor_pan", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"rear_crossmember", "floor_pan", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"a_pillar_left", "rocker_left", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"b_pillar_left", "rocker_left", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"c_pillar_left", "rocker_left", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"a_pillar_left", "roof_rail_left", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"b_pillar_left", "roof_rail_left", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"c_pillar_left", "roof_rail_left", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"front_tower_left", "front_rail_left", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"rear_tower_left", "rear_rail_left", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"a_pillar_right", "rocker_right", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"b_pillar_right", "rocker_right", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"c_pillar_right", "rocker_right", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"a_pillar_right", "roof_rail_right", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"b_pillar_right", "roof_rail_right", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"c_pillar_right", "roof_rail_right", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"front_tower_right", "front_rail_right", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"rear_tower_right", "rear_rail_right", "spot_weld_and_adhesive"},
    }},
    {{"occupant_front_left_head", "occupant_front_left_torso", "occupant_front_right_head", "occupant_front_right_torso", "occupant_rear_left_head", "occupant_rear_left_torso", "occupant_rear_right_head", "occupant_rear_right_torso"}},
    {{"engine_envelope", "rear_differential_envelope", ""}},
    2
},
GeneratedMcsMv2VariantRecord{
    "crossover", "Urban Crossover EV AWD", "C-HR-sized battery-electric variation with a long wheelbase, tall cabin, raised ride height and underfloor battery.", "2026 Toyota C-HR BEV dimensions, original surface construction",
    GeneratedMcsMv2ReferenceFrameRecord{"MCSMv2.0.1.VehicleReferenceFrame.v1", "MCSMv2VehicleFrame", {{0, 0, 0}}, {{1, 0, 0}}, {{0, 1, 0}}, {{0, 0, 1}}, "right", "m", "deg", {{{{0, 1, 0, 0}}, {{0, 0, 1, 0}}, {{1, 0, 0, 0}}, {{0, 0, 0, 1}}}}},
    GeneratedMcsMv2FieldCalibrationRecord{"MCSMv2.0.1.FieldCalibration.v1", "iterative inverse affine field prewarp", 3, {{1.0000306603, 1.0109525288000001, 1.0091102185}}, {{-5.8944599999999998e-05, -1.63e-08, -0.0097892683000000008}}, 0.01095252882904707, false, 0, {{-2.2946868, -0.93472, 0.14498242}}, {{2.2239732000000001, 0.93472, 1.62052}}, {{-2.29468614, -0.93490213, 0.14621753000000001}}, {{2.2239731300000001, 0.93490209000000002, 1.62081422}}, 0.0025000000000000001},
    GeneratedMcsMv2SourceAssuranceRecord{"MCSMv2.0.1.Assurance.v1", "V2", "Package and semantic geometry consistency", true, {{GeneratedMcsMv2AssuranceLevelRecord{"V0", true, false, "finite, framed, acyclic parameter model", ""}, GeneratedMcsMv2AssuranceLevelRecord{"V1", true, false, "watertight, wound, non-self-intersecting final body mesh", ""}, GeneratedMcsMv2AssuranceLevelRecord{"V2", true, false, "package, continuous sections, curve network, datums and BIW graph consistent", ""}, GeneratedMcsMv2AssuranceLevelRecord{"V3", false, true, "independent closure, glass and suspension kinematics", "Static occupant and sampled tyre checks exist; full closure/glass/suspension kinematics remain deferred."}, GeneratedMcsMv2AssuranceLevelRecord{"V4", false, false, "manufacturing and structural analysis", ""}, GeneratedMcsMv2AssuranceLevelRecord{"V5", false, false, "CFD, crash, thermal, ergonomic and physical validation", ""}}}, {{"panel patches remain semantic mesh masks rather than exclusive surface-domain partitions", "BIW validation is graph connectivity rather than geometric joint/load-path analysis", "semantic-surface to implicit-field agreement is diagnostic, not yet constrained", "inverse fitting remains a synthetic semantic metric demonstration"}}},
    GeneratedMcsMv2PackageRecord{4.5186599999999997, 1.8694399999999998, 1.6205199999999997, 2.7508199999999996, 1.6000000000000001, 1.6000000000000001, 0.185, 0.84856320000000007, 0.91927680000000012},
    GeneratedMcsMv2PlatformRecord{0.315, 0.16504919999999998, -0.79773779999999983, 0.66500000000000004, 1.2850000000000001, 0.085000000000000006, 0.2617216},
    GeneratedMcsMv2StyleRecord{1.1200000000000001, 1.02, 1.0800000000000001, 1, 0.93000000000000005, 0.47999999999999998, 0.56000000000000005, 0.26000000000000001, 0.059999999999999998, 0.059999999999999998, 0.065000000000000002, 0.070000000000000007, 0.035000000000000003, 0.02, 0.90000000000000002, 0.71999999999999997, 0.45000000000000001},
    GeneratedMcsMv2WheelRecord{0.36499999999999999, 0.245, 1.6000000000000001, 1.6000000000000001, -32, 32, -0.085000000000000006, 0.074999999999999997, 0.028000000000000001},
    GeneratedMcsMv2PowertrainRecord{"BEV", "AWD", true, 0.14499999999999999, 0.47999999999999998, 1.1799999999999999, 0.34000000000000002, 0},
    GeneratedMcsMv2ClosureRecord{68, 68, 62, 72, 0.55000000000000004, 0.0040000000000000001},
    GeneratedMcsMv2ColorRecord{176, 35, 18}, true,
    {{
        GeneratedMcsMv2ParameterDependencyRecord{"battery_pack", "true", "bool", "bool", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"ArchitectureReference", "2026 Toyota C-HR BEV dimensions, original surface construction", 0.71999999999999997, false, 0, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"battery_thickness", "0.145", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"ArchitectureReference", "2026 Toyota C-HR BEV dimensions, original surface construction", 0.71999999999999997, false, 0, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"belt_rise", "0.06", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"bonnet_max_deg", "62.0", "deg", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MVPv2.5 closure design ranges", 0.55000000000000004, true, 2, "Angle uncertainty is expressed in degrees."}},
        GeneratedMcsMv2ParameterDependencyRecord{"crossover_cladding", "true", "bool", "bool", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"door_scallop", "0.02", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"drive_layout", "\"AWD\"", "enum", "enum", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"ArchitectureReference", "2026 Toyota C-HR BEV dimensions, original surface construction", 0.71999999999999997, false, 0, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"engine_envelope_height", "0.34", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"ArchitectureReference", "2026 Toyota C-HR BEV dimensions, original surface construction", 0.71999999999999997, false, 0, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"engine_envelope_length", "0.48", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"ArchitectureReference", "2026 Toyota C-HR BEV dimensions, original surface construction", 0.71999999999999997, false, 0, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"engine_envelope_width", "1.18", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"ArchitectureReference", "2026 Toyota C-HR BEV dimensions, original surface construction", 0.71999999999999997, false, 0, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"exhaust_count", "0", "count", "int", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"ArchitectureReference", "2026 Toyota C-HR BEV dimensions, original surface construction", 0.71999999999999997, false, 0, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"eye_z", "1.2850000000000001", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2 platform derivation", 0.81999999999999995, true, 0.01, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"floor_height", "0.315", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2 platform derivation", 0.81999999999999995, true, 0.01, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"wheelbase", "2.7508199999999996", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PublishedPackage", "MCSMv1 official manufacturer package research", 0.94999999999999996, true, 0.002, "Manufacturer-derived package reference; not proprietary CAD."}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_axle_x", "1.3754099999999998", "m", "float", "wheelbase / 2", {{"wheelbase", "", "", ""}}, 1, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_door_max_deg", "68.0", "deg", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MVPv2.5 closure design ranges", 0.55000000000000004, true, 2, "Angle uncertainty is expressed in degrees."}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_overhang", "0.8485632000000001", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PublishedPackage", "MCSMv1 official manufacturer package research", 0.94999999999999996, true, 0.002, "Manufacturer-derived package reference; not proprietary CAD."}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_extent_x", "2.2239731999999997", "m", "float", "front_axle_x + front_overhang", {{"front_axle_x", "front_overhang", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_fender_amplitude", "0.065", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_hpoint_x", "0.16504919999999998", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2 platform derivation", 0.81999999999999995, true, 0.01, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"width", "1.8694399999999998", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PublishedPackage", "MCSMv1 official manufacturer package research", 0.94999999999999996, true, 0.002, "Manufacturer-derived package reference; not proprietary CAD."}},
        GeneratedMcsMv2ParameterDependencyRecord{"track_front", "1.6", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PublishedPackage", "MCSMv1 official manufacturer package research", 0.94999999999999996, true, 0.002, "Manufacturer-derived package reference; not proprietary CAD."}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_track_body_margin", "0.2694399999999997", "m", "float", "width - track_front", {{"width", "track_front", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"wheel_radius", "0.365", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PackageReference", "MCSMv1 tyre and wheel package research", 0.81999999999999995, true, 0.0060000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"wheel_width", "0.245", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PackageReference", "MCSMv1 tyre and wheel package research", 0.81999999999999995, true, 0.0060000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"steer_min_deg", "-32.0", "deg", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"EngineeringAssumption", "MCSMv2 concept-level envelope policy", 0.57999999999999996, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"steer_max_deg", "32.0", "deg", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"EngineeringAssumption", "MCSMv2 concept-level envelope policy", 0.57999999999999996, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"steer_max_abs_deg", "32.0", "deg", "float", "max(abs(steer_min_deg), abs(steer_max_deg))", {{"steer_min_deg", "steer_max_deg", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"wheelhouse_clearance", "0.028", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"EngineeringAssumption", "MCSMv2 concept-level envelope policy", 0.57999999999999996, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_wheelhouse_rx", "0.45791510986856765", "m", "float", "r + 0.5*w*sin(delta_max) + clearance", {{"wheel_radius", "wheel_width", "steer_max_abs_deg", "wheelhouse_clearance"}}, 4, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_wheelhouse_ry", "0.3439205314451198", "m", "float", "0.5*w + r*sin(delta_max) + clearance", {{"wheel_radius", "wheel_width", "steer_max_abs_deg", "wheelhouse_clearance"}}, 4, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"travel_min", "-0.085", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"EngineeringAssumption", "MCSMv2 concept-level envelope policy", 0.57999999999999996, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"travel_down", "0.085", "m", "float", "abs(travel_min)", {{"travel_min", "", "", ""}}, 1, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"travel_max", "0.075", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"EngineeringAssumption", "MCSMv2 concept-level envelope policy", 0.57999999999999996, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"travel_up", "0.075", "m", "float", "abs(travel_max)", {{"travel_max", "", "", ""}}, 1, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"front_wheelhouse_rz", "0.47800000000000004", "m", "float", "r + max(jounce,rebound) + clearance", {{"wheel_radius", "travel_down", "travel_up", "wheelhouse_clearance"}}, 4, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"greenhouse_front_factor", "1.02", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"greenhouse_rear_factor", "1.08", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"grille_scale", "0.45", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"ground_clearance", "0.185", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PublishedPackage", "MCSMv1 official manufacturer package research", 0.94999999999999996, true, 0.002, "Manufacturer-derived package reference; not proprietary CAD."}},
        GeneratedMcsMv2ParameterDependencyRecord{"hatch_max_deg", "72.0", "deg", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MVPv2.5 closure design ranges", 0.55000000000000004, true, 2, "Angle uncertainty is expressed in degrees."}},
        GeneratedMcsMv2ParameterDependencyRecord{"head_clearance", "0.085", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2 platform derivation", 0.81999999999999995, true, 0.01, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"height", "1.6205199999999997", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PublishedPackage", "MCSMv1 official manufacturer package research", 0.94999999999999996, true, 0.002, "Manufacturer-derived package reference; not proprietary CAD."}},
        GeneratedMcsMv2ParameterDependencyRecord{"hood_wedge", "0.48", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"hpoint_z", "0.665", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2 platform derivation", 0.81999999999999995, true, 0.01, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"length", "4.51866", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PublishedPackage", "MCSMv1 official manufacturer package research", 0.94999999999999996, true, 0.002, "Manufacturer-derived package reference; not proprietary CAD."}},
        GeneratedMcsMv2ParameterDependencyRecord{"minimum_roof_from_occupant", "1.47", "m", "float", "H-point + seated-head envelope + clearance", {{"hpoint_z", "head_clearance", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"nominal_panel_gap", "0.004", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MVPv2.5 closure design ranges", 0.55000000000000004, true, 2, "Angle uncertainty is expressed in degrees."}},
        GeneratedMcsMv2ParameterDependencyRecord{"nose_taper", "1.0", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"occupant_roof_margin", "0.15051999999999977", "m", "float", "height - minimum_roof_from_occupant", {{"height", "minimum_roof_from_occupant", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_axle_x", "-1.3754099999999998", "m", "float", "-wheelbase / 2", {{"wheelbase", "", "", ""}}, 1, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_overhang", "0.9192768000000001", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PublishedPackage", "MCSMv1 official manufacturer package research", 0.94999999999999996, true, 0.002, "Manufacturer-derived package reference; not proprietary CAD."}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_extent_x", "-2.2946868", "m", "float", "rear_axle_x - rear_overhang", {{"rear_axle_x", "rear_overhang", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"resolved_length", "4.51866", "m", "float", "front_extent_x - rear_extent_x", {{"front_extent_x", "rear_extent_x", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"package_length_residual", "0.0", "m", "float", "resolved_length - length", {{"resolved_length", "length", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"powertrain_architecture", "\"BEV\"", "enum", "enum", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"ArchitectureReference", "2026 Toyota C-HR BEV dimensions, original surface construction", 0.71999999999999997, false, 0, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_door_max_deg", "68.0", "deg", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MVPv2.5 closure design ranges", 0.55000000000000004, true, 2, "Angle uncertainty is expressed in degrees."}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_haunch_amplitude", "0.07", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_hpoint_x", "-0.7977377999999998", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2 platform derivation", 0.81999999999999995, true, 0.01, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"track_rear", "1.6", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"PublishedPackage", "MCSMv1 official manufacturer package research", 0.94999999999999996, true, 0.002, "Manufacturer-derived package reference; not proprietary CAD."}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_track_body_margin", "0.2694399999999997", "m", "float", "width - track_rear", {{"width", "track_rear", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_wheelhouse_rx", "0.393", "m", "float", "r + clearance", {{"wheel_radius", "wheelhouse_clearance", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_wheelhouse_ry", "0.1505", "m", "float", "0.5*w + clearance", {{"wheel_width", "wheelhouse_clearance", "", ""}}, 2, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"rear_wheelhouse_rz", "0.47800000000000004", "m", "float", "r + max(jounce,rebound) + clearance", {{"wheel_radius", "travel_down", "travel_up", "wheelhouse_clearance"}}, 4, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2.0.1 dependency graph", 0.88, true, 0.0050000000000000001, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"rocker_tuck", "0.035", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"roof_crown", "0.56", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"roof_scale", "1.12", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"seat_lateral", "0.2617216", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"Derived", "MCSMv2 platform derivation", 0.81999999999999995, true, 0.01, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"shoulder_strength", "0.06", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"side_glass_travel", "0.55", "m", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MVPv2.5 closure design ranges", 0.55000000000000004, true, 2, "Angle uncertainty is expressed in degrees."}},
        GeneratedMcsMv2ParameterDependencyRecord{"splitter_scale", "0.72", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"spoiler_scale", "0.9", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"tail_taper", "0.93", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
        GeneratedMcsMv2ParameterDependencyRecord{"tumblehome", "0.26", "1", "float", "explicit", {{"", "", "", ""}}, 0, GeneratedMcsMv2EvidenceRecord{"DesignChoice", "MCSMv2 style field", 0.59999999999999998, true, 0.014999999999999999, ""}},
    }},
    {{
        GeneratedMcsMv2StationRecord{"tail_face", 0, 0, -2.2946868, 0.14498241672474671, 0.2844928298206571, 0.20071033181518824, 0.37433267081665411, 0.30426019173880958, 0.39623528276915493, 0.46904941704147318, 0.40693031002268404, 0.51861807701972973, 0.37844518832109614, 0.5396534296374732, 0.040693031002268405, 0.55886043424439791, 0.026450470151474465, 0.60187730027459474, 0.68000000000000005},
        GeneratedMcsMv2StationRecord{"rear_bumper", 1, 0.059999999999999998, -2.0235672, 0.14498241672474671, 0.48907176545982806, 0.20071033181518824, 0.64351548086819477, 0.36671189694288653, 0.68238824181746549, 0.61389095630700974, 0.70147561750034748, 0.67046281335966851, 0.65237232427532321, 0.69149816597741187, 0.070147561750034754, 0.76984592479871172, 0.045595915137522591, 0.81286279082890855, 0.68000000000000005},
        GeneratedMcsMv2StationRecord{"rear_axle", 2, 0.20344013490725132, -1.3754099999999998, 0.14498241672474671, 0.64199223633047153, 0.20071033181518824, 0.84472662675062038, 0.47614340071701183, 0.90458571887683992, 0.82069333003927858, 0.93471999999999988, 1.0134618266106141, 0.8692896, 1.0344971792283575, 0.61130687999999989, 1.419838040807865, 0.45424834991269086, 1.4628549068380619, 0.80000000000000004},
        GeneratedMcsMv2StationRecord{"rear_door", 3, 0.33000000000000002, -0.80352900000000016, 0.14498241672474671, 0.58768731506846739, 0.20071033181518824, 0.77327278298482549, 0.49950499034873441, 0.83331260613691216, 0.86723003777323793, 0.86391352729623849, 0.99187695517000718, 0.80343958038550178, 1.0129123077877507, 0.56499944685173997, 1.5544333207441363, 0.54081110178726632, 1.5974501867743329, 0.68000000000000005},
        GeneratedMcsMv2StationRecord{"b_pillar", 4, 0.45500000000000002, -0.23869650000000009, 0.14498241672474671, 0.56011678017147815, 0.20071033181518824, 0.73699576338352379, 0.50581089181540073, 0.79580052807460167, 0.87975425874175583, 0.82587537075505135, 0.95864446614430665, 0.76806409480219784, 0.97967981876205013, 0.54012249247380362, 1.5775031339698029, 0.56081480128858541, 1.6205199999999997, 0.80000000000000004},
        GeneratedMcsMv2StationRecord{"front_door", 5, 0.58499999999999996, 0.34872929999999958, 0.14498241672474671, 0.56377522841015326, 0.20071033181518824, 0.74180951106599125, 0.50421399166790426, 0.79969163140336008, 0.87658263761547806, 0.82921146813292612, 0.91034872984368165, 0.77116666536362133, 0.93138408246142501, 0.54230430015893372, 1.457762954812609, 0.50651151112651172, 1.5007798208428058, 0.68000000000000005},
        GeneratedMcsMv2StationRecord{"a_pillar", 6, 0.68999999999999995, 0.82318859999999949, 0.14498241672474671, 0.59646202258917158, 0.20071033181518824, 0.78481845077522572, 0.49815110243339439, 0.83840196600710715, 0.86454106594138225, 0.86523155152085296, 0.86454106594138225, 0.80466534291439318, 0.88535772810696212, 0.23358881280216376, 1.1726049059140371, 0.12542938995259037, 1.2156217719442339, 0.68000000000000005},
        GeneratedMcsMv2StationRecord{"hood_rear", 7, 0.77000000000000002, 1.1846813999999997, 0.14498241672474671, 0.64863213735992786, 0.20071033181518824, 0.85346333863148394, 0.49097511562444773, 0.90059074503377212, 0.8502887588069461, 0.92335926563318782, 0.88036127316707424, 0.85872411703886475, 0.90139662578481761, 0.092335926563318779, 1.1072349543000488, 0.060018352266157211, 1.1502518203302454, 0.68000000000000005},
        GeneratedMcsMv2StationRecord{"front_axle", 8, 0.8122091062394603, 1.3754099999999996, 0.14498241672474671, 0.6609254782754731, 0.20071033181518824, 0.86963878720456989, 0.4849387220841947, 0.91211010084996558, 0.83829981052561031, 0.93211900600614173, 0.8651973756020841, 0.86687067558571185, 0.88623272821982746, 0.09321190060061417, 1.0943247902080375, 0.060587735390399217, 1.1373416562382344, 0.80000000000000004},
        GeneratedMcsMv2StationRecord{"nose", 9, 0.94499999999999995, 1.9754468999999997, 0.14498241672474671, 0.54874830506635763, 0.20071033181518824, 0.72203724350836529, 0.42743065708720185, 0.7492590825902502, 0.72327278894022928, 0.7612479569103201, 0.73526458003386352, 0.70796059992659766, 0.75629993265160689, 0.076124795691032004, 0.93426796202499773, 0.04948111719917081, 0.97728482805519457, 0.68000000000000005},
        GeneratedMcsMv2StationRecord{"front_face", 10, 1, 2.2239731999999997, 0.15823911791573547, 0.3159974359534819, 0.20071033181518824, 0.41578609993879195, 0.3861225620738834, 0.43106054483820549, 0.61083166669397215, 0.43773359205043416, 0.61083166669397215, 0.4070922406069038, 0.63168807848265762, 0.043773359205043418, 0.76285177102836121, 0.028452683483278222, 0.80586863705855805, 0.68000000000000005},
    }},
    {{
        GeneratedMcsMv2SectionSampleRecord{-2.2946868, {{0.14498241672474671, 0.2844928298206571, 0.20071033181518824, 0.37433267081665411, 0.30426019173880958, 0.39623528276915493, 0.46904941704147318, 0.40693031002268404, 0.51861807701972973, 0.37844518832109614, 0.5396534296374732, 0.040693031002268405, 0.55886043424439791, 0.026450470151474465, 0.60187730027459474}}},
        GeneratedMcsMv2SectionSampleRecord{-1.5415768000000001, {{0.14498241672474671, 0.62925913443111892, 0.20071033181518824, 0.8279725453041038, 0.45789456181099186, 0.88588472645076721, 0.78719378089090231, 0.91498014106004599, 0.9485791766063324, 0.85093153118584275, 0.96961452922407576, 0.52835192563236699, 1.2738658925019333, 0.36892246160802789, 1.3168827585321301}}},
        GeneratedMcsMv2SectionSampleRecord{-0.78846680000000013, {{0.14498241672474671, 0.58671006119454228, 0.20071033181518824, 0.7719869226243975, 0.49976817059132306, 0.83199830959647492, 0.8677530456445095, 0.86258973016464413, 0.99094249435593695, 0.80220844905311905, 1.0119778469736804, 0.56413368352767723, 1.5557041856559863, 0.54167104873803584, 1.5987210516861829}}},
        GeneratedMcsMv2SectionSampleRecord{-0.035356800000000188, {{0.14498241672474671, 0.56058664441466777, 0.20071033181518824, 0.73761400580877323, 0.50558049532237304, 0.79629503118166345, 0.87929666912110416, 0.82629617941247147, 0.94425036077531466, 0.76845544685359857, 0.96528571339305813, 0.54072579159474876, 1.5526184713706401, 0.55365075729908031, 1.595635337400837}}},
        GeneratedMcsMv2SectionSampleRecord{0.71775319999999976, {{0.14498241672474671, 0.58650655504291527, 0.20071033181518824, 0.771719151372257, 0.49983186949959413, 0.82657206301369068, 0.86787968092758205, 0.85419795123395814, 0.86788015438874244, 0.79440409464758088, 0.88872428949941595, 0.3033455327471441, 1.2182477139901964, 0.18872971465868588, 1.2612645800203932}}},
        GeneratedMcsMv2SectionSampleRecord{1.4708631999999997, {{0.14498241672474671, 0.6576960998599698, 0.20071033181518824, 0.86538960507890772, 0.47960485955634802, 0.90734080195280065, 0.82808209729774762, 0.9270690084869625, 0.85321310643515313, 0.8621741778928752, 0.8742484590528965, 0.092706900848696247, 1.0813893534959897, 0.060259485551652565, 1.1244062195261866}}},
        GeneratedMcsMv2SectionSampleRecord{2.2239731999999997, {{0.15823911791573547, 0.31599743595348184, 0.20071033181518824, 0.4157860999387919, 0.38612256207388346, 0.43106054483820555, 0.61083166669397215, 0.43773359205043416, 0.6108317666939721, 0.4070922406069038, 0.63168817848265757, 0.043773359205043424, 0.76285187102836116, 0.028452683483278219, 0.80586873705855799}}},
    }},
    {{
        GeneratedMcsMv2FieldSampleRecord{{{-0.035356800000000188, 0, 0.89128599999999991}}, -0.69354520623604632, -0.69354520623604632},
        GeneratedMcsMv2FieldSampleRecord{{{-0.035356800000000188, 0.82629617941247147, 0.87929666912110416}}, 2.5198519698491161e-09, 2.5198519698491161e-09},
        GeneratedMcsMv2FieldSampleRecord{{{-0.035356800000000188, 1.1590527999999998, 0.87929666912110416}}, 0.33275662299587511, 0.33275662299587511},
        GeneratedMcsMv2FieldSampleRecord{{{2.2439731999999997, 0, 0.6482079999999999}}, 0.02676302366709617, 0.02676302366709617},
        GeneratedMcsMv2FieldSampleRecord{{{-0.035356800000000188, 0, 1.6405199999999998}}, 0.053828421203824653, 0.053828421203824653},
        GeneratedMcsMv2FieldSampleRecord{{{1.3754099999999998, -0.80000000000000004, 0.36499999999999999}}, -0.093149577880839649, 0.3439205314451198},
    }},
    {{
        GeneratedMcsMv2CurveSampleRecord{"centre_spine", {{{{-2.2946868, 0, 0.3734298584996707}}, {{-1.5812141684210528, 0, 0.70760037199220061}}, {{-0.82017669473684229, 0, 0.870502587650691}}, {{-0.059139221052631807, 0, 0.87223296026943897}}, {{0.70189825263157868, 0, 0.70735694345917033}}, {{1.4629357263157892, 0, 0.63533006876461129}}, {{2.2239731999999997, 0, 0.48205392748714671}}}}},
        GeneratedMcsMv2CurveSampleRecord{"roof_centre", {{{{-2.2946868, 0, 0.60187730027459474}}, {{-1.5812141684210528, 0, 1.2702183272596546}}, {{-0.82017669473684229, 0, 1.5960227585766353}}, {{-0.059139221052631807, 0, 1.5994835038141313}}, {{0.70189825263157868, 0, 1.269731470193594}}, {{1.4629357263157892, 0, 1.1256777208044759}}, {{2.2239731999999997, 0, 0.80586873705855799}}}}},
        GeneratedMcsMv2CurveSampleRecord{"roof_rail_left", {{{{-2.2946868, -0.026450470151474465, 0.55886043424439791}}, {{-1.5812141684210528, -0.33881179503585146, 1.2272014612294577}}, {{-0.82017669473684229, -0.5398091175269002, 1.5530058925464387}}, {{-0.059139221052631807, -0.55519682157368133, 1.5564666377839345}}, {{0.70189825263157868, -0.20277084684455973, 1.2267146041633972}}, {{1.4629357263157892, -0.060311121055106928, 1.0826608547742791}}, {{2.2239731999999997, -0.028452683483278219, 0.76285187102836116}}}}},
        GeneratedMcsMv2CurveSampleRecord{"roof_rail_right", {{{{-2.2946868, 0.026450470151474465, 0.55886043424439791}}, {{-1.5812141684210528, 0.33881179503585146, 1.2272014612294577}}, {{-0.82017669473684229, 0.5398091175269002, 1.5530058925464387}}, {{-0.059139221052631807, 0.55519682157368133, 1.5564666377839345}}, {{0.70189825263157868, 0.20277084684455973, 1.2267146041633972}}, {{1.4629357263157892, 0.060311121055106928, 1.0826608547742791}}, {{2.2239731999999997, 0.028452683483278219, 0.76285187102836116}}}}},
        GeneratedMcsMv2CurveSampleRecord{"glass_shoulder_left", {{{{-2.2946868, -0.040693031002268405, 0.5396534296374732}}, {{-1.5812141684210528, -0.49003352036397058, 0.94574384994452276}}, {{-0.82017669473684229, -0.56600965907568135, 1.0139942314363379}}, {{-0.059139221052631807, -0.54060942806840762, 0.96690921552111819}}, {{0.70189825263157868, -0.31624765472808114, 0.88923321466247063}}, {{1.4629357263157892, -0.092786340084779886, 0.87537053075860372}}, {{2.2239731999999997, -0.043773359205043424, 0.63168817848265757}}}}},
        GeneratedMcsMv2CurveSampleRecord{"glass_shoulder_right", {{{{-2.2946868, 0.040693031002268405, 0.5396534296374732}}, {{-1.5812141684210528, 0.49003352036397058, 0.94574384994452276}}, {{-0.82017669473684229, 0.56600965907568135, 1.0139942314363379}}, {{-0.059139221052631807, 0.54060942806840762, 0.96690921552111819}}, {{0.70189825263157868, 0.31624765472808114, 0.88923321466247063}}, {{1.4629357263157892, 0.092786340084779886, 0.87537053075860372}}, {{2.2239731999999997, 0.043773359205043424, 0.63168817848265757}}}}},
        GeneratedMcsMv2CurveSampleRecord{"belt_left", {{{{-2.2946868, -0.37844518832109614, 0.51861807701972973}}, {{-1.5812141684210528, -0.84164605587162511, 0.9247084973267794}}, {{-0.82017669473684229, -0.80487612070395043, 0.99295887881859446}}, {{-0.059139221052631807, -0.76837020204646245, 0.94587386290337472}}, {{0.70189825263157868, -0.79289899060125557, 0.86838113983748422}}, {{1.4629357263157892, -0.86291296278845298, 0.85433517814086035}}, {{2.2239731999999997, -0.4070922406069038, 0.6108317666939721}}}}},
        GeneratedMcsMv2CurveSampleRecord{"belt_right", {{{{-2.2946868, 0.37844518832109614, 0.51861807701972973}}, {{-1.5812141684210528, 0.84164605587162511, 0.9247084973267794}}, {{-0.82017669473684229, 0.80487612070395043, 0.99295887881859446}}, {{-0.059139221052631807, 0.76837020204646245, 0.94587386290337472}}, {{0.70189825263157868, 0.79289899060125557, 0.86838113983748422}}, {{1.4629357263157892, 0.86291296278845298, 0.85433517814086035}}, {{2.2239731999999997, 0.4070922406069038, 0.6108317666939721}}}}},
        GeneratedMcsMv2CurveSampleRecord{"shoulder_left", {{{{-2.2946868, -0.40693031002268404, 0.46904941704147318}}, {{-1.5812141684210528, -0.90499575900174734, 0.77612290775573545}}, {{-0.82017669473684229, -0.86545819430532311, 0.86662883632612797}}, {{-0.059139221052631807, -0.82620451832952946, 0.87939371451852688}}, {{0.70189825263157868, -0.8525795597862964, 0.86838039720284144}}, {{1.4629357263157892, -0.92786340084779895, 0.82906691795736864}}, {{2.2239731999999997, -0.43773359205043416, 0.61083166669397215}}}}},
        GeneratedMcsMv2CurveSampleRecord{"shoulder_right", {{{{-2.2946868, 0.40693031002268404, 0.46904941704147318}}, {{-1.5812141684210528, 0.90499575900174734, 0.77612290775573545}}, {{-0.82017669473684229, 0.86545819430532311, 0.86662883632612797}}, {{-0.059139221052631807, 0.82620451832952946, 0.87939371451852688}}, {{0.70189825263157868, 0.8525795597862964, 0.86838039720284144}}, {{1.4629357263157892, 0.92786340084779895, 0.82906691795736864}}, {{2.2239731999999997, 0.43773359205043416, 0.61083166669397215}}}}},
        GeneratedMcsMv2CurveSampleRecord{"rocker_left", {{{{-2.2946868, -0.37433267081665411, 0.20071033181518824}}, {{-1.5812141684210528, -0.81947144699275032, 0.20071033181518824}}, {{-0.82017669473684229, -0.77478140229788883, 0.20071033181518824}}, {{-0.059139221052631807, -0.73747984606946082, 0.20071033181518824}}, {{0.70189825263157868, -0.76980037193220319, 0.20071033181518824}}, {{1.4629357263157892, -0.86605930401298281, 0.20071033181518824}}, {{2.2239731999999997, -0.4157860999387919, 0.20071033181518824}}}}},
        GeneratedMcsMv2CurveSampleRecord{"rocker_right", {{{{-2.2946868, 0.37433267081665411, 0.20071033181518824}}, {{-1.5812141684210528, 0.81947144699275032, 0.20071033181518824}}, {{-0.82017669473684229, 0.77478140229788883, 0.20071033181518824}}, {{-0.059139221052631807, 0.73747984606946082, 0.20071033181518824}}, {{0.70189825263157868, 0.76980037193220319, 0.20071033181518824}}, {{1.4629357263157892, 0.86605930401298281, 0.20071033181518824}}, {{2.2239731999999997, 0.4157860999387919, 0.20071033181518824}}}}},
        GeneratedMcsMv2CurveSampleRecord{"underbody_edge_left", {{{{-2.2946868, -0.2844928298206571, 0.14498241672474671}}, {{-1.5812141684210528, -0.6227982997144903, 0.14498241672474671}}, {{-0.82017669473684229, -0.58883386574639551, 0.14498241672474671}}, {{-0.059139221052631807, -0.56048468301279031, 0.14498241672474671}}, {{0.70189825263157868, -0.58504828266847442, 0.14498241672474671}}, {{1.4629357263157892, -0.65820507104986692, 0.14498241672474671}}, {{2.2239731999999997, -0.31599743595348184, 0.15823911791573547}}}}},
        GeneratedMcsMv2CurveSampleRecord{"underbody_edge_right", {{{{-2.2946868, 0.2844928298206571, 0.14498241672474671}}, {{-1.5812141684210528, 0.6227982997144903, 0.14498241672474671}}, {{-0.82017669473684229, 0.58883386574639551, 0.14498241672474671}}, {{-0.059139221052631807, 0.56048468301279031, 0.14498241672474671}}, {{0.70189825263157868, 0.58504828266847442, 0.14498241672474671}}, {{1.4629357263157892, 0.65820507104986692, 0.14498241672474671}}, {{2.2239731999999997, 0.31599743595348184, 0.15823911791573547}}}}},
    }},
    {{
        GeneratedMcsMv2PanelPatchRecord{"hood", 8662, true},
        GeneratedMcsMv2PanelPatchRecord{"roof", 9830, false},
        GeneratedMcsMv2PanelPatchRecord{"front_door_left", 2968, true},
        GeneratedMcsMv2PanelPatchRecord{"front_door_right", 2975, true},
        GeneratedMcsMv2PanelPatchRecord{"rear_door_left", 4144, true},
        GeneratedMcsMv2PanelPatchRecord{"rear_door_right", 4142, true},
        GeneratedMcsMv2PanelPatchRecord{"rear_quarter_left", 4728, false},
        GeneratedMcsMv2PanelPatchRecord{"rear_quarter_right", 4736, false},
        GeneratedMcsMv2PanelPatchRecord{"rear_hatch", 11709, true},
        GeneratedMcsMv2PanelPatchRecord{"front_bumper", 6914, false},
        GeneratedMcsMv2PanelPatchRecord{"rear_bumper", 5427, false},
    }},
    {{
        GeneratedMcsMv2PanelRelationshipRecord{"hood", "front_bumper", "panel_gap"},
        GeneratedMcsMv2PanelRelationshipRecord{"front_door_left", "rear_door_left", "panel_gap"},
        GeneratedMcsMv2PanelRelationshipRecord{"front_door_right", "rear_door_right", "panel_gap"},
        GeneratedMcsMv2PanelRelationshipRecord{"rear_door_left", "rear_quarter_left", "panel_gap"},
        GeneratedMcsMv2PanelRelationshipRecord{"rear_door_right", "rear_quarter_right", "panel_gap"},
        GeneratedMcsMv2PanelRelationshipRecord{"rear_hatch", "rear_bumper", "panel_gap"},
    }},
    {{
        GeneratedMcsMv2BodyInWhiteMemberRecord{"floor_pan", "floor_pan"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"rocker_left", "rocker_left"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"rocker_right", "rocker_right"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"front_rail_left", "front_rail_left"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"rear_rail_left", "rear_rail_left"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"front_rail_right", "front_rail_right"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"rear_rail_right", "rear_rail_right"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"front_crossmember", "front_crossmember"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"centre_crossmember", "centre_crossmember"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"rear_crossmember", "rear_crossmember"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"a_pillar_left", "a_pillar_left"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"b_pillar_left", "b_pillar_left"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"c_pillar_left", "c_pillar_left"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"roof_rail_left", "roof_rail_left"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"a_pillar_right", "a_pillar_right"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"b_pillar_right", "b_pillar_right"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"c_pillar_right", "c_pillar_right"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"roof_rail_right", "roof_rail_right"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"front_tower_left", "front_tower_left"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"front_tower_right", "front_tower_right"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"rear_tower_left", "rear_tower_left"},
        GeneratedMcsMv2BodyInWhiteMemberRecord{"rear_tower_right", "rear_tower_right"},
    }},
    {{
        GeneratedMcsMv2BodyInWhiteJointRecord{"front_rail_left", "front_crossmember", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"front_rail_right", "front_crossmember", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"rear_rail_left", "rear_crossmember", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"rear_rail_right", "rear_crossmember", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"rocker_left", "floor_pan", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"rocker_right", "floor_pan", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"front_crossmember", "floor_pan", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"centre_crossmember", "floor_pan", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"rear_crossmember", "floor_pan", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"a_pillar_left", "rocker_left", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"b_pillar_left", "rocker_left", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"c_pillar_left", "rocker_left", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"a_pillar_left", "roof_rail_left", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"b_pillar_left", "roof_rail_left", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"c_pillar_left", "roof_rail_left", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"front_tower_left", "front_rail_left", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"rear_tower_left", "rear_rail_left", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"a_pillar_right", "rocker_right", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"b_pillar_right", "rocker_right", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"c_pillar_right", "rocker_right", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"a_pillar_right", "roof_rail_right", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"b_pillar_right", "roof_rail_right", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"c_pillar_right", "roof_rail_right", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"front_tower_right", "front_rail_right", "spot_weld_and_adhesive"},
        GeneratedMcsMv2BodyInWhiteJointRecord{"rear_tower_right", "rear_rail_right", "spot_weld_and_adhesive"},
    }},
    {{"occupant_front_left_head", "occupant_front_left_torso", "occupant_front_right_head", "occupant_front_right_torso", "occupant_rear_left_head", "occupant_rear_left_torso", "occupant_rear_right_head", "occupant_rear_right_torso"}},
    {{"battery_envelope", "front_eaxle_envelope", "rear_eaxle_envelope"}},
    3
}
    }};
    return records;
}
