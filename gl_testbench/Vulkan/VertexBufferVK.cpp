#include "VertexBufferVK.h"

uint32_t VertexBufferVK::usageMapping[3];

VertexBufferVK::VertexBufferVK(size_t size, VertexBuffer::DATA_USAGE usage) {}
VertexBufferVK::~VertexBufferVK() {}

void VertexBufferVK::setData(const void* data, size_t size, size_t offset) {}
void VertexBufferVK::bind(size_t offset, size_t size, unsigned int location) {}
void VertexBufferVK::unbind() {}
size_t VertexBufferVK::getSize() { return 0;  }
