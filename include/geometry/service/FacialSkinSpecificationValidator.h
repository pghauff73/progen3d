#pragma once

#include "geometry/model/FacialSkinShapeSpecification.h"

#include <memory>
#include <string>

struct FacialSkinShapeSpecificationCandidate
{
	glm::vec3 size{0.16f, 0.21f, 0.045f};
	int horizontal_segments = 48;
	int vertical_segments = 64;
	FacialExpressionControls controls;
	FacialSkinResponseMode response_mode = FacialSkinResponseMode::Analytic;
	FacialElasticMaterial elastic_material;
	FacialElasticSolveSettings elastic_solve_settings;
	std::vector<FacialMuscleSpecification> muscles;
	std::vector<FacialAttachmentPatch> attachments;
	std::vector<FacialTissueRegion> tissue_regions;
	FacialBendingSettings bending_settings;
	FacialMouthMeshMode mouth_mesh_mode = FacialMouthMeshMode::Legacy;
	FacialIdentityShape identity_shape;
	bool has_skin_material_option = false;
	bool has_solve_option = false;
	bool has_muscle_option = false;
	bool has_attachment_option = false;
	bool has_tissue_region_option = false;
	bool has_bending_option = false;
	bool has_mouth_mesh_option = false;
};

class FacialSkinSpecificationValidator
{
public:
	std::shared_ptr<const FacialSkinShapeSpecification> validate(
		const FacialSkinShapeSpecificationCandidate &candidate,
		std::string *diagnostic) const;
};
