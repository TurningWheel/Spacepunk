// Generator.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "Generator.hpp"
#include "Console.hpp"
#include "File.hpp"

const Sint32 Generator::TUNNELDIRS[4] = { WEST, NORTH, SOUTH, EAST };
const Sint32 Generator::OPPOSITE[4] = { WEST, NORTH, EAST, SOUTH };
const Sint32 Generator::cos[4] = { 1, 0, -1, 0 };
const Sint32 Generator::sin[4] = { 0, 1, 0, -1 };
const Generator::dungeon_layout_t Generator::dungeonLayout;
const Generator::corridor_layout_t Generator::corridorLayout;
const Generator::map_style_t Generator::mapStyle;

const Generator::closeend_t Generator::closeends[4] = {
	{
		{ {-1, 0}, {-1, -1}, {0, -1}, {1, -1}, {1, 0} },
		{ { 0, 0 } },
		{ 0, 1 }
	},
	{
		{ {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}, {0, 1} },
		{ { 0, 0 } },
		{ 1, 0 }
	},
	{
		{ {-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0} },
		{ { 0, 0 } },
		{ 0, -1 }
	},
	{
		{ {0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1} },
		{ { 0, 0 } },
		{ -1, 0 }
	}
};

static int console_generatorTest(int argc, const char** argv) {
	String inPath, outPath;
	if (argc == 1) {
		inPath = "maps/tilesets/template.json";
		outPath = "output.txt";
	} else if (argc == 2) {
		inPath = "maps/tilesets/template.json";
		outPath = argv[1];
	} else if (argc >= 3) {
		inPath = argv[1];
		outPath = argv[2];
	}

	inPath = mainEngine->buildPath(inPath.get());

	Generator gen(false);
	FileHelper::readObject(inPath.get(), gen);
	gen.createDungeon();
	gen.writeFile(outPath.get());

	return 0;
}

static Ccmd ccmd_generatorTest("generator.test", "test the dungeon generator", &console_generatorTest);

Generator::Generator(bool _clientObj) {
	clientObj = _clientObj;
}

Generator::Generator(bool _clientObj, const options_t& _options) :
	options(_options) {
	clientObj = _clientObj;
}

Generator::~Generator() {
}

void Generator::writeFile(const char* path) {
	Sint32 size = options.dungeonWidth * options.dungeonHeight;
	Uint32 count = size + options.dungeonHeight;

	char* buf = new char[count];
	for (Sint32 y = 0; y < options.dungeonHeight; ++y) {
		for (Sint32 x = 0; x < options.dungeonWidth; ++x) {
			Uint32& tile = tiles[y + x * options.dungeonHeight];
			char& buffer = buf[x + y * (options.dungeonWidth + 1)];
			if (tile & ROOM) {
				buffer = '#';
			} else if (tile & DOOR_SPACE) {
				buffer = '+';
			} else if (tile & CORRIDOR) {
				buffer = '.';
			} else if (tile & STAIRS) {
				buffer = '>';
			} else {
				buffer = ' ';
			}
		}
		buf[options.dungeonWidth + y * (options.dungeonWidth + 1)] = '\n';
	}

	FILE* fp = fopen(path, "w");
	if (!fp) {
		return;
	}
	fwrite(buf, sizeof(char), count, fp);
	fclose(fp);
}

void Generator::createDungeon() {
	subCols = options.dungeonWidth / options.subdivisor;
	subRows = options.dungeonHeight / options.subdivisor;
	maxCols = subCols * options.subdivisor;
	maxRows = subRows * options.subdivisor;

	roomBase = (options.roomMin + 1) / 2;
	roomRadix = (options.roomMax - options.roomMin) / 2 + 1;

	initCells();
	emplaceRooms();
	openRooms();
	corridors();
	if (options.addStairs > 0) {
		emplaceStairs();
	}
	cleanDungeon();
}

void Generator::initCells() {
	Sint32& width = options.dungeonWidth;
	Sint32& height = options.dungeonHeight;
	Sint32 size = width * height;

	tiles.resize(size);
	if (options.seed) {
		rand.seedValue(options.seed);
	} else {
		rand.seedTime();
	}

	if (options.dungeonLayout == "Round") {
		roundMask();
	} else {
		maskCells();
	}
}

