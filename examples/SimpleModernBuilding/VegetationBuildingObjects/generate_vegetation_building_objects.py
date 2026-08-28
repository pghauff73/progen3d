#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
import hashlib
import io
import json
import re
from dataclasses import dataclass
from pathlib import Path


REPOSITORY_ROOT = Path(__file__).resolve().parents[3]
SUITE_DIRECTORY = Path(__file__).resolve().parent
GRAMMAR_DIRECTORY = SUITE_DIRECTORY / "grammars"
EVIDENCE_DIRECTORY = SUITE_DIRECTORY / "evidence"
COMPARISON_VIEWS = ("front", "right", "top")
COMPARISON_FIELD_OF_VIEW_DEGREES = 5.0
MINIMUM_VIEW_MATCH_PERCENT = 95.0
TARGET_VIEW_MATCH_PERCENT = 97.0
ALL_VEGETATION_PRIMITIVES = (
    "Plant",
    "Vine",
    "ScatterRegion",
    "TaperedSweep",
    "BranchJunction",
    "LeafBlade",
    "PetalBlade",
)
EXPECTED_CATEGORY_COUNTS = {
    "Tree": 10,
    "Shrub": 8,
    "Grass": 10,
    "Groundcover": 5,
    "Vine": 7,
    "Plant": 5,
    "EcologicalPlanting": 5,
}
BUILDING_VEGETATION_MODEL_SCHEMA = "ProGen3D-BuildingVegetationObjectModel-v2.1"
BUILDING_VEGETATION_MODEL_ID = "BVO-OMv2.1"
BIOLOGICAL_PROFILE_SCHEMA = "ProGen3D-VegetationBiologicalProfile-v2"
RESEARCH_SNAPSHOT_DATE = "2026-08-28"
CALIBRATION_DOMAINS = (
    "TaxonomicIdentity",
    "ShootTopology",
    "RootArchitecture",
    "CanopyOptics",
    "Phenology",
    "Biomechanics",
    "EnvironmentalResponse",
    "SemanticAnnotation",
    "FunctionalTraits",
    "Hydraulics",
    "SizeAllometry",
    "SubstrateSuitability",
    "BuildingPlacement",
    "RepresentationFidelity",
)
LITERATURE_SOURCES = (
    {
        "source_id": "BVO-LIT-001",
        "domain": "multiscale_architecture",
        "citation": (
            "Godin and Caraglio (1998), A multiscale model of plant "
            "topological structures"
        ),
        "locator": "https://doi.org/10.1006/jtbi.1997.0561",
    },
    {
        "source_id": "BVO-LIT-002",
        "domain": "functional_structural_modeling",
        "citation": (
            "Vos et al. (2010), Functional-structural plant modelling: "
            "a new versatile tool in crop science"
        ),
        "locator": "https://doi.org/10.1093/jxb/erp345",
    },
    {
        "source_id": "BVO-LIT-003",
        "domain": "environment_responsive_growth",
        "citation": (
            "Runions, Lane, and Prusinkiewicz (2007), Modeling Trees with "
            "a Space Colonization Algorithm"
        ),
        "locator": "https://doi.org/10.2312/NPH/NPH07/063-070",
    },
    {
        "source_id": "BVO-LIT-004",
        "domain": "quantitative_structure_models",
        "citation": (
            "Raumonen et al. (2013), Fast Automatic Precision Tree Models "
            "from Terrestrial Laser Scanner Data"
        ),
        "locator": "https://doi.org/10.3390/rs5020491",
    },
    {
        "source_id": "BVO-LIT-005",
        "domain": "branch_architecture_validation",
        "citation": (
            "Gonzalez de Tanago et al. (2018), Quantifying branch "
            "architecture of tropical trees using terrestrial LiDAR and 3D modelling"
        ),
        "locator": "https://doi.org/10.1007/s00468-018-1704-1",
    },
    {
        "source_id": "BVO-LIT-006",
        "domain": "root_architecture",
        "citation": (
            "Postma et al. (2017), OpenSimRoot: widening the scope and "
            "application of root architectural models"
        ),
        "locator": "https://doi.org/10.1111/nph.14641",
    },
    {
        "source_id": "BVO-LIT-007",
        "domain": "root_architecture",
        "citation": (
            "Schnepf et al. (2018), CRootBox: a structural-functional "
            "modelling framework for root systems"
        ),
        "locator": "https://doi.org/10.1093/aob/mcx221",
    },
    {
        "source_id": "BVO-LIT-008",
        "domain": "building_context_placement",
        "citation": "Niese et al. (2022), Procedural Urban Forestry",
        "locator": "https://doi.org/10.1145/3502220",
    },
    {
        "source_id": "BVO-LIT-009",
        "domain": "canopy_optics",
        "citation": (
            "Zhu et al. (2023), A reinterpretation of the gap fraction of "
            "tree crowns from the perspectives of computer graphics and porous media theory"
        ),
        "locator": "https://doi.org/10.3389/fpls.2023.1109443",
    },
    {
        "source_id": "BVO-LIT-010",
        "domain": "plant_biomechanics",
        "citation": "Wang, Zhao, and Barbic (2017), Botanical materials based on biomechanics",
        "locator": "https://doi.org/10.1145/3072959.3073655",
    },
    {
        "source_id": "BVO-LIT-011",
        "domain": "phenotyping_and_calibration",
        "citation": (
            "Louarn and Song (2020), Two decades of functional-structural "
            "plant modelling: now addressing fundamental questions in systems biology and predictive ecology"
        ),
        "locator": "https://doi.org/10.1093/aob/mcaa143",
    },
    {
        "source_id": "BVO-LIT-012",
        "domain": "validation",
        "citation": (
            "Wang et al. (2018), Pattern-oriented modelling as a novel way "
            "to verify and validate functional-structural plant models: a demonstration with avocado"
        ),
        "locator": "https://doi.org/10.1093/aob/mcx187",
    },
    {
        "source_id": "BVO-LIT-013",
        "domain": "plant_semantic_annotation",
        "citation": (
            "Cooper et al. (2013), The Plant Ontology as a Tool for "
            "Comparative Plant Anatomy and Genomic Analyses"
        ),
        "locator": "https://doi.org/10.1093/pcp/pcs163",
    },
    {
        "source_id": "BVO-LIT-014",
        "domain": "functional_traits",
        "citation": (
            "Kattge et al. (2020), TRY plant trait database - enhanced "
            "coverage and open access"
        ),
        "locator": "https://doi.org/10.1111/gcb.14904",
    },
    {
        "source_id": "BVO-LIT-015",
        "domain": "plant_hydraulics",
        "citation": (
            "Christoffersen et al. (2016), Linking hydraulic traits to "
            "tropical forest function in a size-structured and trait-driven model"
        ),
        "locator": "https://doi.org/10.5194/gmd-9-4227-2016",
    },
    {
        "source_id": "BVO-LIT-016",
        "domain": "urban_plant_allometry",
        "citation": (
            "Pretzsch et al. (2015), Crown size and growing space requirement "
            "of common tree species in urban centres, parks, and forests"
        ),
        "locator": "https://doi.org/10.1016/j.ufug.2015.04.006",
    },
    {
        "source_id": "BVO-LIT-017",
        "domain": "rootable_substrate_volume",
        "citation": (
            "Kopinga (1991), The Effects of Restricted Volumes of Soil on "
            "the Growth and Development of Street Trees"
        ),
        "locator": "https://doi.org/10.48044/jauf.1991.016",
    },
    {
        "source_id": "BVO-LIT-018",
        "domain": "root_architecture_exchange",
        "citation": (
            "Lobet et al. (2015), Root System Markup Language: Toward a "
            "Unified Root Architecture Description Language"
        ),
        "locator": "https://doi.org/10.1104/pp.114.253625",
    },
    {
        "source_id": "BVO-LIT-019",
        "domain": "root_architecture_exchange_schema",
        "citation": (
            "RootSystemML RSMLValidator schema, rsml.xsd at commit "
            "0b47b2af172805619309c19650ed92fe3b89512c"
        ),
        "locator": (
            "https://raw.githubusercontent.com/RootSystemML/RSMLValidator/"
            "0b47b2af172805619309c19650ed92fe3b89512c/rsml.xsd"
        ),
    },
    {
        "source_id": "BVO-LIT-020",
        "domain": "quantitative_structure_model_exchange",
        "citation": (
            "TreeQSM save_model_text 1.1.0 exporter at commit "
            "a65ec03ec2646fcc14981e95e632b0c48e579817"
        ),
        "locator": (
            "https://raw.githubusercontent.com/InverseTampere/TreeQSM/"
            "a65ec03ec2646fcc14981e95e632b0c48e579817/"
            "src/tools/save_model_text.m"
        ),
    },
    {
        "source_id": "BVO-LIT-021",
        "domain": "point_cloud_exchange_ply",
        "citation": (
            "Stanford 3D Scanning Repository, The PLY Polygon File Format "
            "specification"
        ),
        "locator": "https://graphics.stanford.edu/data/3Dscanrep/ply.html",
    },
    {
        "source_id": "BVO-LIT-022",
        "domain": "point_cloud_exchange_las",
        "citation": "ASPRS LAS Specification 1.4 Revision 16",
        "locator": "https://github.com/ASPRSorg/LAS/releases/tag/1.4.R16",
    },
    {
        "source_id": "BVO-LIT-023",
        "domain": "point_cloud_exchange_las",
        "citation": "ASPRS LAS Specification 1.5 Revision 00",
        "locator": "https://github.com/ASPRSorg/LAS/releases/tag/1.5.R00",
    },
    {
        "source_id": "BVO-LIT-024",
        "domain": "point_cloud_exchange_e57",
        "citation": (
            "libE57Format, C++ read and write support for the ASTM E57 "
            "3D imaging data format"
        ),
        "locator": "https://github.com/asmaloney/libE57Format",
    },
    {
        "source_id": "BVO-LIT-025",
        "domain": "point_cloud_tree_reconstruction",
        "citation": (
            "Hackenberg et al. (2015), SimpleTree - An Efficient Open "
            "Source Tool to Build Tree Models from TLS Clouds"
        ),
        "locator": "https://doi.org/10.3390/f6114245",
    },
    {
        "source_id": "BVO-LIT-026",
        "domain": "point_cloud_organ_classification",
        "citation": (
            "Vicari et al. (2019), Leaf and wood classification framework "
            "for terrestrial LiDAR point clouds"
        ),
        "locator": "https://doi.org/10.1111/2041-210X.13144",
    },
    {
        "source_id": "BVO-LIT-027",
        "domain": "canopy_voxel_reconstruction",
        "citation": (
            "Hosoi and Omasa (2006), Voxel-Based 3-D Modeling of Individual "
            "Trees for Estimating Leaf Area Density Using High-Resolution "
            "Portable Scanning Lidar"
        ),
        "locator": "https://doi.org/10.1109/TGRS.2006.881743",
    },
    {
        "source_id": "BVO-LIT-028",
        "domain": "canopy_voxel_leaf_area_density",
        "citation": (
            "Béland et al. (2014), A model for deriving voxel-level tree leaf "
            "area density estimates from ground-based LiDAR"
        ),
        "locator": "https://doi.org/10.1016/j.envsoft.2014.09.034",
    },
    {
        "source_id": "BVO-LIT-029",
        "domain": "canopy_voxel_quality",
        "citation": (
            "Fernández-Sarría et al. (2014), Effects of voxel size and "
            "sampling setup on the estimation of forest canopy gap fraction "
            "from terrestrial laser scanning data"
        ),
        "locator": "https://doi.org/10.1016/j.agrformet.2014.05.013",
    },
    {
        "source_id": "BVO-LIT-030",
        "domain": "robust_point_cloud_cylinder_fitting",
        "citation": (
            "Nurunnabi, Sadahiro, and Lindenbergh (2017), Robust Cylinder "
            "Fitting in Three-Dimensional Point Cloud Data"
        ),
        "locator": (
            "https://doi.org/10.5194/isprs-archives-XLII-1-W1-63-2017"
        ),
    },
    {
        "source_id": "BVO-LIT-031",
        "domain": "quantitative_structure_model_volume_validation",
        "citation": (
            "Demol et al. (2021), Forest above-ground volume assessments "
            "with terrestrial laser scanning: a ground-truth validation "
            "experiment in temperate, managed forests"
        ),
        "locator": "https://doi.org/10.1093/aob/mcab110",
    },
    {
        "source_id": "BVO-LIT-032",
        "domain": "branch_architecture_ground_truth_validation",
        "citation": (
            "Wilkes et al. (2021), Terrestrial laser scanning to "
            "reconstruct branch architecture from harvested branches"
        ),
        "locator": "https://doi.org/10.1111/2041-210X.13709",
    },
    {
        "source_id": "BVO-LIT-033",
        "domain": "quantitative_structure_model_parameter_sensitivity",
        "citation": (
            "Zhang et al. (2020), Apple Tree Branch Information "
            "Extraction from Terrestrial Laser Scanning and Backpack-LiDAR"
        ),
        "locator": "https://doi.org/10.3390/rs12213592",
    },
    {
        "source_id": "BVO-LIT-034",
        "domain": "quantitative_structure_model_implementation_authority",
        "citation": (
            "TreeQSM cover-set, branch-segmentation, cylinder-fitting, "
            "and optimum-selection implementation, commit "
            "6630bbf516f8b53adb7d60a2cccbd21e6fe51226"
        ),
        "locator": (
            "https://github.com/InverseTampere/TreeQSM/tree/"
            "6630bbf516f8b53adb7d60a2cccbd21e6fe51226/src"
        ),
    },
)


@dataclass(frozen=True)
class VegetationBuildingObject:
    object_id: str
    object_class: str
    title: str
    category: str
    grammar_name: str
    design_intent: str
    vegetation_primitives: tuple[str, ...]
    generation_methods: tuple[str, ...]
    grammar_source: str
    expected_extent_xyz: tuple[float, float, float] = (4.0, 5.0, 3.0)
    collision_shape: str = "AuthoredAabb"
    orientation_rule: str = "WorldUpWithFreeYaw"


