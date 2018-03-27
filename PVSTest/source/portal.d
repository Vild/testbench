module portal;

import std.bitmanip : BitArray;

import room;
import types;

alias PortalIdx = size_t;

/*size_t portalID(in PortalIdx id, ref const vec2i roomCount) {
	import std.stdio : writeln;

	immutable size_t x = id.x / Room.size.x;
	immutable size_t y = id.y / Room.size.y;
	immutable size_t horizontalPortal = (id.y % Room.size.y) == (Room.size.y - 1) ? 1 : 0;

	immutable size_t ret = 2 * (y * roomCount.x + x) + horizontalPortal;
	//writeln("id: ", id.xy, " -> ", ret);
	return ret;
}*/

PortalIdx portalCounter;

struct Portal {
	PortalIdx id;
	vec4i pos;
	RoomIdx[2] rooms; // Each portal connects two rooms
	BitArray canSeePortal;

	string toString() const {
		import std.format : format;

		return format("\x1b[1;34mPortal(\x1b[1;32mid: \x1b[1;33m%s\x1b[1;34m,\x1b[1;32mposition: \x1b[1;33m%s\x1b[1;34m,\t\x1b[1;32mrooms: \x1b[1;33m%s\x1b[1;34m)\x1b[0m", id, pos, rooms);
	}
}

void calculatePortalVisibilities(ref Portal[PortalIdx] portals, const ref Tile[][] tiles, const ref vec2i roomCount) {
	import std.algorithm : map, each, fold, joiner, sort, uniq, filter;
	import std.array : array;
	import std.range : chain;
	import roundRobin : roundRobin, Result;
	import std.parallelism : parallel;
	import std.algorithm : max;
	import std.stdio : writeln;

	PortalIdx maxIdx = portalCounter;
	portals.each!((ref x) => x.canSeePortal.length = maxIdx);

	PortalIdx[] portalIndices = portals.byKey.array;
	scope (exit)
		portalIndices.destroy;

	// Make sure it's even
	/*if (portalIndices.length % 2 == 1)
		portalIndices ~= PortalIdx.max;*/
	//Result!PortalIdx[] toCheck = roundRobin(portalIndices).sort!().uniq.array;
	/*scope (exit)
		toCheck.destroy;
	writeln("toCheck.length: ", toCheck.length);*/

	// Check everything agains everything
	auto toCheck = portalIndices.map!(a => portalIndices.map!(b => Result!PortalIdx(a, b))).joiner.filter!(x => x.a != x.b).array;

	import std.parallelism;
	foreach (ref Result!PortalIdx r; toCheck.parallel) {
		/*if (r.a == PortalIdx.max || r.b == PortalIdx.max)
			continue;*/
		Portal* a = &portals[r.a];
		Portal* b = &portals[r.b];

		done: foreach (aY; a.pos.y .. a.pos.y + a.pos.w)
			foreach (aX; a.pos.x .. a.pos.x + a.pos.z)
				foreach (bY; b.pos.y .. b.pos.y + b.pos.w)
					foreach (bX; b.pos.x .. b.pos.x + b.pos.z)
						if (validPath(vec2i(aX, aY), vec2i(bX, bY), tiles)) {
							a.canSeePortal[r.b] = true;
							b.canSeePortal[r.a] = true;
							break done;
						}
	}
}