void Generator::maskCells() {
	Sint32& width = options.dungeonWidth;
	Sint32& height = options.dungeonHeight;

	float r_x = width / 3.f;
	float r_y = height / 3.f;

	auto layout = dungeonLayout.get(options.dungeonLayout.get());

	Sint32 index = 0;
	for (Sint32 u = 0; u < width; ++u) {
		Sint32 x = u / r_x;
		for (Sint32 v = 0; v < height; ++v) {
			Sint32 y = v / r_y;
			tiles[index] = layout[y][x] ? NOTHING : BLOCKED;
			++index;
		}
	}
}

void Generator::roundMask() {
	Sint32& width = options.dungeonWidth;
	Sint32& height = options.dungeonHeight;

	Sint32 index = 0;
	for (Sint32 u = 0; u < width; ++u) {
		for (Sint32 v = 0; v < height; ++v) {
			Sint32 r = (v - subRows) * options.subdivisor;
			Sint32 c = (u - subCols) * options.subdivisor;
			Sint32 d = sqrtf((float)(r + c));
			tiles[index] = d <= subCols ? NOTHING : BLOCKED;
			++index;
		}
	}
}

void Generator::emplaceRooms() {
	if (options.roomLayout == "Packed") {
		packRooms();
	} else if (options.roomLayout == "Scattered") {
		scatterRooms();
	} else {
		mainEngine->fmsg(Engine::MSG_WARN, "unknown room layout type, defaulting to Scattered");
		scatterRooms();
	}
}

void Generator::packRooms() {
	for (Sint32 i = 1; i < subCols; ++i) {
		Sint32 c = (i * options.subdivisor) + 1;
		for (Sint32 j = 1; j < subRows; ++j) {
			Sint32 r = (j * options.subdivisor) + 1;

			if (tiles[r + c * options.dungeonHeight] & ROOM) {
				continue;
			} else if ((i == 1 || j == 1) && (rand.getUint8() % 2)) {
				continue;
			}

			emplaceRoom(i, j);
		}
	}
}

void Generator::scatterRooms() {
	Sint32 numRooms = options.complex ? allocRooms() / 2 : allocRooms() * 2;
	for (Sint32 i = 0; i < numRooms; ++i) {
		emplaceRoom();
	}
}

Sint32 Generator::allocRooms() {
	Sint32& width = options.dungeonWidth;
	Sint32& height = options.dungeonHeight;
	Sint32 dungeonArea = width * height;
	Sint32 roomArea = options.roomMax * options.roomMax;
	Sint32 numRooms = dungeonArea / roomArea;
	return numRooms;
}

void Generator::emplaceRoom(Sint32 x, Sint32 y) {
	Rect<Sint32> proto = setRoom(x, y);

	Sint32 r1 = proto.y * options.subdivisor + 1;
	Sint32 c1 = proto.x * options.subdivisor + 1;
	Sint32 r2 = (proto.y + proto.h) * options.subdivisor - 1;
	Sint32 c2 = (proto.x + proto.w) * options.subdivisor - 1;

	if (r1 < 1 || r2 >= maxRows - 1)
		return;
	if (c1 < 1 || c2 >= maxCols - 1)
		return;

	// check for collisions with other rooms
	if (!options.complex && soundRoom(r1, r2, c1, c2))
		return;
	Sint32 roomID = numRooms;
	++numRooms;

	// place room
	for (Sint32 c = c1; c <= c2; ++c) {
		for (Sint32 r = r1; r <= r2; ++r) {
			Uint32 index = r + c * options.dungeonHeight;
			if (tiles[index] & ENTRANCE) {
				tiles[index] &= ~E_SPACE;
			}
			tiles[index] |= ROOM | (roomID << 6);
		}
	}
	Sint32 height = ((r2 - r1) + 1) * 10;
	Sint32 width = ((c2 - c1) + 1) * 10;

	room_t room;
	room.id = roomID;
	room.row = room.north = r1;
	room.col = room.west = c1;
	room.south = r2; room.east = c2;
	room.height = height; room.width = width;
	room.area = height * width;
	rooms.push(room);

	// block corridors on room boundary
	{
		Sint32& width = options.dungeonWidth;
		Sint32& height = options.dungeonHeight;
		for (Sint32 r = r1 - 1; r <= r2 + 1; ++r) {
			if (!(tiles[r + (c1 - 1) * height] & (ROOM | ENTRANCE))) {
				tiles[r + (c1 - 1) * height] |= PERIMETER;
			}
			if (!(tiles[r + (c2 + 1) * height] & (ROOM | ENTRANCE))) {
				tiles[r + (c2 + 1) * height] |= PERIMETER;
			}
		}
		for (Sint32 c = c1 - 1; c <= c2 + 1; ++c) {
			if (!(tiles[(r1 - 1) + c * height] & (ROOM | ENTRANCE))) {
				tiles[(r1 - 1) + c * height] |= PERIMETER;
			}
			if (!(tiles[(r2 + 1) + c * height] & (ROOM | ENTRANCE))) {
				tiles[(r2 + 1) + c * height] |= PERIMETER;
			}
		}
	}
}

