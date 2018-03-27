import std.stdio;
import std.conv;
import std.string;
import std.bitmanip;

import sdl;
import types;
import portal;
import room;

interface IState {
	void update();
	void render();
	@property bool isDone();
}

final class VisualizeState : IState {
public:
	this(Engine engine) {
		_engine = engine;
	}

	void update() {
		vec2i pos;
		immutable int s = SDL_GetMouseState(&pos.x, &pos.y);
		pos = vec2i(cast(int)(pos.x / _engine._window.scale), cast(int)(pos.y / _engine._window.scale));

		pos /= Room.size;
		_currentRoom = pos in _engine._rooms;
	}

	void render() {
		foreach (int y, const ref Tile[] xRow; _engine._map)
			foreach (int x, const ref Tile tile; xRow) {
				auto c = tile.toColor;
				SDL_SetRenderDrawColor(_engine._window.renderer, c.x, c.y, c.z, c.w);
				SDL_RenderDrawPoint(_engine._window.renderer, x, y);
			}

		if (_currentRoom) {
			alias drawRoom = (room) {
				for (size_t y = 1; y < Room.size.y - 1; y++)
					for (size_t x = 1; x < Room.size.x - 1; x++) {
						SDL_SetRenderDrawColor(_engine._window.renderer, cast(ubyte)0xFF, cast(ubyte)0x00, cast(ubyte)0xFF, cast(ubyte)0x40);
						SDL_RenderDrawPoint(_engine._window.renderer, cast(int)(room.position.x + x), cast(int)(room.position.y + y));
					}
			};
			drawRoom(_currentRoom);
			foreach (portalID; _currentRoom.portals) {
				import std.algorithm : countUntil, map;

				Portal* d = &_engine._portals[portalID];
				foreach (room; d.rooms[].map!(x => _engine._rooms[x]))
					drawRoom(room);

				foreach (i; d.canSeePortal.bitsSet)
					if (_currentRoom.portals.countUntil(i) == -1)
						foreach (room; _engine._portals[i].rooms[].map!(x => _engine._rooms[x]))
							drawRoom(room);
			}

			foreach (portalID; _currentRoom.portals) {
				import std.algorithm : countUntil;

				Portal* d = &_engine._portals[portalID];

				foreach (i; d.canSeePortal.bitsSet)
					if (_currentRoom.portals.countUntil(i) == -1) {
						Portal* dOther = &_engine._portals[i];
						SDL_SetRenderDrawColor(_engine._window.renderer, cast(ubyte)0xFF, cast(ubyte)0x00, cast(ubyte)0x00, cast(ubyte)0xFF);
						for (int y = dOther.pos.y; y < dOther.pos.y + dOther.pos.w; y++)
							for (int x = dOther.pos.x; x < dOther.pos.x + dOther.pos.z; x++)
								SDL_RenderDrawPoint(_engine._window.renderer, x, y);

						done: foreach (aY; d.pos.y .. d.pos.y + d.pos.w)
							foreach (aX; d.pos.x .. d.pos.x + d.pos.z)
								foreach (bY; dOther.pos.y .. dOther.pos.y + dOther.pos.w)
									foreach (bX; dOther.pos.x .. dOther.pos.x + dOther.pos.z)
										validPathRender(vec2i(aX, aY), vec2i(bX, bY), _engine._map, _engine._window);
					}

				SDL_SetRenderDrawColor(_engine._window.renderer, cast(ubyte)0xFF, cast(ubyte)0xFF, cast(ubyte)0x00, cast(ubyte)0xFF);
				for (int y = d.pos.y; y < d.pos.y + d.pos.w; y++)
					for (int x = d.pos.x; x < d.pos.x + d.pos.z; x++) {
						SDL_RenderDrawPoint(_engine._window.renderer, x, y);
					}
			}
		}
	}

	@property bool isDone() {
		return false;
	}

private:
	Engine _engine;
	Room* _currentRoom;
}

class Engine {
public:
	this(string file, vec2i roomSize, string output, bool visualize, bool dot) {
		Room.size = roomSize;
		_sdl = new SDL;
		_loadData(file);
		_savePVS(output);
		if (dot)
			_genDot();
		if (visualize)
			_window = new Window(_mapSize.x, _mapSize.y);
	}

	~this() {
		_window.destroy;
		_sdl.destroy;
	}

	int run() {
		enum State {
			explore = 0,
			visualize,

			end
		}

		State state = State.visualize; //State.explore;
		IState[State] states = [ //State.explore : cast(IState)new ExploreState(this),//
		State.visualize : cast(IState)new VisualizeState(this), //
			State.end : cast(IState)null];

		while (states[state] && _sdl.doEvent(_window)) {
			states[state].update();

			_window.reset();
			SDL_SetRenderDrawColor(_window.renderer, 0, 0, 0, 255);
			SDL_RenderClear(_window.renderer);

			states[state].render();

			SDL_RenderPresent(_window.renderer);

			if (states[state].isDone())
				state++;
		}
		return 0;
	}

private:
	SDL _sdl;
	Window _window;

