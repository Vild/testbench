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
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

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
#define ASSETS_FOLDER_PREFIX "../"
#define ASSETS_FOLDER ASSETS_FOLDER_PREFIX "assets"
#else
#define ASSETS_FOLDER_PREFIX ""
#define ASSETS_FOLDER "assets"
#endif

Renderer* renderer;
Camera camera;

// flat scene at the application level...we don't care about this here.
// do what ever you want in your renderer backend.
// all these objects are loosely coupled, creation and destruction is responsibility
// of the testbench, not of the container objects

//vector<Mesh*> scene;
std::vector<Material*> materials;
std::vector<Technique*> techniques;
std::vector<Texture2D*> textures;
std::vector<Sampler2D*> samplers;

ConstantBuffer* cameraMatrices;

// forward decls
void updateScene();
void renderScene();

char gTitleBuff[256];
double gLastDelta = 0.0;

std::ofstream file;

Renderer::BACKEND rendererType = Renderer::BACKEND::GL45;
const char* RENDERER_TYPES[4] = { "GL45", "Vulkan", "DX11", "DX12" };

Mesh* loadModel(std::string filepath, Technique* technique, const std::vector<glm::mat4>& modelMatrices) {
	std::vector<glm::vec4> position;
	std::vector<int> indices;

	glm::vec3 vec3;
	glm::vec2 vec2;

	std::ifstream in(filepath, std::ios::binary);
	if(!in.good()) {
		fprintf(stderr, "Failed to load file: %s\n", filepath.c_str());
		//exit(-1);
	}


	int nrOfMeshes = 0;
	in.read(reinterpret_cast<char*>(&nrOfMeshes), sizeof(int));
	while (nrOfMeshes--) {
		//std::string name = "";
		int nrOfChars = 0;

		in.read(reinterpret_cast<char*>(&nrOfChars), sizeof(int));
		char* tempName = new char[nrOfChars];
		in.read(tempName, nrOfChars);
		//name.append(tempName, nrOfChars);

		delete[] tempName;

		int nrOfControlpoints = 0;
		in.read(reinterpret_cast<char*>(&nrOfControlpoints), sizeof(int));

		for (int k = 0; k < nrOfControlpoints; k++) {
			in.read(reinterpret_cast<char*>(&vec3), sizeof(vec3));
			position.push_back(glm::vec4{vec3, 0});
			in.read(reinterpret_cast<char*>(&vec3), sizeof(vec3));
			in.read(reinterpret_cast<char*>(&vec3), sizeof(vec3));
			in.read(reinterpret_cast<char*>(&vec2), sizeof(vec2));
		}

		int nrOfPrimitives = 0;

		in.read(reinterpret_cast<char*>(&nrOfPrimitives), sizeof(int));

		for (int w = 0; w < nrOfPrimitives; w++) {
			for (int q = 0; q < 3; q++) {
				int indexData = 0;
				in.read(reinterpret_cast<char*>(&indexData), sizeof(int));
				indices.push_back(indexData);
			}
		}
	}
	Mesh* m = renderer->makeMesh();
	VertexBuffer* pos = renderer->makeVertexBuffer(position.size() * sizeof(glm::vec4), VertexBuffer::DATA_USAGE::STATIC);
	pos->setData(position.data(), position.size() * sizeof(glm::vec4), 0);
	m->addIAVertexBufferBinding(pos, 0, position.size(), sizeof(glm::vec4), POSITION);

	VertexBuffer* idx = renderer->makeVertexBuffer(indices.size() * sizeof(int), VertexBuffer::DATA_USAGE::STATIC);
	idx->setData(indices.data(), indices.size() * sizeof(int), 0);
	m->addIAVertexBufferBinding(idx, 0, indices.size(), sizeof(int), INDEX);

	VertexBuffer* model = renderer->makeVertexBuffer(modelMatrices.size() * sizeof(glm::mat4), VertexBuffer::DATA_USAGE::STATIC);
	model->setData(modelMatrices.data(), modelMatrices.size() * sizeof(glm::mat4), 0);
	m->addIAVertexBufferBinding(model, 0, modelMatrices.size(), sizeof(glm::mat4), MODEL);

	m->txBuffer = nullptr;
	m->cameraVPBuffer = cameraMatrices;
	m->technique = technique;

	return m;
}

