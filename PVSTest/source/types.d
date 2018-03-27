module types;

public import vector;

alias Color = Vector!(ubyte, 4);

enum SDL_PixelType {
	PIXELTYPE_UNKNOWN = SDL_PIXELTYPE_UNKNOWN,
	PIXELTYPE_INDEX1 = SDL_PIXELTYPE_INDEX1,
	PIXELTYPE_INDEX4 = SDL_PIXELTYPE_INDEX4,
	PIXELTYPE_INDEX8 = SDL_PIXELTYPE_INDEX8,
	PIXELTYPE_PACKED8 = SDL_PIXELTYPE_PACKED8,
	PIXELTYPE_PACKED16 = SDL_PIXELTYPE_PACKED16,
	PIXELTYPE_PACKED32 = SDL_PIXELTYPE_PACKED32,
	PIXELTYPE_ARRAYU8 = SDL_PIXELTYPE_ARRAYU8,
	PIXELTYPE_ARRAYU16 = SDL_PIXELTYPE_ARRAYU16,
	PIXELTYPE_ARRAYU32 = SDL_PIXELTYPE_ARRAYU32,
	PIXELTYPE_ARRAYF16 = SDL_PIXELTYPE_ARRAYF16,
	PIXELTYPE_ARRAYF32 = SDL_PIXELTYPE_ARRAYF32
}

enum SDL_PackedOrder {
	BITMAPORDER_NONE = SDL_BITMAPORDER_NONE,
	BITMAPORDER_4321 = SDL_BITMAPORDER_4321,
	BITMAPORDER_1234 = SDL_BITMAPORDER_1234,

	PACKEDORDER_NONE = SDL_PACKEDORDER_NONE,
	PACKEDORDER_XRGB = SDL_PACKEDORDER_XRGB,
	PACKEDORDER_RGBX = SDL_PACKEDORDER_RGBX,
	PACKEDORDER_ARGB = SDL_PACKEDORDER_ARGB,
	PACKEDORDER_RGBA = SDL_PACKEDORDER_RGBA,
	PACKEDORDER_XBGR = SDL_PACKEDORDER_XBGR,
	PACKEDORDER_BGRX = SDL_PACKEDORDER_BGRX,
	PACKEDORDER_ABGR = SDL_PACKEDORDER_ABGR,
	PACKEDORDER_BGRA = SDL_PACKEDORDER_BGRA
}


enum SDL_PackedLayout {
	SDL_PACKEDLAYOUT_NONE,
	SDL_PACKEDLAYOUT_332,
	SDL_PACKEDLAYOUT_4444,
	SDL_PACKEDLAYOUT_1555,
	SDL_PACKEDLAYOUT_5551,
	SDL_PACKEDLAYOUT_565,
	SDL_PACKEDLAYOUT_8888,
	SDL_PACKEDLAYOUT_2101010,
	SDL_PACKEDLAYOUT_1010102
}

enum Tile {
	Void,
	Air,
	RoomContent,
	Wall,
	Portal
}

Tile toTile(Color c) {
	if (c == Color(cast(ubyte)0xFF, cast(ubyte)0xFF, cast(ubyte)0xFF, cast(ubyte)0xFF))
		return Tile.Void;
	else if (c == Color(cast(ubyte)0x3F, cast(ubyte)0x3F, cast(ubyte)0x3F, cast(ubyte)0xFF))
		return Tile.Air;
	else if (c == Color(cast(ubyte)0xFF, cast(ubyte)0xFF, cast(ubyte)0x00, cast(ubyte)0xFF))
		return Tile.RoomContent;
	else if (c == Color(cast(ubyte)0x00, cast(ubyte)0x00, cast(ubyte)0x00, cast(ubyte)0xFF))
		return Tile.Wall;
	else if (c == Color(cast(ubyte)0x00, cast(ubyte)0xFF, cast(ubyte)0xFF, cast(ubyte)0xFF))
		return Tile.Portal;
	else {
		import std.stdio : stderr;

		stderr.writeln("Color: ", c, " is undefined!");
		assert(0);
	}
}