Rect<Sint32> Generator::setRoom(Sint32 x, Sint32 y) {
	Rect<Sint32> proto;

	// set y/height
	if (y < 0) {
		proto.h = (rand.getSint32() % roomRadix) + roomBase;
		proto.y = rand.getSint32() % (subRows - proto.h);
	} else {
		Sint32 a = (subRows - roomBase - y);
		a = a < 1 ? 1 : a;
		Sint32 radix = a < roomRadix ? a : roomRadix;
		proto.h = (rand.getSint32() % radix) + roomBase;
		proto.y = y;
	}

	// set x/width
	if (x < 0) {
		proto.w = (rand.getSint32() % roomRadix) + roomBase;
		proto.x = rand.getSint32() % (subCols - proto.w);
	} else {
		Sint32 a = (subCols - roomBase - x);
		a = a < 1 ? 1 : a;
		Sint32 radix = a < roomRadix ? a : roomRadix;
		proto.w = (rand.getSint32() % radix) + roomBase;
		proto.x = x;
	}

	return proto;
}

bool Generator::soundRoom(Sint32 r1, Sint32 r2, Sint32 c1, Sint32 c2) {
	for (Sint32 r = r1; r <= r2; ++r) {
		for (Sint32 c = c1; c <= c2; ++c) {
			Uint32 index = r + c * options.dungeonHeight;
			if (tiles[index] & (BLOCKED | ROOM)) {
				return true;
			}
		}
	}
	return false;
}

void Generator::openRooms() {
	for (Uint32 c = 0; c < rooms.getSize(); ++c) {
		openRoom(rooms[c]);
	}
}

void Generator::openRoom(room_t& room) {
	ArrayList<sill_t> list = doorSills(room);
	if (list.empty())
		return;

	Sint32 numOpens = allocOpens(room);

	for (Sint32 i = 0; i < numOpens; ++i) {
		if (list.getSize() == 0) {
			break;
		}
		sill_t sill = list.remove(rand.getUint16() % list.getSize());
		Uint32& doorTile = tiles[sill.door_r + sill.door_c * options.dungeonHeight];
		if (doorTile & DOOR_SPACE)
			continue;

		for (Sint32 x = 0; x < 3; ++x) {
			Sint32 r = sill.sill_r + sin[sill.dir] * x;
			Sint32 c = sill.sill_c + cos[sill.dir] * x;

			tiles[r + c * options.dungeonHeight] |= ENTRANCE;
		}
		Uint32 door_type = doorType();
		doorTile |= door_type;

		door_t door;
		door.row = sill.door_r;
		door.col = sill.door_c;
		door.key = door_type;
		door.outID = sill.outID;

		room.doors[sill.dir].push(door);
	}
}

Sint32 Generator::allocOpens(room_t& room) {
	Sint32 room_h = (room.south - room.north) / options.subdivisor + 1;
	Sint32 room_w = (room.east - room.west) / options.subdivisor + 1;
	Sint32 flumph = (Sint32)sqrtf(room_w * room_h);
	Sint32 numOpens = flumph + rand.getUint16() % flumph;

	return numOpens;
}

ArrayList<Generator::sill_t> Generator::doorSills(room_t& room) {
	ArrayList<Generator::sill_t> list;

	if (room.north >= 3) {
		for (Sint32 c = room.west; c <= room.east; c += 2) {
			sill_t sill = checkSill(room, room.north, c, NORTH);
			if (sill.valid) {
				list.push(sill);
			}
		}
	}
	if (room.south <= options.dungeonHeight - 3) {
		for (Sint32 c = room.west; c <= room.east; c += 2) {
			sill_t sill = checkSill(room, room.south, c, SOUTH);
			if (sill.valid) {
				list.push(sill);
			}
		}
	}
	if (room.west >= 3) {
		for (Sint32 r = room.north; r <= room.south; r += 2) {
			sill_t sill = checkSill(room, r, room.west, WEST);
			if (sill.valid) {
				list.push(sill);
			}
		}
	}
	if (room.east <= options.dungeonWidth - 3) {
		for (Sint32 r = room.north; r <= room.south; r += 2) {
			sill_t sill = checkSill(room, r, room.east, EAST);
			if (sill.valid) {
				list.push(sill);
			}
		}
	}

	return list;
}

