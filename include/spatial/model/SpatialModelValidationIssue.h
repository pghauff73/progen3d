#pragma once

#include "spatial/model/SpatialObjectId.h"

#include <string>
#include <utility>
#include <vector>

enum class SpatialModelValidationCode {
	SafetyCeilingExceeded,
	InvalidIdentity,
	DuplicateObjectId,
	InvalidFrame,
	InvalidUncertainty,
	InvalidBoundary,
	MissingRootObject,
	MultipleRootObjects,
	UndefinedContainer,
	MissingContainer,
	MultipleContainers,
	SelfContainment,
	ContainmentCycle,
	ContainmentDepthExceeded,
	FrameParentMismatch,
	DuplicateInterfaceId,
	InvalidInterface,
	InvalidInterfaceOwner,
	DuplicateConnectionId,
	UndefinedConnectionEndpoint,
	IncompatibleInterfaces,
	InvalidConnection,
	DuplicateConstraintId,
	UndefinedConstraintEndpoint,
	UnsupportedConstraint,
	InvalidConstraint,
	ConstraintCycle,
	ConstraintDependencyDepthExceeded,
	InvalidGeometryBinding,
	DuplicatePrimitiveBinding,
	InvalidResolutionRecord,
	SpatialQueryFailed,
	PositioningFailed,
	ForbiddenCollision,
	ResidualConstraintViolation,
	PlacementTransactionFailed
};

class SpatialModelValidationIssue {
public:
	SpatialModelValidationIssue(SpatialModelValidationCode code,
	                            std::string message,
	                            std::vector<SpatialObjectId> object_path = {})
		: code_(code),
		  message_(std::move(message)),
		  object_path_(std::move(object_path)) {}

	SpatialModelValidationCode code() const { return code_; }
	const std::string &message() const { return message_; }
	const std::vector<SpatialObjectId> &objectPath() const { return object_path_; }

private:
	SpatialModelValidationCode code_ = SpatialModelValidationCode::InvalidIdentity;
	std::string message_;
	std::vector<SpatialObjectId> object_path_;
};
