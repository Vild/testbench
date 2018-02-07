#include "RenderStateVK.h"

RenderStateVK::RenderStateVK() {}
RenderStateVK::~RenderStateVK() {}

void RenderStateVK::setWireFrame(bool wireFrame) {
	_wireFrame = wireFrame;
}
void RenderStateVK::set() {}

void RenderStateVK::setGlobalWireFrame(bool* global) {
	_globalWireFrame = global;
}
