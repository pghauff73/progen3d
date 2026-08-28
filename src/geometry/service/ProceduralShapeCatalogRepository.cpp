#include "geometry/service/ProceduralShapeCatalogRepository.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace {

std::string lowercase_copy(const std::string &value)
{
	std::string lowered = value;
	std::transform(lowered.begin(), lowered.end(), lowered.begin(),
		[](unsigned char character) {
			return static_cast<char>(std::tolower(character));
		});
	return lowered;
}

ProceduralShapeParameterDocumentation parameter(
	const char *name,
	const char *signature,
	const char *default_value,
	const char *description)
{
	return ProceduralShapeParameterDocumentation(
		name, signature, default_value, description);
}

std::vector<ProceduralShapeParameterDocumentation> cylinder_parameters()
{
	return {
		parameter("radial", "radial(inner outer)", "0 1", "Normalized radial interval."),
		parameter("wall", "wall(thickness)", "none", "Normalized shell wall thickness."),
		parameter("axial", "axial(min max)", "0 1", "Normalized local-Y interval."),
		parameter("azimuth", "azimuth(start sweep)", "0 360", "Angular domain in degrees."),
		parameter("chord", "chord(angle offset side)", "none", "Circle-segment half-space in the XZ profile."),
		parameter("clip", "clip(nx ny nz offset side)", "none", "Ordered local half-space clip."),
		parameter("topology", "topology(surface|solid|shell)", "solid", "Material topology."),
		parameter("close", "close(none|all|axial|angular|clip|rim ...)", "all", "Generated boundary surfaces."),
		parameter("segments", "segments(around [axial])", "40 1", "Deterministic tessellation counts."),
		parameter("mapping", "mapping(cylindrical|triplanar)", "triplanar", "Texture-coordinate policy."),
	};
}

std::vector<ProceduralShapeParameterDocumentation> sphere_parameters()
{
	return {
		parameter("radial", "radial(inner outer)", "0 1", "Normalized radial interval."),
		parameter("wall", "wall(thickness)", "none", "Normalized shell wall thickness."),
		parameter("polar", "polar(min max)", "0 180", "Polar-angle domain in degrees."),
		parameter("azimuth", "azimuth(start sweep)", "0 360", "Azimuth domain in degrees."),
		parameter("clip", "clip(nx ny nz offset side)", "none", "Ordered local plane clip."),
		parameter("slab", "slab(nx ny nz min max)", "none", "Two ordered parallel plane clips."),
		parameter("topology", "topology(surface|solid|shell)", "solid", "Material topology."),
		parameter("close", "close(none|all|angular|clip|rim ...)", "all", "Generated boundary surfaces."),
		parameter("segments", "segments(azimuth [polar])", "40 20", "Deterministic tessellation counts."),
		parameter("mapping", "mapping(spherical|triplanar)", "triplanar", "Texture-coordinate policy."),
	};
}

std::vector<ProceduralShapeParameterDocumentation> axial_profile_parameters()
{
	return {
		parameter("axis", "axis(y)", "y", "AxialProfilev1 local construction axis."),
		parameter("profile", "profile(Name polygon(x z ...))", "none", "Named canonical simple polygon."),
		parameter("at", "at(position Profile [center(x z)] [scale(x z)] [rotate(degrees)])", "none", "Initial explicit section state."),
		parameter("hold", "hold(position)", "none", "Extrude the current section unchanged."),
		parameter("linear", "linear(position Profile [center(x z)] [scale(x z)] [rotate(degrees)])", "none", "Loft corresponding vertices to a target section."),
		parameter("step", "step(Profile [center(x z)] [scale(x z)] [rotate(degrees)])", "none", "Replace the section at the current level with a closed horizontal transition."),
		parameter("cap", "cap(none|all|bottom|top)", "all", "Bottom and top closure policy."),
	};
}

std::vector<ProceduralShapeParameterDocumentation> tapered_sweep_parameters()
{
	return {
		parameter(
			"samples", "samples(x y z radius ...)",
			"0 0 0 0.10 0 1 0 0.04",
			"Ordered centreline samples with one positive radius per point."),
		parameter("up", "up(x y z)", "0 0 1", "Initial non-zero sweep-frame up hint."),
		parameter("segments", "segments(around)", "12", "Circumferential tessellation count."),
		parameter("cap", "cap(all|none|front|back)", "all", "Branch end closure policy."),
	};
}

std::vector<ProceduralShapeParameterDocumentation> branch_junction_parameters()
{
	return {
		parameter(
			"core", "core(radius bulgeScale)", "0.12 1.15",
			"Positive junction-core radius and bounded transition bulge scale."),
		parameter(
			"parent", "parent(dx dy dz radius transitionLength)",
			"0 -1 0 0.12 0.22",
			"Single incoming branch arm directed away from the junction centre."),
		parameter(
			"child", "child(dx dy dz radius transitionLength)",
			"0.75 0.66 0 0.08 0.20",
			"Repeatable outgoing branch arm with an explicit radius and transition length."),
		parameter(
			"segments", "segments(around)", "12",
			"Circumferential tessellation shared by the core and transition arms."),
	};
}

std::vector<ProceduralShapeParameterDocumentation> sweep_profile_parameters()
{
	return {
		parameter("profile", "profile(Rect|RoundedRect|Circle|Ellipse|ChamferRect|Polygon arguments...)", "Rect 0.10 0.05", "Two-dimensional cross-section constructor and arguments."),
		parameter("path", "path(x y z ...)", "0 0 0 0 1 0", "Ordered three-dimensional sweep path."),
		parameter("up", "up(x y z)", "0 0 1", "Initial non-zero sweep-frame up hint."),
		parameter("cap", "cap(all|none|front|back)", "all", "Path-end closure policy."),
		parameter("detail", "detail(LOD0|LOD1|LOD2|LOD3|LOD4|LOD5)", "LOD5", "Geometry detail level."),
	};
}