OBJECTS = (
    VegetationBuildingObject(
        object_id="SMB_VEGETATION_COURTYARD_CANOPY_TREE",
        object_class="ArchitecturalCanopyTree",
        title="Courtyard Canopy Tree",
        category="Tree",
        grammar_name="SMBV01_CourtyardCanopyTree.grammar",
        design_intent=(
            "A broad, layered courtyard shade tree with a tapered trunk, explicit "
            "branch junctions, swept limbs, foliage lobes, and individually modeled leaves."
        ),
        vegetation_primitives=(
            "TaperedSweep", "BranchJunction", "LeafBlade",
        ),
        generation_methods=("ExplicitBranchGraph",),
        grammar_source=r"""# SMBV01 Courtyard Canopy Tree
# Object-local geometry uses Y-up coordinates and a five-degree comparison lens.

Start -> SMB_VEGETATION_COURTYARD_CANOPY_TREE

SMB_VEGETATION_COURTYARD_CANOPY_TREE ->
[
    Object(
        id(SMB_VEGETATION_COURTYARD_CANOPY_TREE)
        class(ArchitecturalCanopyTree)
        taxonomy(SmallModernBuilding BuildingSite LandscapeSystem Tree ArchitecturalCanopyTree)
        layer(Terrain)
        mask(Structure Envelope Interior Furniture Plumbing Hvac Electrical Equipment Terrain Temporary)
    )
    [
        Interface(id(rootSupport) type(Support) origin(0 0 0) normal(0 1 0) tangent(1 0 0) region(Point) tolerance(0.001))
        Interface(id(inspection) type(InspectionInterface) origin(0 3.2 0) normal(0 0 1) tangent(1 0 0) region(Point) tolerance(0.001))

        [ I(TaperedSweep(samples(0 0 0 0.34  0.08 1.10 0.03 0.30  -0.06 2.25 0.08 0.25  0.02 3.45 0.02 0.18  0 4.35 0 0.11) up(0 0 1) segments(20) cap(all)) material(amberwarmwood) 1 0.20) ]
        [ T(0 1.35 0) I(BranchJunction(core(0.29 1.24) parent(0 -1 0 0.30 0.42) child(0.82 0.48 0.10 0.17 0.72) child(-0.74 0.56 -0.16 0.16 0.68) segments(18)) material(amberwarmwood) 1 0.20) ]
        [ T(0 2.45 0) I(BranchJunction(core(0.23 1.20) parent(0 -1 0 0.24 0.35) child(0.58 0.64 -0.50 0.14 0.82) child(-0.62 0.62 0.48 0.14 0.82) segments(18)) material(amberwarmwood) 1 0.20) ]

        SMBV01_Branch(0 2.05 0  1.85 3.05 0.38)
        SMBV01_Branch(0 2.20 0  -1.75 3.18 -0.48)
        SMBV01_Branch(0 2.75 0  1.45 3.86 -1.12)
        SMBV01_Branch(0 2.82 0  -1.38 3.94 1.18)
        SMBV01_Branch(0 3.32 0  0.92 4.62 1.22)
        SMBV01_Branch(0 3.36 0  -0.88 4.70 -1.28)

        SMBV01_FoliageLobe(-1.62 3.52 -0.46 1.20 0.90 1.05)
        SMBV01_FoliageLobe(1.58 3.48 0.42 1.18 0.92 1.05)
        SMBV01_FoliageLobe(-0.88 4.36 0.95 1.12 0.90 1.02)
        SMBV01_FoliageLobe(0.92 4.40 -0.98 1.10 0.88 1.00)
        SMBV01_FoliageLobe(0 4.82 0 1.36 1.02 1.20)

        SMBV01_Leaf(-2.05 3.58 -0.42 -18)
        SMBV01_Leaf(-1.48 4.08 -0.18 24)
        SMBV01_Leaf(-0.86 4.72 0.90 52)
        SMBV01_Leaf(0 5.18 0.10 0)
        SMBV01_Leaf(0.86 4.76 -0.92 -46)
        SMBV01_Leaf(1.52 4.02 0.30 20)
        SMBV01_Leaf(2.02 3.54 0.46 -26)
    ]
]

SMBV01_Branch(x0 y0 z0 x1 y1 z1) ->
[
    I(TaperedSweep(samples(x0 y0 z0 0.15  x0+(x1-x0)*0.48 y0+(y1-y0)*0.58 z0+(z1-z0)*0.40 0.105  x1 y1 z1 0.045) up(0 0 1) segments(16) cap(all)) material(amberwarmwood) 1 0.20)
]

SMBV01_FoliageLobe(x y z sx sy sz) ->
[
    T(x y z) S(sx sy sz) I(Sphere material(sagegreenmattepaint) 1 0.24)
]

SMBV01_Leaf(x y z yaw) ->
[
    T(x y z) A(yaw 1) A(90 1) I(LeafBlade(profile(Ovate) length(0.72) width(0.32) curvature(0.12) camber(0.035) twist(8) thickness(0.012) widthPower(1.08) segments(18 5)) material(sagegreenmattepaint) 1 0.18)
]
""",
    ),
    VegetationBuildingObject(
        object_id="SMB_VEGETATION_SCULPTURAL_LSYSTEM_TREE",
        object_class="SculpturalLSystemTree",
        title="Sculptural L-System Tree",
        category="Tree",
        grammar_name="SMBV02_SculpturalLSystemTree.grammar",
        design_intent=(
            "A narrow architectural tree whose deterministic L-system crown is "
            "supported by a visible warm-timber trunk and clipped cloud foliage."
        ),
        vegetation_primitives=("Plant", "TaperedSweep", "LeafBlade"),
        generation_methods=("LSystem", "Phyllotaxis", "Tropism"),
        grammar_source=r"""# SMBV02 Sculptural L-System Tree

Start -> SMB_VEGETATION_SCULPTURAL_LSYSTEM_TREE

SMB_VEGETATION_SCULPTURAL_LSYSTEM_TREE ->
[
    Object(
        id(SMB_VEGETATION_SCULPTURAL_LSYSTEM_TREE)
        class(SculpturalLSystemTree)
        taxonomy(SmallModernBuilding BuildingSite LandscapeSystem Tree SculpturalLSystemTree)
        layer(Terrain)
        mask(Structure Envelope Interior Furniture Plumbing Hvac Electrical Equipment Terrain Temporary)
    )
    [
        Interface(id(rootSupport) type(Support) origin(0 0 0) normal(0 1 0) tangent(1 0 0) region(Point) tolerance(0.001))
        Interface(id(inspection) type(InspectionInterface) origin(0 3.0 0) normal(0 0 1) tangent(1 0 0) region(Point) tolerance(0.001))

        [ I(TaperedSweep(samples(0 0 0 0.28  0.02 1.35 0.02 0.24  -0.04 2.70 0.05 0.18  0.03 4.55 0 0.08) up(0 0 1) segments(20) cap(all)) material(amberwarmwood) 1 0.18) ]
        [ T(0 0.20 0) S(2.7 1.12 2.7) I(Plant(species(Shrub) age(8) state(Mature) seed(20260828) generation(LSystem) lAxiom(F) lRule(F F Push Plus F Pop F Push Minus F Pop F) lSystem(4 27 0.17 0.060 0.006 2) phyllotaxis(Alternate 137.50776 0.10 1 0 0 0.18) tropism(Up 0 1 0 0.018) leaf(Ovate 0.15 0.065 0.025 8 0.0015 1) detail(LOD4)) material(sagegreenmattepaint) 1 0.18) ]

        SMBV02_Cloud(-0.72 2.15 0.18 0.95 0.52 0.82)
        SMBV02_Cloud(0.70 2.70 -0.24 1.02 0.56 0.86)
        SMBV02_Cloud(-0.58 3.35 -0.16 0.96 0.54 0.84)
        SMBV02_Cloud(0.52 4.02 0.20 0.90 0.50 0.78)
        SMBV02_Leaf(-0.92 2.18 0.28 18)
        SMBV02_Leaf(0.96 2.74 -0.18 -26)
        SMBV02_Leaf(-0.82 3.42 -0.12 42)
        SMBV02_Leaf(0.76 4.10 0.24 -38)
    ]
]

SMBV02_Cloud(x y z sx sy sz) ->
[
    T(x y z) S(sx sy sz) I(Sphere material(sagegreenmattepaint) 1 0.22)
]

SMBV02_Leaf(x y z yaw) ->
[
    T(x y z) A(yaw 1) A(90 1) I(LeafBlade(profile(Lanceolate) length(0.62) width(0.22) curvature(0.16) camber(0.030) twist(12) thickness(0.010) widthPower(1.18) segments(18 5)) material(sagegreenmattepaint) 1 0.16)
]
""",
    ),
    VegetationBuildingObject(
        object_id="SMB_VEGETATION_FLOWERING_PLANT_CLUSTER",
        object_class="LayeredFloweringPlantCluster",
        title="Layered Flowering Plant Cluster",
        category="Plant",
        grammar_name="SMBV03_LayeredFloweringPlantCluster.grammar",
        design_intent=(
            "A modern rectangular planter containing deterministic flowering herbs, "
            "scattered grass underplanting, modeled leaves, petals, soil, and drainage feet."
        ),
        vegetation_primitives=("Plant", "ScatterRegion", "LeafBlade", "PetalBlade"),
        generation_methods=("SpeciesDriven", "GoldenAngleFlowerHead", "DeterministicScatter"),
        grammar_source=r"""# SMBV03 Layered Flowering Plant Cluster

Start -> SMB_VEGETATION_FLOWERING_PLANT_CLUSTER

SMB_VEGETATION_FLOWERING_PLANT_CLUSTER ->
[
    Object(
        id(SMB_VEGETATION_FLOWERING_PLANT_CLUSTER)
        class(LayeredFloweringPlantCluster)
        taxonomy(SmallModernBuilding BuildingSite LandscapeSystem Plant LayeredFloweringPlantCluster)
        layer(Terrain)
        mask(Structure Envelope Interior Furniture Plumbing Hvac Electrical Equipment Terrain Temporary)
    )
    [
        Interface(id(baseSupport) type(Support) origin(0 0 0) normal(0 1 0) tangent(1 0 0) region(Point) tolerance(0.001))
        Interface(id(irrigationConnection) type(ServiceConnection) origin(0 0.34 -0.72) normal(0 0 -1) tangent(1 0 0) region(Point) tolerance(0.01))
        Interface(id(inspection) type(InspectionInterface) origin(0 1.15 0) normal(0 0 1) tangent(1 0 0) region(Point) tolerance(0.001))

        [ T(0 0.22 0) S(3.2 0.44 1.45) I(CubeY material(blackroughmetal) 1 0.18) ]
        [ T(0 0.48 0) S(2.92 0.16 1.18) I(CubeY material(earthmaterial) 1 0.24) ]
        [ T(-1.25 0.02 -0.48) S(0.26 0.08 0.26) I(CubeY material(blackroughmetal) 1 0.16) ]
        [ T(1.25 0.02 -0.48) S(0.26 0.08 0.26) I(CubeY material(blackroughmetal) 1 0.16) ]
        [ T(-1.25 0.02 0.48) S(0.26 0.08 0.26) I(CubeY material(blackroughmetal) 1 0.16) ]
        [ T(1.25 0.02 0.48) S(0.26 0.08 0.26) I(CubeY material(blackroughmetal) 1 0.16) ]

        [ T(-0.88 0.50 0.08) S(4.8 4.8 4.8) I(Plant(species(FloweringHerb) age(1.2) state(Flowering) seed(20260829) flower(Obovate 0.09 0.045 0.03 0.012 -4 0.001 0.90) petiole(0.022 0.0020 0.0008) whorl(7 0.018 0 26) flowerHead(4 0.045 137.50776 0 9 0.985) detail(LOD4)) material(sagegreenmattepaint) 1 0.17) ]
        [ T(0.08 0.50 -0.12) S(5.6 5.6 5.6) I(Plant(species(FloweringHerb) age(1.4) state(Flowering) seed(20260830) flower(Obovate 0.09 0.048 0.035 0.014 6 0.001 0.88) whorl(6 0.020 14 30) flowerHead(5 0.052 137.50776 12 8 0.982) detail(LOD4)) material(sagegreenmattepaint) 1 0.17) ]
        [ T(0.96 0.50 0.10) S(4.5 4.5 4.5) I(Plant(species(FloweringHerb) age(1.1) state(Flowering) seed(20260831) flower(Obovate 0.085 0.043 0.028 0.010 -8 0.001 0.92) whorl(8 0.016 -10 24) flowerHead(4 0.046 137.50776 -8 7 0.986) detail(LOD4)) material(sagegreenmattepaint) 1 0.17) ]
        [ I(ScatterRegion(region(underplanting) species(GrassClump) seed(20260832) surface(planter_soil -1.35 0.48 -0.46 1.35 0.56 0.46) face(MaximumY) density(0.52) separation(0.30) scale(0.42 0.70) orientation(WorldUpRandomAzimuth) collisionRadius(0.055) layer(Terrain) mask(Structure Furniture) detail(LOD2)) material(sagegreenmattepaint) 1 0.15) ]

        SMBV03_Leaf(-1.22 1.08 0.20 -26)
        SMBV03_Leaf(-0.52 1.42 -0.18 18)
        SMBV03_Leaf(0.30 1.60 0.14 -12)
        SMBV03_Leaf(1.08 1.22 -0.12 32)
        SMBV03_Flower(-1.02 1.82 0.06 0)
        SMBV03_Flower(0.06 2.04 -0.08 18)
        SMBV03_Flower(1.02 1.72 0.10 -14)
    ]
]

SMBV03_Leaf(x y z yaw) ->
[
    T(x y z) A(yaw 1) A(90 1) I(LeafBlade(profile(Lanceolate) length(0.58) width(0.20) curvature(0.18) camber(0.040) twist(10) thickness(0.010) widthPower(1.20) segments(18 5)) material(sagegreenmattepaint) 1 0.16)
]

SMBV03_Flower(x y z yaw) ->
[
    T(x y z) A(yaw 1) A(90 1) I(PetalBlade(profile(Obovate) length(0.42) width(0.24) curvature(0.22) camber(0.055) twist(-8) thickness(0.010) widthPower(0.92) segments(18 5)) material(redglossymetal) 1 0.14)
]
""",
    ),
    VegetationBuildingObject(
        object_id="SMB_VEGETATION_IVY_PRIVACY_SCREEN",
        object_class="WallClimbingIvyPrivacyScreen",
        title="Wall-Climbing Ivy Privacy Screen",
        category="Vine",
        grammar_name="SMBV04_WallClimbingIvyPrivacyScreen.grammar",
        design_intent=(
            "A freestanding slatted privacy screen with deterministic wall-seeking ivy, "
            "offset attachment behavior, a planted trough, and visible support feet."
        ),
        vegetation_primitives=("Vine", "LeafBlade"),
        generation_methods=("WallClimbing", "SurfaceSeeking", "OffsetAttachment"),
        grammar_source=r"""# SMBV04 Wall-Climbing Ivy Privacy Screen

Start -> SMB_VEGETATION_IVY_PRIVACY_SCREEN

SMB_VEGETATION_IVY_PRIVACY_SCREEN ->
[
    Object(
        id(SMB_VEGETATION_IVY_PRIVACY_SCREEN)
        class(WallClimbingIvyPrivacyScreen)
        taxonomy(SmallModernBuilding BuildingSite LandscapeSystem Vine WallClimbingIvyPrivacyScreen)
        layer(Terrain)
        mask(Structure Envelope Interior Furniture Plumbing Hvac Electrical Equipment Terrain Temporary)
    )
    [
        Interface(id(baseSupport) type(Support) origin(0 0 0) normal(0 1 0) tangent(1 0 0) region(Point) tolerance(0.001))
        Interface(id(screenConnection) type(Mate) origin(0 2.0 0) normal(0 0 -1) tangent(1 0 0) region(Point) tolerance(0.01))
        Interface(id(inspection) type(InspectionInterface) origin(0 2.0 0.30) normal(0 0 1) tangent(1 0 0) region(Point) tolerance(0.001))

        [ T(0 0.20 0) S(3.6 0.40 0.72) I(CubeY material(blackroughmetal) 1 0.18) ]
        [ T(-1.62 2.10 0) S(0.16 4.20 0.20) I(CubeY material(darkbrownroughwood) 1 0.18) ]
        [ T(1.62 2.10 0) S(0.16 4.20 0.20) I(CubeY material(darkbrownroughwood) 1 0.18) ]
        SMBV04_Slat(-1.28) SMBV04_Slat(-0.96) SMBV04_Slat(-0.64) SMBV04_Slat(-0.32)
        SMBV04_Slat(0) SMBV04_Slat(0.32) SMBV04_Slat(0.64) SMBV04_Slat(0.96) SMBV04_Slat(1.28)
        [ T(0 0.48 0.16) S(3.18 0.18 0.54) I(CubeY material(earthmaterial) 1 0.20) ]

        [ I(Vine(species(IvyVine) start(-1.28 0.52 0.18) direction(0.10 1.0 0.02) radius(0.040) mode(WallClimbing) collision(Seek) attachment(Offset) step(0.15) segments(32) seekDistance(2.0) attachDistance(0.035) tolerance(0.001) radiusDecay(0.965) minimumRadius(0.006) preferred(0 1 0) gamma(2.0) target(privacy_screen -1.48 0.42 -0.12 1.48 4.12 0.12) detail(LOD4)) material(sagegreenmattepaint) 1 0.16) ]
        [ I(Vine(species(IvyVine) start(0.18 0.52 0.18) direction(-0.08 1.0 0.02) radius(0.036) mode(WallClimbing) collision(Seek) attachment(Offset) step(0.14) segments(30) seekDistance(1.8) attachDistance(0.035) tolerance(0.001) radiusDecay(0.965) minimumRadius(0.005) preferred(0 1 0) gamma(2.0) target(privacy_screen -1.48 0.42 -0.12 1.48 4.12 0.12) detail(LOD4)) material(sagegreenmattepaint) 1 0.16) ]
        [ I(Vine(species(IvyVine) start(1.18 0.52 0.18) direction(-0.10 1.0 0.02) radius(0.034) mode(WallClimbing) collision(Seek) attachment(Offset) step(0.14) segments(28) seekDistance(1.8) attachDistance(0.035) tolerance(0.001) radiusDecay(0.965) minimumRadius(0.005) preferred(0 1 0) gamma(2.0) target(privacy_screen -1.48 0.42 -0.12 1.48 4.12 0.12) detail(LOD4)) material(sagegreenmattepaint) 1 0.16) ]
        SMBV04_Leaf(-1.18 1.42 0.28 -16)
        SMBV04_Leaf(-0.52 2.22 0.28 24)
        SMBV04_Leaf(0.22 3.08 0.28 -20)
        SMBV04_Leaf(0.88 2.48 0.28 18)
        SMBV04_Leaf(1.22 3.62 0.28 -28)
        SMBV04_FoliageLobe(-1.16 1.18 0.24 0.34 0.24 0.18)
        SMBV04_FoliageLobe(-1.02 1.86 0.24 0.38 0.26 0.18)
        SMBV04_FoliageLobe(-0.74 2.58 0.24 0.40 0.28 0.18)
        SMBV04_FoliageLobe(-0.42 3.30 0.24 0.38 0.26 0.18)
        SMBV04_FoliageLobe(0.10 1.32 0.24 0.34 0.24 0.18)
        SMBV04_FoliageLobe(0.28 2.06 0.24 0.40 0.28 0.18)
        SMBV04_FoliageLobe(0.48 2.86 0.24 0.38 0.26 0.18)
        SMBV04_FoliageLobe(0.88 1.52 0.24 0.34 0.24 0.18)
        SMBV04_FoliageLobe(1.06 2.36 0.24 0.40 0.28 0.18)
        SMBV04_FoliageLobe(1.20 3.22 0.24 0.38 0.26 0.18)
    ]
]

SMBV04_Slat(x) ->
[
    T(x 2.18 0) S(0.12 3.58 0.14) I(CubeY material(lightoakwood) 1 0.16)
]

SMBV04_Leaf(x y z yaw) ->
[
    T(x y z) A(yaw 1) A(90 1) I(LeafBlade(profile(Ovate) length(0.46) width(0.24) curvature(0.16) camber(0.034) twist(8) thickness(0.010) widthPower(1.04) segments(16 5)) material(sagegreenmattepaint) 1 0.15)
]

SMBV04_FoliageLobe(x y z sx sy sz) ->
[
    T(x y z) S(sx sy sz) I(Sphere material(sagegreenmattepaint) 1 0.18)
]
""",
    ),
    VegetationBuildingObject(
        object_id="SMB_VEGETATION_TENDRIL_PERGOLA_VINE",
        object_class="TendrilPergolaVine",
        title="Tendril Pergola Vine",
        category="Vine",
        grammar_name="SMBV05_TendrilPergolaVine.grammar",
        design_intent=(
            "A compact pergola bay with twining tendril vines, branching overhead "
            "growth, leaf blades, and flowering petals around a clear structural frame."
        ),
        vegetation_primitives=("Vine", "BranchJunction", "LeafBlade", "PetalBlade"),
        generation_methods=("TendrilClimbing", "TwineAttachment"),
        grammar_source=r"""# SMBV05 Tendril Pergola Vine

Start -> SMB_VEGETATION_TENDRIL_PERGOLA_VINE

SMB_VEGETATION_TENDRIL_PERGOLA_VINE ->
[
    Object(
        id(SMB_VEGETATION_TENDRIL_PERGOLA_VINE)
        class(TendrilPergolaVine)
        taxonomy(SmallModernBuilding BuildingSite LandscapeSystem Vine TendrilPergolaVine)
        layer(Terrain)
        mask(Structure Envelope Interior Furniture Plumbing Hvac Electrical Equipment Terrain Temporary)
    )
    [
        Interface(id(baseSupport) type(Support) origin(0 0 0) normal(0 1 0) tangent(1 0 0) region(Point) tolerance(0.001))
        Interface(id(pergolaConnection) type(Mate) origin(0 3.4 0) normal(0 1 0) tangent(1 0 0) region(Point) tolerance(0.01))
        Interface(id(inspection) type(InspectionInterface) origin(0 2.0 1.2) normal(0 0 1) tangent(1 0 0) region(Point) tolerance(0.001))

        [ T(-1.55 1.72 -0.92) S(0.18 3.44 0.18) I(CubeY material(darkbrownroughwood) 1 0.18) ]
        [ T(1.55 1.72 -0.92) S(0.18 3.44 0.18) I(CubeY material(darkbrownroughwood) 1 0.18) ]
        [ T(-1.55 1.72 0.92) S(0.18 3.44 0.18) I(CubeY material(darkbrownroughwood) 1 0.18) ]
        [ T(1.55 1.72 0.92) S(0.18 3.44 0.18) I(CubeY material(darkbrownroughwood) 1 0.18) ]
        [ T(0 3.46 -0.92) S(3.32 0.16 0.18) I(CubeY material(lightoakwood) 1 0.16) ]
        [ T(0 3.46 0.92) S(3.32 0.16 0.18) I(CubeY material(lightoakwood) 1 0.16) ]
        SMBV05_Rafter(-1.20) SMBV05_Rafter(-0.60) SMBV05_Rafter(0) SMBV05_Rafter(0.60) SMBV05_Rafter(1.20)

        [ I(Vine(species(IvyVine) start(-1.50 0.10 -0.86) direction(0.05 1.0 0.04) radius(0.042) mode(TendrilClimbing) collision(Seek) attachment(Twine) step(0.14) segments(38) seekDistance(1.8) attachDistance(0.025) tolerance(0.001) radiusDecay(0.972) minimumRadius(0.005) preferred(0 1 0) gamma(2.0) target(pergola_left -1.64 0 -1.02 -1.42 3.56 1.02) detail(LOD4)) material(sagegreenmattepaint) 1 0.16) ]
        [ I(Vine(species(IvyVine) start(1.50 0.10 0.86) direction(-0.05 1.0 -0.04) radius(0.042) mode(TendrilClimbing) collision(Seek) attachment(Twine) step(0.14) segments(38) seekDistance(1.8) attachDistance(0.025) tolerance(0.001) radiusDecay(0.972) minimumRadius(0.005) preferred(0 1 0) gamma(2.0) target(pergola_right 1.42 0 -1.02 1.64 3.56 1.02) detail(LOD4)) material(sagegreenmattepaint) 1 0.16) ]
        [ T(0 3.30 0) I(BranchJunction(core(0.10 1.18) parent(0 -1 0 0.11 0.20) child(0.72 0.24 0.26 0.055 0.42) child(-0.72 0.24 -0.26 0.055 0.42) segments(14)) material(sagegreenmattepaint) 1 0.14) ]
        SMBV05_Leaf(-1.30 1.28 -0.72 -18)
        SMBV05_Leaf(-1.22 2.38 0.42 22)
        SMBV05_Leaf(-0.58 3.52 -0.48 -12)
        SMBV05_Leaf(0.04 3.60 0.24 18)
        SMBV05_Leaf(0.72 3.54 -0.18 -24)
        SMBV05_Leaf(1.24 2.46 -0.38 16)
        SMBV05_Leaf(1.32 1.30 0.70 -20)
        SMBV05_Flower(-0.82 3.64 0.34 -18)
        SMBV05_Flower(0.02 3.72 -0.26 8)
        SMBV05_Flower(0.86 3.62 0.28 20)
        SMBV05_FoliageLobe(-1.34 1.14 -0.76 0.30 0.24 0.22)
        SMBV05_FoliageLobe(-1.28 2.02 0.48 0.34 0.26 0.22)
        SMBV05_FoliageLobe(-1.02 2.86 -0.38 0.36 0.28 0.22)
        SMBV05_FoliageLobe(-0.62 3.54 0.42 0.38 0.24 0.28)
        SMBV05_FoliageLobe(-0.06 3.62 -0.36 0.40 0.24 0.30)
        SMBV05_FoliageLobe(0.52 3.56 0.38 0.38 0.24 0.28)
        SMBV05_FoliageLobe(1.02 2.88 -0.42 0.36 0.28 0.22)
        SMBV05_FoliageLobe(1.28 2.04 0.46 0.34 0.26 0.22)
        SMBV05_FoliageLobe(1.34 1.16 -0.72 0.30 0.24 0.22)
    ]
]

SMBV05_Rafter(x) ->
[
    T(x 3.54 0) S(0.14 0.14 2.14) I(CubeY material(lightoakwood) 1 0.16)
]

SMBV05_Leaf(x y z yaw) ->
[
    T(x y z) A(yaw 1) A(90 1) I(LeafBlade(profile(Ovate) length(0.50) width(0.25) curvature(0.18) camber(0.038) twist(10) thickness(0.010) widthPower(1.02) segments(16 5)) material(sagegreenmattepaint) 1 0.15)
]

SMBV05_Flower(x y z yaw) ->
[
    T(x y z) A(yaw 1) A(90 1) I(PetalBlade(profile(Obovate) length(0.34) width(0.20) curvature(0.24) camber(0.050) twist(-10) thickness(0.009) widthPower(0.90) segments(16 5)) material(redglossymetal) 1 0.13)
]

SMBV05_FoliageLobe(x y z sx sy sz) ->
[
    T(x y z) S(sx sy sz) I(Sphere material(sagegreenmattepaint) 1 0.18)
]
""",
    ),
)


