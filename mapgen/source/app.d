import std.stdio;
import std.file : readText;
import std.algorithm;
import std.array;
import std.random;
import std.traits;
import std.math;
import std.process;
import std.conv;

import gl3n.linalg : vec2, vec3, vec4, vec2i;
import asdf;

enum int ROOM_UNIT_SIZE = 32;
enum int ROOM_COUNT = 64;
enum MAP_PIXEL_SIZE = ROOM_UNIT_SIZE * ROOM_COUNT;

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
		foreach (y, ref Room[ROOM_COUNT] row; rooms)
			foreach (x, ref Room r; row)
				foreach (ref Model m; r.models)
				{
					m.t[3][0] += x * ROOM_UNIT_SIZE;
					m.t[3][1] += y * ROOM_UNIT_SIZE;
				}
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
	Model[] models;
	bool[Direction] isWall;
	int[2][] canSee;

	bool[ROOM_UNIT_SIZE][ROOM_UNIT_SIZE] layoutMap; // WILL NEVER ROTATE!!!!!!!!!!!!!

	this(this)
	{
		models = models.dup;
		isWall = isWall.dup;
	}

	void rotate(int rotation)
	{
		float angle = rotation * 90f;
		foreach (ref Model m; models)
			m.t *= mat4(vec4(0, sin(angle / 2), 0, cos(angle / 2)));

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
			foreach (int localX; 0 .. ROOM_UNIT_SIZE)
				foreach (int localY; 0 .. ROOM_UNIT_SIZE)
					layoutMap[localY + y * ROOM_UNIT_SIZE][localX + x * ROOM_UNIT_SIZE] = this
						.layoutMap[localY][localX];
			break;
		case 1:
			foreach (int localX; 0 .. ROOM_UNIT_SIZE)
				foreach (int localY; 0 .. ROOM_UNIT_SIZE)
					layoutMap[localY + y * ROOM_UNIT_SIZE][localX + x * ROOM_UNIT_SIZE] = this
						.layoutMap[localX][ROOM_UNIT_SIZE - 1 - localY];
			break;
		case 2:
			foreach (int localX; 0 .. ROOM_UNIT_SIZE)
				foreach (int localY; 0 .. ROOM_UNIT_SIZE)
					layoutMap[localY + y * ROOM_UNIT_SIZE][localX + x * ROOM_UNIT_SIZE] = this
						.layoutMap[ROOM_UNIT_SIZE - 1 - localY][ROOM_UNIT_SIZE - 1 - localX];
			break;
		case 3:
			foreach (int localX; 0 .. ROOM_UNIT_SIZE)
				foreach (int localY; 0 .. ROOM_UNIT_SIZE)
					layoutMap[localY + y * ROOM_UNIT_SIZE][localX + x * ROOM_UNIT_SIZE] = this
						.layoutMap[ROOM_UNIT_SIZE - 1 - localX][localY];
			break;
		default:
			assert(0);
		}
	}
}

struct Model
{
	mat4 t;
	char[64] meshFile;

	this(mat4 t, string meshFile)
	in
	{
		assert(meshFile.length <= this.meshFile.length);
	}
	do
	{
		this.t = t;
		copy(cast(char[]) meshFile, this.meshFile[]);
		foreach (ref ch; this.meshFile[meshFile.length .. $])
			ch = '\0';
	}
}