std::vector<ProceduralShapeParameterDocumentation> variable_section_sweep_parameters()
{
	return {
		parameter("path", "path(x y z ...)", "none", "Ordered three-dimensional member centreline."),
		parameter("station", "station(u Profile arguments...)", "none", "Repeatable normalized section station with corresponding profile topology."),
		parameter("stationTransform", "stationTransform(u centerX centerY scaleX scaleY rotation)", "none", "Optional station-local profile offset, scale, and rotation."),
		parameter("nominalSection", "nominalSection(Profile arguments...)", "none", "Optional repeatable drawing-section component, independent of sweep end stations."),
		parameter("up", "up(x y z)", "0 1 0", "Initial rotation-minimizing frame hint."),
		parameter("frame", "frame(rotationMinimizing|referenceAligned)", "rotationMinimizing", "Rotation-minimizing transport or fixed alignment to the dominant reference axis."),
		parameter("cap", "cap(all|none|front|back)", "all", "End closure policy."),
		parameter("detail", "detail(LOD1..LOD5)", "LOD3", "Mesh realization detail level."),
	};
}

std::vector<ProceduralShapeParameterDocumentation> sweep_disk_parameters()
{
	return {
		parameter("path", "path(x y z ...)", "0 0 0 0 1 0", "Ordered three-dimensional centreline."),
		parameter("radius", "radius(value)", "0.05", "Positive disk radius."),
		parameter("up", "up(x y z)", "0 0 1", "Initial non-zero sweep-frame up hint."),
		parameter("segments", "segments(around)", "16", "Circumferential tessellation count."),
		parameter("cap", "cap(all|none|front|back)", "all", "Path-end closure policy."),
		parameter("detail", "detail(LOD0|LOD1|LOD2|LOD3|LOD4|LOD5)", "LOD5", "Geometry detail level."),
	};
}

std::vector<ProceduralShapeParameterDocumentation> revolve_parameters()
{
	return {
		parameter("profile", "profile(Polygon radius height ...)", "Polygon 0 -0.5 0.5 -0.5 0.5 0.5 0 0.5", "Radial/height profile constructor and arguments."),
		parameter("angle", "angle(start sweep)", "0 360", "Start and positive sweep angles in degrees."),
		parameter("segments", "segments(around)", "32", "Angular tessellation count."),
		parameter("cap", "cap(all|none|front|back)", "all", "Partial-revolution closure policy."),
		parameter("detail", "detail(LOD0|LOD1|LOD2|LOD3|LOD4|LOD5)", "LOD5", "Geometry detail level."),
	};
}

std::vector<ProceduralShapeParameterDocumentation> loft_parameters()
{
	return {
		parameter("section", "section(position Rect|RoundedRect|Circle|Ellipse|ChamferRect|Polygon arguments...)", "none", "Repeatable ordered profile section."),
		parameter("cap", "cap(all|none|front|back)", "all", "First/last section closure policy."),
		parameter("detail", "detail(LOD0|LOD1|LOD2|LOD3|LOD4|LOD5)", "LOD5", "Geometry detail level."),
	};
}

std::vector<ProceduralShapeParameterDocumentation> shell_loft_parameters()
{
	return {
		parameter("section", "section(position outerPointCount outer-x-y... inner-x-y...)", "none", "Repeatable ordered outer/inner loop section."),
		parameter("cap", "cap(all|none|front|back)", "all", "First/last annular closure policy."),
		parameter("detail", "detail(LOD0|LOD1|LOD2|LOD3|LOD4|LOD5)", "LOD5", "Geometry detail level."),
	};
}

std::vector<ProceduralShapeParameterDocumentation> surface_loft_parameters()
{
	return {
		parameter("section", "section(position Rect|RoundedRect|Circle|Ellipse|ChamferRect|Polygon arguments...)", "none", "Repeatable ordered profile section for an intentionally open loft surface."),
		parameter("detail", "detail(LOD0|LOD1|LOD2|LOD3|LOD4|LOD5)", "LOD5", "Geometry detail level."),
	};
}

std::vector<ProceduralShapeParameterDocumentation> shell_offset_parameters()
{
	return {
		parameter("source", "source(ShapeDescriptor)", "none", "One nested surface or mesh-producing shape descriptor."),
		parameter("thickness", "thickness(value)", "0.01", "Positive offset distance."),
		parameter("side", "side(outward|inward|both)", "both", "Requested offset skin or paired finite-thickness shell."),
		parameter("detail", "detail(LOD0|LOD1|LOD2|LOD3|LOD4|LOD5)", "LOD5", "Geometry detail level."),
	};
}

std::vector<ProceduralShapeParameterDocumentation> mirror_shape_parameters()
{
	return {
		parameter("source", "source(ShapeDescriptor)", "none", "One nested shape descriptor to reflect."),
		parameter("plane", "plane(x|y|z offset)", "x 0", "Axis-aligned reflection plane in source-local coordinates."),
		parameter("mode", "mode(mirrored|both)", "both", "Return only the reflected source or preserve source and reflection."),
		parameter("detail", "detail(LOD0|LOD1|LOD2|LOD3|LOD4|LOD5)", "LOD5", "Geometry detail level."),
	};
}

std::vector<ProceduralShapeParameterDocumentation> curved_panel_parameters()
{
	return {
		parameter("width", "width(value)", "1", "Positive local-X panel width."),
		parameter("height", "height(value)", "1", "Positive local-Y panel height."),
		parameter("curvature", "curvature(horizontal [vertical])", "0 0", "Horizontal and optional vertical camber displacement."),
		parameter("thickness", "thickness(value)", "0", "Finite panel thickness; zero requests an open surface."),
		parameter("segments", "segments(horizontal [vertical])", "16 8", "Deterministic panel tessellation."),
		parameter("detail", "detail(LOD0|LOD1|LOD2|LOD3|LOD4|LOD5)", "LOD5", "Geometry detail level."),
	};
}

std::vector<ProceduralShapeParameterDocumentation> formed_panel_parameters()
{
	return {
		parameter("section", "section(position Profile arguments...)", "none", "Repeatable transverse formed-surface section."),
		parameter("thickness", "thickness(value)", "0.001", "Positive sheet thickness applied by shell offset."),
		parameter("side", "side(outward|inward|both)", "both", "Thickness offset direction."),
		parameter("detail", "detail(LOD1..LOD5)", "LOD3", "Mesh realization detail level."),
	};
}