def object_declaration(
    object_id: str,
    object_class: str,
    category: str,
    interface_lines: tuple[str, ...],
    geometry_lines: tuple[str, ...],
    helper_rules: str,
) -> str:
    interfaces = "\n        ".join(interface_lines)
    geometry = "\n        ".join(geometry_lines)
    return f"""# Generated Simple Modern Building vegetation object.
# Object-local geometry uses Y-up coordinates and a five-degree comparison lens.

Start -> {object_id}

{object_id} ->
[
    Object(
        id({object_id})
        class({object_class})
        taxonomy(SmallModernBuilding BuildingSite LandscapeSystem {category} {object_class})
        layer(Terrain)
        mask(Structure Envelope Interior Furniture Plumbing Hvac Electrical Equipment Terrain Temporary)
    )
    [
        {interfaces}

        {geometry}
    ]
]

{helper_rules.strip()}
"""


def generated_tree_grammar(
    sequence: int,
    object_id: str,
    object_class: str,
) -> str:
    prefix = f"SMBV{sequence:02d}"
    seed = 510000 + sequence
    height = 4.4 + (sequence % 4) * 0.36
    spread = 1.25 + (sequence % 3) * 0.22
    crown_height = height - 1.15
    geometry = (
        f"[ I(TaperedSweep(samples(0 0 0 0.30  0.04 {height * 0.28:.3f} 0.03 0.25  -0.05 {height * 0.58:.3f} -0.02 0.18  0.02 {height:.3f} 0 0.075) up(0 0 1) segments(20) cap(all)) material(amberwarmwood) 1 0.18) ]",
        f"[ T(0 {height * 0.34:.3f} 0) I(BranchJunction(core(0.24 1.22) parent(0 -1 0 0.25 0.36) child(0.74 0.55 0.12 0.13 0.66) child(-0.70 0.58 -0.16 0.13 0.66) segments(18)) material(amberwarmwood) 1 0.18) ]",
        f"[ T(0 0.16 0) S({1.35 + (sequence % 2) * 0.35:.2f} 1.0 {1.35 + ((sequence + 1) % 2) * 0.35:.2f}) I(Plant(species(Shrub) age(7) state(Mature) seed({seed}) generation(LSystem) lAxiom(F) lRule(F F Push Plus F Pop F Push Minus F Pop F) lSystem(3 {22 + sequence % 8} 0.16 0.052 0.006 2) phyllotaxis(Alternate 137.50776 0.10 1 0 0 0.16) tropism(Up 0 1 0 0.018) detail(LOD3)) material(sagegreenmattepaint) 1 0.16) ]",
        f"{prefix}_Crown(-{spread:.3f} {crown_height:.3f} -0.28 0.82 0.58 0.72)",
        f"{prefix}_Crown({spread:.3f} {crown_height + 0.12:.3f} 0.30 0.84 0.60 0.74)",
        f"{prefix}_Crown(-0.62 {crown_height + 0.72:.3f} 0.54 0.88 0.62 0.76)",
        f"{prefix}_Crown(0.66 {crown_height + 0.78:.3f} -0.58 0.90 0.64 0.78)",
        f"{prefix}_Crown(0 {crown_height + 1.18:.3f} 0 1.05 0.72 0.90)",
        f"{prefix}_Leaf(-{spread + 0.18:.3f} {crown_height + 0.12:.3f} -0.18 -24)",
        f"{prefix}_Leaf({spread + 0.18:.3f} {crown_height + 0.24:.3f} 0.20 28)",
        f"{prefix}_Leaf(-0.58 {crown_height + 1.12:.3f} 0.48 46)",
        f"{prefix}_Leaf(0.60 {crown_height + 1.18:.3f} -0.50 -42)",
    )
    helpers = f"""
{prefix}_Crown(x y z sx sy sz) ->
[
    T(x y z) S(sx sy sz) I(Sphere material(sagegreenmattepaint) 1 0.20)
]

{prefix}_Leaf(x y z yaw) ->
[
    T(x y z) A(yaw 1) A(90 1) I(LeafBlade(profile(Ovate) length(0.62) width(0.28) curvature(0.16) camber(0.034) twist(10) thickness(0.010) widthPower(1.06) segments(18 5)) material(sagegreenmattepaint) 1 0.15)
]
"""
    return object_declaration(
        object_id,
        object_class,
        "Tree",
        (
            "Interface(id(rootSupport) type(Support) origin(0 0 0) normal(0 1 0) tangent(1 0 0) region(Point) tolerance(0.001))",
            f"Interface(id(inspection) type(InspectionInterface) origin(0 {height * 0.58:.3f} 0) normal(0 0 1) tangent(1 0 0) region(Point) tolerance(0.001))",
        ),
        geometry,
        helpers,
    )


def generated_shrub_grammar(
    sequence: int,
    object_id: str,
    object_class: str,
) -> str:
    prefix = f"SMBV{sequence:02d}"
    seed = 520000 + sequence
    width = 2.3 + (sequence % 4) * 0.25
    plant_scale = 1.35 + (sequence % 3) * 0.20
    geometry = (
        f"[ T(0 0.10 0) S({width:.2f} 0.20 1.35) I(CubeY material(earthmaterial) 1 0.20) ]",
        f"[ T(-0.72 0.20 0.08) S({plant_scale:.2f} {plant_scale:.2f} {plant_scale:.2f}) I(Plant(species(Shrub) age({4 + sequence % 5}) state(Flowering) seed({seed}) detail(LOD4)) material(sagegreenmattepaint) 1 0.16) ]",
        f"[ T(0.04 0.20 -0.10) S({plant_scale + 0.18:.2f} {plant_scale + 0.18:.2f} {plant_scale + 0.18:.2f}) I(Plant(species(Shrub) age({5 + sequence % 4}) state(Mature) seed({seed + 1}) detail(LOD3)) material(sagegreenmattepaint) 1 0.16) ]",
        f"[ T(0.78 0.20 0.10) S({plant_scale - 0.08:.2f} {plant_scale - 0.08:.2f} {plant_scale - 0.08:.2f}) I(Plant(species(Shrub) age({3 + sequence % 6}) state(Flowering) seed({seed + 2}) detail(LOD4)) material(sagegreenmattepaint) 1 0.16) ]",
        f"{prefix}_Lobe(-0.78 1.02 0.04 0.66 0.52 0.58)",
        f"{prefix}_Lobe(0 1.20 -0.08 0.78 0.58 0.66)",
        f"{prefix}_Lobe(0.80 0.98 0.10 0.64 0.50 0.56)",
        f"{prefix}_Leaf(-1.08 1.18 0.12 -28)",
        f"{prefix}_Leaf(0.02 1.58 -0.06 18)",
        f"{prefix}_Leaf(1.08 1.16 0.10 30)",
        f"{prefix}_Flower(-0.54 1.62 0.08 -12)",
        f"{prefix}_Flower(0.58 1.54 -0.04 16)",
    )
    helpers = f"""
{prefix}_Lobe(x y z sx sy sz) ->
[
    T(x y z) S(sx sy sz) I(Sphere material(sagegreenmattepaint) 1 0.20)
]

{prefix}_Leaf(x y z yaw) ->
[
    T(x y z) A(yaw 1) A(90 1) I(LeafBlade(profile(Ovate) length(0.52) width(0.25) curvature(0.18) camber(0.036) twist(8) thickness(0.010) widthPower(1.05) segments(16 5)) material(sagegreenmattepaint) 1 0.15)
]

{prefix}_Flower(x y z yaw) ->
[
    T(x y z) A(yaw 1) A(90 1) I(PetalBlade(profile(Obovate) length(0.34) width(0.20) curvature(0.22) camber(0.050) twist(-8) thickness(0.009) widthPower(0.92) segments(16 5)) material(redglossymetal) 1 0.13)
]
"""
    return object_declaration(
        object_id,
        object_class,
        "Shrub",
        (
            "Interface(id(baseSupport) type(Support) origin(0 0 0) normal(0 1 0) tangent(1 0 0) region(Point) tolerance(0.001))",
            "Interface(id(irrigationConnection) type(ServiceConnection) origin(0 0.18 -0.60) normal(0 0 -1) tangent(1 0 0) region(Point) tolerance(0.01))",
            "Interface(id(inspection) type(InspectionInterface) origin(0 1.0 0) normal(0 0 1) tangent(1 0 0) region(Point) tolerance(0.001))",
        ),
        geometry,
        helpers,
    )


