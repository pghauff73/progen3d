#include "editor/application/ApplicationLaunchOptions.h"

#include <string_view>

ApplicationLaunchOptions ApplicationLaunchOptions::parse(int argument_count, char **argument_values)
{
	ApplicationLaunchOptions options;
	if (argument_count > 0 && argument_values != nullptr && argument_values[0] != nullptr) {
		options.executable_path_ = argument_values[0];
	}
	for (int argument_index = 1; argument_index < argument_count; ++argument_index) {
		if (argument_values == nullptr || argument_values[argument_index] == nullptr) {
			continue;
		}
		if (std::string_view(argument_values[argument_index]) == "--smoke-test") {
			options.smoke_test_requested_ = true;
		} else if (std::string_view(argument_values[argument_index]) ==
		           "--temporal-smoke-test") {
			options.smoke_test_requested_ = true;
			options.temporal_smoke_test_requested_ = true;
		} else if (std::string_view(argument_values[argument_index]) ==
		           "--visual-test") {
			options.smoke_test_requested_ = true;
			options.visual_test_requested_ = true;
		} else if (std::string_view(argument_values[argument_index]) ==
		           "--spatial-overlays") {
			options.spatial_overlays_requested_ = true;
		} else if (std::string_view(argument_values[argument_index]) ==
		           "--building-knowledge-overlays") {
			options.building_knowledge_overlays_requested_ = true;
		} else if (std::string_view(argument_values[argument_index]) ==
		           "--preview-isometric") {
			options.preview_view_selection_ = PreviewViewSelection::Isometric;
		} else if (std::string_view(argument_values[argument_index]) ==
		               "--preview-view" &&
		           argument_index + 1 < argument_count &&
		           argument_values[argument_index + 1] != nullptr) {
			const std::string_view view_name(argument_values[++argument_index]);
			if (view_name == "front") {
				options.preview_view_selection_ = PreviewViewSelection::Front;
			} else if (view_name == "back") {
				options.preview_view_selection_ = PreviewViewSelection::Back;
			} else if (view_name == "left") {
				options.preview_view_selection_ = PreviewViewSelection::Left;
			} else if (view_name == "right") {
				options.preview_view_selection_ = PreviewViewSelection::Right;
			} else if (view_name == "top") {
				options.preview_view_selection_ = PreviewViewSelection::Top;
			} else if (view_name == "bottom") {
				options.preview_view_selection_ = PreviewViewSelection::Bottom;
			} else if (view_name == "isometric") {
				options.preview_view_selection_ = PreviewViewSelection::Isometric;
			}
		} else if (std::string_view(argument_values[argument_index]) ==
		           "--preview-fit-extents") {
			options.preview_fit_extents_requested_ = true;
		} else if (std::string_view(argument_values[argument_index]) ==
		               "--preview-hide-object" &&
		           argument_index + 1 < argument_count &&
		           argument_values[argument_index + 1] != nullptr) {
			options.preview_hidden_object_ids_.emplace_back(
				argument_values[++argument_index]);
		} else if (std::string_view(argument_values[argument_index]) ==
		               "--select-spatial-object" &&
		           argument_index + 1 < argument_count &&
		           argument_values[argument_index + 1] != nullptr) {
			options.selected_spatial_object_id_ = argument_values[++argument_index];
		} else if (std::string_view(argument_values[argument_index]) ==
		               "--capture-preview" &&
		           argument_index + 1 < argument_count &&
		           argument_values[argument_index + 1] != nullptr) {
			options.smoke_test_requested_ = true;
			options.visual_test_requested_ = true;
			options.preview_capture_path_ = argument_values[++argument_index];
		} else if ((std::string_view(argument_values[argument_index]) == "--open" ||
		            std::string_view(argument_values[argument_index]) == "--document") &&
		           argument_index + 1 < argument_count &&
		           argument_values[argument_index + 1] != nullptr) {
			options.startup_document_path_ = argument_values[++argument_index];
		}
	}
	return options;
}

bool ApplicationLaunchOptions::smokeTestRequested() const
{
	return smoke_test_requested_;
}

bool ApplicationLaunchOptions::temporalSmokeTestRequested() const
{
	return temporal_smoke_test_requested_;
}

bool ApplicationLaunchOptions::visualTestRequested() const
{
	return visual_test_requested_;
}

bool ApplicationLaunchOptions::spatialOverlaysRequested() const
{
	return spatial_overlays_requested_;
}

bool ApplicationLaunchOptions::buildingKnowledgeOverlaysRequested() const
{
	return building_knowledge_overlays_requested_;
}

bool ApplicationLaunchOptions::previewIsometricRequested() const
{
	return preview_view_selection_ == PreviewViewSelection::Isometric;
}

bool ApplicationLaunchOptions::previewFitExtentsRequested() const
{
	return preview_fit_extents_requested_;
}

PreviewViewSelection ApplicationLaunchOptions::previewViewSelection() const
{
	return preview_view_selection_;
}

const std::vector<std::string> &ApplicationLaunchOptions::previewHiddenObjectIds() const
{
	return preview_hidden_object_ids_;
}

const std::string &ApplicationLaunchOptions::selectedSpatialObjectId() const
{
	return selected_spatial_object_id_;
}

const std::string &ApplicationLaunchOptions::executablePath() const
{
	return executable_path_;
}

const std::string &ApplicationLaunchOptions::startupDocumentPath() const
{
	return startup_document_path_;
}

const std::string &ApplicationLaunchOptions::previewCapturePath() const
{
	return preview_capture_path_;
}