std::vector<ProceduralShapeParameterDocumentation> hosted_opening_parameters()
{
	return {
		parameter("host", "host(Profile arguments...)", "none", "Planar host boundary."),
		parameter("opening", "opening(Profile arguments...)", "none", "Repeatable opening boundary validated inside the host."),
		parameter("depth", "depth(value)", "0.001", "Positive host extrusion depth."),
		parameter("cap", "cap(all|none|front|back)", "all", "Host face closure policy."),
		parameter("detail", "detail(LOD1..LOD5)", "LOD3", "Mesh realization detail level."),
	};
}

std::vector<ProceduralShapeParameterDocumentation> embossed_bead_parameters()
{
	return {
		parameter("path", "path(x y z ...)", "none", "Explicit host-surface bead path."),
		parameter("up", "up(x y z)", "0 1 0", "Sweep frame hint."),
		parameter("width", "width(value)", "0.03", "Bead cross-section width."),
		parameter("depth", "depth(value)", "0.006", "Bead formed depth."),
		parameter("shoulder", "shoulder(radius)", "0.002", "Rounded shoulder radius."),
		parameter("side", "side(positive|negative)", "positive", "Host-surface side."),
		parameter("end", "end(closed|open)", "closed", "Bead end treatment."),
		parameter("detail", "detail(LOD1..LOD5)", "LOD3", "Mesh realization detail level."),
	};
}

std::vector<ProceduralShapeParameterDocumentation> edge_flange_parameters()
{
	return {
		parameter("path", "path(x y z ...)", "none", "Explicit host boundary path."),
		parameter("up", "up(x y z)", "0 1 0", "Sweep frame hint."),
		parameter("width", "width(value)", "0.02", "Flange projection width."),
		parameter("thickness", "thickness(value)", "0.001", "Sheet thickness."),
		parameter("angle", "angle(degrees)", "90", "Flange angle from the host boundary plane."),
		parameter("bend", "bend(radius)", "0.002", "Documented bend radius."),
		parameter("side", "side(positive|negative)", "positive", "Flange projection side."),
		parameter("detail", "detail(LOD1..LOD5)", "LOD3", "Mesh realization detail level."),
	};
}

std::vector<ProceduralShapeParameterDocumentation> compound_shape_parameters()
{
	return {
		parameter("part", "part(Purpose source(ShapeDescriptor) [transform(m00 ... m33)])", "none", "Repeatable named child shape with an optional column-major local transform."),
		parameter("detail", "detail(LOD1..LOD5)", "LOD3", "Compound realization detail level."),
	};
}

std::vector<ProceduralShapeParameterDocumentation> panel_cut_parameters()
{
	return {
		parameter("path", "path(x y z ...)", "0 0 0 0 1 0", "Ordered panel seam or reveal centreline."),
		parameter("gap", "gap(width [depth])", "0.01 0.005", "Positive visible seam width and optional depth."),
		parameter("up", "up(x y z)", "0 0 1", "Initial non-zero sweep-frame up hint."),
		parameter("cap", "cap(all|none|front|back)", "all", "Path-end closure policy."),
		parameter("detail", "detail(LOD0|LOD1|LOD2|LOD3|LOD4|LOD5)", "LOD5", "Geometry detail level."),
	};
}

std::vector<ProceduralShapeParameterDocumentation> folded_profile_parameters()
{
	return {
		parameter("path", "path(x y ...)", "0 0 0.5 0 0.5 0.5", "Ordered two-dimensional fold path."),
		parameter("thickness", "thickness(value)", "0.01", "Positive sheet thickness."),
		parameter("depth", "depth(value)", "1", "Positive extrusion depth."),
		parameter("cap", "cap(all|none|front|back)", "all", "Fold-path end closure policy."),
		parameter("detail", "detail(LOD0|LOD1|LOD2|LOD3|LOD4|LOD5)", "LOD5", "Geometry detail level."),
	};
}

std::vector<ProceduralShapeParameterDocumentation> instance_array_parameters()
{
	return {
		parameter("source", "source(ShapeDescriptor)", "none", "One nested canonical or alias shape descriptor."),
		parameter("linear", "linear(count spacingX spacingY spacingZ)", "none", "Linear transform pattern."),
		parameter("grid", "grid(xCount yCount spacingX spacingY)", "none", "Local-XY grid transform pattern."),
		parameter("radial", "radial(count radius axisX axisY axisZ startDegrees)", "none", "Radial transform pattern around an arbitrary axis."),
		parameter("detail", "detail(LOD0|LOD1|LOD2|LOD3|LOD4|LOD5)", "LOD5", "Geometry detail level."),
	};
}

std::vector<ProceduralShapeParameterDocumentation> vehicle_body_shell_parameters()
{
	return {
		parameter("thickness", "thickness(value)", "0.04", "Positive body-shell wall thickness."),
		parameter("section", "section(stationZ halfWidth bottomY shoulderY beltY roofY greenhouseHalfWidth)", "none", "Repeatable semantic vehicle transverse section lowered to a symmetric shell-loft profile."),
		parameter("cap", "cap(all|none|front|back)", "all", "First and last body-section closure policy."),
		parameter("detail", "detail(LOD0|LOD1|LOD2|LOD3|LOD4|LOD5)", "LOD3", "Geometry detail level."),
	};
}

std::vector<ProceduralShapeParameterDocumentation> generated_mesh_reference_parameters()
{
	return {
		parameter("meshKey", "meshKey(identifier)", "none", "Immutable provider-owned generated mesh identifier."),
		parameter("detail", "detail(LOD0|LOD1|LOD2|LOD3|LOD4|LOD5)", "LOD2", "Provider-specific bounded generation detail level."),
	};
}