def generated_grass_grammar(
    sequence: int,
    object_id: str,
    object_class: str,
) -> str:
    prefix = f"SMBV{sequence:02d}"
    seed = 530000 + sequence
    blade_height = 0.82 + (sequence % 5) * 0.18
    spread = 1.20 + (sequence % 3) * 0.16
    density = 0.42 + (sequence % 4) * 0.08
    geometry = (
        f"[ T(0 0.08 0) S(3.0 0.16 1.55) I(CubeY material(earthmaterial) 1 0.20) ]",
        f"[ I(ScatterRegion(region(grass_{sequence:02d}) species(GrassClump) seed({seed}) surface(grass_soil -1.38 0.14 -0.62 1.38 0.18 0.62) face(MaximumY) density({density:.2f}) separation({0.26 + (sequence % 3) * 0.04:.2f}) scale({0.55 + (sequence % 3) * 0.08:.2f} {0.92 + (sequence % 4) * 0.08:.2f}) orientation(WorldUpRandomAzimuth) collisionRadius(0.055) layer(Terrain) mask(Structure Furniture) detail(LOD3)) material(sagegreenmattepaint) 1 0.15) ]",
        f"[ T(-0.78 0.16 0.06) S({1.15 + (sequence % 3) * 0.12:.2f} {1.15 + (sequence % 3) * 0.12:.2f} {1.15 + (sequence % 3) * 0.12:.2f}) I(Plant(species(GrassClump) age({0.7 + (sequence % 4) * 0.2:.2f}) state(Mature) seed({seed + 1}) detail(LOD3)) material(sagegreenmattepaint) 1 0.15) ]",
        f"[ T(0.02 0.16 -0.04) S({1.35 + (sequence % 2) * 0.16:.2f} {1.35 + (sequence % 2) * 0.16:.2f} {1.35 + (sequence % 2) * 0.16:.2f}) I(Plant(species(GrassClump) age({0.9 + (sequence % 3) * 0.2:.2f}) state(Mature) seed({seed + 2}) detail(LOD3)) material(sagegreenmattepaint) 1 0.15) ]",
        f"[ T(0.82 0.16 0.08) S({1.10 + (sequence % 4) * 0.10:.2f} {1.10 + (sequence % 4) * 0.10:.2f} {1.10 + (sequence % 4) * 0.10:.2f}) I(Plant(species(GrassClump) age({0.8 + (sequence % 5) * 0.16:.2f}) state(Mature) seed({seed + 3}) detail(LOD3)) material(sagegreenmattepaint) 1 0.15) ]",
        f"{prefix}_Blade(-{spread:.2f} 0.18 -0.18 -24 {blade_height:.2f})",
        f"{prefix}_Blade(-0.76 0.18 0.18 18 {blade_height + 0.14:.2f})",
        f"{prefix}_Blade(-0.26 0.18 -0.10 -12 {blade_height + 0.28:.2f})",
        f"{prefix}_Blade(0.24 0.18 0.14 14 {blade_height + 0.20:.2f})",
        f"{prefix}_Blade(0.72 0.18 -0.16 -18 {blade_height + 0.10:.2f})",
        f"{prefix}_Blade({spread:.2f} 0.18 0.18 26 {blade_height - 0.04:.2f})",
    )
    helpers = f"""
{prefix}_Blade(x y z yaw height) ->
[
    T(x y z) A(yaw 1) A(90 1) I(LeafBlade(profile(Lanceolate) length(height) width(0.10) curvature(0.22) camber(0.030) twist(14) thickness(0.008) widthPower(1.42) segments(22 5)) material(sagegreenmattepaint) 1 0.13)
]
"""
    return object_declaration(
        object_id,
        object_class,
        "Grass",
        (
            "Interface(id(baseSupport) type(Support) origin(0 0 0) normal(0 1 0) tangent(1 0 0) region(Point) tolerance(0.001))",
            "Interface(id(irrigationConnection) type(ServiceConnection) origin(0 0.12 -0.68) normal(0 0 -1) tangent(1 0 0) region(Point) tolerance(0.01))",
            "Interface(id(inspection) type(InspectionInterface) origin(0 0.85 0) normal(0 0 1) tangent(1 0 0) region(Point) tolerance(0.001))",
        ),
        geometry,
        helpers,
    )


def generated_groundcover_grammar(
    sequence: int,
    object_id: str,
    object_class: str,
) -> str:
    prefix = f"SMBV{sequence:02d}"
    seed = 540000 + sequence
    geometry = (
        "[ T(0 0.06 0) S(3.2 0.12 2.10) I(CubeY material(earthmaterial) 1 0.20) ]",
        f"[ I(ScatterRegion(region(groundcover_{sequence:02d}) species(FloweringHerb) seed({seed}) surface(groundcover_soil -1.50 0.10 -0.96 1.50 0.14 0.96) face(MaximumY) density({0.66 + (sequence % 3) * 0.08:.2f}) separation({0.22 + (sequence % 2) * 0.04:.2f}) scale(0.34 {0.52 + (sequence % 3) * 0.08:.2f}) orientation(WorldUpRandomAzimuth) collisionRadius(0.045) layer(Terrain) mask(Structure Furniture) detail(LOD3)) material(sagegreenmattepaint) 1 0.14) ]",
        f"{prefix}_Leaf(-1.12 0.30 -0.48 -26)",
        f"{prefix}_Leaf(-0.62 0.38 0.40 18)",
        f"{prefix}_Leaf(0 0.44 -0.24 -8)",
        f"{prefix}_Leaf(0.66 0.36 0.46 22)",
        f"{prefix}_Leaf(1.18 0.30 -0.42 -24)",
        f"{prefix}_Flower(-0.82 0.52 0.20 -12)",
        f"{prefix}_Flower(0.12 0.58 -0.18 10)",
        f"{prefix}_Flower(0.92 0.50 0.24 18)",
    )
    helpers = f"""
{prefix}_Leaf(x y z yaw) ->
[
    T(x y z) A(yaw 1) A(90 1) I(LeafBlade(profile(Ovate) length(0.42) width(0.22) curvature(0.18) camber(0.032) twist(8) thickness(0.008) widthPower(1.08) segments(16 5)) material(sagegreenmattepaint) 1 0.13)
]

{prefix}_Flower(x y z yaw) ->
[
    T(x y z) A(yaw 1) A(90 1) I(PetalBlade(profile(Obovate) length(0.26) width(0.16) curvature(0.22) camber(0.045) twist(-8) thickness(0.008) widthPower(0.92) segments(14 5)) material(redglossymetal) 1 0.12)
]
"""
    return object_declaration(
        object_id,
        object_class,
        "Groundcover",
        (
            "Interface(id(baseSupport) type(Support) origin(0 0 0) normal(0 1 0) tangent(1 0 0) region(Point) tolerance(0.001))",
            "Interface(id(irrigationConnection) type(ServiceConnection) origin(0 0.10 -0.98) normal(0 0 -1) tangent(1 0 0) region(Point) tolerance(0.01))",
            "Interface(id(inspection) type(InspectionInterface) origin(0 0.38 0) normal(0 0 1) tangent(1 0 0) region(Point) tolerance(0.001))",
        ),
        geometry,
        helpers,
    )


def generated_vine_grammar(
    sequence: int,
    object_id: str,
    object_class: str,
) -> str:
    prefix = f"SMBV{sequence:02d}"
    seed_offset = sequence % 3
    mode = "WallClimbing" if sequence % 2 == 0 else "TendrilClimbing"
    attachment = "Offset" if mode == "WallClimbing" else "Twine"
    geometry = (
        "[ T(-1.55 2.05 0) S(0.16 4.10 0.18) I(CubeY material(darkbrownroughwood) 1 0.17) ]",
        "[ T(1.55 2.05 0) S(0.16 4.10 0.18) I(CubeY material(darkbrownroughwood) 1 0.17) ]",
        f"{prefix}_Slat(-1.20) {prefix}_Slat(-0.80) {prefix}_Slat(-0.40) {prefix}_Slat(0) {prefix}_Slat(0.40) {prefix}_Slat(0.80) {prefix}_Slat(1.20)",
        f"[ I(Vine(species(IvyVine) start(-1.28 0.12 0.18) direction(0.08 1.0 0.02) radius(0.038) mode({mode}) collision(Seek) attachment({attachment}) step(0.14) segments({30 + seed_offset * 3}) seekDistance(1.8) attachDistance(0.030) tolerance(0.001) radiusDecay(0.968) minimumRadius(0.005) preferred(0 1 0) gamma(2.0) target(vine_support -1.46 0 -0.12 1.46 4.10 0.12) detail(LOD4)) material(sagegreenmattepaint) 1 0.15) ]",
        f"[ I(Vine(species(IvyVine) start(0.02 0.12 0.18) direction(-0.06 1.0 0.02) radius(0.035) mode({mode}) collision(Seek) attachment({attachment}) step(0.14) segments({28 + seed_offset * 3}) seekDistance(1.8) attachDistance(0.030) tolerance(0.001) radiusDecay(0.968) minimumRadius(0.005) preferred(0 1 0) gamma(2.0) target(vine_support -1.46 0 -0.12 1.46 4.10 0.12) detail(LOD4)) material(sagegreenmattepaint) 1 0.15) ]",
        f"[ I(Vine(species(IvyVine) start(1.18 0.12 0.18) direction(-0.08 1.0 0.02) radius(0.034) mode({mode}) collision(Seek) attachment({attachment}) step(0.14) segments({26 + seed_offset * 3}) seekDistance(1.8) attachDistance(0.030) tolerance(0.001) radiusDecay(0.968) minimumRadius(0.005) preferred(0 1 0) gamma(2.0) target(vine_support -1.46 0 -0.12 1.46 4.10 0.12) detail(LOD4)) material(sagegreenmattepaint) 1 0.15) ]",
        f"{prefix}_Leaf(-1.12 1.20 0.24 -24)",
        f"{prefix}_Leaf(-0.72 2.08 0.24 18)",
        f"{prefix}_Leaf(-0.20 3.02 0.24 -12)",
        f"{prefix}_Leaf(0.34 1.54 0.24 16)",
        f"{prefix}_Leaf(0.82 2.48 0.24 -20)",
        f"{prefix}_Leaf(1.18 3.32 0.24 24)",
    )
    helpers = f"""
{prefix}_Slat(x) ->
[
    T(x 2.12 0) S(0.10 3.62 0.12) I(CubeY material(lightoakwood) 1 0.15)
]

{prefix}_Leaf(x y z yaw) ->
[
    T(x y z) A(yaw 1) A(90 1) I(LeafBlade(profile(Ovate) length(0.48) width(0.24) curvature(0.16) camber(0.034) twist(8) thickness(0.010) widthPower(1.04) segments(16 5)) material(sagegreenmattepaint) 1 0.14)
]
"""
    return object_declaration(
        object_id,
        object_class,
        "Vine",
        (
            "Interface(id(baseSupport) type(Support) origin(0 0 0) normal(0 1 0) tangent(1 0 0) region(Point) tolerance(0.001))",
            "Interface(id(facadeAttachment) type(Mate) origin(0 2.0 0) normal(0 0 -1) tangent(1 0 0) region(Point) tolerance(0.01))",
            "Interface(id(inspection) type(InspectionInterface) origin(0 2.0 0.28) normal(0 0 1) tangent(1 0 0) region(Point) tolerance(0.001))",
        ),
        geometry,
        helpers,
    )


def generated_planter_grammar(
    sequence: int,
    object_id: str,
    object_class: str,
) -> str:
    prefix = f"SMBV{sequence:02d}"
    seed = 560000 + sequence
    species = ("Shrub", "GrassClump", "Shrub", "FloweringHerb")[sequence % 4]
    geometry = (
        "[ T(0 0.42 0) S(1.65 0.84 1.65) I(Cylinder material(blackroughmetal) 1 0.18) ]",
        "[ T(0 0.82 0) S(1.42 0.12 1.42) I(Cylinder material(earthmaterial) 1 0.20) ]",
        f"[ T(0 0.86 0) S({2.2 + (sequence % 3) * 0.28:.2f} {2.2 + (sequence % 3) * 0.28:.2f} {2.2 + (sequence % 3) * 0.28:.2f}) I(Plant(species({species}) age({1.0 + (sequence % 5) * 0.6:.2f}) state(Flowering) seed({seed}) detail(LOD4)) material(sagegreenmattepaint) 1 0.16) ]",
        f"{prefix}_Leaf(-0.52 1.42 0.20 -22)",
        f"{prefix}_Leaf(0 1.72 -0.16 10)",
        f"{prefix}_Leaf(0.54 1.44 0.18 24)",
        f"{prefix}_Flower(-0.34 2.02 0.10 -12)",
        f"{prefix}_Flower(0.38 1.94 -0.08 14)",
    )
    helpers = f"""
{prefix}_Leaf(x y z yaw) ->
[
    T(x y z) A(yaw 1) A(90 1) I(LeafBlade(profile(Lanceolate) length(0.58) width(0.22) curvature(0.18) camber(0.038) twist(10) thickness(0.010) widthPower(1.16) segments(18 5)) material(sagegreenmattepaint) 1 0.14)
]

{prefix}_Flower(x y z yaw) ->
[
    T(x y z) A(yaw 1) A(90 1) I(PetalBlade(profile(Obovate) length(0.30) width(0.18) curvature(0.22) camber(0.048) twist(-8) thickness(0.008) widthPower(0.92) segments(14 5)) material(redglossymetal) 1 0.12)
]
"""
    return object_declaration(
        object_id,
        object_class,
        "Plant",
        (
            "Interface(id(baseSupport) type(Support) origin(0 0 0) normal(0 1 0) tangent(1 0 0) region(Point) tolerance(0.001))",
            "Interface(id(irrigationConnection) type(ServiceConnection) origin(0 0.48 -0.78) normal(0 0 -1) tangent(1 0 0) region(Point) tolerance(0.01))",
            "Interface(id(inspection) type(InspectionInterface) origin(0 1.45 0) normal(0 0 1) tangent(1 0 0) region(Point) tolerance(0.001))",
        ),
        geometry,
        helpers,
    )


def generated_ecological_grammar(
    sequence: int,
    object_id: str,
    object_class: str,
) -> str:
    prefix = f"SMBV{sequence:02d}"
    seed = 570000 + sequence
    species = "GrassClump" if sequence % 2 == 0 else "FloweringHerb"
    geometry = (
        "[ T(0 0.12 0) S(3.6 0.24 2.60) I(CubeY material(blackroughmetal) 1 0.18) ]",
        "[ T(0 0.28 0) S(3.28 0.10 2.28) I(CubeY material(earthmaterial) 1 0.20) ]",
        f"[ I(ScatterRegion(region(ecological_{sequence:02d}) species({species}) seed({seed}) surface(ecological_soil -1.52 0.30 -1.02 1.52 0.36 1.02) face(MaximumY) density({0.46 + (sequence % 4) * 0.08:.2f}) separation({0.28 + (sequence % 3) * 0.05:.2f}) scale(0.52 {0.92 + (sequence % 3) * 0.12:.2f}) orientation(WorldUpRandomAzimuth) collisionRadius(0.060) layer(Terrain) mask(Structure Furniture) detail(LOD3)) material(sagegreenmattepaint) 1 0.14) ]",
        f"[ T(-0.72 0.34 0.12) S(1.35 1.35 1.35) I(Plant(species({species}) age(1.4) state(Flowering) seed({seed + 1}) detail(LOD3)) material(sagegreenmattepaint) 1 0.15) ]",
        f"[ T(0.72 0.34 -0.12) S(1.20 1.20 1.20) I(Plant(species({species}) age(1.1) state(Mature) seed({seed + 2}) detail(LOD3)) material(sagegreenmattepaint) 1 0.15) ]",
        f"{prefix}_Leaf(-1.18 0.74 -0.44 -22)",
        f"{prefix}_Leaf(-0.34 1.02 0.36 14)",
        f"{prefix}_Leaf(0.42 0.92 -0.30 -12)",
        f"{prefix}_Leaf(1.16 0.70 0.42 24)",
        f"{prefix}_Flower(-0.62 1.18 0.12 -10)",
        f"{prefix}_Flower(0.70 1.12 -0.10 12)",
    )
    helpers = f"""
{prefix}_Leaf(x y z yaw) ->
[
    T(x y z) A(yaw 1) A(90 1) I(LeafBlade(profile(Lanceolate) length(0.68) width(0.20) curvature(0.20) camber(0.036) twist(12) thickness(0.009) widthPower(1.24) segments(18 5)) material(sagegreenmattepaint) 1 0.14)
]

{prefix}_Flower(x y z yaw) ->
[
    T(x y z) A(yaw 1) A(90 1) I(PetalBlade(profile(Obovate) length(0.30) width(0.18) curvature(0.24) camber(0.050) twist(-10) thickness(0.008) widthPower(0.90) segments(14 5)) material(redglossymetal) 1 0.12)
]
"""
    return object_declaration(
        object_id,
        object_class,
        "EcologicalPlanting",
        (
            "Interface(id(baseSupport) type(Support) origin(0 0 0) normal(0 1 0) tangent(1 0 0) region(Point) tolerance(0.001))",
            "Interface(id(stormwaterConnection) type(ServiceConnection) origin(0 0.24 -1.18) normal(0 0 -1) tangent(1 0 0) region(Point) tolerance(0.01))",
            "Interface(id(inspection) type(InspectionInterface) origin(0 0.84 0) normal(0 0 1) tangent(1 0 0) region(Point) tolerance(0.001))",
        ),
        geometry,
        helpers,
    )


