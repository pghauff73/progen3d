#pragma once

class VegetationTriangleImportance
{
public:
	VegetationTriangleImportance(
		double silhouette_contribution,
		double projected_area_contribution,
		double curvature_contribution,
		double topology_contribution,
		double motion_contribution,
		double material_boundary_contribution,
		double visibility_contribution,
		double biological_area_contribution)
		: silhouette_contribution_(silhouette_contribution),
		  projected_area_contribution_(projected_area_contribution),
		  curvature_contribution_(curvature_contribution),
		  topology_contribution_(topology_contribution),
		  motion_contribution_(motion_contribution),
		  material_boundary_contribution_(material_boundary_contribution),
		  visibility_contribution_(visibility_contribution),
		  biological_area_contribution_(biological_area_contribution)
	{
	}

	double silhouetteContribution() const { return silhouette_contribution_; }
	double projectedAreaContribution() const
	{
		return projected_area_contribution_;
	}
	double curvatureContribution() const { return curvature_contribution_; }
	double topologyContribution() const { return topology_contribution_; }
	double motionContribution() const { return motion_contribution_; }
	double materialBoundaryContribution() const
	{
		return material_boundary_contribution_;
	}
	double visibilityContribution() const { return visibility_contribution_; }
	double biologicalAreaContribution() const
	{
		return biological_area_contribution_;
	}

private:
	double silhouette_contribution_ = 0.0;
	double projected_area_contribution_ = 0.0;
	double curvature_contribution_ = 0.0;
	double topology_contribution_ = 0.0;
	double motion_contribution_ = 0.0;
	double material_boundary_contribution_ = 0.0;
	double visibility_contribution_ = 0.0;
	double biological_area_contribution_ = 0.0;
};

class VegetationTriangleImportanceWeights
{
public:
	VegetationTriangleImportanceWeights(
		double silhouette_weight = 0.25,
		double projected_area_weight = 0.20,
		double curvature_weight = 0.12,
		double topology_weight = 0.13,
		double motion_weight = 0.08,
		double material_boundary_weight = 0.05,
		double visibility_weight = 0.07,
		double biological_area_weight = 0.10)
		: silhouette_weight_(silhouette_weight),
		  projected_area_weight_(projected_area_weight),
		  curvature_weight_(curvature_weight),
		  topology_weight_(topology_weight),
		  motion_weight_(motion_weight),
		  material_boundary_weight_(material_boundary_weight),
		  visibility_weight_(visibility_weight),
		  biological_area_weight_(biological_area_weight)
	{
	}

	double silhouetteWeight() const { return silhouette_weight_; }
	double projectedAreaWeight() const { return projected_area_weight_; }
	double curvatureWeight() const { return curvature_weight_; }
	double topologyWeight() const { return topology_weight_; }
	double motionWeight() const { return motion_weight_; }
	double materialBoundaryWeight() const { return material_boundary_weight_; }
	double visibilityWeight() const { return visibility_weight_; }
	double biologicalAreaWeight() const { return biological_area_weight_; }

private:
	double silhouette_weight_ = 0.25;
	double projected_area_weight_ = 0.20;
	double curvature_weight_ = 0.12;
	double topology_weight_ = 0.13;
	double motion_weight_ = 0.08;
	double material_boundary_weight_ = 0.05;
	double visibility_weight_ = 0.07;
	double biological_area_weight_ = 0.10;
};