std::string getDateTime() {
	time_t now = time(0);
	struct tm tstruct;
	char buf[80];
	tstruct = *localtime(&now);
	strftime(buf, sizeof(buf), "%Y-%m-%d;%H.%M.%S", &tstruct);

	return buf;
}

struct FileModel {
	glm::mat4 t;
	char meshFile[64];
};

// :( this one should be moved up
EngineMap map;

void loadMap(Technique* technique) {
	FILE* fp = fopen(ASSETS_FOLDER "/map.cmf", "rb");
	int modelID = 0;
	for (int y = 0; y < ROOM_COUNT; y++)
		for (int x = 0; x < ROOM_COUNT; x++) {
			EngineRoom& r = map.rooms[y][x];
			{
				static std::vector<FileModel> models;
				uint32_t modelsLength;
				fread(&modelsLength, sizeof(uint32_t), 1, fp);
				models.resize(modelsLength);
				fread(models.data(), sizeof(FileModel), modelsLength, fp);
				for (const FileModel& m : models) {
					CachedMesh& cm = map.meshes[std::string{m.meshFile}];

					cm.modelMatrices.push_back(m.t);
					r.meshes[std::string{m.meshFile}].push_back(cm.modelMatrices.size() - 1);
				}
				models.clear();
			}
			{
				uint32_t canSeeLength;
				fread(&canSeeLength, sizeof(uint32_t), 1, fp);
				r.canSee.resize(canSeeLength);
				fread(r.canSee.data(), sizeof(int[2]), canSeeLength, fp);
			}
		}
	fclose(fp);

	for (auto& cm : map.meshes) {
		printf("Name: %s, Used: %zu\n", cm.first.c_str(), cm.second.modelMatrices.size());
		cm.second.mesh = loadModel(ASSETS_FOLDER_PREFIX + cm.first, technique, cm.second.modelMatrices);
		cm.second.mesh->finalize();
	}
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
		while (SDL_PollEvent(&windowEvent))
		{
			if (windowEvent.type == SDL_QUIT) return;
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_ESCAPE) return;
			/*if (windowEvent.type == SDL_KEYDOWN)
				switch (windowEvent.key.keysym.sym) {
				case SDLK_w:
					camera.updatePosition(glm::vec3(0, 0, 1));
					break;
				case SDLK_d:
					camera.updatePosition(glm::vec3(-1, 0, 0));
					break;
				case SDLK_s:
					camera.updatePosition(glm::vec3(0, 0, -1));
					break;
				case SDLK_a:
					camera.updatePosition(glm::vec3(1, 0, 0));
					break;
				case SDLK_SPACE:
					camera.updatePosition(glm::vec3(0, 1, 0));
1					break;
				case SDLK_LCTRL:
					camera.updatePosition(glm::vec3(0, -1, 0));
					break;
				default:
					break;
				}*/
		}

		glm::vec3 vel;
		glm::vec2 yawPitch;
		float speed = 8;
		float lookSpeed = M_PI;

		const Uint8* kb = SDL_GetKeyboardState(nullptr);

		yawPitch.x += !!kb[SDL_SCANCODE_DOWN] * 1.0f;
		yawPitch.x -= !!kb[SDL_SCANCODE_UP] * 1.0f;
		yawPitch.y += !!kb[SDL_SCANCODE_RIGHT] * 1.0f;
		yawPitch.y -= !!kb[SDL_SCANCODE_LEFT] * 1.0f;

		camera.updatePosition(glm::vec3(0), yawPitch * glm::vec2(lookSpeed * (gLastDelta / 1000.0f)));

		vel.x -= !!kb[SDL_SCANCODE_A] * 1.0f;
		vel.x += !!kb[SDL_SCANCODE_D] * 1.0f;
		vel.y += !!kb[SDL_SCANCODE_SPACE] * 1.0f;
		vel.y -= !!kb[SDL_SCANCODE_LCTRL] * 1.0f;
		vel.z -= !!kb[SDL_SCANCODE_W] * 1.0f;
		vel.z += !!kb[SDL_SCANCODE_S] * 1.0f;

		if (kb[SDL_SCANCODE_LSHIFT])
			speed *= 2;

		vel = glm::vec3(glm::vec4(vel, 1) * camera.getMatrices().view);
		camera.updatePosition(vel * glm::vec3(speed * (gLastDelta / 1000.0f)), glm::vec2(0));

		updateScene();
		renderScene();
	}
}