std::vector<ProceduralShapeParameterDocumentation> botanical_blade_parameters(
	const char *default_profile)
{
	return {
		parameter("profile", "profile(Linear|Lanceolate|Elliptic|Ovate|Obovate|Cordate|Palmate|Needle)", default_profile, "Normalized blade width profile."),
		parameter("length", "length(value)", "0.10", "Local-Y blade length."),
		parameter("width", "width(value)", "0.05", "Maximum lateral blade width."),
		parameter("curvature", "curvature(value)", "0", "Longitudinal arch displacement."),
		parameter("camber", "camber(value)", "0", "Transverse blade camber."),
		parameter("twist", "twist(degrees)", "0", "Tip-relative twist in degrees."),
		parameter("thickness", "thickness(value)", "0.001", "Finite thickness; zero requests an open surface."),
		parameter("widthPower", "widthPower(value)", "1", "Exponent applied to the normalized width profile."),
		parameter("segments", "segments(longitudinal lateral)", "12 4", "Deterministic blade tessellation."),
	};
}

std::vector<ProceduralShapeParameterDocumentation> plant_parameters()
{
	return {
		parameter("species", "species(DeciduousTree|Shrub|FloweringHerb|GrassClump|IvyVine)", "DeciduousTree", "Species data record used to generate the authoritative BranchGraph."),
		parameter("architecture", "architecture(Tree|Shrub|Herb|Grass|Vine)", "Tree", "Botanical architecture override."),
		parameter("age", "age(value)", "12", "Finite non-negative developmental age."),
		parameter("state", "state(Seed|Bud|Shoot|Juvenile|Mature|Flowering|Fruiting|Senescent|Dormant|Dead)", "Mature", "Developmental state assigned to generated nodes and organs."),
		parameter("seed", "seed(integer)", "42", "Explicit deterministic hierarchical variation seed."),
		parameter("generation", "generation(RuleBranching|SpaceColonization|LSystem)", "RuleBranching", "Authoritative topology-generation method lowered to BranchGraph."),
		parameter("lAxiom", "lAxiom(F|Plus|Minus|Push|Pop ...)", "F", "Initial L-system sentence using bounded botanical turtle symbols."),
		parameter("lRule", "lRule(predecessor successor...)", "F F Push Plus F Pop F Push Minus F Pop F", "Repeatable deterministic production rule lowered through LSystemBranchGraphGenerationService."),
		parameter("lSystem", "lSystem(iterations angle step baseRadius terminalRadius gamma)", "3 24 0.18 0.055 0.006 2", "Bounded expansion, turtle geometry, taper seed, and branch-radius conservation parameters."),
		parameter("trunk", "trunk(length baseRadius tipRadius internodes)", "3.2 0.24 0.025 7", "Primary-axis and internode contract."),
		parameter("branching", "branching(maxOrder lateral everyN angle azimuth lengthFraction falloff radiusRatio gamma directionVariation lengthVariation tropism)", "2 2 2 43 137.50776 0.48 0.65 0.84 2 5 0.08 0.10", "Rule branching and radius-conservation parameters."),
		parameter("leaf", "leaf(profile length width camber twist thickness minimumOrder)", "Ovate 0.12 0.055 0.004 7 0.0015 1", "Canonical shared leaf-blade source specification."),
		parameter("petiole", "petiole(length baseRadius tipRadius)", "0.025 0.003 0.0015", "TaperedSweep specialization connecting each leaf blade to its host node."),
			parameter("leafArray", "leafArray(nodes rachisLength baseRadius tipRadius pattern divergence upBias scale)", "4 0.08 0.0018 0.0008 Opposite 180 0.12 0.75", "CompoundLeaf rachis and alternate/opposite leaflet array using one shared LeafBlade source."),
			parameter("phyllotaxis", "phyllotaxis(mode divergence spacing [organs phase radialOffset upBias])", "Alternate 137.50776 0.08 1 0 0 0.25", "Organ placement and whorl arrangement."),
			parameter("organArray", "organArray(organ host count spacing azimuth initialScale scaleFalloff orientation jitter minimumOrder)", "Fruit Branch 4 0.16 137.50776 0.04 0.92 Outward 0.08 1", "Deterministic shared-organ placement for Bud, Leaf, Petal, Flower, Fruit, or Thorn on Stem, Branch, FlowerHead, or Surface hosts."),
			parameter("flower", "flower(profile length width curvature camber twist thickness widthPower)", "Obovate 0.09 0.045 0.03 0.012 -4 0.001 0.90", "Shared petal-blade geometry enabled for flowering states."),
			parameter("flowerHead", "flowerHead(florets radius divergence phase tilt scaleFalloff)", "3 0.045 137.50776 0 8 0.985", "Golden-angle floret packing using r = c*sqrt(n) around each active growth tip; larger counts remain bounded by vegetation mesh limits."),
			parameter("inflorescence", "inflorescence(Raceme|Spike|Panicle|Umbel|Corymb|Head flowers spacing radialExtent phase tilt scaleFalloff)", "Panicle 2 0.035 0.018 0 18 0.96", "Higher-level flower topology that places shared flower whorls without introducing a new geometry primitive."),
		parameter("fruit", "fruit(height radius shoulder fullness radialSegments profileSegments)", "0.10 0.055 0.45 4 20 10", "Closed FruitShell profile revolved into one canonical instanced fruit source."),
		parameter("whorl", "whorl(organs radius phase tilt)", "5 0.018 0 28", "Radial petal-whorl attachment arrangement."),
		parameter("growth", "growth(start duration lengthInitial lengthCurve radiusInitial radiusCurve organInitial organCurve)", "0 1 0 Linear 0 Linear 0 Linear", "Deterministic time-varying length, radius, and organ growth channels."),
		parameter("tropism", "tropism(type dx dy dz weight)", "Up 0 1 0 0.10", "Repeatable directional growth influence."),
		parameter("crown", "crown(Sphere|Ellipsoid|Cone|InverseCone|Cylinder|Dome|Lobed arguments...)", "Ellipsoid 0 1.6 0 1.35 1.45 1.20", "Analytic crown volume for space-colonization attraction sampling."),
		parameter("crownCustom", "crownCustom(sampleRadius)", "none", "Declares a custom sampled crown volume and its attraction-point radius."),
		parameter("crownSample", "crownSample(x y z)", "none", "Repeatable attraction point for a custom sampled crown."),
		parameter("spaceColonization", "spaceColonization(influence kill step iterations points radiusDecay minimumRadius gamma)", "0.72 0.18 0.17 32 72 0.93 0.010 2", "Bounded space-colonization topology parameters."),
		parameter("crownObstacle", "crownObstacle(id minx miny minz maxx maxy maxz clearance)", "none", "Repeatable obstacle boundary excluded from crown growth."),
		parameter("floweringAge", "floweringAge(value)", "8", "Species flowering threshold."),
		parameter("matureAge", "matureAge(value)", "12", "Species mature growth threshold."),
		parameter("detail", "detail(LOD0|LOD1|LOD2|LOD3|LOD4|LOD5)", "LOD3", "Vegetation geometry assembly detail level."),
	};
}