Generator::sill_t Generator::checkSill(room_t& room, Sint32 sill_r, Sint32 sill_c, Sint32 dir) {
	Sint32 door_r = sill_r + sin[dir];
	Sint32 door_c = sill_c + cos[dir];

	Uint32& doorTile = tiles[door_r + door_c * options.dungeonHeight];
	if (!(doorTile & PERIMETER))
		return sill_t();
	if (doorTile & ENTRANCE)
		return sill_t();
	if (doorTile & BLOCK_DOOR)
		return sill_t();

	Sint32 out_r = door_r + sin[dir];
	Sint32 out_c = door_c + cos[dir];
	Uint32& outTile = tiles[out_r + out_c * options.dungeonHeight];
	if (outTile & BLOCKED)
		return sill_t();

	Sint32 outID = -1;
	if (outTile & ROOM) {
		outID = (outTile & ROOM_ID) >> 6;
		if (outID == room.id) {
			return sill_t();
		}
	}

	sill_t sill;
	sill.valid = true;
	sill.sill_r = sill_r;
	sill.sill_c = sill_c;
	sill.door_r = door_r;
	sill.door_c = door_c;
	sill.outID = outID;
	sill.dir = dir;
	return sill;
}

Uint32 Generator::doorType() {
	Uint8 i = rand.getUint8();

	if (i < 15) {
		return ARCH;
	} else if (i < 60) {
		return DOOR;
	} else if (i < 75) {
		return LOCKED;
	} else if (i < 90) {
		return TRAPPED;
	} else if (i < 100) {
		return SECRET;
	} else {
		return PORTCULLIS;
	}
};

void Generator::corridors() {
	for (Sint32 j = 1; j < subCols; ++j) {
		Sint32 c = j * options.subdivisor + 1;
		for (Sint32 i = 1; i < subRows; ++i) {
			Sint32 r = i * options.subdivisor + 1;

			if (tiles[r + c * options.dungeonHeight] & CORRIDOR)
				continue;
			tunnel(i, j);
		}
	}
}

void Generator::tunnel(Sint32 i, Sint32 j, Sint32 dir) {
	ArrayList<Sint32> dirs = tunnelDirs(dir);

	for (Sint32 dir : dirs) {
		if (openTunnel(i, j, dir)) {
			Sint32 next_i = i + sin[dir];
			Sint32 next_j = j + cos[dir];

			tunnel(next_i, next_j, dir);
		}
	}
}

ArrayList<Sint32> Generator::tunnelDirs(Sint32 lastDir) {
	int p = corridorLayout.get(options.corridorLayout.get());

	// shuffle the directions
	ArrayList<Sint32> deck;
	deck.push(TUNNELDIRS[0]);
	deck.push(TUNNELDIRS[1]);
	deck.push(TUNNELDIRS[2]);
	deck.push(TUNNELDIRS[3]);

	Sint32 draw;
	ArrayList<Sint32> dirs;
	draw = rand.getSint8() % 4; dirs.push(deck[draw]); deck.remove(draw);
	draw = rand.getSint8() % 3; dirs.push(deck[draw]); deck.remove(draw);
	draw = rand.getSint8() % 2; dirs.push(deck[draw]); deck.remove(draw);
	dirs.push(deck[0]); deck.remove(0);

	if (lastDir != -1) {
		if (rand.getSint32() % 100 < p) {
			dirs.insert(lastDir, 0);
		}
	}

	return dirs;
}

bool Generator::openTunnel(Sint32 i, Sint32 j, Sint32 dir) {
	Sint32 this_r = (i * options.subdivisor) + 1;
	Sint32 this_c = (j * options.subdivisor) + 1;
	Sint32 next_r = ((i + sin[dir]) * options.subdivisor) + 1;
	Sint32 next_c = ((j + cos[dir]) * options.subdivisor) + 1;
	Sint32 mid_r = (this_r + next_r) / 2;
	Sint32 mid_c = (this_c + next_c) / 2;

	if (soundTunnel(mid_r, mid_c, next_r, next_c)) {
		delveTunnel(this_r, this_c, next_r, next_c);
		return true;
	} else {
		return false;
	}
}

