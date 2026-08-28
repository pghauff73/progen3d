TARGET = progen3d-editor-gui
LEGACY_TARGET = progen3d
BUILD_DIR = build/obj
SRC_DIR = src
INCLUDE_DIR = include

VENDOR_PREFIX ?= third_party/vendor/install/linux-x86_64
VENDOR_LIB_DIR ?= $(VENDOR_PREFIX)/lib
VENDOR_LIB64_DIR ?= $(VENDOR_PREFIX)/lib64
IMGUI_CORE_DIR ?= $(firstword $(wildcard third_party/vendor/imgui-1.90.1))
GLAD_INCLUDE_DIR ?= third_party/vendor/glad/include
NFD_DIR ?= third_party/vendor/nativefiledialog-extended-1.3.0

CXX = g++
CC = gcc

IMGUI_INCLUDE_DIR ?= $(IMGUI_CORE_DIR)
IMGUI_BACKEND_INCLUDE_DIR ?= $(IMGUI_CORE_DIR)/backends
IMGUI_BACKEND_DIR ?= $(IMGUI_CORE_DIR)/backends

STATIC_GLFW_LIB ?= $(firstword $(wildcard $(VENDOR_LIB_DIR)/libglfw3.a $(VENDOR_LIB64_DIR)/libglfw3.a))
STATIC_CURL_LIB ?= $(firstword $(wildcard $(VENDOR_LIB_DIR)/libcurl.a $(VENDOR_LIB64_DIR)/libcurl.a))
STATIC_SSL_LIB ?= $(firstword $(wildcard $(VENDOR_LIB_DIR)/libssl.a $(VENDOR_LIB64_DIR)/libssl.a))
STATIC_CRYPTO_LIB ?= $(firstword $(wildcard $(VENDOR_LIB_DIR)/libcrypto.a $(VENDOR_LIB64_DIR)/libcrypto.a))
STATIC_CURL_ARCHIVES := $(strip $(STATIC_CURL_LIB) $(STATIC_SSL_LIB) $(STATIC_CRYPTO_LIB))

GLFW_LIB ?= $(STATIC_GLFW_LIB)

ifeq ($(strip $(GLFW_LIB)),)
GLFW_LIB = -lglfw
endif
GLFW_PLATFORM_LIBS ?= $(shell pkg-config --static --libs x11 xrandr xi xinerama xcursor 2>/dev/null)
DBUS_CFLAGS ?= $(shell pkg-config --cflags dbus-1 2>/dev/null)
DBUS_LIBS ?= $(shell pkg-config --libs dbus-1 2>/dev/null)
JSONCPP_LIBS ?= $(shell pkg-config --libs jsoncpp 2>/dev/null)
LIBXML2_CFLAGS ?= $(shell pkg-config --cflags libxml-2.0 2>/dev/null)
LIBXML2_LIBS ?= $(shell pkg-config --libs libxml-2.0 2>/dev/null)
EIGEN3_CFLAGS ?= $(shell pkg-config --cflags eigen3 2>/dev/null)
CRYPTO_LIBS ?= -lcrypto

ifeq ($(strip $(JSONCPP_LIBS)),)
JSONCPP_LIBS = -ljsoncpp
endif

ifeq ($(words $(STATIC_CURL_ARCHIVES)),3)
CURL_LIBS ?= $(STATIC_CURL_ARCHIVES)
else
CURL_LIBS ?= -lcurl
endif

COMMON_INCLUDE_FLAGS = \
	-I$(INCLUDE_DIR) \
	-I. \
	-I/usr/include \
	-I$(IMGUI_INCLUDE_DIR) \
	-I$(IMGUI_BACKEND_INCLUDE_DIR) \
	-Ithird_party/debs/glfw-dev/usr/include \
	-I$(VENDOR_PREFIX)/include \
	-I/usr/include/glm \
	-I/usr/include/stb \
	-Ithird_party/debs/stb/usr/include/stb \
	-I$(GLAD_INCLUDE_DIR) \
	-I$(NFD_DIR)/include \
	$(DBUS_CFLAGS) \
	$(LIBXML2_CFLAGS) \
	$(EIGEN3_CFLAGS)

IMGUI_USER_CONFIG_DEFINE = -DIMGUI_USER_CONFIG=\"imgui_user_config.h\"
NFD_BACKEND_DEFINE = -DNFD_PORTAL
CXXFLAGS = -g -O3 -std=c++17 $(COMMON_INCLUDE_FLAGS) $(IMGUI_USER_CONFIG_DEFINE) $(NFD_BACKEND_DEFINE)
CFLAGS = -g -O3 $(COMMON_INCLUDE_FLAGS)
DEPFLAGS = -MMD -MP
APPLICATION_WARNING_FLAGS = -Wall -Wextra -Wpedantic

