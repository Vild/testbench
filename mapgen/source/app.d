import std.stdio;
import std.file : readText;
import std.algorithm;
import std.array;
import std.random;
import std.traits;
import std.math;
import std.process;
import std.conv;

import gfm.math.vector : vec3 = vec3f, vec4 = vec4f, vec2i;
import gfm.math.matrix : mat4 = mat4f;
import gfm.math.quaternion : quat = quatf;
import asdf;

enum int ROOM_UNIT_SIZE = 32;
enum int MAP_ROOM_SIZE = 32;
enum int ROOM_COUNT = 64;
enum MAP_PIXEL_SIZE = MAP_ROOM_SIZE * ROOM_COUNT;

/*T getDefault(T, K)(ref T[K] aa, K key, T def = T.init)
{
	return aa[key] = aa.get(key, def);
}*/

struct GenerateMap
{
	Room[ROOM_COUNT][ROOM_COUNT] rooms;

	bool[MAP_PIXEL_SIZE][MAP_PIXEL_SIZE] layoutMap;

	void updatePosition()
	{
		foreach (int y, ref Room[ROOM_COUNT] row; rooms)
			foreach (int x, ref Room r; row)
				if (r.entities.length)
					r.entities[0].pos = vec3((0.5f + x) * ROOM_UNIT_SIZE, 0.0f, (0.5f + y) * ROOM_UNIT_SIZE);
	}

	void doGeneration(ref Room[] roomTemplates)
	{
		rooms[ROOM_COUNT / 2][ROOM_COUNT / 2] = roomTemplates[12];
		rooms[ROOM_COUNT / 2][ROOM_COUNT / 2].insertMap(0, layoutMap, ROOM_COUNT / 2, ROOM_COUNT / 2);
		doGeneration(roomTemplates, ROOM_COUNT / 2, ROOM_COUNT / 2);
	}

	private void doGeneration(ref Room[] roomTemplates, int xParent, int yParent)
	{
		foreach (Direction d; EnumMembers!Direction)
		{
			if (rooms[yParent][xParent].isWall[d])
				continue;
			writeln("trying: ", d);
			int x = xParent + DirectionVec[d].x;
			int y = yParent + DirectionVec[d].y;

			// Don't regen room
			if (x < 0 || y < 0 || x >= ROOM_COUNT || y >= ROOM_COUNT || rooms[y][x].id)
				continue;

			bool[Direction] open = () {
				bool[Direction] open;
				foreach (Direction d; EnumMembers!Direction)
					if (x + DirectionVec[d].x < 0 || x + DirectionVec[d].x >= rooms.length
							|| y + DirectionVec[d].y < 0 || y + DirectionVec[d].y >= rooms.length)
						open[d] = false;
					else
					{
						auto r = &rooms[y + DirectionVec[d].y][x + DirectionVec[d].x];
						if (r.id)
							open[d] = !r.isWall[d.next(2)];
						else
							open[d] = true;
					}
				return open;
			}();
			writeln("open:", open);

			int tries = 32;
			while (tries--)
			{
				// Make no copy
				Room* r = &roomTemplates[uniform(0, roomTemplates.length)];

				int count;
				foreach (Direction d, bool b; r.isWall)
					if (!b)
						count++;
				if (uniform(0, 2) && count < 3)
					continue;

				int rotation = uniform(0, 4);

				if (r.isWall[d.next(2)])
					continue;

				bool canPlace = true;
				foreach (Direction dd; EnumMembers!Direction)
					if (!r.isWall[dd.next(rotation)])
						if (!open[dd])
							canPlace = false;
				if (!canPlace)
					continue;

				rooms[y][x] = *r;

				rooms[y][x].rotate(rotation);
				rooms[y][x].insertMap(rotation, layoutMap, x, y);

				doGeneration(roomTemplates, x, y);
				break;
			}
		}
	}
}

enum Direction : int
{
	north = 0,
	east = 1,
	south = 2,
	west = 3
}

Direction next(Direction d, int amount)
{
	auto r = cast(Direction)((cast(int) d + amount) % 4);
	return r;
}

@property auto DirectionVec()
{
	vec2i[Direction] data = [
		Direction.north : vec2i(0, -1), Direction.east : vec2i(1, 0),
		Direction.south : vec2i(0, 1), Direction.west : vec2i(-1, 0)
	];
	return data;
}

