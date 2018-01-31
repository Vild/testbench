#pragma once
#include <vector>
#include "../RenderState.h"

class RenderStateVK : public RenderState {
public:
	RenderStateVK();
	virtual ~RenderStateVK();
	void setWireFrame(bool wireframe) final;
	void set() final;

	void setGlobalWireFrame(bool* global);

private:
	bool _wireframe;
	bool* globalWireFrame;
};

