#pragma once
#include <vector>
#include "../RenderState.h"

class RenderStateVK : public RenderState
{
public:
	RenderStateVK();
	~RenderStateVK();
	void setWireFrame(bool);
	void set();

	void setGlobalWireFrame(bool* global);
private:
	bool _wireframe;
	bool* globalWireFrame;
};