struct Room
{
	int id;
	Entity[] entities;
	bool[Direction] isWall;
	int[2][] canSee;

	float rotationAngle = 0;

	bool[MAP_ROOM_SIZE][MAP_ROOM_SIZE] layoutMap;

	this(this)
	{
		entities = entities.dup;
		isWall = isWall.dup;
	}

	void rotate(int rotation)
	{
		entities[0].rot = quat.fromAxis(vec3(0, 1, 0), rotation * PI / 2);
		{
			auto copy = isWall.dup;
			foreach (Direction d; EnumMembers!Direction)
				isWall[d] = copy[d.next(rotation)];
		}
	}

	void insertMap(int rotation, ref bool[MAP_PIXEL_SIZE][MAP_PIXEL_SIZE] layoutMap, int x, int y)
	{
		switch (rotation)
		{
		case 0:
			foreach (int localX; 0 .. MAP_ROOM_SIZE)
				foreach (int localY; 0 .. MAP_ROOM_SIZE)
					layoutMap[localY + y * MAP_ROOM_SIZE][localX + x * MAP_ROOM_SIZE] = this
						.layoutMap[localY][localX];
			break;
		case 1:
			foreach (int localX; 0 .. MAP_ROOM_SIZE)
				foreach (int localY; 0 .. MAP_ROOM_SIZE)
					layoutMap[localY + y * MAP_ROOM_SIZE][localX + x * MAP_ROOM_SIZE] = this
						.layoutMap[localX][MAP_ROOM_SIZE - 1 - localY];
			break;
		case 2:
			foreach (int localX; 0 .. MAP_ROOM_SIZE)
				foreach (int localY; 0 .. MAP_ROOM_SIZE)
					layoutMap[localY + y * MAP_ROOM_SIZE][localX + x * MAP_ROOM_SIZE] = this
						.layoutMap[MAP_ROOM_SIZE - 1 - localY][MAP_ROOM_SIZE - 1 - localX];
			break;
		case 3:
			foreach (int localX; 0 .. MAP_ROOM_SIZE)
				foreach (int localY; 0 .. MAP_ROOM_SIZE)
					layoutMap[localY + y * MAP_ROOM_SIZE][localX + x * MAP_ROOM_SIZE] = this
						.layoutMap[MAP_ROOM_SIZE - 1 - localX][localY];
			break;
		default:
			assert(0);
		}
	}
}

struct Entity
{
	bool isMesh;
	char[64] meshFile;
	bool hasTransform;
	vec3 pos = vec3(0);
	vec3 scale = vec3(1);
	quat rot = quat(1, 0, 0, 0);

	size_t parentIdx = size_t.max;

	mat4 getTransform(ref Room r)
	{
		mat4 parentM = (parentIdx != size_t.max && r.entities[parentIdx].hasTransform) ? r.entities[parentIdx].getTransform(
				r) : mat4.identity;
		return parentM * (mat4.translation(pos) * cast(mat4) rot * mat4.scaling(scale));
	}
}

void doEntity(ref Room room, Asdf j, size_t parentIdx)
{
	Entity e;
	e.parentIdx = parentIdx;
	foreach (component; j["components"].byKeyValue)
	{
		switch (component.key)
		{
		case "TransformComponent":
			e.hasTransform = true;
			auto p = component.value["position"].byElement().map!"a.get!float(0)".array;
			auto s = component.value["scale"].byElement().map!"a.get!float(0)".array;
			auto r = component.value["rotation"].byElement().map!"a.get!float(0)".array;
			e.pos = vec3(p[0], p[1], p[2]);
			e.scale = vec3(s[0], s[1], s[2]);
			e.rot = quat(r[3], r[0], r[1], r[2]);
			break;
		case "MeshComponent":
			auto meshFile = component.value["meshFile"].get!string(null);
			copy(cast(char[]) meshFile, e.meshFile[]);
			foreach (ref ch; e.meshFile[meshFile.length .. $])
				ch = '\0';
			e.isMesh = true;
			break;
		default:
			break;
		}
	}

	room.entities ~= e;

	size_t myID = room.entities.length - 1;
	foreach (child; j["children"].byElement)
		doEntity(room, child, myID);
}

