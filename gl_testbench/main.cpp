#include <string>
#include <SDL_keyboard.h>
#include <SDL_events.h>
#include <SDL_timer.h>
#include <type_traits> 
#include <assert.h>
#include <math.h>
#ifdef _WIN32
#include <filesystem>
#else
#include <experimental/filesystem>
#endif

#include "Renderer.h"
#include "Mesh.h"
#include "Texture2D.h"
#include <math.h>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>




#include <fstream>
#include <ctime>

#include "Camera.h"

#if defined(_WIN32) || defined(_WIN64)
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif

#ifdef _WIN32
#define ASSETS_FOLDER "../assets"
#else
#define ASSETS_FOLDER "assets"
#endif

using namespace std;
Renderer* renderer;
Camera camera;

// flat scene at the application level...we don't care about this here.
// do what ever you want in your renderer backend.
// all these objects are loosely coupled, creation and destruction is responsibility
// of the testbench, not of the container objects
vector<Mesh*> scene;
vector<Material*> materials;
vector<Technique*> techniques;
vector<Texture2D*> textures;
vector<Sampler2D*> samplers;

VertexBuffer* pos;
VertexBuffer* nor;
VertexBuffer* uvs;
ConstantBuffer* cameraMatrices;

// forward decls
void updateScene();
void renderScene();

char gTitleBuff[256];
double gLastDelta = 0.0;

ofstream file;

Renderer::BACKEND rendererType = Renderer::BACKEND::VULKAN;
const char* RENDERER_TYPES[4] = { "GL45", "Vulkan", "DX11", "DX12" };

constexpr int ROOM_UNIT_SIZE = 32;
constexpr int ROOM_COUNT = 64;
constexpr int MAP_PIXEL_SIZE = ROOM_UNIT_SIZE * ROOM_COUNT;

