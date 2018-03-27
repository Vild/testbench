module sdl;

public import derelict.sdl2.sdl;
public import derelict.sdl2.image;

import std.stdio;
import types;

shared static this() {
	DerelictSDL2.load();
	DerelictSDL2Image.load();
}

void sdlAssert(T, Args...)(T cond, Args args) {
	import std.string : fromStringz;

	if (!!cond)
		return;
	stderr.writeln(args);
	stderr.writeln("SDL_ERROR: ", SDL_GetError().fromStringz);
	assert(0);
}

class SDL {
public:
	this() {
		sdlAssert(!SDL_Init(SDL_INIT_EVERYTHING), "SDL could not initialize!");
		sdlAssert(IMG_Init(IMG_INIT_PNG), "SDL_image could not initialize!");
	}

	~this() {
		IMG_Quit();
		SDL_Quit();
	}

	bool doEvent(Window w) {
		SDL_Event event;
		bool quit = false;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				quit = true;
				break;

			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_ESCAPE)
					quit = true;
				break;

			case SDL_MOUSEWHEEL:
				w.scale += event.wheel.y * 0.01f;
				break;

			default:
				break;
			}
		}

		return !quit;
	}
}

class Window {
public:
	SDL_Window* window;
	SDL_Renderer* renderer;
	int w;
	int h;
	float scale = 2;

	this(int w, int h) {
		this.w = w;
		this.h = h;
		SDL_SetHintWithPriority(SDL_HINT_RENDER_VSYNC, "0", SDL_HINT_OVERRIDE);
		sdlAssert(!SDL_CreateWindowAndRenderer(cast(int)(w * scale), cast(int)(h * scale), 0, &window, &renderer),
				"Failed to create window and renderer");
	}

	~this() {
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
	}

	void reset() {
		SDL_SetRenderTarget(renderer, null);
		SDL_SetWindowSize(window, cast(int)(w * scale), cast(int)(h * scale));
		SDL_RenderSetScale(renderer, scale, scale);
		SDL_RenderSetIntegerScale(renderer, SDL_TRUE);
		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	}
}