bool Generator::soundTunnel(Sint32 mid_r, Sint32 mid_c, Sint32 next_r, Sint32 next_c) {
	if (next_r < 0 || next_r >= options.dungeonHeight)
		return false;
	if (next_c < 0 || next_c >= options.dungeonWidth)
		return false;

	Sint32 r1 = mid_r < next_r ? mid_r : next_r;
	Sint32 r2 = mid_r > next_r ? mid_r : next_r;
	Sint32 c1 = mid_c < next_c ? mid_c : next_c;
	Sint32 c2 = mid_c > next_c ? mid_c : next_c;

	for (Sint32 r = r1; r <= r2; ++r) {
		for (Sint32 c = c1; c <= c2; ++c) {
			if (tiles[r + c * options.dungeonHeight] & BLOCK_CORRIDOR) {
				return false;
			}
		}
	}
	return true;
}

void Generator::delveTunnel(Sint32 this_r, Sint32 this_c, Sint32 next_r, Sint32 next_c) {
	Sint32 r1 = this_r < next_r ? this_r : next_r;
	Sint32 r2 = this_r > next_r ? this_r : next_r;
	Sint32 c1 = this_c < next_c ? this_c : next_c;
	Sint32 c2 = this_c > next_c ? this_c : next_c;

	for (Sint32 r = r1; r <= r2; ++r) {
		for (Sint32 c = c1; c <= c2; ++c) {
			Uint32 index = r + c * options.dungeonHeight;
			if (!(tiles[index] & BLOCK_CORRIDOR)) {
				tiles[index] &= ~ENTRANCE;
				tiles[index] |= CORRIDOR;
			}
		}
	}
}

void Generator::emplaceStairs() {
}

void Generator::cleanDungeon() {
	if (options.removeDeadends > 0) {
		removeDeadends(options.removeDeadends);
	}
	fixDoors();
	emptyBlocks();
}

void Generator::removeDeadends(Sint32 percentage) {
	bool all = percentage >= 100;

	for (Sint32 j = 0; j < subCols; ++j) {
		Sint32 c = j * options.subdivisor + 1;
		for (Sint32 i = 0; i < subRows; ++i) {
			Sint32 r = i * options.subdivisor + 1;
			Sint32 index = r + c * options.dungeonHeight;

			if (!(tiles[index] & OPEN_SPACE))
				continue;
			if (tiles[index] & STAIRS)
				continue;
			if (!all && (rand.getSint32() % 100 < percentage))
				continue;

			collapse(r, c);
		}
	}
}

void Generator::collapse(Sint32 r, Sint32 c) {
	Sint32 index = r + c * options.dungeonHeight;

	if (!(tiles[index] & OPEN_SPACE))
		return;

	for (Sint32 dir = 0; dir < 4; ++dir) {
		if (checkTunnel(r, c, dir)) {
			for (Sint32 i = 0; i < 1; ++i) {
				const Sint32(&p)[2] = closeends[dir].close[i];
				tiles[(r + p[0]) + (c + p[1]) * options.dungeonHeight] = NOTHING;
			}
			const Sint32(&p)[2] = closeends[dir].recurse;
			collapse(r + p[0], c + p[1]);
		}
	}
}

bool Generator::checkTunnel(Sint32 r, Sint32 c, Sint32 dir) {
	for (Sint32 i = 0; i < 5; ++i) {
		const Sint32(&p)[2] = closeends[dir].walled[i];
		Sint32 index = (r + p[0]) + (c + p[1]) * options.dungeonHeight;
		if ((Uint32)index < tiles.getSize() && tiles[index] & (OPEN_SPACE | DOOR_SPACE)) {
			return false;
		}
	}

	return true;
}