void loadModel(std::string filepath) {
	std::vector<VertexBuffer> vertices;
	std::vector<int> indices;

	glm::vec3 vec3;
	glm::vec2 vec2;

	std::ifstream in(filepath, std::ios::binary);
	//if(!in.good())
		//gör skit

	int nrOfMeshes = 0;
	in.read(reinterpret_cast<char*>(&nrOfMeshes), sizeof(int));

	for (int i = 0; i < nrOfMeshes; i++) {
		std::string name = "";
		int nrOfChars = 0;

		in.read(reinterpret_cast<char*>(&nrOfChars), sizeof(int));
		char* tempName = new char[nrOfChars];
		in.read(tempName, nrOfChars);
		name.append(tempName, nrOfChars);

		delete[] tempName;

		int nrOfControlpoints = 0;
		in.read(reinterpret_cast<char*>(&nrOfControlpoints), sizeof(int));

		std::vector<glm::vec3> position;
		std::vector<glm::vec3> normal;
		std::vector<glm::vec2> uv;
		for (int k = 0; k < nrOfControlpoints; k++) {
		
			vec3 = glm::vec3(0);

			in.read(reinterpret_cast<char*>(&vec3), sizeof(vec3));
			position.push_back(vec3);
			in.read(reinterpret_cast<char*>(&vec3), sizeof(vec3));
			normal.push_back(vec3);
			in.read(reinterpret_cast<char*>(&vec3), sizeof(vec3)); 

			in.read(reinterpret_cast<char*>(&vec2), sizeof(vec2));
			uv.push_back(vec2);
		}
		pos->setData(position.data(), sizeof(position.data()), 0);
		nor->setData(normal.data(), sizeof(normal.data()), 0);
		uvs->setData(uv.data(), sizeof(uv.data()), 0);


		int nrOfPrimitives = 0;

		in.read(reinterpret_cast<char*>(&nrOfPrimitives), sizeof(int));

		for (int w = 0; w < nrOfPrimitives; w++) {
			for (int q = 0; q < 3; q++) {
				int indexData = 0;
				in.read(reinterpret_cast<char*>(&indexData), sizeof(int));
				indices.push_back(indexData);
			}
		}

		int fileNameLength = 0;
		glm::vec3 diffuse;
		float specular = 0;

		std::string fileName = "";
		std::string glowName = "";
		in.read(reinterpret_cast<char*>(&fileNameLength), sizeof(int));
		char* tempFileName = new char[fileNameLength];
		in.read(tempFileName, fileNameLength);

		fileName.append(tempFileName, fileNameLength);
		char lastChar = fileName.back();
		if (lastChar == 'd') {
			fileName.erase(fileName.size() - 2, fileName.size());
			fileName.append("ng", fileNameLength - 2);
		}
		//if (name != "AlienBossModel") {
		//	if (fileName != "NULL" && fileNameLength != 0)
		//		_material.diffuse = IEngine::getInstance()->getState()->getTextureLoader()->getTexture("assets/textures/" + fileName);
		//	else
		//		_material.diffuse = IEngine::getInstance()->getState()->getTextureLoader()->getTexture("assets/textures/Floor_specular.png");
		//}
		//else
		//	_material.diffuse = IEngine::getInstance()->getState()->getTextureLoader()->getTexture("assets/textures/AlienBossTexture.png");

		delete[] tempFileName;

		in.read(reinterpret_cast<char*>(&diffuse), sizeof(diffuse));
		in.read(reinterpret_cast<char*>(&specular), sizeof(specular));

		fileName = "";
		in.read(reinterpret_cast<char*>(&fileNameLength), sizeof(int));
		char *tempNormalFileName = new char[fileNameLength];
		in.read(tempNormalFileName, fileNameLength);
		fileName.append(tempNormalFileName, fileNameLength);
		//if (fileName[0] == 'D' && fileName[1] == ':')
		//	_material.normal = IEngine::getInstance()->getState()->getTextureLoader()->getTexture("assets/textures/normals/LowPolyArmNormalMap.png");
		//else if (fileName != "NULL" && fileNameLength != 0)
		//	_material.normal = IEngine::getInstance()->getState()->getTextureLoader()->getTexture("assets/textures/normals/" + fileName);
		//else
		//	_material.normal = IEngine::getInstance()->getState()->getTextureLoader()->getTexture("assets/textures/normals/1x1ErrorNormal.png");

		delete[] tempNormalFileName;

		fileName = "";
		in.read(reinterpret_cast<char*>(&fileNameLength), sizeof(int));
		char *tempGlowFileName;
		tempGlowFileName = new char[fileNameLength];
		in.read(tempGlowFileName, fileNameLength);
		fileName.append(tempGlowFileName, fileNameLength);
		//if (fileName != "NULL" && fileNameLength != 0)
		//	_material.glow = IEngine::getInstance()->getState()->getTextureLoader()->getTexture("assets/textures/glow/" + fileName);
		//else
		//	_material.glow = IEngine::getInstance()->getState()->getTextureLoader()->getTexture("assets/textures/glow/errorGlow.png");
		delete[] tempGlowFileName;

		//for (size_t d = 0; d < vertices.size(); d++)
		//	vertices[d].color = diffuse;

		vec3 = glm::vec3(0);
		//Read the position, rotation and scale values
		in.read(reinterpret_cast<char*>(&vec3), sizeof(vec3));
		//info->position = vec3;
		in.read(reinterpret_cast<char*>(&vec3), sizeof(vec3));
		//info->rotation = vec3;
		in.read(reinterpret_cast<char*>(&vec3), sizeof(vec3));
		//info->scale = vec3;

		bool hasAnimation = false;
		//Read if the mesh has animation, in this case we should
		//do a different type of rendering in the vertex shader
		in.read(reinterpret_cast<char*>(&hasAnimation), sizeof(bool));
		//_indicesCount = indices.size();
		//_makeBuffers();

		if (hasAnimation) {

			//_meshHasAnimation = true;
			int nrOfAnimationFiles;
			in.read(reinterpret_cast<char*>(&nrOfAnimationFiles), sizeof(int));
			bool test = true;
			std::string animationFilePath = "assets/objects/characters/";
			std::string animationFileName;
			int nrOfFileChars = 0;

			//Read the weight info
			in.read(reinterpret_cast<char*>(&nrOfFileChars), sizeof(int));
			char *tempAnimationFileName;
			tempAnimationFileName = new char[nrOfFileChars];
			in.read(tempAnimationFileName, nrOfFileChars);
			animationFileName.append(tempAnimationFileName, nrOfFileChars);
			delete[] tempAnimationFileName;
			animationFileName += ".wATTIC";
			animationFilePath += animationFileName;

			//_loadWeight(animationFilePath.c_str(), vertices);

			//Read all the skeleton info. In other words, all different animations
			for (int animationFile = 0; animationFile < nrOfAnimationFiles; animationFile++) {

				in.read(reinterpret_cast<char*>(&nrOfFileChars), sizeof(int));
				animationFilePath = "assets/objects/characters/";
				animationFileName = "";
				tempAnimationFileName = new char[nrOfFileChars];
				in.read(tempAnimationFileName, nrOfFileChars);
				animationFileName.append(tempAnimationFileName, nrOfFileChars);
				animationFileName += ".sATTIC";
				animationFilePath += animationFileName;

				delete[] tempAnimationFileName;

				//_loadSkeleton(animationFilePath.c_str(), vertices);


			}
			//_uploadData(vertices, indices, true, modelMatrixBuffer, 0, 0);

		}
		else {
			//_meshHasAnimation = false;
			//_uploadData(vertices, indices, false, modelMatrixBuffer, 0, 0);
		}
	}
}

