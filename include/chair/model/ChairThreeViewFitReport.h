#pragma once

#include "geometry/model/OrthographicProjectionView.h"

#include <string>
#include <utility>
#include <vector>

class ChairViewFitRecord
{
public:
	ChairViewFitRecord(
		OrthographicProjectionView view,
		double silhouette_iou,
		int matched_iteration)
		: view_(view), silhouette_iou_(silhouette_iou),
		  matched_iteration_(matched_iteration)
	{
	}
	OrthographicProjectionView view() const { return view_; }
	double silhouetteIoU() const { return silhouette_iou_; }
	int matchedIteration() const { return matched_iteration_; }
	bool passes(double threshold) const { return silhouette_iou_ > threshold; }
private:
	OrthographicProjectionView view_ = OrthographicProjectionView::Front;
	double silhouette_iou_ = 0.0;
	int matched_iteration_ = -1;
};

class ChairThreeViewFitReport
{
public:
	ChairThreeViewFitReport(std::string chair_identifier, std::vector<ChairViewFitRecord> views)
		: chair_identifier_(std::move(chair_identifier)), views_(std::move(views))
	{
	}
	const std::string &chairIdentifier() const { return chair_identifier_; }
	const std::vector<ChairViewFitRecord> &views() const { return views_; }
	bool passes(double threshold) const;
private:
	std::string chair_identifier_;
	std::vector<ChairViewFitRecord> views_;
};