int main(string[] args)
{
	import core.memory : GC;
	import std.file : dirEntries, SpanMode;

	GC.disable();
	scope (exit)
		GC.enable();
	static Room[] rooms;
	int idCounter = 1;
	string[] names;

	foreach (file; dirEntries("rooms", "*.json", SpanMode.depth))
	{
		assert(file.name.endsWith(".json"));
		Room room;
		Asdf j = readText(file.name).parseJson;
		string name = j["name"].get!string(null);
		writeln("Room name: ", name);
		names ~= file.name;
		room.id = idCounter++;

		room.entities ~= Entity(false, "", true, vec3(0), vec3(1), quat.fromAxis(vec3(0, 1, 0), 0));

		{
			auto roomC = j["data"]["components"]["RoomComponent"];
			room.isWall[Direction.north] = !roomC["doorsN"].get!bool(false);
			room.isWall[Direction.east] = !roomC["doorsE"].get!bool(false);
			room.isWall[Direction.south] = !roomC["doorsS"].get!bool(false);
			room.isWall[Direction.west] = !roomC["doorsW"].get!bool(false);
			int y;
			foreach (row; roomC["map"].byElement)
			{
				int x;
				foreach (val; row.byElement)
					room.layoutMap[x++][y] = val.get!bool(false);
				y++;
			}
		}

		doEntity(room, j["data"], 0);
		rooms ~= room;
	}

	static GenerateMap gm;
	foreach (const ref Room r; rooms)
	{
		assert(r.id);
		assert(Direction.north in r.isWall);
		assert(Direction.east in r.isWall);
		assert(Direction.south in r.isWall);
		assert(Direction.west in r.isWall);
		assert(r.entities.length > 1);
	}

	gm.doGeneration(rooms);
	gm.updatePosition();
	writeln();
	foreach (int id, string name; names)
		writeln(id + 1, ": ", name, "\t", rooms[id].isWall.get(Direction.north,
				true) ? ' ' : '^', rooms[id].isWall.get(Direction.east, true) ? ' ' : '>',
				rooms[id].isWall.get(Direction.south, true) ? ' ' : 'V',
				rooms[id].isWall.get(Direction.west, true) ? ' ' : '<');
	writeln("#####################################################");
	foreach (const ref row; gm.rooms)
	{
		write('|');
		foreach (const ref Room room; row)
			writef("  %c  %s", room.isWall.get(Direction.north, true) ? ' ' : '^', "|");
		writeln;
		write('|');
		foreach (const ref Room room; row)
			writef("%c%2d %c%s", room.isWall.get(Direction.west, true) ? ' ' : '<',
					room.id, room.isWall.get(Direction.east, true) ? ' ' : '>', "|");
		writeln;
		write('|');
		foreach (const ref Room room; row)
			writef("  %c  %s", room.isWall.get(Direction.south, true) ? ' ' : 'V', "|");
		writeln;
	}
	writeln("#####################################################");
	writeln();
	writeMap(gm);

	import std.range;

	int[2][][ROOM_COUNT][ROOM_COUNT] canSeeCache;
	{
		auto p = execute(["../../PVSTest/bin/PVSTest", "-f", "./map.bmp", "-s",
				"32", "-o", "./map.pvs"]);
		if (p.status)
		{
			stderr.writeln(p.output);
			assert(0);
		}
		else
			writeln(p.output);
		Asdf pvs = readText("map.pvs").parseJson;

		foreach (entry; pvs.byKeyValue)
		{
			int id = entry.key.to!int;
			int x = id % ROOM_COUNT;
			int y = id / ROOM_COUNT;
			with (gm.rooms[y][x])
			{
				canSee ~= [x, y];
				foreach (Asdf other; entry.value.byElement)
				{
					int oID = other.get!int(int.max);
					int oX = oID % ROOM_COUNT;
					int oY = oID / ROOM_COUNT;
					canSee ~= [oX, oY];
				}
				canSeeCache[y][x] = canSee.sort.array;
			}
		}
	}

	foreach (y, ref row; gm.rooms)
		foreach (x, ref Room r; row)
		{
			int[2][] canSeeLists;
			foreach (yy; -1 .. 2)
				if (y + yy > 0 && y + yy < ROOM_COUNT)
					foreach (xx; -1 .. 2)
						if (x + xx > 0 && x + xx < ROOM_COUNT)
							foreach (see; canSeeCache[y + yy][x + xx])
								canSeeLists ~= see;
			r.canSee = canSeeLists.sort.uniq.array;
		}

	File f = File("map.cmf", "wb"); // Compiled Map Format
	scope (exit)
		f.close();
	foreach (y, ref Room[ROOM_COUNT] row; gm.rooms)
		foreach (x, ref Room r; row)
		{
			import std.algorithm : filter, count;

			auto models = r.entities.filter!(x => x.isMesh);
			uint a = cast(uint) models.count;
			f.rawWrite(cast(ubyte[])(&a)[0 .. 1]);
			foreach (ref Entity m; models)
			{
				auto t = m.getTransform(r).transposed;
				f.rawWrite(cast(ubyte[])(t.ptr[0 .. 4 * 4]));
				f.rawWrite(cast(ubyte[])(m.meshFile));
			}

			a = cast(uint) r.canSee.length;
			f.rawWrite(cast(ubyte[])(&a)[0 .. 1]);
			foreach (ref int[2] cs; r.canSee)
				f.rawWrite(cast(ubyte[]) cs[]);
		}

	return 0;
}