std::string getDateTime() {
	time_t now = time(0);
	struct tm tstruct;
	char buf[80];
	tstruct = *localtime(&now);
	strftime(buf, sizeof(buf), "%Y-%m-%d;%H.%M.%S", &tstruct);

	return buf;
}

struct Model {
	glm::mat4 t;
	char meshFile[64];
};

struct Room {
	std::vector<Model> models;

	// NOTE: Can see will also include the position for the room this list is in
	std::vector<glm::ivec2> canSee;
};

struct Map {
	Room rooms[ROOM_COUNT][ROOM_COUNT];
};

Map findModels() {
	Map map;
	FILE* fp = fopen(ASSETS_FOLDER "/map.cmf", "rb");
	for (int y = 0; y < ROOM_COUNT; y++)
		for (int x = 0; x < ROOM_COUNT; x++) {
			Room& r = map.rooms[y][x];
			{
				uint32_t modelsLength;
				fread(&modelsLength, sizeof(uint32_t), 1, fp);
				r.models.resize(modelsLength);
				fread(r.models.data(), sizeof(Model), modelsLength, fp);
				for (const Model& m : r.models) {
					printf("%s\n", m.meshFile);
				}
			}
			{
				uint32_t canSeeLength;
				fread(&canSeeLength, sizeof(uint32_t), 1, fp);
				r.canSee.resize(canSeeLength);
				fread(r.canSee.data(), sizeof(int[2]), canSeeLength, fp);
			}
		}
	fclose(fp);
	return std::move(map);
}

void updateDelta()
{
	#define WINDOW_SIZE 10
	static Uint64 start = 0;
	static Uint64 last = 0;
	static double avg[WINDOW_SIZE] = { 0.0 };
	static double lastSum = 10.0;
	static int loop = 0;

	last = start;
	start = SDL_GetPerformanceCounter();
	double deltaTime = (double)((start - last) * 1000.0 / SDL_GetPerformanceFrequency());
	// moving average window of WINDOWS_SIZE
	lastSum -= avg[loop];
	lastSum += deltaTime;
	avg[loop] = deltaTime;
	loop = (loop + 1) % WINDOW_SIZE;
	gLastDelta = (lastSum / WINDOW_SIZE);
		
	file << gLastDelta << "\n";
	file.flush();
};

// TOTAL_TRIS pretty much decides how many drawcalls in a brute force approach.
constexpr int TOTAL_TRIS = 100.0f;
// this has to do with how the triangles are spread in the screen, not important.
constexpr int TOTAL_PLACES = 2 * TOTAL_TRIS;
float xt[TOTAL_PLACES], yt[TOTAL_PLACES];