ADDITIONAL_OBJECT_DEFINITIONS = (
    ("SMB_VEGETATION_COLUMNAR_ENTRY_TREE", "ColumnarEntryTree", "Columnar Entry Tree", "Tree", "tree"),
    ("SMB_VEGETATION_MULTI_STEM_BIRCH_TREE", "MultiStemBirchTree", "Multi-Stem Birch Tree", "Tree", "tree"),
    ("SMB_VEGETATION_OLIVE_COURTYARD_TREE", "OliveCourtyardTree", "Olive Courtyard Tree", "Tree", "tree"),
    ("SMB_VEGETATION_PALM_ACCENT_TREE", "PalmAccentTree", "Palm Accent Tree", "Tree", "tree"),
    ("SMB_VEGETATION_FLOWERING_ORNAMENTAL_TREE", "FloweringOrnamentalTree", "Flowering Ornamental Tree", "Tree", "tree"),
    ("SMB_VEGETATION_EVERGREEN_SCREEN_TREE", "EvergreenScreenTree", "Evergreen Screen Tree", "Tree", "tree"),
    ("SMB_VEGETATION_BONSAI_COURTYARD_TREE", "BonsaiCourtyardTree", "Bonsai Courtyard Tree", "Tree", "tree"),
    ("SMB_VEGETATION_RAIN_GARDEN_CANOPY_TREE", "RainGardenCanopyTree", "Rain Garden Canopy Tree", "Tree", "tree"),
    ("SMB_VEGETATION_CLIPPED_BOX_HEDGE", "ClippedBoxHedge", "Clipped Box Hedge", "Shrub", "shrub"),
    ("SMB_VEGETATION_FLOWERING_HYDRANGEA_SHRUB", "FloweringHydrangeaShrub", "Flowering Hydrangea Shrub", "Shrub", "shrub"),
    ("SMB_VEGETATION_SCULPTURAL_AGAVE_CLUSTER", "SculpturalAgaveCluster", "Sculptural Agave Cluster", "Shrub", "shrub"),
    ("SMB_VEGETATION_FERN_UNDERSTOREY_CLUSTER", "FernUnderstoreyCluster", "Fern Understorey Cluster", "Shrub", "shrub"),
    ("SMB_VEGETATION_BAMBOO_SCREEN_CLUSTER", "BambooScreenCluster", "Bamboo Screen Cluster", "Shrub", "shrub"),
    ("SMB_VEGETATION_SUCCULENT_ROCKERY_CLUSTER", "SucculentRockeryCluster", "Succulent Rockery Cluster", "Shrub", "shrub"),
    ("SMB_VEGETATION_TOPIARY_SPHERE_SHRUB", "TopiarySphereShrub", "Topiary Sphere Shrub", "Shrub", "shrub"),
    ("SMB_VEGETATION_NATIVE_PRIVACY_SHRUB", "NativePrivacyShrub", "Native Privacy Shrub", "Shrub", "shrub"),
    ("SMB_VEGETATION_FEATHER_REED_GRASS", "FeatherReedGrass", "Feather Reed Grass", "Grass", "grass"),
    ("SMB_VEGETATION_FOUNTAIN_GRASS", "FountainGrass", "Fountain Grass", "Grass", "grass"),
    ("SMB_VEGETATION_BLUE_FESCUE_GRASS", "BlueFescueGrass", "Blue Fescue Grass", "Grass", "grass"),
    ("SMB_VEGETATION_LOMANDRA_GRASS", "LomandraGrass", "Lomandra Grass", "Grass", "grass"),
    ("SMB_VEGETATION_MISCANTHUS_GRASS", "MiscanthusGrass", "Miscanthus Grass", "Grass", "grass"),
    ("SMB_VEGETATION_PAMPAS_GRASS", "PampasGrass", "Pampas Grass", "Grass", "grass"),
    ("SMB_VEGETATION_MONDO_GRASS", "MondoGrass", "Mondo Grass", "Grass", "grass"),
    ("SMB_VEGETATION_SWITCHGRASS", "Switchgrass", "Switchgrass", "Grass", "grass"),
    ("SMB_VEGETATION_ARCHITECTURAL_SEDGE", "ArchitecturalSedge", "Architectural Sedge", "Grass", "grass"),
    ("SMB_VEGETATION_DWARF_BAMBOO_GRASS", "DwarfBambooGrass", "Dwarf Bamboo Grass", "Grass", "grass"),
    ("SMB_VEGETATION_CREEPING_THYME_GROUNDCOVER", "CreepingThymeGroundcover", "Creeping Thyme Groundcover", "Groundcover", "groundcover"),
    ("SMB_VEGETATION_DICHONDRA_SILVER_GROUNDCOVER", "DichondraSilverGroundcover", "Dichondra Silver Groundcover", "Groundcover", "groundcover"),
    ("SMB_VEGETATION_IVY_GROUNDCOVER", "IvyGroundcover", "Ivy Groundcover", "Groundcover", "groundcover"),
    ("SMB_VEGETATION_FLOWERING_MEADOW_MAT", "FloweringMeadowMat", "Flowering Meadow Mat", "Groundcover", "groundcover"),
    ("SMB_VEGETATION_MOSS_FERN_CARPET", "MossFernCarpet", "Moss and Fern Carpet", "Groundcover", "groundcover"),
    ("SMB_VEGETATION_JASMINE_WALL_CLIMBER", "JasmineWallClimber", "Jasmine Wall Climber", "Vine", "vine"),
    ("SMB_VEGETATION_WISTERIA_PERGOLA", "WisteriaPergola", "Wisteria Pergola", "Vine", "vine"),
    ("SMB_VEGETATION_BOUGAINVILLEA_TRELLIS", "BougainvilleaTrellis", "Bougainvillea Trellis", "Vine", "vine"),
    ("SMB_VEGETATION_GRAPE_ARBOR_VINE", "GrapeArborVine", "Grape Arbor Vine", "Vine", "vine"),
    ("SMB_VEGETATION_GREEN_FACADE_CABLE_VINE", "GreenFacadeCableVine", "Green Facade Cable Vine", "Vine", "vine"),
    ("SMB_VEGETATION_INDOOR_FICUS_PLANTER", "IndoorFicusPlanter", "Indoor Ficus Planter", "Plant", "planter"),
    ("SMB_VEGETATION_SNAKE_PLANT_TROUGH", "SnakePlantTrough", "Snake Plant Trough", "Plant", "planter"),
    ("SMB_VEGETATION_HANGING_FERN_PLANTER", "HangingFernPlanter", "Hanging Fern Planter", "Plant", "planter"),
    ("SMB_VEGETATION_KITCHEN_HERB_PLANTER", "KitchenHerbPlanter", "Kitchen Herb Planter", "Plant", "planter"),
    ("SMB_VEGETATION_RAIN_GARDEN_REED_BED", "RainGardenReedBed", "Rain Garden Reed Bed", "EcologicalPlanting", "ecological"),
    ("SMB_VEGETATION_POOLSIDE_TROPICAL_CLUSTER", "PoolsideTropicalCluster", "Poolside Tropical Cluster", "EcologicalPlanting", "ecological"),
    ("SMB_VEGETATION_BIOSWALE_SEDGE_MODULE", "BioswaleSedgeModule", "Bioswale Sedge Module", "EcologicalPlanting", "ecological"),
    ("SMB_VEGETATION_AQUATIC_LILY_PLANTER", "AquaticLilyPlanter", "Aquatic Lily Planter", "EcologicalPlanting", "ecological"),
    ("SMB_VEGETATION_GREEN_ROOF_MODULE", "GreenRoofVegetationModule", "Green Roof Vegetation Module", "EcologicalPlanting", "ecological"),
)


def create_additional_object(
    sequence: int,
    definition: tuple[str, str, str, str, str],
) -> VegetationBuildingObject:
    object_id, object_class, title, category, template_name = definition
    grammar_factory = {
        "tree": generated_tree_grammar,
        "shrub": generated_shrub_grammar,
        "grass": generated_grass_grammar,
        "groundcover": generated_groundcover_grammar,
        "vine": generated_vine_grammar,
        "planter": generated_planter_grammar,
        "ecological": generated_ecological_grammar,
    }[template_name]
    primitive_families = {
        "tree": ("Plant", "TaperedSweep", "BranchJunction", "LeafBlade"),
        "shrub": ("Plant", "LeafBlade", "PetalBlade"),
        "grass": ("Plant", "ScatterRegion", "LeafBlade"),
        "groundcover": ("ScatterRegion", "LeafBlade", "PetalBlade"),
        "vine": ("Vine", "LeafBlade"),
        "planter": ("Plant", "LeafBlade", "PetalBlade"),
        "ecological": ("Plant", "ScatterRegion", "LeafBlade", "PetalBlade"),
    }[template_name]
    generation_methods = {
        "tree": ("LSystem", "ExplicitCrownComposition"),
        "shrub": ("SpeciesDriven", "ExplicitFoliageComposition"),
        "grass": ("SpeciesDriven", "DeterministicScatter"),
        "groundcover": ("DeterministicScatter",),
        "vine": ("GuidedClimbing", "SurfaceAttachment"),
        "planter": ("SpeciesDriven",),
        "ecological": ("SpeciesDriven", "DeterministicScatter"),
    }[template_name]
    expected_extents = {
        "tree": (4.8, 6.2, 4.2),
        "shrub": (3.2, 2.4, 2.2),
        "grass": (3.2, 2.4, 2.0),
        "groundcover": (3.4, 0.9, 2.4),
        "vine": (3.6, 4.5, 1.2),
        "planter": (2.4, 3.4, 2.2),
        "ecological": (4.0, 2.6, 3.0),
    }[template_name]
    return VegetationBuildingObject(
        object_id=object_id,
        object_class=object_class,
        title=title,
        category=category,
        grammar_name=f"SMBV{sequence:02d}_{object_class}.grammar",
        design_intent=(
            f"A distinct {title.lower()} building-landscape object with explicit support, "
            "irrigation or attachment semantics and object-local botanical geometry."
        ),
        vegetation_primitives=primitive_families,
        generation_methods=generation_methods,
        grammar_source=grammar_factory(sequence, object_id, object_class),
        expected_extent_xyz=expected_extents,
    )


OBJECTS = OBJECTS + tuple(
    create_additional_object(sequence, definition)
    for sequence, definition in enumerate(ADDITIONAL_OBJECT_DEFINITIONS, start=6)
)


def sha256_bytes(content: bytes) -> str:
    return hashlib.sha256(content).hexdigest()


def parse_connection_points(grammar_source: str) -> list[dict[str, object]]:
    interface_pattern = re.compile(
        r"Interface\(id\(([^)]+)\)\s+type\(([^)]+)\)\s+"
        r"origin\(([^)]+)\)\s+normal\(([^)]+)\)\s+"
        r"tangent\(([^)]+)\)\s+region\(([^)]+)\)\s+"
        r"tolerance\(([^)]+)\)\)"
    )
    purposes = {
        "rootSupport": "RootSeat",
        "baseSupport": "Support",
        "irrigationConnection": "WaterService",
        "stormwaterConnection": "DrainageService",
        "facadeAttachment": "PrimaryAttachment",
        "pergolaConnection": "PrimaryAttachment",
        "screenConnection": "PrimaryAttachment",
        "inspection": "Inspection",
    }
    connection_points = []
    for match in interface_pattern.finditer(grammar_source):
        connection_point_id, interface_type, origin, normal, tangent, region, tolerance = (
            match.groups()
        )
        connection_points.append({
            "connection_point_id": connection_point_id,
            "purpose": purposes.get(connection_point_id, "SpatialReference"),
            "interface_type": interface_type,
            "origin_xyz": [float(value) for value in origin.split()],
            "normal_xyz": [float(value) for value in normal.split()],
            "tangent_xyz": [float(value) for value in tangent.split()],
            "region": region,
            "tolerance": float(tolerance),
            "connectable": connection_point_id != "inspection",
        })
    if not connection_points:
        raise RuntimeError("Vegetation grammar must author at least one Interface")
    return connection_points


def graph_memberships(connection_points: list[dict[str, object]]) -> list[str]:
    memberships = {
        "landscape_containment_graph",
        "vegetation_growth_graph",
        "vegetation_phenology_graph",
        "root_zone_constraint_graph",
        "environmental_response_graph",
        "vegetation_competition_graph",
        "vegetation_evidence_dependency_graph",
    }
    for connection_point in connection_points:
        point_id = connection_point["connection_point_id"]
        if point_id in {"rootSupport", "baseSupport"}:
            memberships.add("landscape_support_graph")
        elif point_id == "irrigationConnection":
            memberships.add("irrigation_service_graph")
        elif point_id == "stormwaterConnection":
            memberships.add("stormwater_service_graph")
        elif point_id in {"facadeAttachment", "pergolaConnection", "screenConnection"}:
            memberships.add("vegetation_attachment_graph")
        elif point_id == "inspection":
            memberships.add("maintenance_access_graph")
    return sorted(memberships)


def plant_architecture_name(category: str) -> str:
    return {
        "Tree": "Tree",
        "Shrub": "Shrub",
        "Grass": "Grass",
        "Groundcover": "Herb",
        "Vine": "Vine",
        "Plant": "Herb",
        "EcologicalPlanting": "MixedPlanting",
    }[category]


def vegetation_composition_name(category: str) -> str:
    return {
        "Tree": "IndividualPlant",
        "Shrub": "IndividualPlant",
        "Grass": "ClonalColony",
        "Groundcover": "ClonalColony",
        "Vine": "HostedPlantSystem",
        "Plant": "PlantCluster",
        "EcologicalPlanting": "MixedPlanting",
    }[category]


def supported_phenology_states(category: str) -> list[str]:
    states = ["Juvenile", "Mature", "Senescent", "Dormant"]
    if category not in {"Grass", "Groundcover"}:
        states.extend(["Flowering", "Fruiting"])
    return states


def root_representation_name(category: str) -> str:
    if category == "EcologicalPlanting":
        return "AggregateSafetyEnvelope"
    if category == "Vine":
        return "RootSafetyEnvelopeWithHostAttachment"
    return "RootSafetyEnvelope"


def environmental_drivers(category: str) -> list[str]:
    drivers = [
        "Light",
        "Water",
        "Temperature",
        "Wind",
        "Soil",
        "AvailableSpace",
    ]
    if category in {"Vine", "Plant", "EcologicalPlanting"}:
        drivers.append("HostSupport")
    if category == "EcologicalPlanting":
        drivers.append("StormwaterHydrology")
    return drivers


