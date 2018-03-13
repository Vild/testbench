#pragma once

#include <unordered_map>
#ifdef _WIN32
#include <Windows.h>
#endif

#include "RenderState.h"
#include "Technique.h"
#include "ConstantBuffer.h"
#include "VertexBuffer.h"

#include <glm/glm.hpp>

class Mesh;
class Texture2D;
class Sampler2D;

//CRITICAL_SECTION protectHere;
//#define LOCK EnterCriticalSection(&protectHere)
//#define UNLOCK LeaveCriticalSection(&protectHere)

namespace CLEAR_BUFFER_FLAGS {
	static const int COLOR = 1;
	static const int DEPTH = 2;
	static const int STENCIL = 4;
};

constexpr int ROOM_COUNT = 64;
constexpr int ROOM_UNIT_SIZE = 34;
constexpr int MAP_ROOM_SIZE = 32;
constexpr int MAP_PIXEL_SIZE = MAP_ROOM_SIZE * ROOM_COUNT;

struct EngineRoom {
	std::map<std::string, std::vector<int> /* Model matrix index */> meshes;

	// NOTE: Can see will also include the position for the room this list is in
	std::vector<glm::ivec2> canSee;
};

struct CachedMesh {
	Mesh* mesh;
	std::vector<glm::mat4> modelMatrices;
};

struct EngineMap {
	std::map<std::string, CachedMesh> meshes;
	EngineRoom rooms[ROOM_COUNT][ROOM_COUNT];
};

class Renderer {
public:
	enum class BACKEND { GL45, VULKAN, DX11, DX12 };

	/*
	Return concrete objects of the BACKEND
	*/
	static Renderer* makeRenderer(BACKEND backend);
	virtual Material* makeMaterial(const std::string& name) = 0;
	virtual Mesh* makeMesh() = 0;
	virtual VertexBuffer* makeVertexBuffer(size_t size, VertexBuffer::DATA_USAGE usage) = 0;
	virtual Texture2D* makeTexture2D() = 0;
	virtual Sampler2D* makeSampler2D() = 0;
	virtual RenderState* makeRenderState() = 0;
	virtual std::string getShaderPath() = 0;
	virtual std::string getShaderExtension() = 0;
	virtual ConstantBuffer* makeConstantBuffer(std::string NAME, unsigned int location) = 0;
	virtual Technique* makeTechnique(Material*, RenderState*) = 0;

	Renderer() { /*InitializeCriticalSection(&protectHere);*/ }
	virtual ~Renderer() {}
	virtual int initialize(unsigned int width = 800, unsigned int height = 600) = 0;
	virtual void setWinTitle(const char* title) = 0;
	virtual void present() = 0;
	virtual int shutdown() = 0;

	virtual void setClearColor(float, float, float, float) = 0;
	virtual void clearBuffer(unsigned int) = 0;
	// can be partially overriden by a specific Technique.
	virtual void setRenderState(RenderState* ps) = 0;
	// submit work (to render) to the renderer.
	inline void submitMap(EngineMap* map) { this->map = map; }
	inline void submitPosition(int roomX, int roomY) { this->roomX = roomX; this->roomY = roomY; }
	virtual void submit() = 0;
	virtual void frame() = 0;

	BACKEND IMPL;

protected:
	EngineMap* map;
	int roomX;
	int roomY;
};
