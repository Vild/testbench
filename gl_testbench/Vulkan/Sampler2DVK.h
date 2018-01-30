#pragma once
#include "../Sampler2D.h"

class Sampler2DVK :
	public Sampler2D
{
public:
	Sampler2DVK();
	~Sampler2DVK();
	void setMagFilter(FILTER filter);
	void setMinFilter(FILTER filter);
	void setWrap(WRAPPING s, WRAPPING t);

private:
};

