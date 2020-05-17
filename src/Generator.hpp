//! @file Generator.hpp

#pragma once

#include "Main.hpp"
#include "String.hpp"
#include "ArrayList.hpp"
#include "Random.hpp"
#include "Rect.hpp"

class TileWorld;
class FileInterface;

//! The Generator class is used to generate a random 2D maze. It can be used in a script for user purposes.
class Generator {
public:
	//! Dungeon layout
	struct dungeon_layout_t {
		bool fullLayout[3][3] = {
			{ 1, 1, 1 },
			{ 1, 1, 1 },
			{ 1, 1, 1 }
		};

		bool boxLayout[3][3] = {
			{ 1, 1, 1 },
			{ 1, 0, 1 },
			{ 1, 1, 1 }
		};

		bool crossLayout[3][3] = {
			{ 0, 1, 0 },
			{ 1, 1, 1 },
			{ 0, 1, 0 }
		};

		bool daggerLayout[3][3] = {
			{ 0, 0, 0 },
			{ 1, 1, 1 },
			{ 0, 1, 0 }
		};

		bool saltireLayout[3][3] = {
			{ 1, 0, 1 },
			{ 0, 1, 0 },
			{ 1, 0, 1 }
		};

		const bool(*get(const char *str) const)[3]{
			if (!str) {
				return fullLayout;
			}
			if (strcmp(str, "Full") == 0) {
				return fullLayout;
			}
			if (strcmp(str, "Box") == 0) {
				return boxLayout;
			}
			if (strcmp(str, "Cross") == 0) {
				return crossLayout;
			}
			if (strcmp(str, "Dagger") == 0) {
				return daggerLayout;
			}
			if (strcmp(str, "Saltire") == 0) {
				return saltireLayout;
			}
			return fullLayout;
		}
	};
	static const dungeon_layout_t dungeonLayout;

	//! Corridor layout
	struct corridor_layout_t {
		int labyrinth = 0;
		int bent = 50;
		int straight = 100;

		int get(const char* str) const {
			if (!str) {
				return labyrinth;
			}
			if (strcmp(str, "Labyrinth") == 0) {
				return labyrinth;
			}
			if (strcmp(str, "Bent") == 0) {
				return bent;
			}
			if (strcmp(str, "Straight") == 0) {
				return straight;
			}
			return labyrinth;
		}
	};
	static const corridor_layout_t corridorLayout;

	//! Map style
	struct map_style_t {
		String fill = "000000";
		String open = "FFFFFF";
		String grid = "CCCCCC";
	};
	static const map_style_t mapStyle;

	//! Doorway
	struct sill_t {
		bool valid = false;
		Sint32 sill_r, sill_c;
		Sint32 door_r, door_c;
		Sint32 outID;
		Sint32 dir;
	};

	//! Door
	struct door_t {
		Sint32 row, col;
		Sint32 outID;
		Uint32 key;
	};

	//! Generated room
	struct room_t {
		Sint32 id;
		Sint32 row, col;
		Sint32 north, south, east, west;
		Sint32 height, width;
		Sint32 area;

		ArrayList<door_t> doors[4];
	};

	//! Cleaning
	struct closeend_t {
		Sint32 walled[5][2];
		Sint32 close[1][2];
		Sint32 recurse[2];
	};
	static const closeend_t closeends[4];

	//! Room piece
	struct piece_t {
		String path;
		TileWorld* angles[4];

		void serialize(FileInterface * file);
	};

	//! Cell bits
	static const Uint32 NOTHING = 0;
	static const Uint32 BLOCKED = (1 << 0);
	static const Uint32 ROOM = (1 << 1);
	static const Uint32 CORRIDOR = (1 << 2);
	static const Uint32 PERIMETER = (1 << 3);
	static const Uint32 ENTRANCE = (1 << 4);
	static const Uint32 ROOM_ID = (1 << 5);
	static const Uint32 ARCH = (1 << 6);
	static const Uint32 DOOR = (1 << 7);
	static const Uint32 LOCKED = (1 << 8);
	static const Uint32 TRAPPED = (1 << 9);
	static const Uint32 SECRET = (1 << 10);
	static const Uint32 PORTCULLIS = (1 << 11);
	static const Uint32 STAIR_DOWN = (1 << 12);
	static const Uint32 STAIR_UP = (1 << 13);
	static const Uint32 LABEL = (1 << 14);
	static const Uint32 CORRIDOR_EXTRA = (1 << 15);