std::vector<ProceduralShapeParameterDocumentation> vine_parameters()
{
	return {
		parameter("species", "species(IvyVine)", "IvyVine", "Vine species data record supplying leaf and phyllotaxis geometry."),
		parameter("start", "start(x y z)", "0 0 0", "Local seed position for the authoritative VinePath."),
		parameter("direction", "direction(x y z)", "0 1 0", "Initial active growth-tip direction."),
		parameter("radius", "radius(value)", "0.032", "Initial stem radius before deterministic decay and conservation."),
		parameter("mode", "mode(FreeClimbing|WallClimbing|TrellisClimbing|GroundCreeping|Hanging|Twining|TendrilClimbing)", "FreeClimbing", "Guided-growth constraint mode."),
		parameter("collision", "collision(Avoid|Seek|PermitIntersection)", "Avoid", "Environmental collision behavior."),
		parameter("attachment", "attachment(Contact|Offset|Twine)", "Offset", "Surface attachment resolution mode."),
		parameter("step", "step(length)", "0.14", "Deterministic growth increment."),
		parameter("segments", "segments(count)", "18", "Maximum generated VinePath segments."),
		parameter("seekDistance", "seekDistance(value)", "1", "Maximum target-attraction distance."),
		parameter("attachDistance", "attachDistance(value)", "0.02", "Resolved offset from a contacted target surface."),
		parameter("tolerance", "tolerance(value)", "0.001", "Surface-contact resolution tolerance."),
		parameter("radiusDecay", "radiusDecay(ratio)", "0.92", "Per-segment radius decay ratio."),
		parameter("minimumRadius", "minimumRadius(value)", "0.004", "Lower radius limit."),
		parameter("preferred", "preferred(x y z)", "0 1 0", "Environmental preferred growth vector."),
		parameter("gamma", "gamma(value)", "2", "Branch radius-conservation exponent."),
		parameter("target", "target(id minx miny minz maxx maxy maxz)", "none", "Named local AABB surface sought by climbing or twining modes."),
		parameter("obstacle", "obstacle(id minx miny minz maxx maxy maxz clearance)", "none", "Repeatable collision-avoidance AABB."),
		parameter("detail", "detail(LOD0|LOD1|LOD2|LOD3|LOD4|LOD5)", "LOD3", "Vine stem, junction, and leaf assembly detail level."),
	};
}

std::vector<ProceduralShapeParameterDocumentation> scatter_region_parameters()
{
	return {
		parameter("region", "region(identifier)", "vegetation_scatter", "Stable scatter resolution identity."),
		parameter("species", "species(DeciduousTree|Shrub|FloweringHerb|GrassClump|IvyVine)", "GrassClump", "Shared canonical Plant source mesh."),
		parameter("seed", "seed(integer)", "1", "Deterministic candidate sampling and source-plant seed."),
		parameter("surface", "surface(id minx miny minz maxx maxy maxz)", "required", "Named local AABB whose selected face receives placements."),
		parameter("face", "face(MinimumX|MaximumX|MinimumY|MaximumY|MinimumZ|MaximumZ)", "MaximumY", "Surface face used as the scatter plane."),
		parameter("density", "density(countPerArea)", "1", "Requested placement density over surface area."),
		parameter("separation", "separation(distance)", "0.25", "Minimum center-to-center spacing."),
		parameter("scale", "scale(min max)", "1 1", "Deterministic uniform scale range."),
		parameter("orientation", "orientation(SurfaceNormal|SurfaceNormalRandomAzimuth|WorldUpRandomAzimuth)", "WorldUpRandomAzimuth", "Placement frame construction policy."),
		parameter("collisionRadius", "collisionRadius(value)", "0", "Placement collision-proxy radius."),
		parameter("layer", "layer(CollisionLayer)", "Terrain", "Collision layer assigned to accepted placements."),
		parameter("mask", "mask(CollisionLayer...)", "all", "Obstacle layers which reject candidates."),
		parameter("obstacle", "obstacle(id minx miny minz maxx maxy maxz layer)", "none", "Repeatable named obstacle boundary."),
		parameter("detail", "detail(LOD0|LOD1|LOD2|LOD3|LOD4|LOD5)", "LOD2", "Shared source-plant geometry detail."),
	};
}

