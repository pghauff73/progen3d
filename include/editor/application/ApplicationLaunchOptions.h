#pragma once

#include <string>
#include <vector>

enum class PreviewViewSelection {
	Unspecified,
	Front,
	Back,
	Left,
	Right,
	Top,
	Bottom,
	Isometric
};

class ApplicationLaunchOptions
{
public:
	static ApplicationLaunchOptions parse(int argument_count, char **argument_values);

	bool smokeTestRequested() const;
	bool temporalSmokeTestRequested() const;
	bool visualTestRequested() const;
	bool spatialOverlaysRequested() const;
	bool buildingKnowledgeOverlaysRequested() const;
	bool previewIsometricRequested() const;
	bool previewFitExtentsRequested() const;
	PreviewViewSelection previewViewSelection() const;
	const std::vector<std::string> &previewHiddenObjectIds() const;
	const std::string &selectedSpatialObjectId() const;
	const std::string &executablePath() const;
	const std::string &startupDocumentPath() const;
	const std::string &previewCapturePath() const;

private:
	bool smoke_test_requested_ = false;
	bool temporal_smoke_test_requested_ = false;
	bool visual_test_requested_ = false;
	bool spatial_overlays_requested_ = false;
	bool building_knowledge_overlays_requested_ = false;
	bool preview_fit_extents_requested_ = false;
	PreviewViewSelection preview_view_selection_ = PreviewViewSelection::Unspecified;
	std::vector<std::string> preview_hidden_object_ids_;
	std::string selected_spatial_object_id_;
	std::string executable_path_;
	std::string startup_document_path_;
	std::string preview_capture_path_;
};
