#pragma once

enum class PrimitiveVolumeEvidenceKind
{
	Undefined,
	Analytic,
	MeshDerived
};

class PrimitiveVolumeEvidence
{
public:
	static PrimitiveVolumeEvidence createUndefined()
	{
		return PrimitiveVolumeEvidence();
	}

	static PrimitiveVolumeEvidence createAnalytic(float local_volume)
	{
		return PrimitiveVolumeEvidence(
			PrimitiveVolumeEvidenceKind::Analytic, local_volume);
	}

	static PrimitiveVolumeEvidence createMeshDerived(float local_volume)
	{
		return PrimitiveVolumeEvidence(
			PrimitiveVolumeEvidenceKind::MeshDerived, local_volume);
	}

	PrimitiveVolumeEvidenceKind kind() const
	{
		return kind_;
	}

	bool hasVolume() const
	{
		return kind_ != PrimitiveVolumeEvidenceKind::Undefined;
	}

	float localVolume() const
	{
		return local_volume_;
	}

private:
	PrimitiveVolumeEvidence() = default;
	PrimitiveVolumeEvidence(PrimitiveVolumeEvidenceKind kind, float local_volume)
		: kind_(kind), local_volume_(local_volume)
	{
	}

	PrimitiveVolumeEvidenceKind kind_ = PrimitiveVolumeEvidenceKind::Undefined;
	float local_volume_ = 0.0f;
};