// lissajous points
typedef union { 
	struct { float x, y, z, w; };
	struct { float r, g, b, a; };
} float4;

typedef union { 
	struct { float x, y; };
	struct { float u, v; };
} float2;


void run() {
	SDL_Event windowEvent;
	while (true)
	{
		if (SDL_PollEvent(&windowEvent))
		{
			if (windowEvent.type == SDL_QUIT) break;
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_ESCAPE) break;
			switch (windowEvent.key.keysym.sym) {
			case SDLK_w:
				camera.updatePosition(glm::vec3(0, 0, 1));
				break;
			case SDLK_a:
				camera.updatePosition(glm::vec3(-1, 0, 0));
				break;
			case SDLK_s:
				camera.updatePosition(glm::vec3(0, 0, -1));
				break;
			case SDLK_d:
				camera.updatePosition(glm::vec3(1, 0, 1));
				break;
			default:
				break;
			}
		}
		updateScene();
		renderScene();
	}
}

/*
 update positions of triangles in the screen changing a translation only
*/
void updateScene()
{
	/*
	    For each mesh in scene list, update their position 
	*/
	{
		static long long shift = 0;
		const int size = scene.size();
		for (int i = 0; i < size; i++)
		{
			const float4 trans { 
				xt[(int)(float)(i + shift) % (TOTAL_PLACES)], 
				yt[(int)(float)(i + shift) % (TOTAL_PLACES)], 
				i * (-1.0f / TOTAL_PLACES),
				0.0
			};
			scene[i]->txBuffer->setData(&trans, sizeof(trans), scene[i]->technique->getMaterial(), TRANSLATION);
		}
		// just to make them move...
		shift+=glm::max(TOTAL_TRIS / 1000.0,TOTAL_TRIS / 100.0);
	}

	{ // Camera update stuffs
		auto matrices = camera.getMatrices();
		cameraMatrices->setData(&matrices, sizeof(matrices), nullptr, CAMERA_VIEW_PROJECTION);
	}

	return;
};


void renderScene()
{
	renderer->clearBuffer(CLEAR_BUFFER_FLAGS::COLOR | CLEAR_BUFFER_FLAGS::DEPTH);
	for (auto m : scene)
	{
		renderer->submit(m);
	}
	renderer->frame();
	renderer->present();
	updateDelta();
	sprintf(gTitleBuff, "%s - %3.1lffps (%3.1lfms)", RENDERER_TYPES[static_cast<int>(rendererType)], 1000.0 / gLastDelta, gLastDelta);
	renderer->setWinTitle(gTitleBuff);
}

