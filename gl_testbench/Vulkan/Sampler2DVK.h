#pragma once
#include "../Sampler2D.h"

class Sampler2DVK : public Sampler2D {
public:
	Sampler2DVK();
	virtual ~Sampler2DVK();
	void setMagFilter(FILTER filter) final;
	void setMinFilter(FILTER filter) final;
	void setWrap(WRAPPING s, WRAPPING t) final;

private:
};

