#pragma once

#include "ProGen3dGl.h"
#include "lighting/model/GpuLightRecord.h"

#include <cstddef>
#include <vector>

class PreviewLightBuffer
{
public:
	static constexpr GLuint shader_storage_binding_point = 2u;
	static constexpr std::size_t maximum_record_count = 32u;

	bool initialize();
	void shutdown();
	void upload(const std::vector<GpuLightRecord> &records);
	void bind() const;

	GLuint bufferId() const { return buffer_id_; }
	std::size_t uploadedRecordCount() const { return uploaded_record_count_; }
	std::size_t uploadedByteCount() const
	{
		return uploaded_record_count_ * sizeof(GpuLightRecord);
	}

private:
	GLuint buffer_id_ = 0;
	std::size_t uploaded_record_count_ = 0;
};