def biological_profile(
    vegetation_object: VegetationBuildingObject,
    grammar_sha256: str,
) -> dict[str, object]:
    category = vegetation_object.category
    return {
        "schema": BIOLOGICAL_PROFILE_SCHEMA,
        "identity": {
            "resolution": "ArchitecturalArchetype",
            "architectural_archetype": vegetation_object.object_class,
            "plant_architecture": plant_architecture_name(category),
            "botanical_taxon": None,
            "cultivar": None,
            "specimen_identifier": None,
            "calibration_state": "UncalibratedTaxon",
        },
        "composition": {
            "kind": vegetation_composition_name(category),
            "individual_count": None,
            "clonal_or_mixed_membership_graph": (
                "vegetation_growth_graph"
                if category in {"Grass", "Groundcover", "Plant", "EcologicalPlanting"}
                else None
            ),
        },
        "architectural_topology": {
            "scale_hierarchy": [
                "WholePlant",
                "Axis",
                "GrowthUnit",
                "Metamer",
                "Organ",
                "GeometryRegion",
                "RepresentationCluster",
            ],
            "relationship_kinds": [
                "Decomposition",
                "Succession",
                "Branching",
                "Attachment",
            ],
            "root_system_included": True,
            "shoot_system_included": True,
            "maximum_branch_order": None,
            "topology_evidence_identifiers": [],
            "branch_order_authority": "ExplicitGrowthGraphOrMeasuredQsm",
        },
        "phenology": {
            "reference_state": "MatureLeafOnDesignIntent",
            "supported_states": supported_phenology_states(category),
            "seasonal_schedule": None,
            "schedule_authority": "SpeciesOrSpecimenCalibrationRequired",
        },
        "root_architecture": {
            "representation": root_representation_name(category),
            "maximum_depth_metres": None,
            "maximum_radial_spread_metres": None,
            "maximum_root_order": None,
            "root_graph_identifier": None,
            "evidence_identifiers": [],
            "required_measurements": [
                "RootDepth",
                "RootRadialSpread",
                "RootOrder",
                "RootDiameter",
                "BranchingAngle",
                "RootOccupancyDensity",
            ],
            "collision_policy": "UseAuthoredSafetyEnvelopeUntilCalibrated",
        },
        "canopy_optics": {
            "leaf_area_index": None,
            "leaf_area_density_distribution": None,
            "leaf_angle_distribution": None,
            "crown_gap_fraction": None,
            "projected_foliage_area_by_view": None,
            "calibration_state": "RequiredBeforeLightInterceptionSimulation",
        },
        "biomechanics": {
            "mass_density_kilograms_per_cubic_metre": None,
            "elastic_modulus_pascals": None,
            "damping_ratio": None,
            "drag_coefficient": None,
            "articulation_basis": "AxisBranchAndOrganHierarchy",
            "calibration_state": "RequiredBeforePhysicalWindSimulation",
        },
        "environmental_response": {
            "required_drivers": environmental_drivers(category),
            "response_curves": None,
            "tropism_authority": "AuthoredGenerationMethodUntilCalibrated",
            "competition_volume": "VegetationGrowthEnvelope",
        },
        "semantic_annotations": {
            "ontology_identifier": None,
            "ontology_release_identifier": None,
            "anatomical_entity_identifiers": [],
            "development_stage_identifiers": [],
            "calibration_state": "VersionedOntologyCrosswalkRequired",
        },
        "functional_traits": {
            "specific_leaf_area_square_metres_per_kilogram": None,
            "leaf_dry_matter_content_kilograms_per_kilogram": None,
            "leaf_nitrogen_content_kilograms_per_kilogram": None,
            "maximum_mature_height_metres": None,
            "observation_scope": None,
            "calibration_state": "TraitObservationEvidenceRequired",
        },
        "hydraulics": {
            "maximum_leaf_specific_conductance_millimoles_per_square_metre_per_second_per_megapascal": None,
            "hydraulic_capacitance_kilograms_per_megapascal": None,
            "xylem_water_potential_at_fifty_percent_conductivity_loss_megapascals": None,
            "stomatal_water_potential_at_fifty_percent_conductance_loss_megapascals": None,
            "leaf_to_sapwood_area_ratio": None,
            "calibration_state": "HydraulicMeasurementEvidenceRequired",
        },
        "size_allometry": {
            "reference_height_metres": None,
            "reference_horizontal_radius_metres": None,
            "reference_supporting_axis_diameter_metres": None,
            "size_relationship_model_identifier": None,
            "calibration_state": "TaxonAndSiteSpecificAllometryRequired",
        },
        "substrate_requirements": {
            "minimum_rootable_volume_cubic_metres": None,
            "minimum_rootable_depth_metres": None,
            "maximum_bulk_density_kilograms_per_cubic_metre": None,
            "minimum_air_filled_porosity_fraction": None,
            "minimum_available_water_capacity_fraction": None,
            "calibration_state": "PlantAndSiteSpecificSubstrateEvidenceRequired",
        },
        "evidence_provenance": {
            "geometry_authority": "DeterministicGrammarAndAuthoredExtents",
            "geometry_sha256": grammar_sha256,
            "biological_authority": "ArchitecturalArchetypeOnly",
            "literature_source_ids": [
                source["source_id"] for source in LITERATURE_SOURCES
            ],
            "measurement_evidence_ids": [],
            "uncertainty_policy": (
                "Unknown biological values remain null and may not be inferred "
                "from architectural category alone."
            ),
        },
    }


def calibration_readiness_contract() -> dict[str, object]:
    not_ready_reasons = {
        "TaxonomicIdentity": "ArchitecturalArchetypeHasNoResolvedTaxon",
        "ShootTopology": "NoMeasuredOrCalibratedTopologyEvidence",
        "RootArchitecture": "RootSafetyEnvelopeIsNotAnExplicitRootGraph",
        "CanopyOptics": "OpticalMeasurementsRemainNull",
        "Phenology": "SeasonalScheduleEvidenceIsAbsent",
        "Biomechanics": "MechanicalMeasurementsRemainNull",
        "EnvironmentalResponse": "ResponseCurvesAndCalibrationEvidenceAreAbsent",
        "SemanticAnnotation": "VersionedOntologyCrosswalkIsAbsent",
        "FunctionalTraits": "TraitMeasurementsRemainNull",
        "Hydraulics": "HydraulicMeasurementsRemainNull",
        "SizeAllometry": "SizeRelationshipModelIsAbsent",
        "SubstrateSuitability": "RootableSubstrateRequirementsRemainNull",
        "RepresentationFidelity": "ThresholdContractHasNoAttachedObservationEvidence",
    }
    domains = {}
    for domain in CALIBRATION_DOMAINS:
        if domain == "BuildingPlacement":
            domains[domain] = {
                "ready": True,
                "authority": "AuthoredSpatialProfile",
                "evidence_requirements": [
                    "SpatialApplicability",
                    "PlacementPolicy",
                    "CollisionParticipation",
                    "OrientationProfile",
                    "VisualCollisionAndGrowthExtents",
                    "SupportAndGrowthGraphMemberships",
                ],
            }
        else:
            domains[domain] = {
                "ready": False,
                "reason": not_ready_reasons[domain],
            }
    return {
        "validation_service": "VegetationBiologicalProfileValidationService",
        "readiness_service": "VegetationCalibrationReadinessEvaluationService",
        "profile_validation_expected": True,
        "all_biological_domains_ready": False,
        "domains": domains,
        "acceptance_rule": (
            "Each requested domain must pass independently; readiness in one "
            "domain cannot certify another."
        ),
    }


def visual_feature_requirements(category: str) -> list[str]:
    shared = [
        "object-local support or attachment geometry",
        "recognizable botanical silhouette in front right and top views",
        "non-flat layered organic geometry",
    ]
    category_features = {
        "Tree": ["tapered trunk", "branch hierarchy", "multi-lobed crown"],
        "Shrub": ["dense foliage mass", "grounded base", "asymmetric crown rhythm"],
        "Grass": ["multiple blade clumps", "individual blade accents", "directional tufting"],
        "Groundcover": ["low spreading mat", "edge variation", "surface coverage rhythm"],
        "Vine": ["host structure", "climbing paths", "attached foliage layers"],
        "Plant": ["container or soil host", "distinct stems and leaves", "maintainable access"],
        "EcologicalPlanting": ["water or drainage host", "mixed planting density", "service connection"],
    }[category]
    return shared + category_features


def triangle_distribution_contract(category: str) -> dict[str, object]:
    reference_budgets = {
        "Tree": 24000,
        "Shrub": 12000,
        "Grass": 8000,
        "Groundcover": 6000,
        "Vine": 10000,
        "Plant": 10000,
        "EcologicalPlanting": 14000,
    }
    protected_regions = {
        "Tree": ["RootFlare", "PrimaryStem", "BranchJunction", "FoliageSilhouette"],
        "Shrub": ["GroundContact", "BranchJunction", "FoliageSilhouette"],
        "Grass": ["GroundContact", "FoliageSilhouette", "FoliageInterior"],
        "Groundcover": ["GroundContact", "FoliageSilhouette", "FoliageInterior"],
        "Vine": ["SupportStructure", "BranchSegment", "FoliageSilhouette"],
        "Plant": ["SupportStructure", "GroundContact", "FoliageSilhouette"],
        "EcologicalPlanting": [
            "SupportStructure", "GroundContact", "FoliageSilhouette", "FlowerOrFruit"
        ],
    }
    return {
        "allocation_service": "VegetationTriangleDistributionService",
        "reference_triangle_budget": reference_budgets[category],
        "maximum_meshlet_triangle_count": 124,
        "preserve_organ_area": True,
        "importance_weights": {
            "silhouette": 0.25,
            "projected_area": 0.20,
            "curvature": 0.12,
            "topology": 0.13,
            "motion": 0.08,
            "material_boundary": 0.05,
            "visibility": 0.07,
            "biological_area": 0.10,
        },
        "protected_semantic_regions": protected_regions[category],
        "lod_contracts": [
            {
                "level": "FullSemanticGeometry",
                "maximum_projected_error_pixels": 0.35,
                "triangle_budget_scale": 1.0,
            },
            {
                "level": "AreaPreservingOrganGeometry",
                "maximum_projected_error_pixels": 0.85,
                "triangle_budget_scale": 0.55,
            },
            {
                "level": "ClusteredOrganGeometry",
                "maximum_projected_error_pixels": 1.75,
                "triangle_budget_scale": 0.25,
            },
            {
                "level": "BillboardCloud",
                "maximum_projected_error_pixels": 4.0,
                "triangle_budget_scale": 0.08,
            },
        ],
        "selection_rule": (
            "Select by projected representation error and view contribution, not fixed distance."
        ),
    }


def representation_fidelity_contract() -> dict[str, object]:
    return {
        "evaluation_service": "VegetationRepresentationFidelityEvaluationService",
        "required_views": list(COMPARISON_VIEWS),
        "minimum_independent_metric_percent": MINIMUM_VIEW_MATCH_PERCENT,
        "target_independent_metric_percent": TARGET_VIEW_MATCH_PERCENT,
        "metrics": [
            "Silhouette",
            "ProjectedFoliageArea",
            "CrownGapFraction",
            "BranchTopology",
            "MaterialArea",
            "MotionEnvelope",
        ],
        "acceptance_rule": (
            "Every metric must pass in every required view; composite averaging cannot hide failure."
        ),
    }


def spatial_contract(
    vegetation_object: VegetationBuildingObject,
    grammar_sha256: str,
) -> dict[str, object]:
    width, height, depth = vegetation_object.expected_extent_xyz
    connection_points = parse_connection_points(vegetation_object.grammar_source)
    collision_behavior = (
        "AggregateBoundary"
        if vegetation_object.category in {"Plant", "EcologicalPlanting"}
        else "SoftBoundary"
    )
    return {
        "manifestation_kind": "PhysicalAggregate",
        "placement_policy": (
            "HostedByObject" if vegetation_object.category == "Vine" else "ConstraintSolved"
        ),
        "extent_awareness": {
            "authored_extent_xyz": [width, height, depth],
            "visual_bounds_min_xyz": [-width / 2.0, 0.0, -depth / 2.0],
            "visual_bounds_max_xyz": [width / 2.0, height, depth / 2.0],
            "collision_extent_source": "AuthoredBoundary",
            "growth_extent_source": "VegetationGrowthEnvelope",
            "clearance_inflation_xyz": [0.25, 0.35, 0.25],
        },
        "collision_positioning": {
            "boundary_shape": vegetation_object.collision_shape,
            "behavior": collision_behavior,
            "layer": "Terrain",
            "mask": [
                "Structure", "Envelope", "Interior", "Furniture", "Plumbing",
                "Hvac", "Electrical", "Equipment", "Terrain", "Temporary",
            ],
            "resolution_policy": "ResolveSupportThenRejectOverlap",
        },
        "orientation_awareness": {
            "policy": vegetation_object.orientation_rule,
            "local_front_axis": [0.0, 0.0, 1.0],
            "local_right_axis": [1.0, 0.0, 0.0],
            "local_up_axis": [0.0, 1.0, 0.0],
            "rotational_symmetry": (
                "ContinuousYaw"
                if vegetation_object.category in {"Tree", "Shrub", "Grass", "Groundcover"}
                else "None"
            ),
        },
        "connection_points": connection_points,
        "connection_graph_memberships": graph_memberships(connection_points),
        "growth_awareness": {
            "primitive_families": list(vegetation_object.vegetation_primitives),
            "generation_methods": list(vegetation_object.generation_methods),
            "growth_envelope_is_collision_aware": True,
            "growth_may_cross_authored_visual_extent": False,
        },
        "biological_profile": biological_profile(
            vegetation_object,
            grammar_sha256,
        ),
        "calibration_readiness": calibration_readiness_contract(),
        "triangle_distribution_contract": triangle_distribution_contract(
            vegetation_object.category
        ),
        "representation_fidelity_contract": representation_fidelity_contract(),
        "camera_view_contract": {
            "views": list(COMPARISON_VIEWS),
            "projection": "narrow-perspective",
            "vertical_field_of_view_degrees": COMPARISON_FIELD_OF_VIEW_DEGREES,
            "fit_scope": "individual-object-authored-extent",
            "minimum_match_percent": MINIMUM_VIEW_MATCH_PERCENT,
            "target_match_percent": TARGET_VIEW_MATCH_PERCENT,
        },
    }