	static const Uint32 OPEN_SPACE = ROOM | CORRIDOR;
	static const Uint32 DOOR_SPACE = ARCH | DOOR | LOCKED | TRAPPED | SECRET | PORTCULLIS;
	static const Uint32 E_SPACE = ENTRANCE | DOOR_SPACE | LABEL;
	static const Uint32 STAIRS = STAIR_UP | STAIR_DOWN;
	static const Uint32 BLOCK_ROOM = BLOCKED | ROOM;
	static const Uint32 BLOCK_CORRIDOR = BLOCKED | PERIMETER | CORRIDOR;
	static const Uint32 BLOCK_DOOR = BLOCKED | DOOR_SPACE;

	//! directions
	static const Sint32 EAST = 0;
	static const Sint32 SOUTH = 1;
	static const Sint32 WEST = 2;
	static const Sint32 NORTH = 3;
	static const Sint32 OPPOSITE[4];
	static const Sint32 TUNNELDIRS[4];

	//! cos/sin
	static const Sint32 cos[4];
	static const Sint32 sin[4];

	//! Options
	struct options_t {
		Uint32 seed = 0;			//! random seed
		Sint32 dungeonWidth = 39;			//! must be an odd number
		Sint32 dungeonHeight = 39;			//! must be an odd number
		String dungeonLayout = "None";
		Sint32 roomMin = 3;
		Sint32 roomMax = 9;
		String roomLayout = "Scattered";	//! Packed or Scattered
		String corridorLayout = "Bent";
		Sint32 removeDeadends = 0;			//! percentage
		Sint32 addStairs = 2;			//! number of stairs
		Sint32 subdivisor = 2;
		bool complex = false;		//! whether to create "complex" (non-rectangle) rooms
		bool power = false;
		bool gravity = false;
		bool lifeSupport = false;

		//! save/load this object to a file
		//! @param file interface to serialize with
		void serialize(FileInterface * file);
	};

	Generator(bool _clientObj);
	Generator(bool _clientObj, const options_t& _options);
	~Generator();

	//! Generate the dungeon with the supplied options
	void createDungeon();

	//! Write the dungeon to a file
	void writeFile(const char* path);

	//! save/load this object to a file
	//! @param file interface to serialize with
	void serialize(FileInterface * file);

	const char*						getName() const { return name; }
	const options_t&				getOptions() const { return options; }
	const ArrayList<Uint32>&		getTiles() const { return tiles; }

	void				setName(const char* _name) { name = _name; }
	void				setOptions(const options_t& _options) { options = _options; }

private:
	String name;
	options_t options;
	Random rand;
	ArrayList<room_t> rooms;
	bool clientObj = false;

	//! create blank dungeon
	void initCells();
	void maskCells();
	void roundMask();

	//! place rooms
	void emplaceRooms();
	void packRooms();
	void scatterRooms();
	Sint32 allocRooms();
	void emplaceRoom(Sint32 x = -1, Sint32 y = -1);
	Rect<Sint32> setRoom(Sint32 x, Sint32 y);
	bool soundRoom(Sint32 r1, Sint32 r2, Sint32 c1, Sint32 c2);

	//! place exits
	void openRooms();
	void openRoom(room_t& room);
	Sint32 allocOpens(room_t& room);
	ArrayList<sill_t> doorSills(room_t& room);
	sill_t checkSill(room_t& room, Sint32 sill_r, Sint32 sill_c, Sint32 dir);
	Uint32 doorType();

	//! place corridors
	void corridors();
	void tunnel(Sint32 i, Sint32 j, Sint32 dir = -1);
	ArrayList<Sint32> tunnelDirs(Sint32 lastDir);
	bool openTunnel(Sint32 i, Sint32 j, Sint32 dir);
	bool soundTunnel(Sint32 mid_r, Sint32 mid_c, Sint32 next_r, Sint32 next_c);
	void delveTunnel(Sint32 this_r, Sint32 this_c, Sint32 next_r, Sint32 next_c);

	//! place stairs
	void emplaceStairs();
	//void stairEnds();

	//! finalize
	void cleanDungeon();
	void removeDeadends(Sint32 percentage);
	void collapse(Sint32 r, Sint32 c);
	bool checkTunnel(Sint32 r, Sint32 c, Sint32 dir);
	void fixDoors();
	void emptyBlocks();

	Sint32 subRows, subCols;
	Sint32 maxRows, maxCols;
	Sint32 roomBase, roomRadix;
	Sint32 numRooms = 0;

	//! raw dungeon data
	ArrayList<Uint32> tiles;

	//! level kit
	String levelkit;
};