const std::vector<ProceduralShapeCatalogEntry> &catalog_entries()
{
	static const std::vector<ProceduralShapeCatalogEntry> entries = {
		{"Cylinder", "Cylinder", ShapeFamily::Cylinder, false,
		 "Cylinder(radial(0 1) axial(0 1) azimuth(0 360) topology(solid) close(all) segments(40 1))",
		 "Canonical normalized cylinder family; S(...) owns external dimensions.",
		 cylinder_parameters()},
		{"Sphere", "Sphere", ShapeFamily::Sphere, false,
		 "Sphere(radial(0 1) polar(0 180) azimuth(0 360) topology(solid) close(all) segments(40 20))",
		 "Canonical normalized sphere family; nonuniform S(...) creates ellipsoids.",
		 sphere_parameters()},
			{"AxialProfile", "Axial Profile", ShapeFamily::AxialProfile, false,
		 "AxialProfile(axis(y) profile(Name polygon(x z ...)) at(position Name) hold(position) cap(all))",
		 "Deterministic profile-driven extrusion, loft, and contained step primitive.",
			 axial_profile_parameters()},
			{"Extrude", "Extrude Profile", ShapeFamily::ExtrudeProfile, false,
			 "Extrude(Rect(width height) depth cap(all))",
			 "Deterministic extrusion of a named two-dimensional profile constructor.",
			 {parameter("profile", "Rect|RoundedRect|Circle|Ellipse|ChamferRect|Polygon", "Rect(1 1)", "Profile constructor."),
			  parameter("depth", "depth-expression", "1", "Positive local-Z extrusion depth."),
			  parameter("cap", "cap(all|none|front|back)", "all", "Front/back closure policy.")}},
		{"SweepProfile", "Sweep Profile", ShapeFamily::SweepProfile, false,
		 "SweepProfile(profile(Rect 0.10 0.05) path(0 0 0 0 1 0) up(0 0 1) cap(all))",
		 "Sweep a reusable Profile2D along an ordered three-dimensional path.",
		 sweep_profile_parameters()},
		{"VariableSectionSweep", "Variable Section Sweep", ShapeFamily::VariableSectionSweep, false,
		 "VariableSectionSweep(path(0 0 0 0 1 0) station(0 ThinWallBox 0.10 0.06 0.002 0.006) station(1 ThinWallBox 0.14 0.08 0.002 0.008) nominalSection(ThinWallBox 0.12 0.07 0.002 0.007) frame(rotationMinimizing) cap(all) detail(LOD3))",
		 "Sweep corresponding profiles through independently transformed stations while retaining optional multi-component nominal drawing sections.",
		 variable_section_sweep_parameters()},
		{"SweepDisk", "Sweep Disk", ShapeFamily::SweepDisk, false,
		 "SweepDisk(path(0 0 0 0 1 0) radius(0.05) up(0 0 1) segments(16) cap(all))",
		 "Sweep a circular section along an ordered three-dimensional path.",
		 sweep_disk_parameters()},
		{"Revolve", "Revolve", ShapeFamily::Revolve, false,
		 "Revolve(profile(Polygon 0 -0.5 0.5 -0.5 0.5 0.5 0 0.5) angle(0 360) segments(32) cap(all))",
		 "Revolve a radial Profile2D around the local Y axis.",
		 revolve_parameters()},
		{"Loft", "Loft", ShapeFamily::Loft, false,
		 "Loft(section(0 Rect 1 1) section(1 Rect 0.5 0.5) cap(all))",
		 "Loft corresponding Profile2D vertices through ordered axial sections.",
		 loft_parameters()},
		{"ShellLoft", "Shell Loft", ShapeFamily::ShellLoft, false,
		 "ShellLoft(section(0 4 -1 -1 1 -1 1 1 -1 1 -0.8 -0.8 -0.8 0.8 0.8 0.8 0.8 -0.8) section(1 4 -0.8 -0.8 0.8 -0.8 0.8 0.8 -0.8 0.8 -0.6 -0.6 -0.6 0.6 0.6 0.6 0.6 -0.6) cap(all))",
		 "Loft corresponding outer and inner loops into a finite-thickness shell.",
		 shell_loft_parameters()},
		{"SurfaceLoft", "Surface Loft", ShapeFamily::SurfaceLoft, false,
		 "SurfaceLoft(section(0 RoundedRect 1 0.5 0.08 4) section(1 RoundedRect 0.7 0.35 0.06 4) detail(LOD3))",
		 "Loft corresponding Profile2D vertices into an intentionally open surface without end caps.",
		 surface_loft_parameters()},
		{"ShellOffset", "Shell Offset", ShapeFamily::ShellOffset, false,
		 "ShellOffset(source(SurfaceLoft(section(0 Rect 1 0.5) section(1 Rect 0.7 0.35))) thickness(0.02) side(both) detail(LOD3))",
		 "Offset a nested mesh along generated surface normals, optionally producing a closed paired shell.",
		 shell_offset_parameters()},
		{"MirrorShape", "Mirror Shape", ShapeFamily::MirrorShape, false,
		 "MirrorShape(source(Extrude(RoundedRect(0.3 0.2 0.04 4) 0.5 cap(all))) plane(x 0.6) mode(both) detail(LOD3))",
		 "Reflect a nested shape across an axis-aligned plane while preserving valid mirrored winding.",
		 mirror_shape_parameters()},
		{"CurvedPanel", "Curved Panel", ShapeFamily::CurvedPanel, false,
		 "CurvedPanel(width(1.2) height(0.6) curvature(0.06 0.02) thickness(0.012) segments(24 10) detail(LOD3))",
		 "Generate an open or finite-thickness tessellated panel with deterministic two-axis camber.",
		 curved_panel_parameters()},
		{"FormedPanel", "Formed Panel", ShapeFamily::FormedPanel, false,
		 "FormedPanel(section(-0.5 RoundedRect 1.2 0.5 0.04 4) section(0.5 RoundedRect 0.9 0.4 0.03 4) thickness(0.0012) side(both) detail(LOD3))",
		 "Create a finite-thickness formed sheet from ordered transverse profiles.",
		 formed_panel_parameters()},
		{"HostedOpening", "Hosted Opening", ShapeFamily::HostedOpening, false,
		 "HostedOpening(host(RoundedRect 1.0 0.6 0.04 4) opening(Circle 0.08 24) depth(0.002) cap(all) detail(LOD3))",
		 "Extrude a planar host profile with deterministic validated opening boundaries and tagged cut walls.",
		 hosted_opening_parameters()},
		{"EmbossedBead", "Embossed Bead", ShapeFamily::EmbossedBead, false,
		 "EmbossedBead(path(-0.4 0 0 0.4 0 0) width(0.03) depth(0.006) shoulder(0.002) side(positive) end(closed) detail(LOD3))",
		 "Create a tagged finite-thickness swept bead addition for formed sheet composition.",
		 embossed_bead_parameters()},
		{"EdgeFlange", "Edge Flange", ShapeFamily::EdgeFlange, false,
		 "EdgeFlange(path(-0.5 0 0 0.5 0 0) width(0.02) thickness(0.0012) angle(90) bend(0.002) side(positive) detail(LOD3))",
		 "Sweep an explicit folded sheet flange along a supplied host boundary path.",
		 edge_flange_parameters()},
		{"CompoundShape", "Compound Shape", ShapeFamily::CompoundShape, false,
		 "CompoundShape(part(Panel source(FormedPanel(section(-0.5 Rect 1 0.5) section(0.5 Rect 0.8 0.4) thickness(0.0012)))) detail(LOD3))",
		 "Compose named child shape specifications with deterministic transforms and retained child-part surface provenance.",
		 compound_shape_parameters()},
		{"PanelCut", "Panel Cut", ShapeFamily::PanelCut, false,
		 "PanelCut(path(0 0 0 0 0.5 0 0 1 0) gap(0.01 0.006) up(0 0 1) cap(all) detail(LOD3))",
		 "Sweep a semantic rectangular panel seam or reveal along an ordered path.",
		 panel_cut_parameters()},
		{"FoldedProfile", "Folded Profile", ShapeFamily::FoldedProfile, false,
		 "FoldedProfile(path(0 0 0.5 0 0.5 0.5) thickness(0.01) depth(1) cap(all))",
		 "Sweep a finite sheet section along a two-dimensional fold path.",
		 folded_profile_parameters()},
		{"InstanceArray", "Instance Array", ShapeFamily::InstanceArray, false,
		 "InstanceArray(source(Extrude(Rect(0.08 0.02) 0.30 cap(all))) radial(8 0.40 0 0 1 0) detail(LOD3))",
		 "Realize one nested source shape through deterministic linear, grid, or radial transforms.",
		 instance_array_parameters()},
		{"VehicleBodyShell", "Vehicle Body Shell", ShapeFamily::ShellLoft, false,
		 "VehicleBodyShell(thickness(0.04) section(-2.14 0.70 0.25 0.70 0.77 0.92 0.32) section(2.14 0.70 0.30 0.64 0.70 0.78 0.30) cap(all) detail(LOD3))",
		 "Semantic MVPv1 body-section grammar lowered deterministically to FGKv1 ShellLoft geometry.",
		 vehicle_body_shell_parameters()},
		{"GeneratedMeshReference", "Generated Mesh Reference", ShapeFamily::GeneratedMeshReference, false,
		 "GeneratedMeshReference(meshKey(MCSMv2ReferenceBodyMesh) detail(LOD2))",
		 "Resolve an immutable generated mesh provider key without embedding vertices in grammar text.",
		 generated_mesh_reference_parameters()},
		{"TaperedSweep", "Tapered Sweep", ShapeFamily::TaperedSweep, false,
		 "TaperedSweep(samples(0 0 0 0.10 0 1 0 0.04) up(0 0 1) segments(12) cap(all))",
		 "Deterministic continuously tapered sweep for branches, stems, roots, petioles, vines, and tendrils.",
		 tapered_sweep_parameters()},
		{"BranchJunction", "Branch Junction", ShapeFamily::BranchJunction, false,
		 "BranchJunction(core(0.12 1.15) parent(0 -1 0 0.12 0.22) child(0.75 0.66 0 0.08 0.20) child(-0.75 0.66 0 0.07 0.18) segments(12))",
		 "Deterministic botanical fork transition with one parent arm, two or more child arms, and a bounded junction bulge.",
		 branch_junction_parameters()},
		{"LeafBlade", "Leaf Blade", ShapeFamily::LeafBlade, false,
		 "LeafBlade(profile(Ovate) length(0.10) width(0.05) curvature(0) camber(0) twist(0) thickness(0.001) widthPower(1) segments(12 4))",
		 "Parameterized botanical leaf surface or finite-thickness blade.",
		 botanical_blade_parameters("Ovate")},
		{"PetalBlade", "Petal Blade", ShapeFamily::PetalBlade, false,
		 "PetalBlade(profile(Obovate) length(0.08) width(0.04) curvature(0.02) camber(0.01) twist(0) thickness(0.001) widthPower(1) segments(12 4))",
		 "Parameterized flower petal sharing the botanical blade surface model.",
			 botanical_blade_parameters("Obovate")},
		{"Plant", "Plant", ShapeFamily::Plant, false,
			 "Plant(species(DeciduousTree) age(12) state(Mature) seed(42) detail(LOD3))",
			 "Species-driven BranchGraph generation with deterministic topology, branch junctions, phyllotactic organ placement, shared leaf instances, and LOD assembly.",
			 plant_parameters()},
		{"Vine", "Vine", ShapeFamily::Vine, false,
			 "Vine(species(IvyVine) start(0 0 -1) direction(0 1 1) mode(WallClimbing) collision(Seek) attachment(Offset) target(Wall -2 0 -0.1 2 3 0.1) detail(LOD3))",
			 "Guided VinePath growth with avoid, seek, or permit collision behavior, deterministic surface attachment, tapered stems, and phyllotactic shared leaves.",
			 vine_parameters()},
		{"ScatterRegion", "Scatter Region", ShapeFamily::ScatterRegion, false,
			 "ScatterRegion(region(Meadow) species(GrassClump) seed(42) surface(Soil -3 0 -3 3 0.1 3) face(MaximumY) density(0.8) separation(0.35) scale(0.8 1.2) orientation(WorldUpRandomAzimuth) layer(Terrain) mask(Structure Furniture) detail(LOD2))",
			 "Deterministic collision-filtered vegetation placement using one cached species mesh and an evaluated InstanceArray.",
			 scatter_region_parameters()},
		{"Tube", "Tube", ShapeFamily::Cylinder, true,
		 "Cylinder(radial(inner 1) topology(shell) close(all))",
		 "Closed cylindrical shell alias.",
		 {parameter("inner", "inner(radius)", "0.75", "Normalized inner-radius fraction.")}},
		{"CylinderSector", "Cylinder Sector", ShapeFamily::Cylinder, true,
		 "Cylinder(azimuth(0 sweep) topology(solid) close(all))",
		 "Closed angular cylinder-sector alias.",
		 {parameter("sweep", "sweep(degrees)", "180", "Retained angular sweep.")}},
		{"DSection", "D Section", ShapeFamily::Cylinder, true,
		 "Cylinder(chord(angle offset side) topology(solid) close(all))",
		 "Chord-cut cylinder alias with an offset planar face.",
		 {parameter("angle", "angle(degrees)", "0", "Chord-normal angle in the XZ plane."),
		  parameter("offset", "offset(distance)", "0.25", "Signed offset normalized by outer radius."),
		  parameter("side", "side(positive|negative)", "positive", "Retained chord half-space.")}},
		{"HalfCylinder", "Half Cylinder", ShapeFamily::Cylinder, true,
		 "Cylinder(chord(angle 0 side) topology(solid) close(all))",
		 "Explicitly means a centre-chord half cylinder, not half height or a surface patch.",
		 {parameter("angle", "angle(degrees)", "0", "Chord-normal angle in the XZ plane."),
		  parameter("side", "side(positive|negative)", "positive", "Retained chord half-space.")}},
		{"Hemisphere", "Hemisphere", ShapeFamily::Sphere, true,
		 "Sphere(clip(axis 0 side) topology(solid) close(all))",
		 "Centre-plane clipped sphere alias.",
		 {parameter("axis", "axis(x|y|z)", "y", "Clip-plane normal axis."),
		  parameter("side", "side(positive|negative)", "positive", "Retained half-space.")}},
		{"SphereCap", "Sphere Cap", ShapeFamily::Sphere, true,
		 "Sphere(clip(axis offset side) topology(solid) close(all))",
		 "Offset plane-cut spherical cap alias.",
		 {parameter("axis", "axis(x|y|z)", "y", "Clip-plane normal axis."),
		  parameter("offset", "offset(distance)", "0.35", "Signed offset normalized by outer radius."),
		  parameter("side", "side(positive|negative)", "positive", "Retained half-space.")}},
		{"SphereBowl", "Sphere Bowl", ShapeFamily::Sphere, true,
		 "Sphere(radial(inner 1) clip(axis offset side) topology(shell) close(none))",
		 "Open spherical shell alias with no generated clip closure.",
		 {parameter("inner", "inner(radius)", "0.85", "Normalized inner-radius fraction."),
		  parameter("axis", "axis(x|y|z)", "y", "Clip-plane normal axis."),
		  parameter("offset", "offset(distance)", "0", "Signed offset normalized by outer radius."),
		  parameter("side", "side(positive|negative)", "positive", "Retained half-space.")}},
		{"SphereQuarter", "Sphere Quarter", ShapeFamily::Sphere, true,
		 "Sphere(clip(1 0 0 0 positive) clip(0 1 0 0 positive) topology(solid) close(all))",
		 "Two ordered orthogonal centre-plane clips.", {}},
		{"SphereOctant", "Sphere Octant", ShapeFamily::Sphere, true,
		 "Sphere(clip(1 0 0 0 positive) clip(0 1 0 0 positive) clip(0 0 1 0 positive) topology(solid) close(all))",
		 "Three ordered orthogonal centre-plane clips.", {}},
	};
	return entries;
}

}