def generate_outputs() -> tuple[dict[str, object], dict[Path, bytes]]:
    outputs: dict[Path, bytes] = {}
    object_records = []
    coverage_rows = []
    observed_primitives: set[str] = set()
    for index, vegetation_object in enumerate(OBJECTS, start=1):
        grammar_content = (vegetation_object.grammar_source.strip() + "\n").encode()
        grammar_path = GRAMMAR_DIRECTORY / vegetation_object.grammar_name
        grammar_relative = grammar_path.relative_to(REPOSITORY_ROOT)
        outputs[grammar_path] = grammar_content
        observed_primitives.update(vegetation_object.vegetation_primitives)
        grammar_sha256 = sha256_bytes(grammar_content)
        object_spatial_contract = spatial_contract(
            vegetation_object,
            grammar_sha256,
        )
        object_records.append({
            "sequence": index,
            "object_id": vegetation_object.object_id,
            "object_class": vegetation_object.object_class,
            "title": vegetation_object.title,
            "category": vegetation_object.category,
            "taxonomy": [
                "SmallModernBuilding",
                "BuildingSite",
                "LandscapeSystem",
                vegetation_object.category,
                vegetation_object.object_class,
            ],
            "parent_object_id": None,
            "comparison_mode": "direct_geometry",
            "camera_option": "--preview-object",
            "grammar": str(grammar_relative),
            "grammar_sha256": grammar_sha256,
            "design_intent": vegetation_object.design_intent,
            "vegetation_primitives": list(vegetation_object.vegetation_primitives),
            "generation_methods": list(vegetation_object.generation_methods),
            "visual_feature_requirements": visual_feature_requirements(
                vegetation_object.category
            ),
            **object_spatial_contract,
        })
        for primitive_name in vegetation_object.vegetation_primitives:
            coverage_rows.append({
                "object_id": vegetation_object.object_id,
                "object_class": vegetation_object.object_class,
                "category": vegetation_object.category,
                "primitive": primitive_name,
                "covered": "true",
            })

    if tuple(sorted(observed_primitives)) != tuple(sorted(ALL_VEGETATION_PRIMITIVES)):
        raise RuntimeError(
            "Vegetation primitive coverage is incomplete: "
            f"observed={sorted(observed_primitives)}"
        )
    if len({record["grammar_sha256"] for record in object_records}) != len(OBJECTS):
        raise RuntimeError("Vegetation grammar files must be byte-distinct")
    category_counts = {
        category: sum(record["category"] == category for record in object_records)
        for category in EXPECTED_CATEGORY_COUNTS
    }
    if category_counts != EXPECTED_CATEGORY_COUNTS:
        raise RuntimeError(
            f"Vegetation taxonomy counts differ from contract: {category_counts}"
        )
    grass_records = [record for record in object_records if record["category"] == "Grass"]
    if len(grass_records) != 10:
        raise RuntimeError(f"Expected exactly ten grasses, found {len(grass_records)}")

    manifest = {
        "schema": "ProGen3D-SMB-VegetationBuildingObjectSuite-v1",
        "suite_id": "SMB_VEGETATION_BUILDING_OBJECTS_V1",
        "object_count": len(object_records),
        "category_counts": category_counts,
        "comparison_views": list(COMPARISON_VIEWS),
        "comparison_projection": {
            "mode": "narrow-perspective",
            "vertical_field_of_view_degrees": COMPARISON_FIELD_OF_VIEW_DEGREES,
            "purpose": "Elevation-like front, right, and top object comparisons",
        },
        "minimum_view_match_percent": MINIMUM_VIEW_MATCH_PERCENT,
        "target_view_match_percent": TARGET_VIEW_MATCH_PERCENT,
        "all_vegetation_primitives": list(ALL_VEGETATION_PRIMITIVES),
        "objects": object_records,
    }
    outputs[SUITE_DIRECTORY / "suite_manifest.json"] = (
        json.dumps(manifest, indent=2) + "\n"
    ).encode()
    outputs[SUITE_DIRECTORY / "taxonomy.json"] = (
        json.dumps({
            "schema": "ProGen3D-SMB-AdditiveVegetationTaxonomy-v1",
            "compatibility_boundary": (
                "Additive taxonomy; the frozen 124-object Small Modern Building inventory is unchanged."
            ),
            "object_count": len(object_records),
            "renderable_object_count": len(object_records),
            "comparison_views": list(COMPARISON_VIEWS),
            "objects": object_records,
        }, indent=2) + "\n"
    ).encode()
    outputs[SUITE_DIRECTORY / "building_vegetation_object_model.json"] = (
        json.dumps({
            "schema": BUILDING_VEGETATION_MODEL_SCHEMA,
            "model_id": BUILDING_VEGETATION_MODEL_ID,
            "research_snapshot_date": RESEARCH_SNAPSHOT_DATE,
            "compatibility_boundary": (
                "Additive specialization of SMB-OMv2.1 and SMBv3 spatial profiles; "
                "the frozen 124-object building inventory remains unchanged."
            ),
            "object_count": len(object_records),
            "category_counts": category_counts,
            "relationship_authorities": {
                "containment": "landscape_containment_graph",
                "support": "landscape_support_graph",
                "attachment": "vegetation_attachment_graph",
                "irrigation": "irrigation_service_graph",
                "stormwater": "stormwater_service_graph",
                "growth": "vegetation_growth_graph",
                "maintenance": "maintenance_access_graph",
                "phenology": "vegetation_phenology_graph",
                "root_zone": "root_zone_constraint_graph",
                "environment": "environmental_response_graph",
                "competition": "vegetation_competition_graph",
                "evidence": "vegetation_evidence_dependency_graph",
            },
            "biological_model_authority": {
                "profile_schema": BIOLOGICAL_PROFILE_SCHEMA,
                "identity_model": "PlantIdentityProfile",
                "architectural_topology_model": "PlantArchitecturalTopologyProfile",
                "phenology_model": "PlantPhenologyProfile",
                "root_architecture_model": "PlantRootArchitectureProfile",
                "canopy_optical_model": "PlantCanopyOpticalProfile",
                "biomechanical_model": "PlantBiomechanicalProfile",
                "environmental_response_model": "PlantEnvironmentalResponseProfile",
                "semantic_annotation_model": "PlantSemanticAnnotationProfile",
                "functional_trait_model": "PlantFunctionalTraitProfile",
                "hydraulic_model": "PlantHydraulicProfile",
                "size_allometry_model": "PlantSizeAllometryProfile",
                "substrate_requirement_model": "VegetationSubstrateRequirementProfile",
                "evidence_model": "VegetationEvidenceProfile",
                "multiscale_levels": [
                    "WholePlant", "Axis", "GrowthUnit", "Metamer", "Organ",
                    "GeometryRegion", "RepresentationCluster",
                ],
                "uncalibrated_value_policy": (
                    "Preserve unknown measurements as null and fail closed for "
                    "root, light, phenology, wind, hydraulic, allometric, trait, "
                    "semantic, or substrate simulation."
                ),
            },
            "calibration_readiness_authority": {
                "validation_service": "VegetationBiologicalProfileValidationService",
                "readiness_service": "VegetationCalibrationReadinessEvaluationService",
                "domains": list(CALIBRATION_DOMAINS),
                "independent_domain_rule": (
                    "Every requested domain must be ready independently; no "
                    "aggregate score or unrelated ready domain may hide a failure."
                ),
                "current_suite_ready_domains": ["BuildingPlacement"],
                "current_suite_unready_domains": [
                    domain for domain in CALIBRATION_DOMAINS
                    if domain != "BuildingPlacement"
                ],
            },
            "calibration_evidence_bundle_authority": {
                "schema": "ProGen3D-VegetationCalibrationEvidenceBundle-v1",
                "measurement_model": "VegetationCalibrationMeasurement",
                "bundle_model": "VegetationCalibrationEvidenceBundle",
                "serialization_service": "VegetationCalibrationEvidenceBundleSerializationService",
                "payload_hash_service": "VegetationCalibrationPayloadHashService",
                "parsing_service": "VegetationCalibrationEvidenceBundleParsingService",
                "validation_service": "VegetationCalibrationEvidenceBundleValidationService",
                "binding_service": "VegetationCalibrationEvidenceBindingService",
                "binding_report": "VegetationCalibrationEvidenceBindingReport",
                "binding_policy": (
                    "Validated domains bind atomically into a new biological "
                    "profile; incomplete domains are deferred and source state "
                    "is never mutated."
                ),
                "payload_policy": (
                    "SHA-256 covers canonical payload JSON with source and "
                    "measurement ordering normalized."
                ),
            },
            "measured_source_adapter_authority": {
                "schema": "ProGen3D-VegetationMeasuredSourceArtifact-v1",
                "artifact_model": "VegetationMeasuredSourceArtifact",
                "adapter_report": "VegetationMeasuredSourceAdapterReport",
                "payload_hash_service": "VegetationMeasuredSourceArtifactHashService",
                "artifact_validation_service": "VegetationMeasuredSourceArtifactValidationService",
                "coordinate_normalization_service": "VegetationMeasuredCoordinateNormalizationService",
                "unit_normalization_service": "VegetationMeasurementUnitNormalizationService",
                "measurement_factory": "VegetationMeasuredCalibrationMeasurementFactory",
                "bundle_factory": "VegetationMeasuredEvidenceBundleFactory",
                "adapters": [
                    "VegetationQuantitativeStructureModelAdapter",
                    "VegetationRootArchitectureGraphAdapter",
                    "VegetationCanopyObservationAdapter",
                    "VegetationPhenologyObservationSeriesAdapter",
                    "VegetationBiomechanicalMaterialTestAdapter",
                ],
                "source_media_types": [
                    "application/vnd.progen3d.qsm+json",
                    "application/vnd.progen3d.root-architecture+json",
                    "application/vnd.progen3d.canopy-observation+json",
                    "application/vnd.progen3d.phenology-series+json",
                    "application/vnd.progen3d.biomechanical-test+json",
                ],
                "adapter_policy": (
                    "Source-specific adapters validate immutable hashed artifacts, "
                    "normalize supported units to SI, preserve accepted, deferred, "
                    "and rejected source records, and emit calibration evidence "
                    "bundles without mutating biological profiles."
                ),
            },
            "native_measured_source_decoder_authority": {
                "schema_registry": "VegetationMeasuredSourceSchemaRegistry",
                "decode_context": "VegetationMeasuredSourceDecodeContext",
                "decode_report": "VegetationMeasuredSourceDecodeReport",
                "decoding_service": "VegetationMeasuredSourceDecodingService",
                "coordinate_transform_model": (
                    "VegetationMeasuredCoordinateReferenceTransform"
                ),
                "coordinate_transform_service": (
                    "VegetationMeasuredCoordinateReferenceTransformService"
                ),
                "canonical_artifact_factory": (
                    "VegetationDecodedMeasuredSourceArtifactFactory"
                ),
                "decoders": [
                    "VegetationTreeQsmCylinderTableDecoder",
                    "VegetationRootSystemMarkupLanguageDecoder",
                    "VegetationCanopyObservationTableDecoder",
                    "VegetationPhenologyObservationTableDecoder",
                    "VegetationBiomechanicalMaterialTestTableDecoder",
                ],
                "source_schemas": [
                    {
                        "media_type": "text/vnd.treeqsm.cylinder-table",
                        "schema_version": "TreeQSM-save_model_text-1.1.0",
                    },
                    {
                        "media_type": "application/rsml+xml",
                        "schema_version": "RSML-1",
                    },
                    {
                        "media_type": (
                            "text/vnd.progen3d.canopy-observation-table"
                        ),
                        "schema_version": "ProGen3D-CanopyObservationTable-v1",
                    },
                    {
                        "media_type": "text/vnd.progen3d.phenology-series-table",
                        "schema_version": (
                            "ProGen3D-PhenologyObservationTable-v1"
                        ),
                    },
                    {
                        "media_type": (
                            "text/vnd.progen3d.biomechanical-test-table"
                        ),
                        "schema_version": (
                            "ProGen3D-BiomechanicalMaterialTestTable-v1"
                        ),
                    },
                ],
                "decoder_policy": (
                    "Exact media-type and schema-version registrations fail "
                    "closed. Raw payload hashes remain unchanged; canonical "
                    "artifacts embed source and transform provenance. Lossy, "
                    "derived, deferred, and rejected records remain explicit."
                ),
            },
            "point_cloud_reconstruction_authority": {
                "metadata_schema": (
                    "ProGen3D-VegetationPointCloudSourceMetadata-v1"
                ),
                "capability_catalog": (
                    "VegetationPointCloudFormatCapabilityCatalog"
                ),
                "ingestion_service": "VegetationPointCloudIngestionService",
                "ingestion_report": "VegetationPointCloudIngestionReport",
                "reconstruction_admission_service": (
                    "VegetationPointCloudReconstructionAdmissionService"
                ),
                "reconstruction_job_factory": (
                    "VegetationPointCloudReconstructionJobFactory"
                ),
                "point_record_decoders": [
                    "VegetationPlyPointCloudDecoder",
                    "VegetationLasPointCloudDecoder",
                ],
                "point_record_support": [
                    {
                        "media_type": "application/ply",
                        "schema_version": "PLY-1.0",
                        "encodings": [
                            "Ascii",
                            "BinaryLittleEndian",
                            "BinaryBigEndian",
                        ],
                        "capability": "PointRecords",
                        "scalar_types": [
                            "int8",
                            "uint8",
                            "int16",
                            "uint16",
                            "int32",
                            "uint32",
                            "float32",
                            "float64",
                        ],
                    },
                    {
                        "media_type": "application/vnd.las",
                        "schema_versions": [
                            "LAS-1.0",
                            "LAS-1.1",
                            "LAS-1.2",
                            "LAS-1.3",
                            "LAS-1.4",
                        ],
                        "encoding": "BinaryLittleEndian",
                        "capability": "PointRecords",
                    },
                ],
                "metadata_only_support": [],
                "fail_closed_formats": [
                    {
                        "media_type": "application/vnd.las",
                        "schema_version": "LAS-1.5",
                        "reason": (
                            "LAS 1.5 is current at the 2026-08-28 research "
                            "snapshot but is newer than the frozen D3A decoder."
                        ),
                    },
                    {
                        "media_type": "application/vnd.laz",
                        "required_dependency": "LASzip or PDAL",
                    },
                    {
                        "media_type": "model/e57",
                        "required_dependency": "libE57Format",
                    },
                ],
                "organ_classification_policy": (
                    "Woody and foliage are explicit organ labels. LAS low, "
                    "medium, and high vegetation classifications remain "
                    "Unknown because height classes do not imply botanical organ."
                ),
                "admission_policy": (
                    "Reconstruction jobs require immutable source identity, "
                    "bounded point count, three-dimensional extent, controlled "
                    "duplicate fraction, and target-specific organ-label coverage."
                ),
                "canopy_occupancy_reconstruction": {
                    "schema": (
                        "ProGen3D-VegetationCanopyOccupancyReconstruction-v1"
                    ),
                    "algorithm_identifier": (
                        "ProGen3D-CanopyVoxelOccupancy-v1"
                    ),
                    "policy_model": (
                        "VegetationCanopyOccupancyReconstructionPolicy"
                    ),
                    "quality_evidence_model": (
                        "VegetationCanopyOccupancyQualityEvidence"
                    ),
                    "quality_report_model": (
                        "VegetationCanopyOccupancyQualityReport"
                    ),
                    "reconstruction_model": (
                        "VegetationCanopyOccupancyReconstruction"
                    ),
                    "quality_service": (
                        "VegetationCanopyOccupancyQualityEvaluationService"
                    ),
                    "reconstruction_service": (
                        "VegetationCanopyOccupancyReconstructionService"
                    ),
                    "quality_metrics": [
                        "organ_label_fraction",
                        "deterministic_holdout_neighborhood_recall",
                        "xy_projected_occupancy_area_square_metres",
                        "xz_projected_occupancy_area_square_metres",
                        "yz_projected_occupancy_area_square_metres",
                        "xy_projected_gap_proxy",
                        "xz_projected_gap_proxy",
                        "yz_projected_gap_proxy",
                    ],
                    "geometry_use_admitted": True,
                    "biological_calibration_admitted": False,
                    "calibration_boundary": (
                        "Voxel occupancy and projected gap proxies are geometry "
                        "evidence only. They are not leaf area index, optical "
                        "gap fraction, or leaf inclination measurements and "
                        "cannot enter the canopy observation adapter."
                    ),
                },
                "primary_woody_axis_reconstruction": {
                    "schema": (
                        "ProGen3D-VegetationPrimaryWoodyAxisReconstruction-v1"
                    ),
                    "scope": "PrimaryAxisOnly",
                    "algorithm_identifier": (
                        "ProGen3D-PrimaryWoodyAxis-v1"
                    ),
                    "numerical_dependency": "Eigen3",
                    "policy_model": (
                        "VegetationWoodyAxisReconstructionPolicy"
                    ),
                    "radius_station_model": (
                        "VegetationWoodyAxisRadiusStation"
                    ),
                    "cylinder_model": "VegetationWoodyAxisCylinder",
                    "quality_evidence_model": (
                        "VegetationWoodyAxisQualityEvidence"
                    ),
                    "quality_report_model": (
                        "VegetationWoodyAxisQualityReport"
                    ),
                    "reconstruction_model": (
                        "VegetationWoodyAxisReconstruction"
                    ),
                    "quality_service": (
                        "VegetationWoodyAxisQualityEvaluationService"
                    ),
                    "reconstruction_service": (
                        "VegetationPrimaryWoodyAxisReconstructionService"
                    ),
                    "quality_metrics": [
                        "woody_label_fraction",
                        "bounded_radial_trim_fraction",
                        "principal_variance_fraction",
                        "training_cylinder_surface_rmse_metres",
                        "holdout_cylinder_surface_rmse_metres",
                        "holdout_axial_coverage_fraction",
                        "taper_violation_fraction",
                    ],
                    "geometry_use_admitted": True,
                    "complete_qsm_admitted": False,
                    "calibration_boundary": (
                        "The station and cylinder chain represents only the "
                        "dominant woody axis. It has no branch segmentation, "
                        "parent-child topology, branch order, or complete woody "
                        "volume authority and cannot enter the QSM adapter."
                    ),
                },
                "segmented_woody_branch_graph_reconstruction": {
                    "segmentation_schema": (
                        "ProGen3D-VegetationWoodyPointSegmentation-v1"
                    ),
                    "segmentation_models": [
                        "VegetationWoodyAxisSegmentDefinition",
                        "VegetationWoodyPointSegmentAssignment",
                        "VegetationWoodyPointSegmentation",
                        "VegetationWoodyPointSegmentationValidationReport",
                    ],
                    "segmentation_validation_service": (
                        "VegetationWoodyPointSegmentationValidationService"
                    ),
                    "reconstruction_schema": (
                        "ProGen3D-VegetationWoodyBranchGraphReconstruction-v1"
                    ),
                    "scope": "CompleteForObservedWoodyEvidence",
                    "algorithm_identifier": (
                        "ProGen3D-SegmentedWoodyBranchGraph-v1"
                    ),
                    "numerical_dependency": "Eigen3",
                    "policy_model": (
                        "VegetationWoodyBranchGraphReconstructionPolicy"
                    ),
                    "axis_model": "VegetationWoodyBranchAxis",
                    "cylinder_model": "VegetationWoodyBranchCylinder",
                    "connection_model": "VegetationWoodyBranchConnection",
                    "quality_evidence_model": (
                        "VegetationWoodyBranchGraphQualityEvidence"
                    ),
                    "quality_report_model": (
                        "VegetationWoodyBranchGraphQualityReport"
                    ),
                    "reconstruction_model": (
                        "VegetationWoodyBranchGraphReconstruction"
                    ),
                    "quality_service": (
                        "VegetationWoodyBranchGraphQualityEvaluationService"
                    ),
                    "reconstruction_service": (
                        "VegetationSegmentedWoodyBranchGraphReconstructionService"
                    ),
                    "qsm_artifact_factory": (
                        "VegetationWoodyBranchGraphQuantitativeStructureModelArtifactFactory"
                    ),
                    "quality_metrics": [
                        "assignment_coverage_fraction",
                        "minimum_assignment_confidence",
                        "minimum_principal_variance_fraction",
                        "maximum_training_surface_rmse_metres",
                        "maximum_holdout_surface_rmse_metres",
                        "minimum_holdout_surface_coverage_fraction",
                        "maximum_attachment_surface_gap_metres",
                        "maximum_taper_violation_fraction",
                        "maximum_branch_order",
                        "axis_cylinder_and_connection_counts",
                    ],
                    "canonical_qsm_artifact_admitted": True,
                    "complete_biological_tree_admitted": False,
                    "segmentation_boundary": (
                        "D3B2C1 consumes explicit point-to-axis assignments "
                        "whose source identity, completeness, hierarchy, "
                        "confidence, and woody-point coverage pass validation. "
                        "It does not infer segmentation from an unsegmented "
                        "point cloud."
                    ),
                    "calibration_boundary": (
                        "An accepted graph is complete only for the observed "
                        "woody evidence represented by the explicit "
                        "segmentation. Occluded or unobserved biological "
                        "branches remain outside the claim, and whole-tree "
                        "volume or biomass requires separate ground-truth and "
                        "parameter-sensitivity admission."
                    ),
                },
                "automated_woody_segmentation_ensemble": {
                    "candidate_algorithm_identifier": (
                        "ProGen3D-WoodyCoverSetSegmentation-v1"
                    ),
                    "ensemble_algorithm_identifier": (
                        "ProGen3D-AutomatedWoodySegmentationEnsemble-v1"
                    ),
                    "parameter_model": (
                        "VegetationWoodySegmentationParameterSet"
                    ),
                    "cover_set_model": "VegetationWoodyCoverSet",
                    "cover_connection_model": (
                        "VegetationWoodyCoverConnection"
                    ),
                    "axis_candidate_model": (
                        "VegetationWoodySegmentationAxisCandidate"
                    ),
                    "candidate_model": (
                        "VegetationWoodySegmentationCandidate"
                    ),
                    "candidate_report_model": (
                        "VegetationWoodySegmentationCandidateReport"
                    ),
                    "candidate_evaluation_model": (
                        "VegetationWoodySegmentationCandidateEvaluation"
                    ),
                    "sensitivity_policy_model": (
                        "VegetationWoodySegmentationSensitivityPolicy"
                    ),
                    "sensitivity_report_model": (
                        "VegetationWoodySegmentationSensitivityReport"
                    ),
                    "ensemble_report_model": (
                        "VegetationAutomatedWoodySegmentationEnsembleReport"
                    ),
                    "candidate_service": (
                        "VegetationWoodyCoverSetSegmentationService"
                    ),
                    "sensitivity_service": (
                        "VegetationWoodySegmentationSensitivityEvaluationService"
                    ),
                    "ensemble_service": (
                        "VegetationAutomatedWoodySegmentationEnsembleService"
                    ),
                    "candidate_parameters": [
                        "cover_cell_size_metres",
                        "neighbour_radius_multiplier",
                        "maximum_cover_index_delta",
                        "minimum_cover_point_count",
                        "maximum_cover_set_count",
                        "maximum_axis_count",
                        "minimum_continuation_cosine",
                        "minimum_assignment_confidence",
                        "minimum_cylinders_per_axis",
                        "minimum_axial_station_spacing_metres",
                    ],
                    "sensitivity_metrics": [
                        "accepted_candidate_fraction",
                        "selected_assignment_agreement_fraction",
                        "axis_count_consensus_fraction",
                        "branch_order_consensus_fraction",
                        "cylinder_count_coefficient_of_variation",
                        "axis_length_coefficient_of_variation",
                        "woody_volume_coefficient_of_variation",
                    ],
                    "classification_independence": (
                        "Candidate construction uses only canonical metric "
                        "positions and the explicit Woody organ class. Vendor "
                        "or fixture classification values never define branch "
                        "identity."
                    ),
                    "selection_boundary": (
                        "A candidate is selectable only after its complete "
                        "point-to-axis segmentation and D3B2C1 graph pass, then "
                        "the cross-parameter assignment, topology, cylinder, "
                        "length, and volume sensitivity policy passes."
                    ),
                    "complete_biological_tree_admitted": False,
                    "volume_calibration_admitted": False,
                    "ground_truth_boundary": (
                        "Sensitivity agreement measures reconstruction "
                        "stability, not biological correctness. Component-level "
                        "measured branch detection, diameter, length, and volume "
                        "comparison remain required before calibrated volume or "
                        "complete-tree claims."
                    ),
                },
                "implementation_boundary": (
                    "D3A and D3B1 admit ASCII and binary PLY plus uncompressed "
                    "LAS datasets. D3B2A reconstructs bounded canopy occupancy "
                    "for geometry use after an independent holdout quality "
                    "gate. D3B2B reconstructs a quality-gated primary woody "
                    "axis only. D3B2C1 constructs and exports a quality-admitted "
                    "branch graph from explicit segmentation. D3B2C2 now "
                    "constructs classification-independent cover candidates and "
                    "selects a graph only after cross-parameter sensitivity "
                    "passes, while keeping complete biological-tree, measured "
                    "branch-detection, leaf-optics, volume, and biomass claims "
                    "closed."
                ),
            },
            "literature_sources": list(LITERATURE_SOURCES),
            "triangle_distribution_authority": {
                "allocation_service": "VegetationTriangleDistributionService",
                "fidelity_service": "VegetationRepresentationFidelityEvaluationService",
                "semantic_region_roles": [
                    "RootFlare", "PrimaryStem", "BranchJunction", "BranchSegment",
                    "FoliageSilhouette", "FoliageInterior", "FlowerOrFruit",
                    "GroundContact", "SupportStructure",
                ],
                "selection_basis": "projected-error-and-semantic-importance",
                "organ_area_preservation": "required-for-reduced-foliage",
            },
            "objects": object_records,
        }, indent=2) + "\n"
    ).encode()

    spatial_rows = []
    for record in object_records:
        spatial_rows.append({
            "object_id": record["object_id"],
            "category": record["category"],
            "extent_awareness": "true" if record["extent_awareness"] else "false",
            "collision_positioning": "true" if record["collision_positioning"] else "false",
            "orientation_awareness": "true" if record["orientation_awareness"] else "false",
            "connection_point_count": len(record["connection_points"]),
            "connection_graph_count": len(record["connection_graph_memberships"]),
            "growth_awareness": "true" if record["growth_awareness"] else "false",
            "biological_profile": "true" if record["biological_profile"] else "false",
            "biological_profile_validation_expected": (
                "true"
                if record["calibration_readiness"]["profile_validation_expected"]
                else "false"
            ),
            "building_placement_ready": (
                "true"
                if record["calibration_readiness"]["domains"]["BuildingPlacement"]["ready"]
                else "false"
            ),
            "all_biological_domains_ready": (
                "true"
                if record["calibration_readiness"]["all_biological_domains_ready"]
                else "false"
            ),
            "identity_resolution": record["biological_profile"]["identity"]["resolution"],
            "root_calibration_state": record["biological_profile"]["root_architecture"]["collision_policy"],
            "canopy_calibration_state": record["biological_profile"]["canopy_optics"]["calibration_state"],
            "biomechanical_calibration_state": record["biological_profile"]["biomechanics"]["calibration_state"],
            "camera_contract": "true" if record["camera_view_contract"] else "false",
            "triangle_distribution_contract": (
                "true" if record["triangle_distribution_contract"] else "false"
            ),
            "fidelity_contract": (
                "true" if record["representation_fidelity_contract"] else "false"
            ),
            "coverage_passed": "true",
        })
    spatial_stream = io.StringIO(newline="")
    spatial_writer = csv.DictWriter(
        spatial_stream,
        fieldnames=spatial_rows[0].keys(),
        lineterminator="\n",
    )
    spatial_writer.writeheader()
    spatial_writer.writerows(spatial_rows)
    outputs[EVIDENCE_DIRECTORY / "vegetation_spatial_contract_coverage.csv"] = (
        spatial_stream.getvalue().encode()
    )

    model_markdown = [
        "# Improved Building Vegetation Object Model",
        "",
        "`BVO-OMv2.1` enriches the existing building spatial object model without changing the frozen 124-object SMB inventory.",
        "",
        "Research snapshot: August 28, 2026.",
        "",
        "## Object Contract",
        "",
        "Each vegetation object owns an authored extent, collision boundary, orientation frame, support or attachment interfaces, graph memberships, growth envelope, primitive/generation provenance, flattened front/right/top camera contract, and an explicit biological profile.",
        "",
        "The biological profile separates identity, multiscale topology, phenology, root architecture, canopy optics, biomechanics, environmental response, semantic annotations, functional traits, hydraulics, size allometry, substrate requirements, and evidence provenance. Architectural archetypes do not invent species values: unresolved taxon, specimen, root, optical, seasonal, mechanical, trait, hydraulic, allometric, semantic, and substrate measurements remain null until evidence is attached.",
        "",
        "## Taxonomy",
        "",
    ]
    for category, count in category_counts.items():
        model_markdown.append(f"- {category}: {count}")
    model_markdown.extend([
        "",
        "## Relationship Authorities",
        "",
        "- `landscape_containment_graph`: site and planter ownership",
        "- `landscape_support_graph`: root and base bearing relationships",
        "- `vegetation_attachment_graph`: facade, screen, and pergola climbing hosts",
        "- `irrigation_service_graph`: irrigation service connectivity",
        "- `stormwater_service_graph`: rain-garden and bioswale drainage connectivity",
        "- `vegetation_growth_graph`: generated branch, blade, leaf, petal, and vine topology",
        "- `maintenance_access_graph`: inspection and maintenance access",
        "- `vegetation_phenology_graph`: state and seasonal transitions",
        "- `root_zone_constraint_graph`: below-ground occupancy and exclusion zones",
        "- `environmental_response_graph`: light, water, temperature, wind, soil, space, and host inputs",
        "- `vegetation_competition_graph`: crown, root, and resource competition",
        "- `vegetation_evidence_dependency_graph`: literature, measurement, grammar, and validation provenance",
        "",
        "## Biological Scales",
        "",
        "The research-backed decomposition is `WholePlant -> Axis -> GrowthUnit -> Metamer -> Organ -> GeometryRegion -> RepresentationCluster`. Decomposition, succession, branching, and attachment remain distinct relationship meanings.",
        "",
        "## Calibration Boundary",
        "",
        "Root simulation requires measured depth, radial spread, root order, diameter, branching angle, and occupancy density. Light interception requires leaf area, leaf area density, leaf angle distribution, crown gap fraction, and projected foliage area. Physical wind simulation requires mass density, elastic modulus, damping, drag, and axis/organ articulation evidence. Water transport, mature size projection, trait inference, ontology exchange, and substrate suitability each require their own observations and cannot be inferred from an architectural label.",
        "",
        "`VegetationBiologicalProfileValidationService` rejects contradictory identity, topology, phenology, root, optical, biomechanical, semantic, trait, hydraulic, allometric, substrate, environmental, and provenance records. `VegetationCalibrationReadinessEvaluationService` evaluates all fourteen calibration domains independently.",
        "",
        "The current architectural-archetype suite is ready for authored building placement only. It is intentionally not marked ready for calibrated biological simulation or measured representation fidelity.",
        "",
        "## Calibration Evidence Bundles",
        "",
        "External calibration evidence uses `ProGen3D-VegetationCalibrationEvidenceBundle-v1`. The parser creates an immutable candidate, validation checks schema, subject identity, architecture, SI units, source hashes, canonical payload SHA-256, measurement domains, and evidence references, and binding creates a new biological profile without mutating the source object.",
        "",
        "Binding is domain-atomic. Complete domains are accepted, incomplete but valid domains are deferred, and invalid bundles reject every measurement. The binding report records accepted, deferred, and rejected measurements plus before/after readiness domains.",
        "",
        "## Measured Source Adapters",
        "",
        "`VegetationMeasuredSourceArtifact` is the immutable boundary for native measured-source payloads. It owns source identity, citation, locator, media type, coordinate and unit systems, acquisition method, uncertainty, raw payload, and exact SHA-256. Adapter validation rejects stale hashes, unsupported media, malformed payloads, architecture mismatch, invalid graphs, unsupported units, and invalid observation series before evidence creation.",
        "",
        "Five purpose-specific adapters convert QSM cylinder graphs, root architecture graphs, canopy optical observations, phenology observation series, and biomechanical material tests into the calibration evidence bundle contract. `VegetationMeasurementUnitNormalizationService` performs explicit supported conversions to SI, and `VegetationMeasuredSourceAdapterReport` preserves accepted, deferred, and rejected source records.",
        "",
        "## Native Measured-Source Decoders",
        "",
        "`VegetationMeasuredSourceSchemaRegistry` resolves exact source media type and schema version pairs. D2 supports the TreeQSM `save_model_text` 1.1.0 cylinder table, three-dimensional RSML v1 with metric resolution and polyline-domain diameter evidence, and three declared ProGen3D measurement tables. Unknown or ambiguous formats fail closed.",
        "",
        "`VegetationMeasuredCoordinateReferenceTransform` owns a source-to-`LocalPlantXYZ-ZUp` affine transform, coordinate units, uncertainty, evidence identifier, and evidence SHA-256. Decoder output preserves raw payload identity and hash in canonical source provenance. Derived endpoints, averaged RSML diameters, nearest-axis attachment, deferred fields, and rejected rows remain explicit decode observations.",
        "",
        "## Segmented Woody Branch Graphs",
        "",
        "D3B2C1 separates explicit woody point segmentation from branch-graph reconstruction. `VegetationWoodyPointSegmentation` owns point-to-axis assignments, confidence, hierarchy, branch order, completeness for observed woody points, dataset identity, and exact source hash. Validation rejects missing or duplicate assignments, non-woody ownership, cycles, invalid order transitions, low confidence, and provenance drift.",
        "",
        "`VegetationSegmentedWoodyBranchGraphReconstructionService` constructs purpose-specific axis, tapered-cylinder, and parent-child connection objects only from an admitted `ShootArchitecture` job and the exact `ProGen3D-SegmentedWoodyBranchGraph-v1` algorithm. Independent quality evidence records assignment coverage, principal variance, training and holdout surface residuals, holdout coverage, attachment gap, taper, branch order, and bounded component counts.",
        "",
        "A quality-admitted graph can be exported as a deterministic canonical QSM artifact, but its scope is `CompleteForObservedWoodyEvidence`. Automated segmentation, parameter-stability evidence, unobserved biological branches, whole-tree volume, and biomass remain outside the claim.",
        "",
        "D3B2C2 adds deterministic cover-cell partitioning, a bounded neighbour graph, a rooted minimum spanning tree, direction-continuity axis construction, refined centreline fitting, and geometry-derived assignment confidence. Multiple parameter candidates must each pass the explicit-segmentation graph gate before `VegetationWoodySegmentationSensitivityEvaluationService` compares assignment agreement, topology consensus, cylinder count, axis length, and derived woody-volume variation.",
        "",
        "The selected candidate remains complete only for observed woody evidence. Cross-parameter agreement establishes reconstruction stability, not component-level biological correctness or calibrated whole-tree volume.",
        "",
        "## Literature Review",
        "",
        "See `docs/BUILDING_VEGETATION_OBJECT_MODEL_LITERATURE_REVIEW.md`. The generated model records thirty-four stable source identifiers (`BVO-LIT-001` through `BVO-LIT-034`) so every research-derived field can name its evidence domain.",
        "",
        "## Acceptance Boundary",
        "",
        "The grammar and deterministic camera render remain geometry authority. ImageGen references provide appearance intent constrained back toward the rendered silhouette, extents, orientation, and component placement before scoring.",
        "",
    ])
    outputs[SUITE_DIRECTORY / "BUILDING_VEGETATION_OBJECT_MODEL.md"] = (
        "\n".join(model_markdown).encode()
    )

    stream = io.StringIO(newline="")
    writer = csv.DictWriter(
        stream,
        fieldnames=coverage_rows[0].keys(),
        lineterminator="\n",
    )
    writer.writeheader()
    writer.writerows(coverage_rows)
    outputs[EVIDENCE_DIRECTORY / "vegetation_primitive_coverage.csv"] = (
        stream.getvalue().encode()
    )
    return manifest, outputs


def write_outputs(outputs: dict[Path, bytes]) -> None:
    for path, content in outputs.items():
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(content)


def check_outputs(outputs: dict[Path, bytes]) -> None:
    mismatches = [
        str(path.relative_to(REPOSITORY_ROOT))
        for path, expected in outputs.items()
        if not path.is_file() or path.read_bytes() != expected
    ]
    if mismatches:
        raise SystemExit("Vegetation object suite is stale: " + ", ".join(mismatches))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    arguments = parser.parse_args()
    manifest, outputs = generate_outputs()
    if arguments.check:
        check_outputs(outputs)
        print(
            "Vegetation building-object grammars are current: "
            f"objects={manifest['object_count']} primitives={len(manifest['all_vegetation_primitives'])}"
        )
    else:
        write_outputs(outputs)
        print(
            "Generated vegetation building-object grammars: "
            f"objects={manifest['object_count']} primitives={len(manifest['all_vegetation_primitives'])}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