align(1) struct RGB
{
	align(1) ubyte r, g, b;
}

enum Tile : RGB
{
	Void = RGB(0xFF, 0xFF, 0xFF),
	Air = RGB(0x3F, 0x3F, 0x3F),
	RoomContent = RGB(0xFF, 0xFF, 0x00),
	Wall = RGB(0x00, 0x00, 0x00),
	Portal = RGB(0x00, 0xFF, 0xFF)
}

void writeMap(ref GenerateMap gm)
{
	import imageformats.bmp;

	static RGB[MAP_PIXEL_SIZE][MAP_PIXEL_SIZE] pixMap;

	foreach (ref row; pixMap)
		foreach (ref p; row)
			p = Tile.Void;
	foreach (int y; 0 .. ROOM_COUNT)
	{
		foreach (int x; 0 .. ROOM_COUNT)
		{
			Room* room = &gm.rooms[y][x];
			if (room.id)
			{
				// Walls
				alias drawWallPixel = (int x, int y) {
					auto color = Tile.Wall;
					if (gm.layoutMap[y][x])
						color = Tile.Portal;
					pixMap[y][x] = color;
				};
				alias drawRoomPixel = (int x, int y) {
					auto color = Tile.Air;
					if (gm.layoutMap[y][x])
						color = Tile.RoomContent;
					pixMap[y][x] = color;
				};
				foreach (int i; 0 .. MAP_ROOM_SIZE)
					drawWallPixel(x * MAP_ROOM_SIZE + i, y * MAP_ROOM_SIZE);
				foreach (int i; 0 .. MAP_ROOM_SIZE)
					drawWallPixel(x * MAP_ROOM_SIZE + i, (y + 1) * MAP_ROOM_SIZE - 1);

				foreach (int i; 0 .. MAP_ROOM_SIZE)
					drawWallPixel(x * MAP_ROOM_SIZE, y * MAP_ROOM_SIZE + i);

				foreach (int i; 0 .. MAP_ROOM_SIZE)
					drawWallPixel((x + 1) * MAP_ROOM_SIZE - 1, y * MAP_ROOM_SIZE + i);

				// Center of room
				foreach (yy; 0 .. MAP_ROOM_SIZE - 2)
					foreach (xx; 0 .. MAP_ROOM_SIZE - 2)
						pixMap[yy + y * MAP_ROOM_SIZE + 1][xx + x * MAP_ROOM_SIZE + 1] = Tile.Air;
				foreach (int i; 1 .. MAP_ROOM_SIZE - 1)
					foreach (int j; 1 .. MAP_ROOM_SIZE - 1)
						drawRoomPixel(x * MAP_ROOM_SIZE + i, y * MAP_ROOM_SIZE + j);
			}
		}
	}
	write_bmp("map.bmp", MAP_PIXEL_SIZE, MAP_PIXEL_SIZE,
			(cast(ubyte*) pixMap.ptr)[0 .. MAP_PIXEL_SIZE * MAP_PIXEL_SIZE * RGB.sizeof], 3);
}