const std::vector<ProceduralShapeCatalogEntry> &
ProceduralShapeCatalogRepository::entries() const
{
	return catalog_entries();
}

const ProceduralShapeCatalogEntry *ProceduralShapeCatalogRepository::find(
	const std::string &identifier) const
{
	const std::string lowered_identifier = lowercase_copy(identifier);
	for (const ProceduralShapeCatalogEntry &entry : catalog_entries()) {
		if (lowercase_copy(entry.identifier()) == lowered_identifier) return &entry;
	}
	return nullptr;
}

std::vector<std::string> ProceduralShapeCatalogRepository::completionNames() const
{
	std::vector<std::string> names;
	names.reserve(catalog_entries().size());
	for (const ProceduralShapeCatalogEntry &entry : catalog_entries()) {
		names.push_back(entry.identifier());
	}
	return names;
}

std::vector<std::string> ProceduralShapeCatalogRepository::optionSignatures(
	const std::string &identifier) const
{
	std::vector<std::string> signatures;
	const ProceduralShapeCatalogEntry *entry = find(identifier);
	if (entry == nullptr) return signatures;
	for (const ProceduralShapeParameterDocumentation &parameter_documentation :
	     entry->parameters()) {
		signatures.push_back(parameter_documentation.signature());
	}
	return signatures;
}