APP_CPP_SOURCES = \
	$(SRC_DIR)/vegetation/service/VegetationTriangleDistributionService.cpp \
	$(SRC_DIR)/vegetation/service/VegetationRepresentationFidelityEvaluationService.cpp \
	$(SRC_DIR)/vegetation/service/VegetationBiologicalProfileValidationService.cpp \
	$(SRC_DIR)/vegetation/service/VegetationCalibrationReadinessEvaluationService.cpp \
	$(SRC_DIR)/vegetation/model/VegetationCalibrationMeasurement.cpp \
	$(SRC_DIR)/vegetation/model/VegetationCalibrationEvidenceBundle.cpp \
	$(SRC_DIR)/vegetation/service/VegetationCalibrationEvidenceBundleSerializationService.cpp \
	$(SRC_DIR)/vegetation/service/VegetationCalibrationPayloadHashService.cpp \
	$(SRC_DIR)/vegetation/service/VegetationCalibrationEvidenceBundleParsingService.cpp \
	$(SRC_DIR)/vegetation/service/VegetationCalibrationEvidenceBundleValidationService.cpp \
	$(SRC_DIR)/vegetation/service/VegetationCalibrationEvidenceBindingService.cpp \
	$(SRC_DIR)/vegetation/model/VegetationMeasuredSourceArtifact.cpp \
	$(SRC_DIR)/vegetation/service/VegetationMeasuredSourceArtifactHashService.cpp \
	$(SRC_DIR)/vegetation/service/VegetationMeasuredSourceArtifactValidationService.cpp \
	$(SRC_DIR)/vegetation/service/VegetationMeasuredCoordinateNormalizationService.cpp \
	$(SRC_DIR)/vegetation/service/VegetationMeasurementUnitNormalizationService.cpp \
	$(SRC_DIR)/vegetation/service/VegetationMeasuredCalibrationMeasurementFactory.cpp \
	$(SRC_DIR)/vegetation/service/VegetationMeasuredEvidenceBundleFactory.cpp \
	$(SRC_DIR)/vegetation/service/VegetationMeasuredSourceJsonDocumentReader.cpp \
	$(SRC_DIR)/vegetation/service/VegetationQuantitativeStructureModelAdapter.cpp \
	$(SRC_DIR)/vegetation/service/VegetationRootArchitectureGraphAdapter.cpp \
	$(SRC_DIR)/vegetation/service/VegetationCanopyObservationAdapter.cpp \
	$(SRC_DIR)/vegetation/service/VegetationPhenologyObservationSeriesAdapter.cpp \
	$(SRC_DIR)/vegetation/service/VegetationBiomechanicalMaterialTestAdapter.cpp \
	$(SRC_DIR)/vegetation/service/VegetationMeasuredSourceSchemaRegistry.cpp \
	$(SRC_DIR)/vegetation/service/VegetationMeasuredCoordinateReferenceTransformService.cpp \
	$(SRC_DIR)/vegetation/service/VegetationMeasuredPointCanonicalizationService.cpp \
	$(SRC_DIR)/vegetation/service/VegetationMeasuredSourceDecoderArtifactValidationService.cpp \
	$(SRC_DIR)/vegetation/service/VegetationDecodedMeasuredSourceArtifactFactory.cpp \
	$(SRC_DIR)/vegetation/service/VegetationDelimitedSourceTableReader.cpp \
	$(SRC_DIR)/vegetation/service/VegetationTreeQsmCylinderTableDecoder.cpp \
	$(SRC_DIR)/vegetation/service/VegetationRootSystemMarkupLanguageDecoder.cpp \
	$(SRC_DIR)/vegetation/service/VegetationCanopyObservationTableDecoder.cpp \
	$(SRC_DIR)/vegetation/service/VegetationPhenologyObservationTableDecoder.cpp \
	$(SRC_DIR)/vegetation/service/VegetationBiomechanicalMaterialTestTableDecoder.cpp \
	$(SRC_DIR)/vegetation/service/VegetationMeasuredSourceDecodingService.cpp \
	$(SRC_DIR)/vegetation/service/VegetationPointCloudFormatCapabilityCatalog.cpp \
	$(SRC_DIR)/vegetation/service/VegetationPointCloudSourceArtifactValidationService.cpp \
	$(SRC_DIR)/vegetation/service/VegetationPlyPointCloudDecoder.cpp \
	$(SRC_DIR)/vegetation/service/VegetationLasPointCloudDecoder.cpp \
	$(SRC_DIR)/vegetation/service/VegetationPointCloudIngestionService.cpp \
	$(SRC_DIR)/vegetation/service/VegetationPointCloudReconstructionAdmissionService.cpp \
	$(SRC_DIR)/vegetation/service/VegetationPointCloudReconstructionJobFactory.cpp \
	$(SRC_DIR)/vegetation/service/VegetationCanopyOccupancyQualityEvaluationService.cpp \
	$(SRC_DIR)/vegetation/service/VegetationCanopyOccupancyReconstructionService.cpp \
	$(SRC_DIR)/vegetation/service/VegetationWoodyAxisQualityEvaluationService.cpp \
	$(SRC_DIR)/vegetation/service/VegetationPrimaryWoodyAxisReconstructionService.cpp \
	$(SRC_DIR)/vegetation/service/VegetationWoodyPointSegmentationValidationService.cpp \
	$(SRC_DIR)/vegetation/service/VegetationWoodyBranchGraphQualityEvaluationService.cpp \
	$(SRC_DIR)/vegetation/service/VegetationSegmentedWoodyBranchGraphReconstructionService.cpp \
	$(SRC_DIR)/vegetation/service/VegetationWoodyBranchGraphQuantitativeStructureModelArtifactFactory.cpp \
	$(SRC_DIR)/vegetation/service/VegetationWoodyCoverSetSegmentationService.cpp \
	$(SRC_DIR)/vegetation/service/VegetationWoodySegmentationSensitivityEvaluationService.cpp \
	$(SRC_DIR)/vegetation/service/VegetationAutomatedWoodySegmentationEnsembleService.cpp \
	$(SRC_DIR)/chair/model/ChairObjectModel.cpp \
	$(SRC_DIR)/chair/model/ChairThreeViewFitReport.cpp \
	$(SRC_DIR)/chair/service/ChairCatalog.cpp \
	$(SRC_DIR)/chair/service/ChairAssemblyBuilder.cpp \
	$(SRC_DIR)/chair/service/ChairThreeViewFittingService.cpp \
	$(SRC_DIR)/lighting/model/SceneLight.cpp \
	$(SRC_DIR)/lighting/model/PreviewLightCollection.cpp \
	$(SRC_DIR)/lighting/model/LightingSceneDefinition.cpp \
	$(SRC_DIR)/lighting/rendering/PreviewLightBuffer.cpp \
	$(SRC_DIR)/lighting/service/PhotometricColorService.cpp \
	$(SRC_DIR)/lighting/service/GpuLightRecordFactory.cpp \
	$(SRC_DIR)/lighting/service/LightGizmoOverlayService.cpp \
	$(SRC_DIR)/lighting/service/LightGizmoPickingService.cpp \
	$(SRC_DIR)/lighting/service/LightingPresetCatalog.cpp \
	$(SRC_DIR)/lighting/service/LightingScenePersistenceService.cpp \
	$(SRC_DIR)/lighting/service/ShadowAssignmentService.cpp \
	$(SRC_DIR)/lighting/service/PreviewLightingFrameFactory.cpp \
	$(SRC_DIR)/lighting/service/LightingValidationService.cpp \
	$(SRC_DIR)/lighting/service/LightingEvidenceHashService.cpp \
	$(SRC_DIR)/electrical/model/ElectricalControlGraph.cpp \
	$(SRC_DIR)/electrical/service/LightingStateEvaluator.cpp \
	$(SRC_DIR)/electrical/service/ElectricalControlValidationService.cpp \
	$(SRC_DIR)/geometry/model/Curve3D.cpp \
	$(SRC_DIR)/geometry/model/Surface3D.cpp \
	$(SRC_DIR)/geometry/model/CurveNetworkSurface.cpp \
	$(SRC_DIR)/geometry/model/CurveArcLengthTable.cpp \
	$(SRC_DIR)/geometry/model/TransformedCurve3D.cpp \
	$(SRC_DIR)/geometry/model/MirrorCurve3D.cpp \
	$(SRC_DIR)/geometry/model/BinarySilhouette.cpp \
	$(SRC_DIR)/geometry/model/ShapeClosurePolicy.cpp \
	$(SRC_DIR)/geometry/model/ResolvedPrimitiveGeometry.cpp \
	$(SRC_DIR)/geometry/model/InstanceArraySpecification.cpp \
	$(SRC_DIR)/geometry/service/ShapeAliasExpansionService.cpp \
	$(SRC_DIR)/geometry/service/CurveArcLengthService.cpp \
	$(SRC_DIR)/geometry/service/ProceduralShapeCatalogRepository.cpp \
	$(SRC_DIR)/geometry/service/ShapeSpecificationValidator.cpp \
	$(SRC_DIR)/geometry/service/GeneratedMeshReferenceSpecificationValidator.cpp \
	$(SRC_DIR)/geometry/service/GeneratedMeshReferenceResolutionService.cpp \
	$(SRC_DIR)/geometry/service/ClosedSurfaceFaceOrientationService.cpp \
	$(SRC_DIR)/geometry/service/MeshTopologyAnalyzer.cpp \
	$(SRC_DIR)/geometry/service/TriangleGeometryRelationshipService.cpp \
	$(SRC_DIR)/geometry/service/MeshSurfaceDistanceEvaluationService.cpp \
	$(SRC_DIR)/geometry/service/MeshIntersectionScreeningService.cpp \
	$(SRC_DIR)/geometry/service/MeshRayParityClassificationService.cpp \
	$(SRC_DIR)/geometry/service/RigidTransformValidationService.cpp \
	$(SRC_DIR)/geometry/service/MeshRigidTransformationService.cpp \
	$(SRC_DIR)/geometry/service/ConvexHullMeshGenerationService.cpp \
	$(SRC_DIR)/geometry/service/StlMeshLoadingService.cpp \
	$(SRC_DIR)/geometry/service/ImplicitSurfaceMeshingService.cpp \
	$(SRC_DIR)/geometry/service/ThreeViewProjectionService.cpp \
	$(SRC_DIR)/geometry/service/BinarySilhouetteComparisonService.cpp \
	$(SRC_DIR)/geometry/service/ProfileSectionProjectionService.cpp \
	$(SRC_DIR)/geometry/service/HalfSpaceMeshClipper.cpp \
	$(SRC_DIR)/geometry/service/SimplePolygonValidator.cpp \
	$(SRC_DIR)/geometry/service/SimplePolygonTriangulator.cpp \
	$(SRC_DIR)/geometry/service/PolygonContainmentAnalyzer.cpp \
	$(SRC_DIR)/geometry/service/Profile2DValidator.cpp \
	$(SRC_DIR)/geometry/service/Profile2DFactory.cpp \
	$(SRC_DIR)/geometry/service/ProfileCapTriangulator.cpp \
	$(SRC_DIR)/geometry/service/ExtrudeProfileSpecificationValidator.cpp \
	$(SRC_DIR)/geometry/service/ExtrudeProfileMeshGenerator.cpp \
	$(SRC_DIR)/geometry/service/SweepProfileSpecificationValidator.cpp \
	$(SRC_DIR)/geometry/service/SweepProfileMeshGenerator.cpp \
	$(SRC_DIR)/geometry/service/ThinWallProfileFactory.cpp \
	$(SRC_DIR)/geometry/service/VariableSectionSweepSpecificationValidator.cpp \
	$(SRC_DIR)/geometry/service/VariableSectionSweepMeshGenerator.cpp \
	$(SRC_DIR)/geometry/service/FormedPanelSpecificationValidator.cpp \
	$(SRC_DIR)/geometry/service/HostedOpeningSpecificationValidator.cpp \
	$(SRC_DIR)/geometry/service/EmbossedBeadSpecificationValidator.cpp \
	$(SRC_DIR)/geometry/service/EdgeFlangeSpecificationValidator.cpp \
	$(SRC_DIR)/geometry/service/CompoundShapeSpecificationValidator.cpp \
	$(SRC_DIR)/geometry/service/SweepDiskSpecificationValidator.cpp \
	$(SRC_DIR)/geometry/service/SweepDiskMeshGenerator.cpp \
	$(SRC_DIR)/geometry/service/TaperedSweepSpecificationValidator.cpp \
	$(SRC_DIR)/geometry/service/TaperedSweepMeshGenerator.cpp \
	$(SRC_DIR)/geometry/service/BranchJunctionSpecificationValidator.cpp \
	$(SRC_DIR)/geometry/service/BranchJunctionMeshGenerator.cpp \
	$(SRC_DIR)/geometry/service/BotanicalBladeSpecificationValidator.cpp \
	$(SRC_DIR)/geometry/service/BotanicalBladeMeshGenerator.cpp \
	$(SRC_DIR)/geometry/service/RevolveSpecificationValidator.cpp \
	$(SRC_DIR)/geometry/service/RevolveMeshGenerator.cpp \
	$(SRC_DIR)/geometry/service/LoftSpecificationValidator.cpp \
	$(SRC_DIR)/geometry/service/LoftMeshGenerator.cpp \
	$(SRC_DIR)/geometry/service/CurvedPanelSpecificationValidator.cpp \
	$(SRC_DIR)/geometry/service/CurvedPanelMeshGenerator.cpp \
	$(SRC_DIR)/geometry/service/CurveNetworkSurfaceMeshGenerator.cpp \
	$(SRC_DIR)/geometry/service/CurveNetworkSurfaceSpecificationValidator.cpp \
	$(SRC_DIR)/geometry/service/GeneratedMeshComposer.cpp \
	$(SRC_DIR)/geometry/service/MirroredShapeGeometryBuilder.cpp \
	$(SRC_DIR)/geometry/service/ShellOffsetGeometryBuilder.cpp \
	$(SRC_DIR)/geometry/service/RoundedBoxShapeSpecificationFactory.cpp \
	$(SRC_DIR)/geometry/service/FoldedProfileSpecificationValidator.cpp \
	$(SRC_DIR)/geometry/service/FoldedProfileMeshGenerator.cpp \
	$(SRC_DIR)/geometry/service/InstanceArrayGeometryBuilder.cpp \
	$(SRC_DIR)/geometry/service/AxialProfileSpecificationValidator.cpp \
	$(SRC_DIR)/geometry/service/AxialProfileMeshGenerator.cpp \
	$(SRC_DIR)/geometry/service/CylinderMeshGenerator.cpp \
	$(SRC_DIR)/geometry/service/SphereMeshGenerator.cpp \
	$(SRC_DIR)/geometry/service/ProceduralGeometryEvidenceService.cpp \
	$(SRC_DIR)/geometry/service/ProceduralMeshRepository.cpp \
	$(SRC_DIR)/geometry/service/PrimitiveGeometryResolver.cpp \
	$(SRC_DIR)/vegetation/service/BranchGraphValidationService.cpp \
	$(SRC_DIR)/vegetation/service/BranchRadiusConservationService.cpp \
	$(SRC_DIR)/vegetation/service/OrganPlacementService.cpp \
	$(SRC_DIR)/vegetation/service/OrganArrayPlacementService.cpp \
	$(SRC_DIR)/vegetation/service/FlowerHeadPlacementService.cpp \
	$(SRC_DIR)/vegetation/service/InflorescencePlacementService.cpp \
	$(SRC_DIR)/vegetation/service/CompoundLeafPlacementService.cpp \
	$(SRC_DIR)/vegetation/service/FruitShellGeometryFactory.cpp \
	$(SRC_DIR)/vegetation/service/PlantOrganGeometryFactory.cpp \
	$(SRC_DIR)/vegetation/service/TropismDirectionService.cpp \
	$(SRC_DIR)/vegetation/service/DeterministicPlantVariationService.cpp \
	$(SRC_DIR)/vegetation/service/BranchGraphDeterministicHashService.cpp \
	$(SRC_DIR)/vegetation/service/PlantTopologyDecompositionService.cpp \
	$(SRC_DIR)/vegetation/service/PlantLSystemSpecificationValidationService.cpp \
	$(SRC_DIR)/vegetation/service/LSystemBranchGraphGenerationService.cpp \
	$(SRC_DIR)/vegetation/service/PlantGrowthService.cpp \
	$(SRC_DIR)/vegetation/service/SpaceColonizationSpecificationValidationService.cpp \
	$(SRC_DIR)/vegetation/service/SpaceColonizationSeedGraphFactory.cpp \
	$(SRC_DIR)/vegetation/service/PlantOrganAttachmentService.cpp \
	$(SRC_DIR)/vegetation/service/PlantTopologyGenerationService.cpp \
	$(SRC_DIR)/vegetation/service/CrownVolumeContainmentService.cpp \
	$(SRC_DIR)/vegetation/service/CrownAttractionPointSamplingService.cpp \
	$(SRC_DIR)/vegetation/service/SpaceColonizationService.cpp \
	$(SRC_DIR)/vegetation/service/SurfaceAttachmentService.cpp \
	$(SRC_DIR)/vegetation/service/VineGrowthService.cpp \
	$(SRC_DIR)/vegetation/service/ScatterRegionService.cpp \
	$(SRC_DIR)/vegetation/service/PlantSpeciesCatalog.cpp \
	$(SRC_DIR)/vegetation/service/PlantShapeSpecificationValidator.cpp \
	$(SRC_DIR)/vegetation/service/PlantMeshGenerator.cpp \
	$(SRC_DIR)/vegetation/service/VineShapeSpecificationValidator.cpp \
	$(SRC_DIR)/vegetation/service/VineMeshGenerator.cpp \
	$(SRC_DIR)/vegetation/service/ScatterRegionShapeSpecificationValidator.cpp \
	$(SRC_DIR)/vegetation/service/ScatterRegionMeshGenerator.cpp \
	$(SRC_DIR)/vegetation/service/RuleBranchingGenerationService.cpp \
	$(SRC_DIR)/vegetation/service/VegetationGeometryAssemblyService.cpp \
	$(SRC_DIR)/architecture/model/ArchitecturalReferenceBindingCatalog.cpp \
	$(SRC_DIR)/architecture/service/LayerSetGeometryBuilder.cpp \
	$(SRC_DIR)/architecture/service/ArchitecturalProfileLibrary.cpp \
	$(SRC_DIR)/architecture/service/WindowOpeningCollisionPositioningService.cpp \
	$(SRC_DIR)/architecture/service/WindowFrameAssemblyBuilder.cpp \
	$(SRC_DIR)/architecture/service/HostedWindowWallAssemblyBuilder.cpp \
	$(SRC_DIR)/architecture/service/GlazingUnitAssemblyBuilder.cpp \
	$(SRC_DIR)/architecture/service/PanelBoxAssemblyBuilder.cpp \
	$(SRC_DIR)/architecture/service/PanelArrayAssemblyBuilder.cpp \
	$(SRC_DIR)/architecture/service/StairAssemblyBuilder.cpp \
	$(SRC_DIR)/architecture/service/SanitaryBasinAssemblyBuilder.cpp \
	$(SRC_DIR)/architecture/service/PoolAssemblyBuilder.cpp \
	$(SRC_DIR)/architecture/service/SurfaceTilingGeometryBuilder.cpp \
	$(SRC_DIR)/vehicle/model/VehiclePackage.cpp \
	$(SRC_DIR)/vehicle/model/VehicleDatum.cpp \
	$(SRC_DIR)/vehicle/model/VehicleValidationReport.cpp \
	$(SRC_DIR)/vehicle/model/VehicleJointGraph.cpp \
	$(SRC_DIR)/vehicle/parametric/model/ParametricVehicleValidationReport.cpp \
	$(SRC_DIR)/vehicle/parametric/service/ModernCarParametricCatalogFactory.cpp \
	$(SRC_DIR)/vehicle/parametric/service/ModernCarFieldEvaluationService.cpp \
	$(SRC_DIR)/vehicle/parametric/service/ModernCarIsoSurfaceGenerationService.cpp \
	$(SRC_DIR)/vehicle/parametric/service/ModernCarCharacterCurveExtractionService.cpp \
	$(SRC_DIR)/vehicle/parametric/service/ModernCarBodySectionExtractionService.cpp \
	$(SRC_DIR)/vehicle/parametric/service/ModernCarObservationGenerationService.cpp \
	$(SRC_DIR)/vehicle/parametric/service/ModernCarParametricDeterministicHashService.cpp \
	$(SRC_DIR)/vehicle/parametric/service/ModernCarParametricValidationService.cpp \
	$(SRC_DIR)/vehicle/parametric/service/ModernCarParametricGrammarLoweringService.cpp \
	$(SRC_DIR)/vehicle/parametric/service/Mvp26ParametricVehicleIntegrationService.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/model/VehicleSemanticRoles.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/model/VehicleSectionScalarField.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/model/ShapePreservingCubicInterpolation.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/model/ImplicitVehicleFields.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/ModernCarSemanticCatalogFactory.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/VehicleWheelMotionEnvelopeConstructionService.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/VehicleSemanticSectionInterpolationService.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/VehicleSemanticSectionFieldEvaluationService.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/VehicleSectionInterpolationRelationshipService.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/VehicleCharacterCurveExtractionService.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/VehicleSemanticSurfaceGenerationService.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/ModernCarSemanticBodyFieldFactory.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/ImplicitFieldCalibrationResolutionService.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/ModernCarSemanticIsoSurfaceGenerationService.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/McsMv2GeneratedMeshProvider.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/McsMv21SemanticCatalogFactory.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/VehicleSurfaceProjectionService.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/SemanticImplicitAgreementEvaluationService.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/VehicleSemanticSurfaceRegistrationService.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/VehicleSurfaceDomainEvaluationService.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/VehicleSurfaceDomainPartitionService.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/VehicleGlassApertureConstructionService.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/McsMv21SemanticGeometryGenerationService.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/McsMv21GeneratedMeshProvider.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/VehicleWheelGrammarLoweringService.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/ModernCarSemanticGrammarLoweringService.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/McsMv21SemanticGrammarLoweringService.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/McsMv22KinematicCatalogFactory.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/McsMv22NativePartitionGeometryLoadingService.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/McsMv22NativeKinematicEvidenceLoadingService.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/McsMv22SemanticKinematicBindingService.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/McsMv22SuspensionKinematicEvaluationService.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/McsMv22ClosureKinematicEvaluationService.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/McsMv22KinematicAssuranceEvaluationService.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/McsMv22KinematicGeometryGenerationService.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/McsMv22KinematicPoseEvaluationService.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/McsMv22GeneratedMeshProvider.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/McsMv22KinematicGrammarLoweringService.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/VehiclePanelPatchExtractionService.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/VehiclePanelPatchValidationService.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/VehicleBodyInWhiteAssemblyService.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/VehicleBodyInWhiteValidationService.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/VehicleOccupantEnvelopeConstructionService.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/VehicleOccupantEnvelopeValidationService.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/VehicleFunctionalPackageConstructionService.cpp \
	$(SRC_DIR)/vehicle/mcsmv2/service/VehicleFunctionalPackageValidationService.cpp \
	$(SRC_DIR)/vehicle/service/VehiclePackageValidationService.cpp \
	$(SRC_DIR)/vehicle/service/VehicleDatumDerivationService.cpp \
	$(SRC_DIR)/vehicle/service/VehicleAssemblyCompositionService.cpp \
	$(SRC_DIR)/vehicle/service/VehicleBodyAssemblyBuilder.cpp \
	$(SRC_DIR)/vehicle/service/WheelAssemblyBuilder.cpp \
	$(SRC_DIR)/vehicle/service/SuspensionCornerAssemblyBuilder.cpp \
	$(SRC_DIR)/vehicle/service/VehicleKinematicValidationService.cpp \
	$(SRC_DIR)/vehicle/service/VehicleDeterministicHashService.cpp \
	$(SRC_DIR)/vehicle/service/VehicleClosureKinematicsService.cpp \
	$(SRC_DIR)/vehicle/service/VehicleSuspensionKinematicEvaluationService.cpp \
	$(SRC_DIR)/vehicle/service/VehicleReferenceFrameTransformationService.cpp \
	$(SRC_DIR)/vehicle/service/VehicleTyreMeshGenerationService.cpp \
	$(SRC_DIR)/vehicle/service/VehicleTyrePoseTransformationService.cpp \
	$(SRC_DIR)/vehicle/service/VehicleTyreSweepValidationService.cpp \
	$(SRC_DIR)/vehicle/service/VehicleClosureMotionEvaluationService.cpp \
	$(SRC_DIR)/vehicle/service/VehicleClosureSweepValidationService.cpp \
	$(SRC_DIR)/vehicle/service/VehicleGlassMotionValidationService.cpp \
	$(SRC_DIR)/vehicle/service/VehicleFittingValidationService.cpp \
	$(SRC_DIR)/vehicle/service/VehicleFittingDeterministicHashService.cpp \
	$(SRC_DIR)/vehicle/service/VehicleProjectionService.cpp \
	$(SRC_DIR)/vehicle/service/VehicleClassAValidationService.cpp \
	$(SRC_DIR)/vehicle/service/AutomotiveClosureGeometryService.cpp \
	$(SRC_DIR)/vehicle/service/VehicleChassisKinematicsService.cpp \
	$(SRC_DIR)/vehicle/service/VehicleFitObjectiveService.cpp \
	$(SRC_DIR)/vehicle/service/VehicleMvp25ValidationService.cpp \
	$(SRC_DIR)/vehicle/service/VehicleMvp25DeterministicHashService.cpp \
	$(SRC_DIR)/vehicle/service/VehicleMvp26ValidationService.cpp \
	$(SRC_DIR)/vehicle/service/VehicleMvp26DeterministicHashService.cpp \
	$(SRC_DIR)/vehicle/service/VehicleValidationService.cpp \
	$(SRC_DIR)/vehicle/service/ModernEvFastbackBuilder.cpp \
	$(SRC_DIR)/vehicle/service/RedAwdHatchbackMvpv2Builder.cpp \
	$(SRC_DIR)/vehicle/service/RedAwdHatchbackMvp25Builder.cpp \
	$(SRC_DIR)/vehicle/service/ModernCarMvp26FamilyBuilder.cpp \
	$(SRC_DIR)/physics/service/ConvexCollisionDetector.cpp \
	$(SRC_DIR)/spatial/graph/SpatialObjectRegistry.cpp \
	$(SRC_DIR)/spatial/graph/SpatialContainmentTree.cpp \
	$(SRC_DIR)/spatial/model/SpatialBuildingModel.cpp \
	$(SRC_DIR)/spatial/service/SpatialFrameValidationService.cpp \
	$(SRC_DIR)/spatial/service/SpatialContainmentValidationService.cpp \
	$(SRC_DIR)/spatial/service/SpatialInterfaceCompatibilityService.cpp \
	$(SRC_DIR)/spatial/service/SpatialConstraintDependencyAnalyzer.cpp \
	$(SRC_DIR)/spatial/service/SpatialObjectStateTransitionService.cpp \
	$(SRC_DIR)/spatial/service/SpatialConnectionStateTransitionService.cpp \
	$(SRC_DIR)/spatial/service/SpatialWorldFrameResolutionService.cpp \
	$(SRC_DIR)/spatial/service/SpatialResolutionEvidenceHashService.cpp \
	$(SRC_DIR)/spatial/service/SpatialBuildingModelConstructionService.cpp \
	$(SRC_DIR)/spatial/service/SpatialBuildingModelConstructionContext.cpp \
	$(SRC_DIR)/spatial/service/SpatialAabbQueryService.cpp \
	$(SRC_DIR)/spatial/service/SpatialDistanceQueryService.cpp \
	$(SRC_DIR)/spatial/service/SpatialContainmentQueryService.cpp \
	$(SRC_DIR)/spatial/service/SpatialInterfaceQueryService.cpp \
	$(SRC_DIR)/spatial/service/SpatialObjectBoundaryDerivationService.cpp \
	$(SRC_DIR)/spatial/service/SpatialDirectionResolutionService.cpp \
	$(SRC_DIR)/spatial/service/SpatialCollisionQueryService.cpp \
	$(SRC_DIR)/spatial/service/CollisionPositioningSolver.cpp \
	$(SRC_DIR)/spatial/service/SpatialPlacementTransaction.cpp \
	$(SRC_DIR)/spatial/service/SpatialConstraintResidualValidationService.cpp \
	$(SRC_DIR)/spatial/service/SpatialAssemblyResolutionService.cpp \
	$(SRC_DIR)/building/generated/SmallModernBuildingKnowledgeCatalog.cpp \
	$(SRC_DIR)/building/service/BuildingClassificationValidationService.cpp \
	$(SRC_DIR)/building/service/BuildingEvidenceHashService.cpp \
	$(SRC_DIR)/building/service/BuildingFunctionCoverageEvaluationService.cpp \
	$(SRC_DIR)/building/service/BuildingObjectSemanticProfileHashService.cpp \
	$(SRC_DIR)/building/service/BuildingRequirementEvaluationService.cpp \
	$(SRC_DIR)/building/service/BuildingRelationshipAssertionValidationService.cpp \
	$(SRC_DIR)/building/service/BuildingScenarioStateTransitionService.cpp \
	$(SRC_DIR)/building/service/BuildingScenarioValidationService.cpp \
	$(SRC_DIR)/building/service/BuildingStateSnapshotHashService.cpp \
	$(SRC_DIR)/building/service/BuildingServiceTopologyValidationService.cpp \
	$(SRC_DIR)/building/service/SmallModernBuildingDeterministicHashService.cpp \
	$(SRC_DIR)/building/service/SmallModernBuildingModelConstructionContext.cpp \
	$(SRC_DIR)/building/service/SmallModernBuildingModelConstructionService.cpp \
	$(SRC_DIR)/building/service/SmallModernBuildingModelValidationService.cpp \
	$(SRC_DIR)/grammar/service/ShapeSpecificationParser.cpp \
	$(SRC_DIR)/grammar/service/ShapeSpecificationEvaluator.cpp \
	$(SRC_DIR)/grammar/service/LightingDeclarationParser.cpp \
	$(SRC_DIR)/grammar/service/SpatialObjectSpecificationParser.cpp \
	$(SRC_DIR)/grammar/service/SpatialInterfaceDeclarationParser.cpp \
	$(SRC_DIR)/grammar/service/SpatialConnectionDeclarationParser.cpp \
	$(SRC_DIR)/grammar/service/SpatialConstraintDeclarationParser.cpp \
	$(SRC_DIR)/grammar/service/VehicleJointDeclarationParser.cpp \
	$(SRC_DIR)/editor/model/AiGrammarProposalReview.cpp \
	$(SRC_DIR)/editor/model/ApplicationLog.cpp \
	$(SRC_DIR)/editor/model/DocumentDiagnosticCollection.cpp \
	$(SRC_DIR)/editor/model/DocumentReplacementWorkflow.cpp \
	$(SRC_DIR)/editor/model/GeneratedSceneSnapshot.cpp \
	$(SRC_DIR)/editor/model/GrammarSymbolIndex.cpp \
	$(SRC_DIR)/editor/model/PreviewProjectionConfiguration.cpp \
	$(SRC_DIR)/editor/model/PreviewTexturePresentation.cpp \
	$(SRC_DIR)/editor/controller/PreviewCameraController.cpp \
	$(SRC_DIR)/editor/controller/PreviewCameraMotionController.cpp \
	$(SRC_DIR)/editor/controller/PreviewTimelineController.cpp \
	$(SRC_DIR)/editor/controller/SceneSelectionController.cpp \
	$(SRC_DIR)/editor/relationship/ContextSceneSourceAssociationFactory.cpp \
	$(SRC_DIR)/editor/relationship/SceneSourceAssociationIndex.cpp \
	$(SRC_DIR)/editor/relationship/SpatialObjectPrimitiveAssociationIndex.cpp \
	$(SRC_DIR)/editor/application/ApplicationLaunchOptions.cpp \
	$(SRC_DIR)/editor/application/ApplicationStartupProgress.cpp \
	$(SRC_DIR)/editor/application/EditorRuntimeEnvironment.cpp \
	$(SRC_DIR)/editor/application/GlfwApplicationWindow.cpp \
	$(SRC_DIR)/editor/application/ImGuiRuntimeContext.cpp \
	$(SRC_DIR)/editor/presentation/ConsolePanel.cpp \
	$(SRC_DIR)/editor/presentation/DiagnosticsPanel.cpp \
	$(SRC_DIR)/editor/presentation/ElectricalControlsPanel.cpp \
	$(SRC_DIR)/editor/presentation/GrammarAutocompletePresentation.cpp \
	$(SRC_DIR)/editor/presentation/GrammarEditorPanel.cpp \
	$(SRC_DIR)/editor/presentation/GrammarDiagnosticPresentation.cpp \
	$(SRC_DIR)/editor/presentation/GrammarSymbolInspectorPanel.cpp \
	$(SRC_DIR)/editor/presentation/GrammarSyntaxPresentation.cpp \
	$(SRC_DIR)/editor/presentation/LightingPanel.cpp \
	$(SRC_DIR)/editor/presentation/PreviewOrientationControl.cpp \
	$(SRC_DIR)/editor/presentation/SceneInspectorPanel.cpp \
	$(SRC_DIR)/editor/presentation/SpatialObjectInspectorPanel.cpp \
	$(SRC_DIR)/editor/service/BackendAiGrammarProposalService.cpp \
	$(SRC_DIR)/editor/service/BackendCloudGrammarRepository.cpp \
	$(SRC_DIR)/editor/service/BackendTextureLibraryRepository.cpp \
	$(SRC_DIR)/editor/service/BuildingKnowledgeInspectionService.cpp \
	$(SRC_DIR)/editor/service/DocumentPersistenceService.cpp \
	$(SRC_DIR)/editor/service/FirebaseAuthenticationService.cpp \
	$(SRC_DIR)/editor/service/GrammarCompilationService.cpp \
	$(SRC_DIR)/editor/service/MeshExportService.cpp \
	$(SRC_DIR)/editor/service/NativeLocalFileDialogService.cpp \
	$(SRC_DIR)/editor/service/OpenGl46CapabilityValidator.cpp \
	$(SRC_DIR)/editor/service/PreviewFramebufferCaptureService.cpp \
	$(SRC_DIR)/editor/service/SceneMeshExportService.cpp \
	$(SRC_DIR)/editor/service/SceneRegenerationCoordinator.cpp \
	$(SRC_DIR)/editor/service/SpatialDeclarationCompletionService.cpp \
	$(SRC_DIR)/editor/service/SpatialObjectInspectionService.cpp \
	$(SRC_DIR)/editor/service/SpatialOverlayEvidenceService.cpp \
	$(SRC_DIR)/editor/service/StructuredShapeCompletionService.cpp \
	$(SRC_DIR)/editor/service/StlPartCatalogRepository.cpp \
	$(SRC_DIR)/main.cpp \
	$(SRC_DIR)/imgui_main.cpp \
	$(SRC_DIR)/imgui_render.cpp \
	$(SRC_DIR)/BackendApiClient.cpp \
	$(SRC_DIR)/AppPaths.cpp \
	$(SRC_DIR)/CloudAuthWorkflow.cpp \
	$(SRC_DIR)/FirebaseAuth.cpp \
	$(SRC_DIR)/FirebaseStorage.cpp \
	$(SRC_DIR)/imgui_stb_impl.cpp \
	$(SRC_DIR)/Grammar.cpp \
	$(SRC_DIR)/Context.cpp \
	$(SRC_DIR)/Mesh.cpp \
	$(SRC_DIR)/PLYWriter.cpp \
	$(SRC_DIR)/Scope.cpp \
	$(SRC_DIR)/Solution.cpp \
	$(SRC_DIR)/StlCatalog.cpp \
	$(NFD_DIR)/nfd_portal.cpp \
	stbimage/stb_image_impl.cpp

