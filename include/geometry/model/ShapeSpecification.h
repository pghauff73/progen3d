#pragma once

#include "geometry/model/GeometryDetailLevel.h"
#include "geometry/model/ShapeSpecificationKey.h"

#include <string>
#include <utility>

enum class ShapeFamily
{
	Box,
	Cylinder,
	Sphere,
	AxialProfile,
	ExtrudeProfile,
	SweepProfile,
	VariableSectionSweep,
	SweepDisk,
	TaperedSweep,
	BranchJunction,
	LeafBlade,
	PetalBlade,
	Plant,
	Vine,
	ScatterRegion,
	Revolve,
	Loft,
	SurfaceLoft,
	CurveNetworkSurface,
	ShellLoft,
	ShellOffset,
	MirrorShape,
	CurvedPanel,
	FormedPanel,
	PanelCut,
	HostedOpening,
	EmbossedBead,
	EdgeFlange,
	CompoundShape,
	LayerSet,
	FoldedProfile,
	PanelBox,
	PanelArray,
	GlazingPanel,
	SurfaceTiling,
	InstanceArray,
	GeneratedMeshReference
};

class ShapeSpecification
{
public:
	ShapeSpecification(
		ShapeFamily family,
		ShapeSpecificationKey key,
		GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals)
		: family_(family),
		  key_(std::move(key)),
		  detail_level_(detail_level)
	{
	}

	virtual ~ShapeSpecification() = default;

	ShapeFamily family() const
	{
		return family_;
	}

	const ShapeSpecificationKey &key() const
	{
		return key_;
	}

	GeometryDetailLevel detailLevel() const
	{
		return detail_level_;
	}

	virtual std::string canonicalText() const = 0;
	virtual bool isDefaultFamilyShape() const = 0;
	virtual bool requestsClosedGeometry() const = 0;

private:
	ShapeFamily family_;
	ShapeSpecificationKey key_;
	GeometryDetailLevel detail_level_ = GeometryDetailLevel::FastenersAndSeals;
};