std::vector<std::string> ProceduralShapeCatalogRepository::optionCompletions(
	const std::string &identifier) const
{
	std::vector<std::string> completions;
	const ProceduralShapeCatalogEntry *entry = find(identifier);
	if (entry == nullptr) return completions;
	for (const ProceduralShapeParameterDocumentation &parameter_documentation :
	     entry->parameters()) {
		if (!parameter_documentation.defaultValue().empty() &&
		    parameter_documentation.defaultValue() != "none") {
			completions.push_back(parameter_documentation.name() + "(" +
			                      parameter_documentation.defaultValue() + ")");
		}
		else {
			completions.push_back(parameter_documentation.signature());
		}
	}
	return completions;
}

std::string ProceduralShapeCatalogRepository::tooltipFor(
	const std::string &identifier) const
{
	const ProceduralShapeCatalogEntry *entry = find(identifier);
	if (entry == nullptr) return {};
	std::ostringstream tooltip;
	tooltip << entry->summary() << " Canonical form: " << entry->canonicalExpansion();
	if (!entry->parameters().empty()) {
		tooltip << " Options: ";
		for (std::size_t index = 0; index < entry->parameters().size(); ++index) {
			if (index > 0) tooltip << "; ";
			const auto &parameter_documentation = entry->parameters()[index];
			tooltip << parameter_documentation.signature();
			if (!parameter_documentation.defaultValue().empty() &&
			    parameter_documentation.defaultValue() != "none") {
				tooltip << " default " << parameter_documentation.defaultValue();
			}
		}
		tooltip << ".";
	}
	return tooltip.str();
}
