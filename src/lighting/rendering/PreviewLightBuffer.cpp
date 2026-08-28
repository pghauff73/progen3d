#include "lighting/rendering/PreviewLightBuffer.h"

#include <algorithm>

bool PreviewLightBuffer::initialize()
{
	if (buffer_id_ != 0) return true;
	glCreateBuffers(1, &buffer_id_);
	if (buffer_id_ == 0) return false;
	glNamedBufferStorage(
		buffer_id_,
		static_cast<GLsizeiptr>(maximum_record_count * sizeof(GpuLightRecord)),
		nullptr,
		GL_DYNAMIC_STORAGE_BIT);
	return glGetError() == GL_NO_ERROR;
}

void PreviewLightBuffer::shutdown()
{
	if (buffer_id_ != 0) {
		glDeleteBuffers(1, &buffer_id_);
		buffer_id_ = 0;
	}
	uploaded_record_count_ = 0;
}

void PreviewLightBuffer::upload(const std::vector<GpuLightRecord> &records)
{
	if (buffer_id_ == 0 && !initialize()) return;
	uploaded_record_count_ = std::min(records.size(), maximum_record_count);
	if (uploaded_record_count_ > 0u) {
		glNamedBufferSubData(
			buffer_id_,
			0,
			static_cast<GLsizeiptr>(uploadedByteCount()),
			records.data());
	}
	bind();
}

void PreviewLightBuffer::bind() const
{
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, shader_storage_binding_point, buffer_id_);
}