int initialiseTestbench()
{
	std::string definePos = "#define POSITION " + std::to_string(POSITION) + "\n";
	std::string defineNor = "#define NORMAL " + std::to_string(NORMAL) + "\n";
	std::string defineUV = "#define TEXTCOORD " + std::to_string(TEXTCOORD) + "\n";

	std::string defineTX = "#define TRANSLATION " + std::to_string(TRANSLATION) + "\n";
	std::string defineTXName = "#define TRANSLATION_NAME " + std::string(TRANSLATION_NAME) + "\n";
	
	std::string defineDiffCol = "#define DIFFUSE_TINT " + std::to_string(DIFFUSE_TINT) + "\n";
	std::string defineDiffColName = "#define DIFFUSE_TINT_NAME " + std::string(DIFFUSE_TINT_NAME) + "\n";

	std::string defineViewProj = "#define CAMERA_VIEW_PROJECTION " + std::to_string(CAMERA_VIEW_PROJECTION) + "\n";
	std::string defineViewProjName = "#define CAMERA_VIEW_PROJECTION_NAME " + std::string(CAMERA_VIEW_PROJECTION_NAME) + "\n";


	std::string defineDiffuse = "#define DIFFUSE_SLOT " + std::to_string(DIFFUSE_SLOT) + "\n";

	std::vector<std::vector<std::string>> materialDefs = {
		// vertex shader, fragment shader, defines
		// shader filename extension must be asked to the renderer
		// these strings should be constructed from the IA.h file!!!

		{ "VertexShader", "FragmentShader", definePos + defineNor + defineUV + defineTX + 
		   defineTXName + defineDiffCol + defineDiffColName + defineViewProj + defineViewProjName},

		{ "VertexShader", "FragmentShader", definePos + defineNor + defineUV + defineTX + 
		   defineTXName + defineDiffCol + defineDiffColName + defineViewProj + defineViewProjName},

		{ "VertexShader", "FragmentShader", definePos + defineNor + defineUV + defineTX + 
		   defineTXName + defineDiffCol + defineDiffColName + defineDiffuse	+ defineViewProj + defineViewProjName},

		{ "VertexShader", "FragmentShader", definePos + defineNor + defineUV + defineTX + 
		   defineTXName + defineDiffCol + defineDiffColName + defineViewProj + defineViewProjName },
	};

	float degToRad = M_PI / 180.0;
	float scale = (float)TOTAL_PLACES / 359.9;
	for (int a = 0; a < TOTAL_PLACES; a++)
	{
		xt[a] = 0.8f * cosf(degToRad * ((float)a/scale) * 3.0);
		yt[a] = 0.8f * sinf(degToRad * ((float)a/scale) * 2.0);
	};

	// triangle geometry:
	float4 triPos[3] = { { 0.0f,  0.05, 0.0f, 1.0f },{ 0.05, -0.05, 0.0f, 1.0f },{ -0.05, -0.05, 0.0f, 1.0f } };
	float4 triNor[3] = { { 0.0f,  0.0f, 1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f, 0.0f } };
	float2 triUV[3] =  { { 0.5f,  -0.99f },{ 1.49f, 1.1f },{ -0.51, 1.1f } };

	// load Materials.
	std::string shaderPath = renderer->getShaderPath();
	std::string shaderExtension = renderer->getShaderExtension();
	float diffuse[4][4] = {
		0.0,0.0,1.0,1.0,
		0.0,1.0,0.0,1.0,
		1.0,1.0,1.0,1.0,
		1.0,0.0,0.0,1.0
	};

	for (size_t i = 0; i < materialDefs.size(); i++)
	{
		// set material name from text file?
		Material* m = renderer->makeMaterial("material_" + std::to_string(i));
		m->setShader(shaderPath + materialDefs[i][0] + shaderExtension, Material::ShaderType::VS);
		m->setShader(shaderPath + materialDefs[i][1] + shaderExtension, Material::ShaderType::PS);

		m->addDefine(materialDefs[i][2], Material::ShaderType::VS);
		m->addDefine(materialDefs[i][2], Material::ShaderType::PS);

		std::string err;
		m->compileMaterial(err);

		// add a constant buffer to the material, to tint every triangle using this material
		m->addConstantBuffer(DIFFUSE_TINT_NAME, DIFFUSE_TINT);
		// no need to update anymore
		// when material is bound, this buffer should be also bound for access.

		m->updateConstantBuffer(diffuse[i], 4 * sizeof(float), DIFFUSE_TINT);

		materials.push_back(m);
	}

	// one technique with wireframe
	RenderState* renderState1 = renderer->makeRenderState();
	renderState1->setWireFrame(true);

	// basic technique
	techniques.push_back(renderer->makeTechnique(materials[0], renderState1));
	techniques.push_back(renderer->makeTechnique(materials[1], renderer->makeRenderState()));
	techniques.push_back(renderer->makeTechnique(materials[2], renderer->makeRenderState()));
	techniques.push_back(renderer->makeTechnique(materials[3], renderer->makeRenderState()));

	// create texture
	Texture2D* fatboy = renderer->makeTexture2D();
	fatboy->loadFromFile(ASSETS_FOLDER "/textures/fatboy.png");
	Sampler2D* sampler = renderer->makeSampler2D();
	sampler->setWrap(WRAPPING::REPEAT, WRAPPING::REPEAT);
	fatboy->sampler = sampler;

	textures.push_back(fatboy);
	samplers.push_back(sampler);

	// pre-allocate one single vertex buffer for ALL triangles
	pos = renderer->makeVertexBuffer(TOTAL_TRIS * sizeof(triPos), VertexBuffer::DATA_USAGE::STATIC);
	nor = renderer->makeVertexBuffer(TOTAL_TRIS * sizeof(triNor), VertexBuffer::DATA_USAGE::STATIC);
	uvs = renderer->makeVertexBuffer(TOTAL_TRIS * sizeof(triUV), VertexBuffer::DATA_USAGE::STATIC);
	cameraMatrices = renderer->makeConstantBuffer(std::string(CAMERA_VIEW_PROJECTION_NAME), CAMERA_VIEW_PROJECTION);
	// Create a mesh array with 3 basic vertex buffers.
	for (int i = 0; i < TOTAL_TRIS; i++) {

		Mesh* m = renderer->makeMesh();

		constexpr auto numberOfPosElements = std::extent<decltype(triPos)>::value;
		size_t offset = i * sizeof(triPos);
		pos->setData(triPos, sizeof(triPos), offset);
		m->addIAVertexBufferBinding(pos, offset, numberOfPosElements, sizeof(float4), POSITION);

		constexpr auto numberOfNorElements = std::extent<decltype(triNor)>::value;
		offset = i * sizeof(triNor);
		nor->setData(triNor, sizeof(triNor), offset);
		m->addIAVertexBufferBinding(nor, offset, numberOfNorElements, sizeof(float4), NORMAL);

		constexpr auto numberOfUVElements = std::extent<decltype(triUV)>::value;
		offset = i * sizeof(triUV);
		uvs->setData(triUV, sizeof(triUV), offset);
		m->addIAVertexBufferBinding(uvs, offset, numberOfUVElements , sizeof(float2), TEXTCOORD);

		// we can create a constant buffer outside the material, for example as part of the Mesh.
		m->txBuffer = renderer->makeConstantBuffer(std::string(TRANSLATION_NAME), TRANSLATION);
		m->cameraVPBuffer = cameraMatrices;
		m->technique = techniques[ i % 4];
		if (i % 4 == 2)
			m->addTexture(textures[0], DIFFUSE_SLOT);

		scene.push_back(m);
	}

	std::string filename;
	if (rendererType == ::Renderer::BACKEND::GL45)
		filename = "GL";
	else if (rendererType == ::Renderer::BACKEND::VULKAN)
		filename = "VK";

	filename += getDateTime() + ".txt";
	file.open(filename.c_str());

	return 0;
}

