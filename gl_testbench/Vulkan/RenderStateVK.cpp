#include "RenderStateVK.h"
#include "VulkanRenderer.h"

RenderStateVK::RenderStateVK() {}
RenderStateVK::~RenderStateVK() {}

void RenderStateVK::setWireFrame(bool wireFrame) {
	STUB();
	_wireFrame = wireFrame;
}
void RenderStateVK::set() {
	// STUB();
}

void RenderStateVK::setGlobalWireFrame(bool* global) {
	STUB();
	_globalWireFrame = global;
}