Color toColor(Tile t) {
	switch (t) {
	case Tile.Void:
		return Color(cast(ubyte)0xFF, cast(ubyte)0xFF, cast(ubyte)0xFF, cast(ubyte)0xFF);
	case Tile.Air:
		return Color(cast(ubyte)0x3F, cast(ubyte)0x3F, cast(ubyte)0x3F, cast(ubyte)0xFF);
	case Tile.RoomContent:
		return Color(cast(ubyte)0xFF, cast(ubyte)0xFF, cast(ubyte)0x00, cast(ubyte)0xFF);
	case Tile.Wall:
		return Color(cast(ubyte)0x00, cast(ubyte)0x00, cast(ubyte)0x00, cast(ubyte)0xFF);
	case Tile.Portal:
		return Color(cast(ubyte)0x00, cast(ubyte)0xFF, cast(ubyte)0xFF, cast(ubyte)0xFF);

	default:
		import std.stdio : stderr;

		stderr.writeln("Tile: ", t, " is undefined!");
		assert(0);
	}
}

bool isSolid(Tile t) pure nothrow @nogc {
	// Tile.Void is Solid
	return t != Tile.Air && t != Tile.RoomContent && t != Tile.Portal;
}

//Bresenham's line algorithm
bool validPath(vec2i start, vec2i end, const ref Tile[][] map) {
	import std.math : abs;

	immutable int dx = end.x - start.x;
	immutable int ix = (dx > 0) - (dx < 0);
	immutable size_t dx2 = abs(dx) * 2;
	immutable int dy = end.y - start.y;
	immutable int iy = (dy > 0) - (dy < 0);
	immutable size_t dy2 = abs(dy) * 2;

	vec2i pos = start;
	if (map[pos.y][pos.x].isSolid)
		return false;

	if (dx2 >= dy2) {
		long error = cast(long)(dy2 - (dx2 / 2));
		while (pos.x != end.x) {
			if (error >= 0 && (error || (ix > 0))) {
				error -= dx2;
				pos.y += iy;
			}

			error += dy2;
			pos.x += ix;
			if (map[pos.y][pos.x].isSolid)
				return false;
		}
	} else {
		long error = cast(long)(dx2 - (dy2 / 2));
		while (pos.y != end.y) {
			if (error >= 0 && (error || (iy > 0))) {
				error -= dy2;
				pos.x += ix;
			}

			error += dx2;
			pos.y += iy;
			if (map[pos.y][pos.x].isSolid)
				return false;
		}
	}
	return true;
}

import sdl;

bool validPathRender(vec2i start, vec2i end, const ref Tile[][] map, Window w) {
	import std.math : abs;

	immutable int dx = end.x - start.x;
	immutable int ix = (dx > 0) - (dx < 0);
	immutable size_t dx2 = abs(dx) * 2;
	int dy = end.y - start.y;
	immutable int iy = (dy > 0) - (dy < 0);
	immutable size_t dy2 = abs(dy) * 2;

	bool result = true;
	vec2i pos = start;
	SDL_SetRenderDrawColor(w.renderer, cast(ubyte)0x00, cast(ubyte)0xFF, cast(ubyte)0x00, cast(ubyte)0x10);

	if (map[pos.y][pos.x].isSolid) {
		SDL_SetRenderDrawColor(w.renderer, cast(ubyte)0x7F, cast(ubyte)0x00, cast(ubyte)0x00, cast(ubyte)0x10);
		result = false;
	}

	SDL_RenderDrawPoint(w.renderer, pos.x, pos.y);

	if (dx2 >= dy2) {
		long error = cast(long)(dy2 - (dx2 / 2));
		while (pos.x != end.x) {
			if (error >= 0 && (error || (ix > 0))) {
				error -= dx2;
				pos.y += iy;
			}

			error += dy2;
			pos.x += ix;
			if (map[pos.y][pos.x].isSolid && result) {
				SDL_SetRenderDrawColor(w.renderer, cast(ubyte)0x7F, cast(ubyte)0x00, cast(ubyte)0x00, cast(ubyte)0x10);
				result = false;
			}
			SDL_RenderDrawPoint(w.renderer, pos.x, pos.y);
		}
	} else {
		long error = cast(long)(dx2 - (dy2 / 2));
		while (pos.y != end.y) {
			if (error >= 0 && (error || (iy > 0))) {
				error -= dy2;
				pos.x += ix;
			}

			error += dx2;
			pos.y += iy;
			if (map[pos.y][pos.x].isSolid && result) {
				SDL_SetRenderDrawColor(w.renderer, cast(ubyte)0x7F, cast(ubyte)0x00, cast(ubyte)0x00, cast(ubyte)0x10);
				result = false;
			}
			SDL_RenderDrawPoint(w.renderer, pos.x, pos.y);
		}
	}
	return result;
}

enum Direction {
	posX,
	negX,
	posY,
	negY
}