void shutdown() {
	// shutdown.
	// delete dynamic objects
	for (auto m : materials)
	{
		delete(m);
	}
	for (auto t : techniques)
	{
		delete(t);
	}
	for (auto m : scene)
	{
		delete(m);
	};
	assert(pos->refCount() == 0);
	delete pos;
	assert(nor->refCount() == 0);
	delete nor;
	assert(uvs->refCount() == 0);
	delete uvs;
	
	for (auto s : samplers)
	{
		delete s;
	}

	for (auto t : textures)
	{
		delete t;
	}
	renderer->shutdown();
	file.close();
};

int main(int argc, char *argv[])
{
	for (int i = 1; i < argc; i++)
		if (!strcasecmp(argv[i], "gl45"))
			rendererType = Renderer::BACKEND::GL45;
		else if (!strcasecmp(argv[i], "vulkan"))
			rendererType = Renderer::BACKEND::VULKAN;

	findModels();

	renderer = Renderer::makeRenderer(rendererType);
	if (renderer->initialize(800, 600))
		return -1;
	renderer->setWinTitle(RENDERER_TYPES[static_cast<int>(rendererType)]);
	renderer->setClearColor(0.0, 0.1, 0.1, 1.0);
	initialiseTestbench();
	run();
	shutdown();
	return 0;
};
