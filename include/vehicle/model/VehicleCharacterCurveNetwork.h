#pragma once

#include "geometry/model/Curve3D.h"
#include "vehicle/model/VehicleFittingEvidence.h"

#include <string>
#include <utility>
#include <vector>

enum class VehicleCharacterCurveRole
{
	RoofLine,
	HoodCentre,
	HoodOuterEdge,
	BeltLine,
	UpperShoulder,
	LowerDoorCrease,
	RockerLine,
	FrontFenderCrest,
	RearHaunch,
	WindowUpperLine,
	WindowLowerLine
};

enum class CharacterCurveRelationshipType
{
	Meets,
	Crosses,
	Continues,
	Tangent,
	TerminatesAt
};

class VehicleCharacterCurve
{
public:
	VehicleCharacterCurve(
		std::string identifier,
		VehicleCharacterCurveRole role,
		Curve3D curve,
		std::vector<std::string> landmark_identifiers,
		VehicleEvidenceClassification evidence_classification)
		: identifier_(std::move(identifier)),
		  role_(role),
		  curve_(std::move(curve)),
		  landmark_identifiers_(std::move(landmark_identifiers)),
		  evidence_classification_(evidence_classification)
	{
	}

	const std::string &identifier() const { return identifier_; }
	VehicleCharacterCurveRole role() const { return role_; }
	const Curve3D &curve() const { return curve_; }
	const std::vector<std::string> &landmarkIdentifiers() const
	{
		return landmark_identifiers_;
	}
	VehicleEvidenceClassification evidenceClassification() const
	{
		return evidence_classification_;
	}

private:
	std::string identifier_;
	VehicleCharacterCurveRole role_ = VehicleCharacterCurveRole::RoofLine;
	Curve3D curve_{Curve3DType::Line, {glm::vec3(0.0f), glm::vec3(1.0f)}};
	std::vector<std::string> landmark_identifiers_;
	VehicleEvidenceClassification evidence_classification_ =
		VehicleEvidenceClassification::Observed;
};

class CharacterCurveRelationship
{
public:
	CharacterCurveRelationship(
		std::string source_curve_identifier,
		std::string target_curve_identifier,
		CharacterCurveRelationshipType relationship_type,
		float tolerance)
		: source_curve_identifier_(std::move(source_curve_identifier)),
		  target_curve_identifier_(std::move(target_curve_identifier)),
		  relationship_type_(relationship_type),
		  tolerance_(tolerance)
	{
	}

	const std::string &sourceCurveIdentifier() const
	{
		return source_curve_identifier_;
	}
	const std::string &targetCurveIdentifier() const
	{
		return target_curve_identifier_;
	}
	CharacterCurveRelationshipType relationshipType() const
	{
		return relationship_type_;
	}
	float tolerance() const { return tolerance_; }

private:
	std::string source_curve_identifier_;
	std::string target_curve_identifier_;
	CharacterCurveRelationshipType relationship_type_ =
		CharacterCurveRelationshipType::Meets;
	float tolerance_ = 0.0f;
};

class CharacterCurveNetwork
{
public:
	CharacterCurveNetwork(
		std::vector<VehicleCharacterCurve> curves,
		std::vector<CharacterCurveRelationship> relationships)
		: curves_(std::move(curves)), relationships_(std::move(relationships))
	{
	}

	const std::vector<VehicleCharacterCurve> &curves() const { return curves_; }
	const std::vector<CharacterCurveRelationship> &relationships() const
	{
		return relationships_;
	}

	const VehicleCharacterCurve *findCurve(const std::string &identifier) const
	{
		for (const VehicleCharacterCurve &curve : curves_) {
			if (curve.identifier() == identifier) return &curve;
		}
		return nullptr;
	}

private:
	std::vector<VehicleCharacterCurve> curves_;
	std::vector<CharacterCurveRelationship> relationships_;
};
