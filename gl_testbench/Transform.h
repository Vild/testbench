#pragma once
class Transform
{
public:
	Transform();
	virtual ~Transform();
	float translate[3] = { 0,0,0 };
	float rotate[3] = { 0,0,0 };
};

