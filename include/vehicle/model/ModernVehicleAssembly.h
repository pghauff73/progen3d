#pragma once

#include "vehicle/model/VehicleAssemblyGeometry.h"
#include "vehicle/model/VehicleBodySpecification.h"
#include "vehicle/model/VehicleDefinition.h"
#include "vehicle/model/VehicleFittingArchitecture.h"
#include "vehicle/model/VehicleJoint.h"
#include "vehicle/model/VehicleMvp25Architecture.h"
#include "vehicle/model/VehicleMvp26Architecture.h"
#include "vehicle/model/VehicleValidationReport.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class VehiclePlacedAssembly
{
public:
	VehiclePlacedAssembly(
		std::string object_identifier,
		VehicleAssemblyGeometry geometry,
		glm::mat4 local_transform)
		: object_identifier_(std::move(object_identifier)),
		  geometry_(std::move(geometry)),
		  local_transform_(local_transform)
	{
	}

	const std::string &objectIdentifier() const { return object_identifier_; }
	const VehicleAssemblyGeometry &geometry() const { return geometry_; }
	const glm::mat4 &localTransform() const { return local_transform_; }

	VehiclePlacedAssembly withLocalTransform(glm::mat4 local_transform) const
	{
		return VehiclePlacedAssembly(
			object_identifier_, geometry_, local_transform);
	}

private:
	std::string object_identifier_;
	VehicleAssemblyGeometry geometry_;
	glm::mat4 local_transform_{1.0f};
};

class ModernVehicleAssembly
{
public:
	ModernVehicleAssembly(
		VehicleDefinition definition,
		VehicleBodySpecification body_specification,
		std::vector<VehiclePlacedAssembly> assemblies,
		std::vector<VehicleJoint> joints,
		GeneratedPrimitiveMesh combined_mesh,
		VehicleValidationReport validation_report,
		std::uint64_t deterministic_hash,
		std::shared_ptr<const VehicleFittingArchitecture> fitting_architecture = {},
		std::uint64_t fitting_architecture_hash = 0u,
		std::shared_ptr<const VehicleMvp25Architecture> mvp25_architecture = {},
		std::uint64_t mvp25_architecture_hash = 0u,
		std::shared_ptr<const VehicleMvp26Architecture> mvp26_architecture = {},
		std::uint64_t mvp26_architecture_hash = 0u)
		: definition_(std::move(definition)),
		  body_specification_(std::move(body_specification)),
		  assemblies_(std::move(assemblies)),
		  joints_(std::move(joints)),
		  combined_mesh_(std::move(combined_mesh)),
		  validation_report_(std::move(validation_report)),
		  deterministic_hash_(deterministic_hash),
		  fitting_architecture_(std::move(fitting_architecture)),
		  fitting_architecture_hash_(fitting_architecture_hash),
		  mvp25_architecture_(std::move(mvp25_architecture)),
		  mvp25_architecture_hash_(mvp25_architecture_hash),
		  mvp26_architecture_(std::move(mvp26_architecture)),
		  mvp26_architecture_hash_(mvp26_architecture_hash)
	{
	}

	const VehicleDefinition &definition() const { return definition_; }
	const VehicleBodySpecification &bodySpecification() const
	{
		return body_specification_;
	}
	const std::vector<VehiclePlacedAssembly> &assemblies() const
	{
		return assemblies_;
	}
	const std::vector<VehicleJoint> &joints() const { return joints_; }
	const GeneratedPrimitiveMesh &combinedMesh() const { return combined_mesh_; }
	const VehicleValidationReport &validationReport() const
	{
		return validation_report_;
	}
	std::uint64_t deterministicHash() const { return deterministic_hash_; }
	const std::shared_ptr<const VehicleFittingArchitecture> &fittingArchitecture() const
	{
		return fitting_architecture_;
	}
	std::uint64_t fittingArchitectureHash() const
	{
		return fitting_architecture_hash_;
	}
	const std::shared_ptr<const VehicleMvp25Architecture> &mvp25Architecture() const
	{
		return mvp25_architecture_;
	}
	std::uint64_t mvp25ArchitectureHash() const
	{
		return mvp25_architecture_hash_;
	}
	const std::shared_ptr<const VehicleMvp26Architecture> &mvp26Architecture() const
	{
		return mvp26_architecture_;
	}
	std::uint64_t mvp26ArchitectureHash() const
	{
		return mvp26_architecture_hash_;
	}

	ModernVehicleAssembly withFittingArchitecture(
		std::shared_ptr<const VehicleFittingArchitecture> fitting_architecture,
		std::uint64_t fitting_architecture_hash,
		const VehicleValidationReport &fitting_validation_report) const
	{
		VehicleValidationReport combined_validation_report = validation_report_;
		combined_validation_report.append(fitting_validation_report);
		return ModernVehicleAssembly(
			definition_, body_specification_, assemblies_, joints_, combined_mesh_,
			std::move(combined_validation_report), deterministic_hash_,
			std::move(fitting_architecture), fitting_architecture_hash,
			mvp25_architecture_, mvp25_architecture_hash_,
			mvp26_architecture_, mvp26_architecture_hash_);
	}

	ModernVehicleAssembly withMvp25Architecture(
		std::shared_ptr<const VehicleMvp25Architecture> mvp25_architecture,
		std::uint64_t mvp25_architecture_hash,
		const VehicleValidationReport &mvp25_validation_report) const
	{
		VehicleValidationReport combined_validation_report = validation_report_;
		combined_validation_report.append(mvp25_validation_report);
		return ModernVehicleAssembly(
			definition_, body_specification_, assemblies_, joints_, combined_mesh_,
			std::move(combined_validation_report), deterministic_hash_,
			fitting_architecture_, fitting_architecture_hash_,
			std::move(mvp25_architecture), mvp25_architecture_hash,
			mvp26_architecture_, mvp26_architecture_hash_);
	}

	ModernVehicleAssembly withMvp26Architecture(
		std::shared_ptr<const VehicleMvp26Architecture> mvp26_architecture,
		std::uint64_t mvp26_architecture_hash,
		const VehicleValidationReport &mvp26_validation_report) const
	{
		VehicleValidationReport combined_validation_report = validation_report_;
		combined_validation_report.append(mvp26_validation_report);
		return ModernVehicleAssembly(
			definition_, body_specification_, assemblies_, joints_, combined_mesh_,
			std::move(combined_validation_report), deterministic_hash_,
			fitting_architecture_, fitting_architecture_hash_,
			mvp25_architecture_, mvp25_architecture_hash_,
			std::move(mvp26_architecture), mvp26_architecture_hash);
	}

private:
	VehicleDefinition definition_;
	VehicleBodySpecification body_specification_;
	std::vector<VehiclePlacedAssembly> assemblies_;
	std::vector<VehicleJoint> joints_;
	GeneratedPrimitiveMesh combined_mesh_{std::make_shared<Mesh>(), {}};
	VehicleValidationReport validation_report_;
	std::uint64_t deterministic_hash_ = 0;
	std::shared_ptr<const VehicleFittingArchitecture> fitting_architecture_;
	std::uint64_t fitting_architecture_hash_ = 0;
	std::shared_ptr<const VehicleMvp25Architecture> mvp25_architecture_;
	std::uint64_t mvp25_architecture_hash_ = 0;
	std::shared_ptr<const VehicleMvp26Architecture> mvp26_architecture_;
	std::uint64_t mvp26_architecture_hash_ = 0;
};
