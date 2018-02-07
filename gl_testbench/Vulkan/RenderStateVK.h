#pragma once
#include <vector>
#include "../RenderState.h"

class RenderStateVK : public RenderState {
public:
	RenderStateVK();
	virtual ~RenderStateVK();
	void setWireFrame(bool wireFrame) final;
	void set() final;

	void setGlobalWireFrame(bool* global);

	inline bool get() const { return _wireFrame || (_globalWireFrame ? *_globalWireFrame : false); }

private:
	bool _wireFrame;
	bool* _globalWireFrame;
};