/*
	update positions of triangles in the screen changing a translation only
*/
void updateScene()
{
	{ // Camera update stuffs
		auto matrices = camera.getMatrices();
		cameraMatrices->setData(&matrices, sizeof(matrices), nullptr, CAMERA_VIEW_PROJECTION);
	}

}


void renderScene()
{
	renderer->clearBuffer(CLEAR_BUFFER_FLAGS::COLOR | CLEAR_BUFFER_FLAGS::DEPTH);
	int roomX = (int)(camera._position.x) / MAP_ROOM_SIZE;
	int roomY = (int)(camera._position.z) / MAP_ROOM_SIZE;
	roomX = std::min(std::max(roomX, 0), ROOM_COUNT);
	roomY = std::min(std::max(roomY, 0), ROOM_COUNT);
	renderer->submitPosition(roomX, roomY);
	printf("Camera: [%.2f, %.2f] Room: [%d, %d]\n", camera._position.x, camera._position.z, roomX, roomY);
	renderer->submit();
	/*for (int y = 0; y < ROOM_COUNT; y++)
			for (int x = 0; x < ROOM_COUNT; x++)
				for (auto m : map.rooms[y][x].meshes)
					{
							for (int id : m.second)
								renderer->submit(map.meshes[m.first].mesh, id);
					}*/
	renderer->frame();
	renderer->present();
	updateDelta();
	sprintf(gTitleBuff, "%s - %3.1lffps (%3.1lfms)", RENDERER_TYPES[static_cast<int>(rendererType)], 1000.0 / gLastDelta, gLastDelta);
	renderer->setWinTitle(gTitleBuff);
}

int initialiseTestbench()
{
	std::string definePos = "#define POSITION " + std::to_string(POSITION) + "\n";
	std::string defineNor = "#define INDEX " + std::to_string(INDEX) + "\n";
	std::string defineMod = "#define MODEL " + std::to_string(MODEL) + "\n";

	std::string defineViewProj = "#define CAMERA_VIEW_PROJECTION " + std::to_string(CAMERA_VIEW_PROJECTION) + "\n";
	std::string defineViewProjName = "#define CAMERA_VIEW_PROJECTION_NAME " + std::string(CAMERA_VIEW_PROJECTION_NAME) + "\n";

	std::vector<std::vector<std::string>> materialDefs = {
		{ "VertexShader", "FragmentShader", definePos + defineNor + defineMod + defineViewProj + defineViewProjName},
	};

	// load Materials.
	std::string shaderPath = renderer->getShaderPath();
	std::string shaderExtension = renderer->getShaderExtension();


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

		materials.push_back(m);
	}

	// one technique with wireframe
	RenderState* renderState1 = renderer->makeRenderState();

	// basic technique
	techniques.push_back(renderer->makeTechnique(materials[0], renderState1));

	cameraMatrices = renderer->makeConstantBuffer(std::string(CAMERA_VIEW_PROJECTION_NAME), CAMERA_VIEW_PROJECTION);

	loadMap(techniques[0]);

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
	//NOTE: We probably leak memory, but that doesn't matter as we are shutting
	//      down the testbench.

	/*
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

	//delete cameraVPBuffer;
	//delete transform;
	/ *assert(pos->refCount() == 0);
	delete pos;
	assert(nor->refCount() == 0);
	delete nor;
	assert(indicess->refCount() == 0);
	delete uvs;* /

	for (auto s : samplers)
	{
		delete s;
	}

	for (auto t : textures)
	{
		delete t;
	}*/
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

	renderer = Renderer::makeRenderer(rendererType);
	if (renderer->initialize(800, 600))
		return -1;
	camera._position = glm::vec3((1 + ROOM_COUNT)*MAP_ROOM_SIZE*0.5f, 1.5, (1 + ROOM_COUNT)*MAP_ROOM_SIZE*0.5f);
	renderer->submitMap(&map);
	renderer->setWinTitle(RENDERER_TYPES[static_cast<int>(rendererType)]);
	renderer->setClearColor(0.0, 0.1, 0.1, 1.0);
	initialiseTestbench();
	run();
	shutdown();
	return 0;
};
