module room;

import std.stdio;
import std.bitmanip;

import portal;
import types;

alias RoomIdx = vec2i;

size_t roomID(in RoomIdx id, vec2i roomCount) {
	return id.y * roomCount.x + id.x;
}

size_t[vec4i] lookup;

struct Room {
	RoomIdx id;
	vec2i position;
	static vec2i size;

	PortalIdx[] portals;

	void findPortals(ref Portal[PortalIdx] globalPortals, ref Room[RoomIdx] rooms, const ref Tile[][] map,
			const ref vec2i mapSize, const ref vec2i roomCount) {
		void walk(vec2i pos, vec2i dir) {
			enum State {
				LookingForPortal,
				BuildingPortal
			}
			vec2i outwards = dir.yx;

			State state = State.LookingForPortal;
			Portal portal;

			void finishPortal() {
				writeln("\tFinalizing: ", portal.pos);

				if (portal.pos in lookup)
					return;

				// Verify portal
				for (size_t y; y < portal.pos.w; y++)
					for (size_t x; x < portal.pos.z; x++)
						if (map[portal.pos.y + y][portal.pos.x + x] != Tile.Portal) {
							stderr.writeln("\x1b[93;41mMAP HAS A LEAK (", map[portal.pos.y + y][portal.pos.x + x],
									" instead of portal) [", portal.pos.x + x, ", ", portal.pos.y + y, "], will recalc\x1b[0m");
							return;
						}

				portal.rooms[0] = (portal.pos.xy / size);
				portal.rooms[1] = ((portal.pos.xy + outwards) / size);

				// Verify size
				{
					import std.format : format;

					// TODO: Probably not needed
					assert(portal.pos.z == 2 || portal.pos.w == 2,
							format("Invalid door width or height is not 2: rooms: [%s, %s], [%s, %s],\tpos:[%s,%s]",
								portal.rooms[0].x, portal.rooms[0].y, portal.rooms[1].x, portal.rooms[1].y, portal.pos.x, portal.pos.y));
				}
				portal.id = portalCounter++;
				lookup[portal.pos] = portal.id;
				globalPortals[portal.id] = portal;
				rooms[portal.rooms[0]].portals ~= portal.id;
				rooms[portal.rooms[1]].portals ~= portal.id;

				writeln("\t\x1b[1;32mFinalized: ", portal, "\x1b[0m");
				state = State.LookingForPortal;
				portal = Portal.init;
			}

			for (auto walker = pos; walker != pos + dir * size; walker += dir) {
				final switch (map[walker.y][walker.x]) {
				case Tile.Portal:
					Tile extension = map[walker.y + outwards.y][walker.x + outwards.x];
					if (extension != Tile.Portal) {
						if (extension == Tile.Wall) {
							if (state == State.BuildingPortal)
								goto tileWall;
							else
								break;
						}
						stderr.writeln("\x1b[93;41mMAP HAS A LEAK (", extension, " instead of portal) ", walker, "\x1b[0m");
						return;
					}

					if (state == State.LookingForPortal) {
						portal.pos = vec4i(walker, 1 + outwards.x, 1 + outwards.y);
						state = State.BuildingPortal;
						writeln("Starting at pos: ", portal.pos);
					} else {
						portal.pos.z += dir.x;
						portal.pos.w += dir.y;
						writeln("\tFound next tile, extending: ", portal.pos);
					}
					break;
				case Tile.Wall:
				tileWall:
					if (state == State.BuildingPortal) {
						finishPortal();
						state = State.LookingForPortal;
					}
					break;
				case Tile.Void:
				case Tile.Air:
				case Tile.RoomContent:
					stderr.writeln("\x1b[93;41mMAP HAS A LEAK (",
							map[walker.y][walker.x], " instead of portal or wall) ", walker, "\x1b[0m");
					state = State.LookingForPortal;
					return;
				}
			}
			if (state == State.BuildingPortal) {
				finishPortal();
				state = State.LookingForPortal;
			}
		}

		walk(position + vec2i(0, size.y - 1), vec2i(1, 0)); // Bottom

		walk(position + vec2i(size.x - 1, 0), vec2i(0, 1)); // Right
	}
}