void Generator::fixDoors() {
	ArrayList<bool> fixed;
	fixed.resize(tiles.getSize());
	for (Uint32 c = 0; c < tiles.getSize(); ++c) {
		fixed[c] = false;
	}

	for (auto &room : rooms) {
		for (Sint32 dir = 0; dir < 4; ++dir) {
			auto &doors = room.doors[dir];
			for (Uint32 c = 0; c < doors.getSize(); ++c) {
				auto &door = doors[c];
				Sint32 index = door.row + door.col * options.dungeonHeight;
				Sint32 next = index;
				switch (dir) {
				case WEST:
					--next;
					break;
				case EAST:
					++next;
					break;
				case NORTH:
					next -= options.dungeonHeight;
					break;
				case SOUTH:
					next += options.dungeonHeight;
					break;
				default:
					break;
				}
				Uint32& doorTile = tiles[next];
				if (!(doorTile & (OPEN_SPACE))) {
					doorTile &= ~DOOR_SPACE;
					doors.remove(c);
					--c;
					continue;
				}

				if (!fixed[index]) {
					Sint32 outDir = OPPOSITE[dir];
					if (door.outID != -1) {
						rooms[door.outID].doors[outDir].push(door);
					}
					fixed[index] = 1;
				}
			}
		}
	}
}

void Generator::emptyBlocks() {
	Uint32 index;

	// uncomment this to make hallways 2x2
	/*
	index = 0;
	for (Sint32 c = 0; c < options.dungeonWidth; ++c) {
		for (Sint32 r = 0; r < options.dungeonHeight; ++r) {
			if (tiles[index] & CORRIDOR) {
				if (!(tiles[index] & CORRIDOR_EXTRA)) {
					if (r < options.dungeonHeight - 1) {
						if (!(tiles[index + 1] & CORRIDOR)) {
							tiles[index + 1] |= CORRIDOR | CORRIDOR_EXTRA;
						}
					}
					if (c < options.dungeonWidth - 1) {
						if (!(tiles[index + options.dungeonHeight] & CORRIDOR)) {
							tiles[index + options.dungeonHeight] |= CORRIDOR | CORRIDOR_EXTRA;
						}
					}
					if (r < options.dungeonHeight - 1 && c < options.dungeonWidth - 1) {
						if (!(tiles[index + options.dungeonHeight + 1] & CORRIDOR)) {
							tiles[index + options.dungeonHeight + 1] |= CORRIDOR | CORRIDOR_EXTRA;
						}
					}
				}
			}
			++index;
		}
	}*/

	index = 0;
	for (Sint32 c = 0; c < options.dungeonWidth; ++c) {
		for (Sint32 r = 0; r < options.dungeonHeight; ++r) {
			if (tiles[index] & CORRIDOR && tiles[index] & PERIMETER) {
				tiles[index] |= DOOR;
			}
			if (tiles[index] & BLOCKED) {
				tiles[index] = NOTHING;
			}
			++index;
		}
	}

	index = 0;
	for (Sint32 c = 0; c < options.dungeonWidth; ++c) {
		for (Sint32 r = 0; r < options.dungeonHeight; ++r) {
			if (tiles[index] & ROOM) {
				tiles[index] &= ~DOOR_SPACE;
				tiles[index] &= ~CORRIDOR;
			} else if (tiles[index] & DOOR_SPACE) {
				tiles[index] &= ~ROOM;
				tiles[index] &= ~CORRIDOR;
			} else if (tiles[index] & CORRIDOR) {
				tiles[index] &= ~ROOM;
				tiles[index] &= ~DOOR_SPACE;
			} else {
				tiles[index] = NOTHING;
			}
			++index;
		}
	}
}

void Generator::serialize(FileInterface * file) {
	Uint32 version = 2;
	file->property("Generator::version", version);
	file->property("name", name);
	file->property("options", options);
	if (version < 1) {
		//file->property("rooms", roomLibs);
		//file->property("tunnels", tunnelLibs);
	} else {
		file->property("levelkit", levelkit);
	}
}

void Generator::options_t::serialize(FileInterface * file) {
	Uint32 version = 1;
	file->property("Generator::options_t::version", version);
	file->property("seed", seed);
	file->property("width", dungeonWidth);
	file->property("height", dungeonHeight);
	file->property("layout", dungeonLayout);
	file->property("roomMin", roomMin);
	file->property("roomMax", roomMax);
	file->property("roomLayout", roomLayout);
	file->property("corridorLayout", corridorLayout);
	file->property("removeDeadends", removeDeadends);
	file->property("addStairs", addStairs);

	Sint32 hallwaySize = subdivisor + 1;
	file->property("hallwaySize", hallwaySize);
	subdivisor = hallwaySize + 1;

	if (version >= 1) {
		file->property("complex", complex);
	}
	file->property("power", power);
	file->property("gravity", gravity);
	file->property("lifeSupport", lifeSupport);
}

void Generator::piece_t::serialize(FileInterface * file) {
	file->property("path", path);
}