IMGUI_CPP_SOURCES = \
	$(IMGUI_CORE_DIR)/imgui.cpp \
	$(IMGUI_CORE_DIR)/imgui_draw.cpp \
	$(IMGUI_CORE_DIR)/imgui_tables.cpp \
	$(IMGUI_CORE_DIR)/imgui_widgets.cpp \
	$(IMGUI_BACKEND_DIR)/imgui_impl_glfw.cpp \
	$(IMGUI_BACKEND_DIR)/imgui_impl_opengl3.cpp

C_SOURCES = \
	third_party/vendor/glad/src/gl.c

APP_OBJS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(APP_CPP_SOURCES))
IMGUI_OBJS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(IMGUI_CPP_SOURCES))
C_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
OBJS = $(APP_OBJS) $(IMGUI_OBJS) $(C_OBJS)

$(APP_OBJS): CXXFLAGS += $(APPLICATION_WARNING_FLAGS)

LDFLAGS =
LIBS = $(GLFW_LIB) $(GLFW_PLATFORM_LIBS) -ldl -lm -lpthread $(CURL_LIBS) $(DBUS_LIBS) $(JSONCPP_LIBS) $(LIBXML2_LIBS) $(CRYPTO_LIBS)

.PHONY: all clean

all: $(TARGET) $(LEGACY_TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

$(LEGACY_TARGET): $(TARGET)
	ln -sfn $(TARGET) $(LEGACY_TARGET)

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c -o $@ $<

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(LEGACY_TARGET)

-include $(OBJS:.o=.d)