align(1) struct mat4
{
align(1):
	float[4][4] data = [[1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]];
	alias data this;

	this(float[4][4] d)
	{
		data = d;
	}

	this(vec4 rotation)
	{
		this(vec3(0, 0, 0), vec3(1, 1, 1), rotation);
	}

	this(vec3 pos, vec3 scale, vec4 rotation)
	{
		data[0][0] = scale.x;
		data[1][1] = scale.y;
		data[2][2] = scale.z;
		data[3][3] = 1;

		data[3][0] = pos.x;
		data[3][1] = pos.y;
		data[3][2] = pos.z;

		const float q00 = rotation.x * rotation.x;
		const float q11 = rotation.y * rotation.y;
		const float q22 = rotation.z * rotation.z;
		const float q02 = rotation.x * rotation.z;
		const float q01 = rotation.x * rotation.y;
		const float q12 = rotation.y * rotation.z;
		const float q30 = rotation.w * rotation.x;
		const float q31 = rotation.w * rotation.y;
		const float q32 = rotation.w * rotation.z;

		data[0][0] *= 1.0f - 2.0f * (q11 + q22);
		data[0][1] *= 2.0f * (q01 + q32);
		data[0][2] *= 2.0f * (q02 - q31);

		data[1][0] *= 2.0f * (q01 - q32);
		data[1][1] *= 1.0f - 2.0f * (q00 + q22);
		data[1][2] *= 2.0f * (q12 + q30);

		data[2][0] *= 2.0f * (q02 + q31);
		data[2][1] *= 2.0f * (q12 - q30);
		data[2][2] *= 1.0f - 2.0f * (q00 + q11);
	}

	mat4 opBinary(string op : "*")(mat4 rhs)
	{
		float[4][4] x;
		foreach (c; 0 .. 4)
			foreach (r; 0 .. 4)
				x[c][r] = data[c][0] * rhs[0][r] + data[c][1] * rhs[1][r] + data[c][2]
					* rhs[2][r] + data[c][3] * rhs[3][r];

		return mat4(x);
	}

	void opOpAssign(string op : "*")(mat4 rhs)
	{
		this = this * rhs;
	}
}

void doEntity(ref Room room, Asdf e, mat4 parentTransform)
{
	mat4 transform = parentTransform;
	string meshFile;

	foreach (component; e["components"].byKeyValue)
	{
		switch (component.key)
		{
		case "TransformComponent":
			transform *= mat4(vec3(component.value["position"].byElement()
					.map!"a.get!float(0)".array), vec3(component.value["scale"].byElement()
					.map!"a.get!float(0)".array), vec4(component.value["rotation"].byElement()
					.map!"a.get!float(0)".array));
			break;
		case "MeshComponent":
			meshFile = component.value["meshFile"].get!string(null);
			break;
		default:
			break;
		}
	}

	if (meshFile)
	{
		writeln("\tTransform: ", transform, "\tMesh: ", meshFile);
		room.models ~= Model(transform, meshFile);
	}

	foreach (child; e["children"].byElement)
		doEntity(room, child, transform);
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

		doEntity(room, j["data"], mat4());

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
		assert(r.models.length);
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
			}
		}
	}

	File f = File("map.cmf", "wb"); // Compiled Map Format
	scope (exit)
		f.close();

	foreach (y, ref Room[ROOM_COUNT] row; gm.rooms)
		foreach (x, ref Room r; row)
		{
			uint a = cast(uint) r.models.length;
			f.rawWrite(cast(ubyte[])(&a)[0 .. 1]);
			foreach (ref Model m; r.models)
				f.rawWrite(cast(ubyte[])(&m)[0 .. 1]);

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

				foreach (int i; 0 .. ROOM_UNIT_SIZE)
					drawWallPixel(x * ROOM_UNIT_SIZE + i, y * ROOM_UNIT_SIZE);

				foreach (int i; 0 .. ROOM_UNIT_SIZE)
					drawWallPixel(x * ROOM_UNIT_SIZE + i, (y + 1) * ROOM_UNIT_SIZE - 1);

				foreach (int i; 0 .. ROOM_UNIT_SIZE)
					drawWallPixel(x * ROOM_UNIT_SIZE, y * ROOM_UNIT_SIZE + i);

				foreach (int i; 0 .. ROOM_UNIT_SIZE)
					drawWallPixel((x + 1) * ROOM_UNIT_SIZE - 1, y * ROOM_UNIT_SIZE + i);

				// Center of room
				foreach (yy; 0 .. ROOM_UNIT_SIZE - 2)
					foreach (xx; 0 .. ROOM_UNIT_SIZE - 2)
						pixMap[yy + y * ROOM_UNIT_SIZE + 1][xx + x * ROOM_UNIT_SIZE + 1] = Tile.Air;

				foreach (int i; 1 .. ROOM_UNIT_SIZE - 1)
					foreach (int j; 1 .. ROOM_UNIT_SIZE - 1)
						drawRoomPixel(x * ROOM_UNIT_SIZE + i, y * ROOM_UNIT_SIZE + j);
			}
		}
	}
	write_bmp("map.bmp", MAP_PIXEL_SIZE, MAP_PIXEL_SIZE,
			(cast(ubyte*) pixMap.ptr)[0 .. MAP_PIXEL_SIZE * MAP_PIXEL_SIZE * RGB.sizeof], 3);
}
