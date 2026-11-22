#pragma once

#include "geometry/model/FacialExpressionControls.h"
#include "geometry/model/FacialAttachmentPatch.h"
#include "geometry/model/FacialElasticMaterial.h"
#include "geometry/model/FacialElasticSolveSettings.h"
#include "geometry/model/FacialMuscleSpecification.h"
#include "geometry/model/FacialSkinResponseMode.h"
#include "geometry/model/FacialTissueRegion.h"
#include "geometry/model/ShapeSpecification.h"

#include <glm/glm.hpp>
#include <vector>

// An open, continuous front-face surface with three anatomical apertures.
// The default analytic rig remains independent of the optional neutral-reference
// elastic membrane and explicit tensile-muscle specifications.
class FacialSkinShapeSpecification : public ShapeSpecification
{
public:
	FacialSkinShapeSpecification(glm::vec3 size,
	                            int horizontal_segments,
	                            int vertical_segments,
	                            FacialExpressionControls controls,
	                            ShapeSpecificationKey key,
	                            std::string canonical_text,
	                            FacialSkinResponseMode response_mode = FacialSkinResponseMode::Analytic,
	                            FacialElasticMaterial elastic_material = {},
	                            FacialElasticSolveSettings elastic_solve_settings = {},
	                            std::vector<FacialMuscleSpecification> muscles = {},
	                            std::vector<FacialAttachmentPatch> attachments = {},
	                            std::vector<FacialTissueRegion> tissue_regions = {})
		: ShapeSpecification(ShapeFamily::FacialSkin, std::move(key)),
		  size_(size),
		  horizontal_segments_(horizontal_segments),
		  vertical_segments_(vertical_segments),
		  controls_(controls),
		  canonical_text_(std::move(canonical_text)),
		  response_mode_(response_mode),
		  elastic_material_(elastic_material),
		  elastic_solve_settings_(elastic_solve_settings),
		  muscles_(std::move(muscles)),
		  attachments_(std::move(attachments)),
		  tissue_regions_(std::move(tissue_regions))
	{
	}

	const glm::vec3 &size() const { return size_; }
	int horizontalSegments() const { return horizontal_segments_; }
	int verticalSegments() const { return vertical_segments_; }
	const FacialExpressionControls &controls() const { return controls_; }
	FacialSkinResponseMode responseMode() const { return response_mode_; }
	const FacialElasticMaterial &elasticMaterial() const { return elastic_material_; }
	const FacialElasticSolveSettings &elasticSolveSettings() const { return elastic_solve_settings_; }
	const std::vector<FacialMuscleSpecification> &muscles() const { return muscles_; }
	const std::vector<FacialAttachmentPatch> &attachments() const { return attachments_; }
	const std::vector<FacialTissueRegion> &tissueRegions() const { return tissue_regions_; }
	std::string canonicalText() const override { return canonical_text_; }
	bool isDefaultFamilyShape() const override { return false; }
	bool requestsClosedGeometry() const override { return false; }

private:
	glm::vec3 size_{0.16f, 0.21f, 0.045f};
	int horizontal_segments_ = 48;
	int vertical_segments_ = 64;
	FacialExpressionControls controls_;
	std::string canonical_text_;
	FacialSkinResponseMode response_mode_ = FacialSkinResponseMode::Analytic;
	FacialElasticMaterial elastic_material_;
	FacialElasticSolveSettings elastic_solve_settings_;
	std::vector<FacialMuscleSpecification> muscles_;
	std::vector<FacialAttachmentPatch> attachments_;
	std::vector<FacialTissueRegion> tissue_regions_;
};