	Tile[][] _map;
	Portal[size_t] _portals;
	Room[RoomIdx] _rooms;
	vec2i _mapSize;
	vec2i _roomCount;

	void _loadData(string file) {
		{ // Create tiles
			SDL_Surface* tmp = SDL_LoadBMP(file.toStringz);
			assert(tmp, file ~ " is missing");
			SDL_Surface* mapSurface = SDL_ConvertSurfaceFormat(tmp, SDL_PIXELFORMAT_RGBA32, 0);
			SDL_FreeSurface(tmp);
			assert(mapSurface, "Failed to convert format");
			_mapSize = vec2i(mapSurface.w, mapSurface.h);
			_roomCount = _mapSize / Room.size;
			auto f = mapSurface.format.format;

			writeln("Map size: ", _mapSize);
			foreach (y; 0 .. _mapSize.y) {
				Tile[] xRow;
				foreach (x; 0 .. _mapSize.x)
					xRow ~= (cast(Color*)mapSurface.pixels)[y * mapSurface.w + x].toTile;

				_map ~= xRow;
			}
		}

		{ // Create rooms
			foreach (y; 0 .. _roomCount.y)
				foreach (x; 0 .. _roomCount.x) {
					auto r = Room(RoomIdx(x, y), vec2i(x * Room.size.x, y * Room.size.y));
					_rooms[r.id] = r;
				}
		}

		{ // Find portals
			foreach (ref Room r; _rooms)
				r.findPortals(_portals, _rooms, _map, _mapSize, _roomCount);
		}

		calculatePortalVisibilities(_portals, _map, _roomCount);
	}

	void _savePVS(string output) {
		import std.json : JSONValue;
		import std.algorithm : sort, uniq;
		import std.array : array;
		import std.file : write;
		JSONValue root;
		foreach (currentRoom; _rooms) {
			size_t[] canSee;
			//writeln("Exploring: ", currentRoom.id.roomID(_roomCount));
			alias drawRoom = (room) {
				//writeln("\tCan see: ", room.id.roomID(_roomCount));
				if (room.id != currentRoom.id)
					canSee ~= room.id.roomID(_roomCount);
			};
			drawRoom(currentRoom);
			foreach (portalID; currentRoom.portals) {
				import std.algorithm : countUntil, map;

				Portal* d = &_portals[portalID];
				foreach (room; d.rooms[].map!(x => _rooms[x]))
					drawRoom(room);

				foreach (i; d.canSeePortal.bitsSet)
					if (currentRoom.portals.countUntil(i) == -1)
						foreach (room; _portals[i].rooms[].map!(x => _rooms[x]))
							drawRoom(room);
			}

			root[currentRoom.id.roomID(_roomCount).to!string] = canSee.sort.uniq.array;
		}
		write(output, root.toString());
	}

	void _genDot() {
		import std.stdio : File;

		File f = File("map.dot", "w");
		scope (exit)
			f.close;
		f.writeln("digraph Rooms {");

		bool[PortalIdx] haveOutput;

		foreach (const ref Portal p; _portals) {
			f.writefln("P_%s[label=\"%s\",color=blue,style=filled];", p.id, p.pos);
			size_t canSee;
			foreach (otherP; p.canSeePortal.bitsSet) {
				import std.algorithm : min, max;

				PortalIdx a = min(p.id, otherP);
				PortalIdx b = max(p.id, otherP);
				if ((((a & 0xFF) << 8) | b) in haveOutput)
					continue;
				f.writefln("P_%s -> P_%s[dir=none];\n", a, b);
				haveOutput[((a & 0xFF) << 8) | b] = true;
				canSee++;
			}
			//writeln(p.id, " can see ", canSee, " portals");
		}
		f.writeln("}");
	}
}

int main(string[] args) {
	import std.getopt : getopt, arraySep, defaultGetoptPrinter;

	string file;
	int roomSize;
	string output;
	bool visualize;
	bool dot;
	auto helpInformation = getopt(args, //
			"f|file", "The map file", &file, //
			"s|size", "The room size", &roomSize, //
			"o|output",
			"The pvs output file", &output, //
			"v|visualize", "Visualize the PVS?", &visualize, //
			"d|dot", "Generate a map.dot", &dot //
			);
	if (helpInformation.helpWanted) {
		defaultGetoptPrinter("PVSTest", helpInformation.options);
		return 0;
	}

	bool err;
	if (!file) {
		stderr.writeln("You need to specify a map file! (-f map.bmp)");
		err = true;
	}
	if (!roomSize) {
		stderr.writeln("You need to specify a room size! (-s 32)");
		err = true;
	}
	if (!output) {
		stderr.writeln("You need to specify a output file! (-o map.pvs)");
		err = true;
	}
	if (err)
		return -1;

	Engine e = new Engine(file, vec2i(roomSize), output, visualize, dot);
	scope (exit)
		e.destroy;
	if (visualize)
		return e.run();
	else
		return 0;
}
