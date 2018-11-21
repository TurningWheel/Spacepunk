// TileWorld.cpp

#include "Main.hpp"

#ifdef PLATFORM_LINUX
#include <btBulletDynamicsCommon.h>
#else
#include <bullet3/btBulletDynamicsCommon.h>
#endif

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "LinkedList.hpp"
#include "Node.hpp"
#include "TileWorld.hpp"
#include "Tile.hpp"
#include "Chunk.hpp"
#include "Entity.hpp"
#include "Renderer.hpp"
#include "Client.hpp"
#include "Engine.hpp"
#include "ShaderProgram.hpp"
#include "Script.hpp"
#include "Mixer.hpp"
#include "Editor.hpp"
#include "Directory.hpp"
#include "Random.hpp"
#include "Server.hpp"
#include "Dictionary.hpp"
#include "Generator.hpp"

//Component headers.
#include "Component.hpp"
#include "BBox.hpp"
#include "Model.hpp"
#include "Light.hpp"
#include "Camera.hpp"
#include "Speaker.hpp"
#include "Character.hpp"

// there are two strings so we can convert different formats if need be.
const char* TileWorld::fileMagicNumber = "SPACEPUNK_WLD_12";
const char* TileWorld::fileMagicNumberSaveOut = "SPACEPUNK_WLD_12";

int generateThread(void* data) {
	TileWorld* src = (TileWorld*)data;

	return 0;
}

// create a world using a generator
TileWorld::TileWorld(bool _clientObj, Uint32 _id, const char* _zone, const Generator& gen)
	: pathFinder(*(new PathFinder(*this)))
{
	clientObj = _clientObj;
	id = _id;

	const Generator::options_t& options = gen.getOptions();

	// build empty level space
	generated = true;
	seed = options.seed;
	zone = _zone;
	nameStr = gen.getName();
	width = options.dungeonWidth * 3;
	height = options.dungeonHeight * 3;
	tiles.resize(width * height);
	int w = calcChunksWidth();
	int h = calcChunksHeight();
	chunks = new Chunk[w*h];

	Random rand;
	rand.seedValue(seed);

	// setup generator stuff
	const ArrayList<Uint32>& tiles = gen.getTiles();
	ArrayList<Uint8> placed;
	placed.resize(tiles.getSize());
	for (size_t c = 0; c < placed.getSize(); ++c) {
		placed[c] = 0;
	}

	static int maintHatchChance = 10;

	// place main pieces (corridors, rooms, doors)
	for (Sint32 x = 0; x < options.dungeonWidth; ++x) {
		for (Sint32 y = 0; y < options.dungeonHeight; ++y) {
			if (placed[y + x * options.dungeonHeight]) {
				continue;
			}

			const Uint32& tile = tiles[y + x * options.dungeonHeight];
			const Uint32* e = x < options.dungeonWidth - 1 ?
				&tiles[y + (x + 1) * options.dungeonHeight] : nullptr;
			const Uint32* se = x < options.dungeonWidth - 1 && y < options.dungeonHeight - 1 ?
				&tiles[(y + 1) + (x + 1) * options.dungeonHeight] : nullptr;
			const Uint32* s = y < options.dungeonHeight - 1 ?
				&tiles[(y + 1) + x * options.dungeonHeight] : nullptr;
			const Uint32* sw = x > 0 && y < options.dungeonHeight - 1 ?
				&tiles[(y + 1) + (x - 1) * options.dungeonHeight] : nullptr;
			const Uint32* w = x > 0 ?
				&tiles[y + (x - 1) * options.dungeonHeight] : nullptr;
			const Uint32* nw = x > 0 && y > 0 ?
				&tiles[(y - 1) + (x - 1) * options.dungeonHeight] : nullptr;
			const Uint32* n = y > 0 ?
				&tiles[(y - 1) + x * options.dungeonHeight] : nullptr;
			const Uint32* ne = x < options.dungeonWidth - 1 && y > 0 ?
				&tiles[(y - 1) + (x + 1) * options.dungeonHeight] : nullptr;

			Uint32 WALKABLE = Generator::ROOM | tile & Generator::CORRIDOR;
			Sint32 room = (tile >> 6) % gen.getRoomPieces().getSize();

			if (tile & WALKABLE) {
				if ((e && *e) && (s && *s) &&
					(w && *w) && (n && *n)) {
					if (!(nw && *nw)) {
						Sint32 variation = rand.getSint8() % gen.getRoomPieces()[room].cornersInner.getSize();
						const TileWorld& piece = *gen.getRoomPieces()[room].cornersInner[variation].angles[0];
						placeRoom(piece, 0, x * 3, y * 3, 0);
					} else if (!(ne && *ne)) {
						Sint32 variation = rand.getSint8() % gen.getRoomPieces()[room].cornersInner.getSize();
						const TileWorld& piece = *gen.getRoomPieces()[room].cornersInner[variation].angles[1];
						placeRoom(piece, 0, x * 3, y * 3, 0);
					} else if (!(se && *se)) {
						Sint32 variation = rand.getSint8() % gen.getRoomPieces()[room].cornersInner.getSize();
						const TileWorld& piece = *gen.getRoomPieces()[room].cornersInner[variation].angles[2];
						placeRoom(piece, 0, x * 3, y * 3, 0);
					} else if (!(sw && *sw)) {
						Sint32 variation = rand.getSint8() % gen.getRoomPieces()[room].cornersInner.getSize();
						const TileWorld& piece = *gen.getRoomPieces()[room].cornersInner[variation].angles[3];
						placeRoom(piece, 0, x * 3, y * 3, 0);
					} else {
						Sint32 variation = rand.getSint8() % gen.getRoomPieces()[room].mids.getSize();
						const TileWorld& piece = *gen.getRoomPieces()[room].mids[variation].angles[0];
						placeRoom(piece, 0, x * 3, y * 3, 0);
					}
				} else {
					if (!(e && *e)) {
						if (!(s && *s)) {
							Sint32 variation = rand.getSint8() % gen.getRoomPieces()[room].corners.getSize();
							const TileWorld& piece = *gen.getRoomPieces()[room].corners[variation].angles[2];
							placeRoom(piece, 0, x * 3, y * 3, 0);
						} else if (!(n && *n)) {
							Sint32 variation = rand.getSint8() % gen.getRoomPieces()[room].corners.getSize();
							const TileWorld& piece = *gen.getRoomPieces()[room].corners[variation].angles[1];
							placeRoom(piece, 0, x * 3, y * 3, 0);
						} else {
							Sint32 variation = rand.getSint8() % gen.getRoomPieces()[room].walls.getSize();
							if (rand.getSint8() % maintHatchChance == 0) {
								const TileWorld& piece = *gen.getRoomPieces()[room].tunnelDoors[variation].angles[2];
								placeRoom(piece, 0, x * 3, y * 3, 0);
								placed[y + x * options.dungeonHeight] |= 2;
							} else {
								const TileWorld& piece = *gen.getRoomPieces()[room].walls[variation].angles[2];
								placeRoom(piece, 0, x * 3, y * 3, 0);
							}
						}
					} else if (!(w && *w)) {
						if (!(s && *s)) {
							Sint32 variation = rand.getSint8() % gen.getRoomPieces()[room].corners.getSize();
							const TileWorld& piece = *gen.getRoomPieces()[room].corners[variation].angles[3];
							placeRoom(piece, 0, x * 3, y * 3, 0);
						} else if (!(n && *n)) {
							Sint32 variation = rand.getSint8() % gen.getRoomPieces()[room].corners.getSize();
							const TileWorld& piece = *gen.getRoomPieces()[room].corners[variation].angles[0];
							placeRoom(piece, 0, x * 3, y * 3, 0);
						} else {
							Sint32 variation = rand.getSint8() % gen.getRoomPieces()[room].walls.getSize();
							if (rand.getSint8() % maintHatchChance == 0) {
								const TileWorld& piece = *gen.getRoomPieces()[room].tunnelDoors[variation].angles[0];
								placeRoom(piece, 0, x * 3, y * 3, 0);
								placed[y + x * options.dungeonHeight] |= 2;
							} else {
								const TileWorld& piece = *gen.getRoomPieces()[room].walls[variation].angles[0];
								placeRoom(piece, 0, x * 3, y * 3, 0);
							}
						}
					} else if (!(s && *s)) {
						Sint32 variation = rand.getSint8() % gen.getRoomPieces()[room].walls.getSize();
						if (rand.getSint8() % maintHatchChance == 0) {
							const TileWorld& piece = *gen.getRoomPieces()[room].tunnelDoors[variation].angles[3];
							placeRoom(piece, 0, x * 3, y * 3, 0);
							placed[y + x * options.dungeonHeight] |= 2;
						} else {
							const TileWorld& piece = *gen.getRoomPieces()[room].walls[variation].angles[3];
							placeRoom(piece, 0, x * 3, y * 3, 0);
						}
					} else if (!(n && *n)) {
						Sint32 variation = rand.getSint8() % gen.getRoomPieces()[room].walls.getSize();
						if (rand.getSint8() % maintHatchChance == 0) {
							const TileWorld& piece = *gen.getRoomPieces()[room].tunnelDoors[variation].angles[1];
							placeRoom(piece, 0, x * 3, y * 3, 0);
							placed[y + x * options.dungeonHeight] |= 2;
						} else {
							const TileWorld& piece = *gen.getRoomPieces()[room].walls[variation].angles[1];
							placeRoom(piece, 0, x * 3, y * 3, 0);
						}
					}
				}

				placed[y + x * options.dungeonHeight] |= 1;
			} else if (tile & Generator::DOOR_SPACE) {
				if ((s && *s & Generator::DOOR_SPACE)) {
					Sint32 door = rand.getSint8() % gen.getRoomPieces()[room].doors.getSize();
					const TileWorld& doorPiece = *gen.getRoomPieces()[room].doors[door].angles[0];
					placeRoom(doorPiece, 0, x * 3, y * 3, 0);

					placed[y + x * options.dungeonHeight] |= 1;
					placed[(y + 1) + x * options.dungeonHeight] |= 1;
				} else if ((e && *e & Generator::DOOR_SPACE)) {
					Sint32 room = rand.getSint8() % gen.getRoomPieces().getSize();
					Sint32 door = rand.getSint8() % gen.getRoomPieces()[room].doors.getSize();
					const TileWorld& doorPiece = *gen.getRoomPieces()[room].doors[door].angles[1];
					placeRoom(doorPiece, 0, x * 3, y * 3, 0);

					placed[y + x * options.dungeonHeight] |= 1;
					placed[y + (x + 1) * options.dungeonHeight] |= 1;
				}
			}
		}
	}

	// TODO: different per section
	static int tunnelType = 0;

	// maintenance tunnels
	for (Sint32 x = 0; x < options.dungeonWidth; ++x) {
		for (Sint32 y = 0; y < options.dungeonHeight; ++y) {
			if (placed[y + x * options.dungeonHeight]) {
				continue;
			}

			bool e = false;
			if (x < options.dungeonWidth - 1) {
				Uint32 index = y + (x+1) * options.dungeonHeight;
				if (!placed[index] || placed[index] & 2) {
					e = true;
				}
			}

			bool s = false;
			if (y < options.dungeonHeight - 1) {
				Uint32 index = (y+1) + x * options.dungeonHeight;
				if (!placed[index] || placed[index] & 2) {
					s = true;
				}
			}

			bool w = false;
			if (x > 0) {
				Uint32 index = y + (x-1) * options.dungeonHeight;
				if (!placed[index] || placed[index] & 2) {
					w = true;
				}
			}

			bool n = false;
			if (y > 0) {
				Uint32 index = (y-1) + x * options.dungeonHeight;
				if (!placed[index] || placed[index] & 2) {
					n = true;
				}
			}

			if (e) {
				if (s) {
					if (w) {
						if (n) {
							Sint32 variation = rand.getSint8() % gen.getTunnelPieces()[tunnelType].intersections.getSize();
							const TileWorld& piece = *gen.getTunnelPieces()[tunnelType].intersections[variation].angles[0];
							placeRoom(piece, 0, x * 3, y * 3, 0);
						} else {
							Sint32 variation = rand.getSint8() % gen.getTunnelPieces()[tunnelType].tJunctions.getSize();
							const TileWorld& piece = *gen.getTunnelPieces()[tunnelType].tJunctions[variation].angles[0];
							placeRoom(piece, 0, x * 3, y * 3, 0);
						}
					} else {
						if (n) {
							Sint32 variation = rand.getSint8() % gen.getTunnelPieces()[tunnelType].tJunctions.getSize();
							const TileWorld& piece = *gen.getTunnelPieces()[tunnelType].tJunctions[variation].angles[3];
							placeRoom(piece, 0, x * 3, y * 3, 0);
						} else {
							Sint32 variation = rand.getSint8() % gen.getTunnelPieces()[tunnelType].corners.getSize();
							const TileWorld& piece = *gen.getTunnelPieces()[tunnelType].corners[variation].angles[0];
							placeRoom(piece, 0, x * 3, y * 3, 0);
						}
					}
				} else {
					if (w) {
						if (n) {
							Sint32 variation = rand.getSint8() % gen.getTunnelPieces()[tunnelType].tJunctions.getSize();
							const TileWorld& piece = *gen.getTunnelPieces()[tunnelType].tJunctions[variation].angles[2];
							placeRoom(piece, 0, x * 3, y * 3, 0);
						} else {
							Sint32 variation = rand.getSint8() % gen.getTunnelPieces()[tunnelType].straights.getSize();
							const TileWorld& piece = *gen.getTunnelPieces()[tunnelType].straights[variation].angles[0];
							placeRoom(piece, 0, x * 3, y * 3, 0);
						}
					} else {
						if (n) {
							Sint32 variation = rand.getSint8() % gen.getTunnelPieces()[tunnelType].corners.getSize();
							const TileWorld& piece = *gen.getTunnelPieces()[tunnelType].corners[variation].angles[3];
							placeRoom(piece, 0, x * 3, y * 3, 0);
						} else {
							Sint32 variation = rand.getSint8() % gen.getTunnelPieces()[tunnelType].deadEnds.getSize();
							const TileWorld& piece = *gen.getTunnelPieces()[tunnelType].deadEnds[variation].angles[2];
							placeRoom(piece, 0, x * 3, y * 3, 0);
						}
					}
				}
			} else {
				if (s) {
					if (w) {
						if (n) {
							Sint32 variation = rand.getSint8() % gen.getTunnelPieces()[tunnelType].tJunctions.getSize();
							const TileWorld& piece = *gen.getTunnelPieces()[tunnelType].tJunctions[variation].angles[1];
							placeRoom(piece, 0, x * 3, y * 3, 0);
						} else {
							Sint32 variation = rand.getSint8() % gen.getTunnelPieces()[tunnelType].corners.getSize();
							const TileWorld& piece = *gen.getTunnelPieces()[tunnelType].corners[variation].angles[1];
							placeRoom(piece, 0, x * 3, y * 3, 0);
						}
					} else {
						if (n) {
							Sint32 variation = rand.getSint8() % gen.getTunnelPieces()[tunnelType].straights.getSize();
							const TileWorld& piece = *gen.getTunnelPieces()[tunnelType].straights[variation].angles[1];
							placeRoom(piece, 0, x * 3, y * 3, 0);
						} else {
							Sint32 variation = rand.getSint8() % gen.getTunnelPieces()[tunnelType].deadEnds.getSize();
							const TileWorld& piece = *gen.getTunnelPieces()[tunnelType].deadEnds[variation].angles[3];
							placeRoom(piece, 0, x * 3, y * 3, 0);
						}
					}
				} else {
					if (w) {
						if (n) {
							Sint32 variation = rand.getSint8() % gen.getTunnelPieces()[tunnelType].corners.getSize();
							const TileWorld& piece = *gen.getTunnelPieces()[tunnelType].corners[variation].angles[2];
							placeRoom(piece, 0, x * 3, y * 3, 0);
						} else {
							Sint32 variation = rand.getSint8() % gen.getTunnelPieces()[tunnelType].deadEnds.getSize();
							const TileWorld& piece = *gen.getTunnelPieces()[tunnelType].deadEnds[variation].angles[0];
							placeRoom(piece, 0, x * 3, y * 3, 0);
						}
					} else {
						if (n) {
							Sint32 variation = rand.getSint8() % gen.getTunnelPieces()[tunnelType].deadEnds.getSize();
							const TileWorld& piece = *gen.getTunnelPieces()[tunnelType].deadEnds[variation].angles[1];
							placeRoom(piece, 0, x * 3, y * 3, 0);
						} else {
							// totally inaccessible tunnel. No piece!
							continue;
						}
					}
				}
			}
		}
	}

	// done
	loaded = true;
}

// generate a new world
TileWorld::TileWorld(bool _clientObj, Uint32 _id, const char* _zone, Uint32 _seed, Uint32 _width, Uint32 _height, const char* _nameStr)
	: pathFinder(*(new PathFinder(*this)))
{
	clientObj = _clientObj;
	id = _id;

	mainEngine->fmsg(Engine::MSG_INFO, "generating new world from '%s' and seed: %d", _zone, _seed);

	// build empty level space
	generated = true;
	seed = _seed;
	zone = _zone;
	nameStr = _nameStr;
	width = _width;
	height = _height;
	tiles.resize(width * height);
	int w = calcChunksWidth();
	int h = calcChunksHeight();
	chunks = new Chunk[w*h];

	// seed random generator
	Random rand;
	rand.seedValue(seed);

	// load all rooms in this zone
	LinkedList<TileWorld*> rooms;
	StringBuf<128> path("maps/%s",_zone);
	path = mainEngine->buildPath(path.get()).get();
	Directory dir(path.get());
	StringBuf<16> startLvl("Start");
	for( auto& str : dir.getList() ) {
		// don't include the start room
		if( str == startLvl.get() ) {
			continue;
		}

		for( int sideInt=Tile::SIDE_EAST; sideInt<Tile::SIDE_TYPE_LENGTH; ++sideInt ) {
			Tile::side_t side = static_cast<Tile::side_t>(sideInt);

			StringBuf<128> path("maps/%s/%s", _zone, str.get());
			path = mainEngine->buildPath(path.get()).get();
			TileWorld* world = new TileWorld(true, _clientObj, UINT32_MAX, side, path.get());

			// discard rooms that failed to load or are too big
			if( !world->isLoaded() ) {
				delete world;
			} else if( world->getWidth() > width || world->getHeight() > height ) {
				delete world;
			} else {
				rooms.addNodeLast(world);
			}
		}
	}
	if( rooms.getSize() == 0 ) {
		mainEngine->fmsg(Engine::MSG_ERROR, "Can't generate level, no rooms available!");
		return;
	}

	// load starting room
	StringBuf<128> sRoomPath("maps/%s/Start", _zone);
	sRoomPath = mainEngine->buildPath(sRoomPath.get()).get();
	TileWorld* sRoom = new TileWorld(true, _clientObj, UINT32_MAX, Tile::SIDE_EAST, sRoomPath.get());

	if( !sRoom->isLoaded() ) {
		mainEngine->fmsg(Engine::MSG_ERROR, "Can't generate level, start room too big!");
		return;
	} else if( sRoom->getWidth() > width || sRoom->getHeight() > height ) {
		mainEngine->fmsg(Engine::MSG_ERROR, "Can't generate level, start room too big!");
		return;
	}

	// place starting room
	Uint32 sRoomX = (width - sRoom->getWidth()) / 2;
	Uint32 sRoomY = (height - sRoom->getHeight()) / 2;
	placeRoom(*sRoom, UINT32_MAX, sRoomX, sRoomY);
	delete sRoom;

	Uint32 roomsPlaced = 1;
	Uint32 exitsPlaced = 0;

	// start adjoining rooms
	for( const Node<exit_t>* nodeExit = exits.getFirst(); nodeExit != nullptr; nodeExit = nodeExit->getNext() ) {
		const exit_t& exit = nodeExit->getData();

		// build a list of candidate rooms to fit the exit
		LinkedList<candidate_t> candidates;
		for( Node<TileWorld*>* nodeRoom = rooms.getFirst(); nodeRoom != nullptr; nodeRoom = nodeRoom->getNext() ) {
			TileWorld* world = nodeRoom->getData();

			candidate_t room;
			room.world = world;

			// find matching exits for the given room...
			for( const Node<exit_t>* node = world->getExits().getFirst(); node != nullptr; node = node->getNext() ) {
				const exit_t& candExit = node->getData();

				bool locked = false;
				Sint32 startX = 0;
				Sint32 startY = 0;
				Sint32 endX = 0;
				Sint32 endY = 0;

				switch( exit.side ) {
					case Tile::SIDE_EAST:
						if( candExit.side == Tile::SIDE_WEST && exit.size.h == candExit.size.h ) {
							startX = exit.size.x + 1;
							startY = exit.size.y - candExit.size.y;
							endX = startX + world->getWidth();
							endY = startY + world->getHeight();
						} else {
							locked = true;
						}
						break;
					case Tile::SIDE_SOUTH:
						if( candExit.side == Tile::SIDE_NORTH && exit.size.w == candExit.size.w ) {
							startX = exit.size.x - candExit.size.x;
							startY = exit.size.y + 1;
							endX = startX + world->getWidth();
							endY = startY + world->getHeight();
						} else {
							locked = true;
						}
						break;
					case Tile::SIDE_WEST:
						if( candExit.side == Tile::SIDE_EAST && exit.size.h == candExit.size.h ) {
							startX = exit.size.x - world->getWidth();
							startY = exit.size.y - candExit.size.y;
							endX = startX + world->getWidth();
							endY = startY + world->getHeight();
						} else {
							locked = true;
						}
						break;
					case Tile::SIDE_NORTH:
						if( candExit.side == Tile::SIDE_SOUTH && exit.size.w == candExit.size.w ) {
							startX = exit.size.x - candExit.size.x;
							startY = exit.size.y - world->getHeight();
							endX = startX + world->getWidth();
							endY = startY + world->getHeight();
						} else {
							locked = true;
						}
						break;
					default:
						assert(0); // shouldn't happen
						break;
				}

				if( locked ) {
					continue;
				}

				if( startX < 0 || startY < 0 || endX > (Sint32)width || endY > (Sint32)height ) {
					locked = true;
				} else {
					for( Sint32 x = startX; x < endX; ++x ) {
						for( Sint32 y = startY; y < endY; ++y ) {
							if( tiles[y + x * height].isLocked() ) {
								locked = true;
								break;
							}
						}
						if( locked ) {
							break;
						}
					}
				}

				if( !locked ) {
					room.exits.push(candExit);
				}
			}

			// we have matching exits, so add it to our list of candidates...
			if( room.exits.getSize() > 0 ) {
				candidates.addNodeLast(room);
			}
		}

		// no rooms will fit this exit!
		if( candidates.getSize() == 0 ) {
#if 0
			Uint32 startX = exit.size.x;
			Uint32 startY = exit.size.y;
			Uint32 endX = exit.size.x + exit.size.w;
			Uint32 endY = exit.size.y + exit.size.h;
			for( Uint32 u = startX; u < endX; ++u ) {
				for( Uint32 v = startY; v < endY; ++v ) {
					Tile& tile = tiles[v + u * height];
					tile.setFloorHeight(tile.getFloorHeight() + tile.getFloorSlopeSize());
					tile.setCeilingHeight(tile.getFloorHeight());
					tile.setFloorSlopeSize(0);
					tile.setCeilingSlopeSize(0);
				}
			}
#endif

			continue;
		}

		// choose a room and an exit randomly
		Uint32 pickedRoomIndex = rand.getUint32() % candidates.getSize();
		const candidate_t& pickedRoom = candidates[pickedRoomIndex]->getData();
		Uint32 pickedExitIndex = rand.getUint32() % pickedRoom.exits.getSize();
		const exit_t& pickedExit = pickedRoom.exits[pickedExitIndex];

		// determine placement location
		Sint32 roomX = 0;
		Sint32 roomY = 0;
		switch( exit.side ) {
			case Tile::SIDE_EAST:
				if( pickedExit.side == Tile::SIDE_WEST ) {
					roomX = exit.size.x + 1;
					roomY = exit.size.y - pickedExit.size.y;
				}
				break;
			case Tile::SIDE_SOUTH:
				if( pickedExit.side == Tile::SIDE_NORTH ) {
					roomX = exit.size.x - pickedExit.size.x;
					roomY = exit.size.y + 1;
				}
				break;
			case Tile::SIDE_WEST:
				if( pickedExit.side == Tile::SIDE_EAST ) {
					roomX = exit.size.x - pickedRoom.world->getWidth();
					roomY = exit.size.y - pickedExit.size.y;
				}
				break;
			case Tile::SIDE_NORTH:
				if( pickedExit.side == Tile::SIDE_SOUTH ) {
					roomX = exit.size.x - pickedExit.size.x;
					roomY = exit.size.y - pickedRoom.world->getHeight();
				}
				break;
			default:
				assert(0); // shouldn't happen
				break;
		}

		// finally, place the room
		Sint32 floorDiff = exit.floorHeight - pickedExit.floorHeight;
		placeRoom(*pickedRoom.world, pickedExit.id, roomX, roomY, floorDiff);
		++roomsPlaced;

		// place door entity
#if 0
		const Entity::def_t* def = nullptr;
		if( exit.side == Tile::SIDE_EAST || exit.side == Tile::SIDE_WEST ) {
			if( exit.size.h == 6 ) {
				def = Entity::findDef("Door (Small) (E/W)");
			}
		} else if( exit.side == Tile::SIDE_NORTH || exit.side == Tile::SIDE_SOUTH ) {
			if( exit.size.w == 6 ) {
				def = Entity::findDef("Door (Small) (N/S)");
			}
		}
		if( def ) {
			Vector pos;
			pos.x = ((float)exit.size.x + (float)exit.size.w / 2.f) * Tile::size;
			pos.y = ((float)exit.size.y + (float)exit.size.h / 2.f) * Tile::size;
			pos.z = (float)exit.floorHeight;
			Entity::spawnFromDef(this, *def, pos, Angle());
		}
#endif

		// scrap candidate list
		candidates.removeAll();
		++exitsPlaced;
	}

	// finalize
	while( rooms.getFirst() ) {
		delete rooms.getFirst()->getData();
		rooms.removeNode(rooms.getFirst());
	}
	loaded = true;

	mainEngine->fmsg(Engine::MSG_INFO, "placed %d rooms", roomsPlaced);
}

// load a world file or create a blank one
TileWorld::TileWorld(bool _silent, bool _clientObj, Uint32 _id, Tile::side_t orientation, const char* _filename, Uint32 _width, Uint32 _height, const char* _nameStr)
	: pathFinder(*(new PathFinder(*this)))
{
	silent = _silent;
	clientObj = _clientObj;
	id = _id;
	if (_filename && _filename[0] != '\0') {
		changeFilename(_filename);
	}

	if (!silent) {
		if (shortname.empty()) {
			mainEngine->fmsg(Engine::MSG_INFO, "creating new world");
		}
		else {
			mainEngine->fmsg(Engine::MSG_INFO, "opening world file '%s'", shortname.get());
		}
	}

	// get texture dictionary
	Dictionary& textureStrings = mainEngine->getTextureDictionary();
	if( filetype != FILE_WLD && !filename.empty() ) {
		FileHelper::readObject(filename, *this);

		int w = calcChunksWidth();
		int h = calcChunksHeight();
		chunks = new Chunk[w*h];
	} else {
		// open map from file
		FILE* fp = NULL;
		if (filename.empty() || (fp = fopen(filename.get(), "rb")) == NULL || _width <= 0 || _height <= 0) {
			if (!filename.empty()) {
				mainEngine->fmsg(Engine::MSG_WARN, "failed to open world file '%s': %s", _filename, strerror(errno));
			}
			if (_width <= 0 || _height <= 0) {
				mainEngine->fmsg(Engine::MSG_WARN, "provided dimensions for map are no good!");
			}

			// default empty map
			nameStr = _nameStr;
			width = _width > 0 ? _width : 4;
			height = _height > 0 ? _height : 4;
			tiles.resize(width * height);
			int w = calcChunksWidth();
			int h = calcChunksHeight();
			chunks = new Chunk[w*h];

			if (fp != NULL) {
				fclose(fp);
			}
		}
		else {
			// read "magic number"
			char magicNumber[fileMagicNumberLen + 1] = { 0 };
			Engine::freadl(magicNumber, sizeof(char), fileMagicNumberLen, fp, shortname.get(), "TileWorld::TileWorld()");

			if (strncmp(magicNumber, fileMagicNumber, fileMagicNumberLen)) {
				// incorrect magic number
				mainEngine->fmsg(Engine::MSG_WARN, "failed to load world file '%s': incorrect version number", shortname.get());

				// default empty map
				nameStr = _nameStr;
				width = _width;
				height = _height;
				tiles.resize(width * height);
				int w = calcChunksWidth();
				int h = calcChunksHeight();
				chunks = new Chunk[w*h];
			}
			else {
				Uint32 reserved = 0;

				// read name string
				Uint32 len = 0;
				Engine::freadl(&len, sizeof(Uint32), 1, fp, shortname.get(), "TileWorld::TileWorld()");
				if (len > 0 && len < 128) {
					char* nameStrChar = (char *)calloc(len + 1, sizeof(char));
					Engine::freadl(nameStrChar, sizeof(char), len, fp, shortname.get(), "TileWorld::TileWorld()");
					nameStr = nameStrChar;
					free(nameStrChar);
				}
				else if (len >= 128) {
					assert(0);
				}

				// read number of entities
				Uint32 numEntities = 0;
				Engine::freadl(&numEntities, sizeof(Uint32), 1, fp, shortname.get(), "TileWorld::TileWorld()");

				// read world width/height
				Engine::freadl(&width, sizeof(Uint32), 1, fp, shortname.get(), "TileWorld::TileWorld()");
				Engine::freadl(&height, sizeof(Uint32), 1, fp, shortname.get(), "TileWorld::TileWorld()");
				tiles.resize(width * height);
				int w = calcChunksWidth();
				int h = calcChunksHeight();
				chunks = new Chunk[w*h];

				// reserved 4 bytes
				Engine::freadl(&reserved, sizeof(Uint32), 1, fp, shortname.get(), "TileWorld::TileWorld()");

				// read entity data
				for (Uint32 c = 0; c < numEntities; ++c) {
					Uint32 len = 0;

					Engine::freadl(&len, sizeof(Uint32), 1, fp, shortname.get(), "TileWorld::TileWorld()");
					char* nameStr = nullptr;
					if (len > 0 && len < 128) {
						nameStr = (char *)calloc(len + 1, sizeof(char));
						Engine::freadl(nameStr, sizeof(char), len, fp, shortname.get(), "TileWorld::TileWorld()");
					}
					else if (len >= 128) {
						assert(0);
					}

					Vector pos;
					Engine::freadl(&pos, sizeof(Vector), 1, fp, shortname.get(), "TileWorld::TileWorld()");

					Angle ang;
					Engine::freadl(&ang, sizeof(Angle), 1, fp, shortname.get(), "TileWorld::TileWorld()");

					Vector scale;
					Engine::freadl(&scale, sizeof(Vector), 1, fp, shortname.get(), "TileWorld::TileWorld()");

					len = 0;
					Engine::freadl(&len, sizeof(Uint32), 1, fp, shortname.get(), "TileWorld::TileWorld()");
					char* scriptStr = nullptr;
					if (len > 0 && len < 128) {
						scriptStr = (char *)calloc(len + 1, sizeof(char));
						Engine::freadl(scriptStr, sizeof(char), len, fp, shortname.get(), "TileWorld::TileWorld()");
					}
					else if (len >= 128) {
						assert(0);
					}

					Uint32 flags = 0;
					Engine::freadl(&flags, sizeof(Uint32), 1, fp, shortname.get(), "TileWorld::TileWorld()");

					bool falling = false;
					Engine::freadl(&falling, sizeof(bool), 1, fp, shortname.get(), "TileWorld::TileWorld()");

					Entity::sort_t sort = Entity::sort_t::SORT_ANY;
					Engine::freadl(&sort, sizeof(Entity::sort_t), 1, fp, shortname.get(), "TileWorld::TileWorld()");

					// reserved 4 bytes
					Engine::freadl(&reserved, sizeof(Uint32), 1, fp, shortname.get(), "TileWorld::TileWorld()");

					// assign base attributes
					Entity* entity = new Entity(this);
					entity->setName(nameStr);
					entity->setPos(pos);
					entity->setNewPos(pos);
					entity->setAng(ang);
					entity->setNewAng(ang);
					entity->setScale(scale);
					entity->setScriptStr(scriptStr);
					entity->setFlags(flags);
					entity->setFalling(falling);
					entity->setSort(sort);

					free(nameStr);
					free(scriptStr);

					// load components
					Uint32 numComponents = 0;
					Engine::freadl(&numComponents, sizeof(Uint32), 1, fp, shortname.get(), "TileWorld::TileWorld()");
					for (Uint32 c = 0; c < numComponents; ++c) {
						Component::type_t type = Component::type_t::COMPONENT_BASIC;
						Engine::freadl(&type, sizeof(Component::type_t), 1, fp, shortname.get(), "TileWorld::TileWorld()");

						Component* component = entity->addComponent(type);
						if (!component) {
							mainEngine->fmsg(Engine::MSG_ERROR, "failed to load component for entity '%s'", entity->getName().get());
						}
						else {
							component->load(fp);
						}
					}
				}
				if (!silent) {
					mainEngine->fmsg(Engine::MSG_INFO, "read %d entities", numEntities);
				}

				// read tile geometry
				for (Uint32 x = 0; x < width; ++x) {
					for (Uint32 y = 0; y < height; ++y) {
						Tile& tile = tiles[y + x*height];

						Sint32 shouldRead = 0;
						Engine::freadl(&shouldRead, sizeof(Sint32), 1, fp, shortname.get(), "TileWorld::TileWorld()");

						if (shouldRead) {
							// ceiling and floor height
							Sint32 ceilingHeight = 0;
							Engine::freadl(&ceilingHeight, sizeof(Sint32), 1, fp, shortname.get(), "TileWorld::TileWorld()");
							tile.setCeilingHeight(ceilingHeight);

							Sint32 floorHeight = 0;
							Engine::freadl(&floorHeight, sizeof(Sint32), 1, fp, shortname.get(), "TileWorld::TileWorld()");
							tile.setFloorHeight(floorHeight);

							// slope data
							Tile::side_t ceilingSlopeSide;
							Engine::freadl(&ceilingSlopeSide, sizeof(Tile::side_t), 1, fp, shortname.get(), "TileWorld::TileWorld()");
							tile.setCeilingSlopeSide(ceilingSlopeSide);

							Sint32 ceilingSlopeSize = 0;
							Engine::freadl(&ceilingSlopeSize, sizeof(Sint32), 1, fp, shortname.get(), "TileWorld::TileWorld()");
							tile.setCeilingSlopeSize(ceilingSlopeSize);

							Tile::side_t floorSlopeSide;
							Engine::freadl(&floorSlopeSide, sizeof(Tile::side_t), 1, fp, shortname.get(), "TileWorld::TileWorld()");
							tile.setFloorSlopeSide(floorSlopeSide);

							Sint32 floorSlopeSize = 0;
							Engine::freadl(&floorSlopeSize, sizeof(Sint32), 1, fp, shortname.get(), "TileWorld::TileWorld()");
							tile.setFloorSlopeSize(floorSlopeSize);

							// reserved 4 bytes
							Engine::freadl(&reserved, sizeof(Uint32), 1, fp, shortname.get(), "TileWorld::TileWorld()");
						}
					}
				}

				// read tile textures
				for (Uint32 x = 0; x < width; ++x) {
					for (Uint32 y = 0; y < height; ++y) {
						Tile& tile = tiles[y + x*height];

						if (tile.hasVolume()) {
							// ceiling texture
							Uint32 len = 0;
							Engine::freadl(&len, sizeof(Uint32), 1, fp, shortname.get(), "TileWorld::TileWorld()");
							if (len > 0 && len < 128) {
								char* ceilingTexture = (char *)calloc(len + 1, sizeof(char));
								Engine::freadl(ceilingTexture, sizeof(char), len, fp, shortname.get(), "TileWorld::TileWorld()");

								if (ceilingTexture) {
									size_t index = textureStrings.find(ceilingTexture);
									if (index == Dictionary::nindex) {
										tile.setCeilingTexture((Uint32)textureStrings.getWords().getSize());
										textureStrings.insert(ceilingTexture);
									}
									else {
										tile.setCeilingTexture((Uint32)index);
									}
								}
								else {
									tile.setCeilingTexture(0);
								}

								free(ceilingTexture);

								Rect<GLdouble> ceilingTextureUV;
								Engine::freadl(&ceilingTextureUV, sizeof(Rect<GLdouble>), 1, fp, shortname.get(), "TileWorld::TileWorld()");

								// reserved 4 bytes
								Engine::freadl(&reserved, sizeof(Uint32), 1, fp, shortname.get(), "TileWorld::TileWorld()");
							}
							else if (len >= 128) {
								assert(0);
							}

							// floor texture
							len = 0;
							Engine::freadl(&len, sizeof(Uint32), 1, fp, shortname.get(), "TileWorld::TileWorld()");
							if (len > 0 && len < 128) {
								char* floorTexture = (char *)calloc(len + 1, sizeof(char));
								Engine::freadl(floorTexture, sizeof(char), len, fp, shortname.get(), "TileWorld::TileWorld()");

								if (floorTexture) {
									size_t index = textureStrings.find(floorTexture);
									if (index == Dictionary::nindex) {
										tile.setFloorTexture((Uint32)textureStrings.getWords().getSize());
										textureStrings.insert(floorTexture);
									}
									else {
										tile.setFloorTexture((Uint32)index);
									}
								}
								else {
									tile.setFloorTexture(0);
								}

								free(floorTexture);

								Rect<GLdouble> floorTextureUV;
								Engine::freadl(&floorTextureUV, sizeof(Rect<GLdouble>), 1, fp, shortname.get(), "TileWorld::TileWorld()");

								// reserved 4 bytes
								reserved = 0;
								Engine::freadl(&reserved, sizeof(Uint32), 1, fp, shortname.get(), "TileWorld::TileWorld()");
								if (reserved) {
									// new as of 2018-02-15
									// tile colors

									Tile::shadervars_t shaderVars;
									Engine::freadl((void*)shaderVars.customColorR.getArray(), sizeof(float), 3, fp, shortname.get(), "TileWorld::TileWorld()");
									Engine::freadl((void*)shaderVars.customColorG.getArray(), sizeof(float), 3, fp, shortname.get(), "TileWorld::TileWorld()");
									Engine::freadl((void*)shaderVars.customColorB.getArray(), sizeof(float), 3, fp, shortname.get(), "TileWorld::TileWorld()");
									tile.setShaderVars(shaderVars);

									// reserved 4 bytes
									reserved = 0;
									Engine::freadl(&reserved, sizeof(Uint32), 1, fp, shortname.get(), "TileWorld::TileWorld()");
								}
							}

							// texture data
							for (int sideInt = Tile::SIDE_EAST; sideInt < Tile::SIDE_TYPE_LENGTH; sideInt++) {
								Tile::side_t side = static_cast<Tile::side_t>(sideInt);

								// find neighbor
								Tile* neighbor = nullptr;
								switch (side) {
								case Tile::SIDE_EAST:
									if (x < width - 1) {
										neighbor = &tiles[y + (x + 1)*height];
									}
									break;
								case Tile::SIDE_SOUTH:
									if (y < height - 1) {
										neighbor = &tiles[(y + 1) + x*height];
									}
									break;
								case Tile::SIDE_WEST:
									if (x > 0) {
										neighbor = &tiles[y + (x - 1)*height];
									}
									break;
								case Tile::SIDE_NORTH:
									if (y > 0) {
										neighbor = &tiles[(y - 1) + x*height];
									}
									break;
								default:
									break;
								}
								if (neighbor == nullptr)
									continue;

								// upper wall
								if (tile.upperWallHeight(*neighbor, side, Tile::CORNER_LEFT) > 0 ||
									tile.upperWallHeight(*neighbor, side, Tile::CORNER_RIGHT) > 0) {

									Uint32 len = 0;
									Engine::freadl(&len, sizeof(Uint32), 1, fp, shortname.get(), "TileWorld::TileWorld()");
									if (len > 0 && len < 128) {
										char* upperTexture = (char *)calloc(len + 1, sizeof(char));
										Engine::freadl(upperTexture, sizeof(char), len, fp, shortname.get(), "TileWorld::TileWorld()");

										if (upperTexture) {
											size_t index = textureStrings.find(upperTexture);
											if (index == Dictionary::nindex) {
												tile.setUpperTexture(side, (Uint32)textureStrings.getWords().getSize());
												textureStrings.insert(upperTexture);
											}
											else {
												tile.setUpperTexture(side, (Uint32)index);
											}
										}
										else {
											tile.setUpperTexture(side, 0);
										}

										free(upperTexture);

										Rect<GLdouble> upperTextureUV;
										Engine::freadl(&upperTextureUV, sizeof(Rect<GLdouble>), 1, fp, shortname.get(), "TileWorld::TileWorld()");

										// reserved 4 bytes
										Engine::freadl(&reserved, sizeof(Uint32), 1, fp, shortname.get(), "TileWorld::TileWorld()");
									}
									else if (len >= 128) {
										assert(0);
									}
								}

								// lower wall
								if (tile.lowerWallHeight(*neighbor, side, Tile::CORNER_LEFT) > 0 ||
									tile.lowerWallHeight(*neighbor, side, Tile::CORNER_RIGHT) > 0) {

									Uint32 len = 0;
									Engine::freadl(&len, sizeof(Uint32), 1, fp, shortname.get(), "TileWorld::TileWorld()");
									if (len > 0 && len < 128) {
										char* lowerTexture = (char *)calloc(len + 1, sizeof(char));
										Engine::freadl(lowerTexture, sizeof(char), len, fp, shortname.get(), "TileWorld::TileWorld()");

										if (lowerTexture) {
											size_t index = textureStrings.find(lowerTexture);
											if (index == Dictionary::nindex) {
												tile.setLowerTexture(side, (Uint32)textureStrings.getWords().getSize());
												textureStrings.insert(lowerTexture);
											}
											else {
												tile.setLowerTexture(side, (Uint32)index);
											}
										}
										else {
											tile.setLowerTexture(side, 0);
										}

										free(lowerTexture);

										Rect<GLdouble> lowerTextureUV;
										Engine::freadl(&lowerTextureUV, sizeof(Rect<GLdouble>), 1, fp, shortname.get(), "TileWorld::TileWorld()");

										// reserved 4 bytes
										Engine::freadl(&reserved, sizeof(Uint32), 1, fp, shortname.get(), "TileWorld::TileWorld()");
									}
									else if (len >= 128) {
										assert(0);
									}
								}
							}
						}
					}
				}
				if (!silent) {
					mainEngine->fmsg(Engine::MSG_INFO, "read %dx%d tiles", width, height);
				}

				// reserved 4 bytes
				Engine::freadl(&reserved, sizeof(Uint32), 1, fp, shortname.get(), "TileWorld::TileWorld()");
			}

			// close world file
			fclose(fp);
		}
	}

	// now rotate!
	rotate(orientation);

	findExits();

	loaded = true;
}

TileWorld::~TileWorld() {
	if( chunks )
		delete[] chunks;
	chunks = nullptr;
	tiles.clear();

	// delete grid objects
	destroyGrid();
}

void TileWorld::rotate(Tile::side_t orientation) {
	if( orientation == Tile::SIDE_EAST ) {
		return; // no rotation needed
	}

	if( orientation == Tile::SIDE_SOUTH ) {
		ArrayList<Tile> newTiles;
		newTiles.resize(width * height);
		for( Uint32 u = 0; u < width; ++u ) {
			for( Uint32 v = 0; v < height; ++v ) {
				Tile& tile0 = newTiles[u + (height - v - 1) * width];
				const Tile& tile1 = tiles[v + u * height];
				tile0 = tile1;

				// switch slope sides
				switch( tile0.getCeilingSlopeSide() ) {
					case Tile::SIDE_EAST:
						tile0.setCeilingSlopeSide(Tile::SIDE_SOUTH);
						break;
					case Tile::SIDE_SOUTH:
						tile0.setCeilingSlopeSide(Tile::SIDE_WEST);
						break;
					case Tile::SIDE_WEST:
						tile0.setCeilingSlopeSide(Tile::SIDE_NORTH);
						break;
					case Tile::SIDE_NORTH:
						tile0.setCeilingSlopeSide(Tile::SIDE_EAST);
						break;
				}
				switch( tile0.getFloorSlopeSide() ) {
					case Tile::SIDE_EAST:
						tile0.setFloorSlopeSide(Tile::SIDE_SOUTH);
						break;
					case Tile::SIDE_SOUTH:
						tile0.setFloorSlopeSide(Tile::SIDE_WEST);
						break;
					case Tile::SIDE_WEST:
						tile0.setFloorSlopeSide(Tile::SIDE_NORTH);
						break;
					case Tile::SIDE_NORTH:
						tile0.setFloorSlopeSide(Tile::SIDE_EAST);
						break;
				}

				// fix textures
				tile0.setLowerTexture(Tile::SIDE_EAST, tile1.getLowerTexture(Tile::SIDE_NORTH));
				tile0.setLowerTexture(Tile::SIDE_SOUTH, tile1.getLowerTexture(Tile::SIDE_EAST));
				tile0.setLowerTexture(Tile::SIDE_WEST, tile1.getLowerTexture(Tile::SIDE_SOUTH));
				tile0.setLowerTexture(Tile::SIDE_NORTH, tile1.getLowerTexture(Tile::SIDE_WEST));
				tile0.setUpperTexture(Tile::SIDE_EAST, tile1.getUpperTexture(Tile::SIDE_NORTH));
				tile0.setUpperTexture(Tile::SIDE_SOUTH, tile1.getUpperTexture(Tile::SIDE_EAST));
				tile0.setUpperTexture(Tile::SIDE_WEST, tile1.getUpperTexture(Tile::SIDE_SOUTH));
				tile0.setUpperTexture(Tile::SIDE_NORTH, tile1.getUpperTexture(Tile::SIDE_WEST));
			}
		}
		Uint32 temp = width;
		width = height;
		height = temp;
		tiles = newTiles;
	}

	if( orientation == Tile::SIDE_NORTH ) {
		ArrayList<Tile> newTiles;
		newTiles.resize(width * height);
		for( Uint32 u = 0; u < width; ++u ) {
			for( Uint32 v = 0; v < height; ++v ) {
				Tile& tile0 = newTiles[(width - u - 1) + v * width];
				const Tile& tile1 = tiles[v + u * height];
				tile0 = tile1;

				// switch slope sides
				switch( tile0.getCeilingSlopeSide() ) {
					case Tile::SIDE_EAST:
						tile0.setCeilingSlopeSide(Tile::SIDE_NORTH);
						break;
					case Tile::SIDE_SOUTH:
						tile0.setCeilingSlopeSide(Tile::SIDE_EAST);
						break;
					case Tile::SIDE_WEST:
						tile0.setCeilingSlopeSide(Tile::SIDE_SOUTH);
						break;
					case Tile::SIDE_NORTH:
						tile0.setCeilingSlopeSide(Tile::SIDE_WEST);
						break;
				}
				switch( tile0.getFloorSlopeSide() ) {
					case Tile::SIDE_EAST:
						tile0.setFloorSlopeSide(Tile::SIDE_NORTH);
						break;
					case Tile::SIDE_SOUTH:
						tile0.setFloorSlopeSide(Tile::SIDE_EAST);
						break;
					case Tile::SIDE_WEST:
						tile0.setFloorSlopeSide(Tile::SIDE_SOUTH);
						break;
					case Tile::SIDE_NORTH:
						tile0.setFloorSlopeSide(Tile::SIDE_WEST);
						break;
				}

				// fix textures
				tile0.setLowerTexture(Tile::SIDE_EAST, tile1.getLowerTexture(Tile::SIDE_SOUTH));
				tile0.setLowerTexture(Tile::SIDE_SOUTH, tile1.getLowerTexture(Tile::SIDE_WEST));
				tile0.setLowerTexture(Tile::SIDE_WEST, tile1.getLowerTexture(Tile::SIDE_NORTH));
				tile0.setLowerTexture(Tile::SIDE_NORTH, tile1.getLowerTexture(Tile::SIDE_EAST));
				tile0.setUpperTexture(Tile::SIDE_EAST, tile1.getUpperTexture(Tile::SIDE_SOUTH));
				tile0.setUpperTexture(Tile::SIDE_SOUTH, tile1.getUpperTexture(Tile::SIDE_WEST));
				tile0.setUpperTexture(Tile::SIDE_WEST, tile1.getUpperTexture(Tile::SIDE_NORTH));
				tile0.setUpperTexture(Tile::SIDE_NORTH, tile1.getUpperTexture(Tile::SIDE_EAST));
			}
		}
		Uint32 temp = width;
		width = height;
		height = temp;
		tiles = newTiles;
	}
	
	if( orientation == Tile::SIDE_WEST ) {
		ArrayList<Tile> newTiles;
		newTiles.resize(width * height);
		for( Uint32 u = 0; u < width; ++u ) {
			for( Uint32 v = 0; v < height; ++v ) {
				Tile& tile0 = newTiles[(height - v - 1) + (width - u - 1) * height];
				const Tile& tile1 = tiles[v + u * height];
				tile0 = tile1;

				// switch slope sides
				switch( tile0.getCeilingSlopeSide() ) {
					case Tile::SIDE_EAST:
						tile0.setCeilingSlopeSide(Tile::SIDE_WEST);
						break;
					case Tile::SIDE_SOUTH:
						tile0.setCeilingSlopeSide(Tile::SIDE_NORTH);
						break;
					case Tile::SIDE_WEST:
						tile0.setCeilingSlopeSide(Tile::SIDE_EAST);
						break;
					case Tile::SIDE_NORTH:
						tile0.setCeilingSlopeSide(Tile::SIDE_SOUTH);
						break;
				}
				switch( tile0.getFloorSlopeSide() ) {
					case Tile::SIDE_EAST:
						tile0.setFloorSlopeSide(Tile::SIDE_WEST);
						break;
					case Tile::SIDE_SOUTH:
						tile0.setFloorSlopeSide(Tile::SIDE_NORTH);
						break;
					case Tile::SIDE_WEST:
						tile0.setFloorSlopeSide(Tile::SIDE_EAST);
						break;
					case Tile::SIDE_NORTH:
						tile0.setFloorSlopeSide(Tile::SIDE_SOUTH);
						break;
				}

				// fix textures
				tile0.setLowerTexture(Tile::SIDE_EAST, tile1.getLowerTexture(Tile::SIDE_WEST));
				tile0.setLowerTexture(Tile::SIDE_SOUTH, tile1.getLowerTexture(Tile::SIDE_NORTH));
				tile0.setLowerTexture(Tile::SIDE_WEST, tile1.getLowerTexture(Tile::SIDE_EAST));
				tile0.setLowerTexture(Tile::SIDE_NORTH, tile1.getLowerTexture(Tile::SIDE_SOUTH));
				tile0.setUpperTexture(Tile::SIDE_EAST, tile1.getUpperTexture(Tile::SIDE_WEST));
				tile0.setUpperTexture(Tile::SIDE_SOUTH, tile1.getUpperTexture(Tile::SIDE_NORTH));
				tile0.setUpperTexture(Tile::SIDE_WEST, tile1.getUpperTexture(Tile::SIDE_EAST));
				tile0.setUpperTexture(Tile::SIDE_NORTH, tile1.getUpperTexture(Tile::SIDE_SOUTH));
			}
		}
		tiles = newTiles;
	}

	float rot = 0.f;
	switch( orientation ) {
		case Tile::SIDE_SOUTH:
			rot = PI/2.f;
			break;
		case Tile::SIDE_WEST:
			rot = PI;
			break;
		case Tile::SIDE_NORTH:
			rot = 3*PI/2.f;
			break;
		default:
			break;
	}
	
	// rotate entities
	for( Uint32 c=0; c<numBuckets; ++c ) {
		for( Node<Entity*>* node=entities[c].getFirst(); node!=nullptr; node=node->getNext() ) {
			Entity* entity = node->getData();

			if (!entity->isShouldSave())
				continue;

			// update position
			float temp = 0.f;
			Vector pos = entity->getPos();
			float x = (pos.x * cos(rot)) - (pos.y * sin(rot));
			float y = (pos.x * sin(rot)) + (pos.y * cos(rot));
			switch( orientation ) {
				case Tile::SIDE_SOUTH:
					x += width * Tile::size;
					break;
				case Tile::SIDE_WEST:
					x += width * Tile::size;
					y += height * Tile::size;
					break;
				case Tile::SIDE_NORTH:
					y += height * Tile::size;
					break;
				default:
					break;
			}
			pos.x = x;
			pos.y = y;
			entity->setPos(pos);
			entity->setNewPos(pos);

			// update angle
			Angle ang = entity->getAng();
			ang.yaw += rot;
			entity->setAng(ang);
			entity->setNewAng(ang);
			entity->update();
		}
	}
}

void TileWorld::initialize(bool empty) {
	World::initialize(empty);

	// initialize tiles
	for( Uint32 x=0; x<width; ++x ) {
		for( Uint32 y=0; y<height; ++y ) {
			Tile& tile = tiles[y+x*height];

			tile.setWorld(*this);
			tile.setX(x*Tile::size);
			tile.setY(y*Tile::size);
			tile.setDynamicsWorld(*bulletDynamicsWorld);

			// assign chunks to tiles and vice versa
			Uint32 cX = x/Chunk::size;
			Uint32 cY = y/Chunk::size;
			Uint32 cH = calcChunksHeight();
			Chunk& chunk = chunks[cY+cX*cH];
			chunk.setTile((y%Chunk::size)+(x%Chunk::size)*Chunk::size,&tile);
			tile.setChunk(chunk);
		}
	}

	// compile tile vertices
	for( Uint32 x=0; x<width; ++x ) {
		for( Uint32 y=0; y<height; ++y ) {
			Tile& tile = tiles[y+x*height];

			tile.compileFloorVertices();
			tile.compileCeilingVertices();
			if( x<width-1 ) {
				Tile& neighbor = tiles[y+(x+1)*height];
				tile.compileLowerVertices(neighbor,Tile::SIDE_EAST);
				tile.compileUpperVertices(neighbor,Tile::SIDE_EAST);
			}
			if( y<height-1 ) {
				Tile& neighbor = tiles[(y+1)+x*height];
				tile.compileLowerVertices(neighbor,Tile::SIDE_SOUTH);
				tile.compileUpperVertices(neighbor,Tile::SIDE_SOUTH);
			}
			if( x>0 ) {
				Tile& neighbor = tiles[y+(x-1)*height];
				tile.compileLowerVertices(neighbor,Tile::SIDE_WEST);
				tile.compileUpperVertices(neighbor,Tile::SIDE_WEST);
			}
			if( y>0 ) {
				Tile& neighbor = tiles[(y-1)+x*height];
				tile.compileLowerVertices(neighbor,Tile::SIDE_NORTH);
				tile.compileUpperVertices(neighbor,Tile::SIDE_NORTH);
			}
			tile.buildBuffers();
			tile.compileBulletPhysicsMesh();
		}
	}

	// initialize chunks
	Uint32 w = calcChunksWidth();
	Uint32 h = calcChunksHeight();
	for( Uint32 x=0; x<w; ++x ) {
		for( Uint32 y=0; y<h; ++y ) {
			Chunk& chunk = chunks[y+x*h];
			chunk.setWorld(*this);
			chunk.buildBuffers();
			if( !empty ) {
				chunk.optimizeBuffers();
			}
		}
	}

	// create grid object
	createGrid();

	// setup editor pointer
	selectedRect.x = -1;
	selectedRect.y = -1;
	selectedRect.w = 0;
	selectedRect.h = 0;

	// initialize entities
	for( Uint32 c=0; c<numBuckets; ++c ) {
		for( Node<Entity*>* node = entities[c].getFirst(); node != nullptr; node = node->getNext() ) {
			Entity* entity = node->getData();
			entity->update();
		}
	}
}

void TileWorld::createGrid() {
	if (vao)
		return;

	for( int i=0; i<BUFFER_MAX; ++i ) {
		vbo[static_cast<buffer_t>(i)] = 0;
	}

	// create vertex array
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// create vertex data
	GLfloat* vertices = new GLfloat[(width+1+height+1)*3*2];
	GLfloat* colors = new GLfloat[(width+1+height+1)*4*2];
	for( Uint32 x=0; x<=width; ++x ) {
		vertices[x*6]   = x*Tile::size;
		vertices[x*6+1] = 0;
		vertices[x*6+2] = 0;
		vertices[x*6+3] = x*Tile::size;
		vertices[x*6+4] = 0;
		vertices[x*6+5] = height*Tile::size;

		if( x%Chunk::size ) {
			colors[x*8]   = .5f;
			colors[x*8+1] = 0.f;
			colors[x*8+2] = 0.f;
			colors[x*8+3] = 1.f;
			colors[x*8+4] = .5f;
			colors[x*8+5] = 0.f;
			colors[x*8+6] = 0.f;
			colors[x*8+7] = 1.f;
		} else {
			if( x%largeGridSize ) {
				colors[x*8]   = 1.f;
				colors[x*8+1] = 0.f;
				colors[x*8+2] = 0.f;
				colors[x*8+3] = 1.f;
				colors[x*8+4] = 1.f;
				colors[x*8+5] = 0.f;
				colors[x*8+6] = 0.f;
				colors[x*8+7] = 1.f;
			} else {
				colors[x*8]   = 0.f;
				colors[x*8+1] = 1.f;
				colors[x*8+2] = 1.f;
				colors[x*8+3] = 1.f;
				colors[x*8+4] = 0.f;
				colors[x*8+5] = 1.f;
				colors[x*8+6] = 1.f;
				colors[x*8+7] = 1.f;
			}
		}
	}
	for( Uint32 y=0; y<=height; ++y ) {
		vertices[(width+1)*6+y*6]   = 0;
		vertices[(width+1)*6+y*6+1] = 0;
		vertices[(width+1)*6+y*6+2] = y*Tile::size;
		vertices[(width+1)*6+y*6+3] = width*Tile::size;
		vertices[(width+1)*6+y*6+4] = 0;
		vertices[(width+1)*6+y*6+5] = y*Tile::size;

		if( y%Chunk::size ) {
			colors[(width+1)*8+y*8]   = .5f;
			colors[(width+1)*8+y*8+1] = 0.f;
			colors[(width+1)*8+y*8+2] = 0.f;
			colors[(width+1)*8+y*8+3] = 1.f;
			colors[(width+1)*8+y*8+4] = .5f;
			colors[(width+1)*8+y*8+5] = 0.f;
			colors[(width+1)*8+y*8+6] = 0.f;
			colors[(width+1)*8+y*8+7] = 1.f;
		} else {
			if( y%largeGridSize ) {
				colors[(width+1)*8+y*8]   = 1.f;
				colors[(width+1)*8+y*8+1] = 0.f;
				colors[(width+1)*8+y*8+2] = 0.f;
				colors[(width+1)*8+y*8+3] = 1.f;
				colors[(width+1)*8+y*8+4] = 1.f;
				colors[(width+1)*8+y*8+5] = 0.f;
				colors[(width+1)*8+y*8+6] = 0.f;
				colors[(width+1)*8+y*8+7] = 1.f;
			} else {
				colors[(width+1)*8+y*8]   = 0.f;
				colors[(width+1)*8+y*8+1] = 1.f;
				colors[(width+1)*8+y*8+2] = 1.f;
				colors[(width+1)*8+y*8+3] = 1.f;
				colors[(width+1)*8+y*8+4] = 0.f;
				colors[(width+1)*8+y*8+5] = 1.f;
				colors[(width+1)*8+y*8+6] = 1.f;
				colors[(width+1)*8+y*8+7] = 1.f;
			}
		}
	}

	// upload vertex data
	glGenBuffers(1, &vbo[BUFFER_VERTEX]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[BUFFER_VERTEX]);
	glBufferData(GL_ARRAY_BUFFER, (width+1+height+1) * 3 * 2 * sizeof(GLfloat), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(0);
	delete[] vertices;

	// upload color data
	glGenBuffers(1, &vbo[BUFFER_COLOR]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[BUFFER_COLOR]);
	glBufferData(GL_ARRAY_BUFFER, (width+1+height+1) * 4 * 2 * sizeof(GLfloat), colors, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(1);
	delete[] colors;

	// create index data
	GLuint* indices = new GLuint[(width+1+height+1)*2];
	for( Uint32 i=0; i<(width+1+height+1)*2; ++i ) {
		indices[i] = i;
	}

	// upload index data
	glGenBuffers(1, &vbo[BUFFER_INDEX]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[BUFFER_INDEX]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, (width+1+height+1) * 2 * sizeof(GLuint), indices, GL_STATIC_DRAW);
	delete[] indices;

	// unbind vertex array
	glBindVertexArray(0);
}

void TileWorld::destroyGrid() {
	for( int i=0; i<BUFFER_MAX; ++i ) {
		buffer_t buffer = static_cast<buffer_t>(i);
		if( vbo[buffer] ) {
			glDeleteBuffers(1,&vbo[buffer]);
		}
	}
	if( vao ) {
		glDeleteVertexArrays(1,&vao);
	}
}

int TileWorld::calcChunksWidth() const {
	return width/Chunk::size + ((width%Chunk::size)>0 ? 1 : 0);
}

int TileWorld::calcChunksHeight() const {
	return height/Chunk::size + ((height%Chunk::size)>0 ? 1 : 0);
}

void TileWorld::deselectGeometry() {
	selectedRect.x = -1;
	selectedRect.y = -1;
	selectedRect.w = 0;
	selectedRect.h = 0;
}

void TileWorld::findEntitiesInRadius( const Vector& origin, float radius, LinkedList<Entity*>& outList, bool flat ) {
	if( radius <= 0 ) {
		return;
	}

	Sint32 cX = origin.x / (Chunk::size * Tile::size);
	Sint32 cY = origin.y / (Chunk::size * Tile::size);
	Sint32 cW = max( 1, (Sint32)(width / Chunk::size) );
	Sint32 cH = max( 1, (Sint32)(height / Chunk::size) );
	Sint32 chunkRadius = floor((radius / Tile::size) / Chunk::size) + 1;
	Sint32 startX = min( max( 0, cX - chunkRadius), cW - 1);
	Sint32 startY = min( max( 0, cY - chunkRadius), cH - 1);
	Sint32 endX = min( max( 0, cX + chunkRadius), cW - 1);
	Sint32 endY = min( max( 0, cY + chunkRadius), cH - 1);

	for( Sint32 x = startX; x <= endX; ++x ) {
		for( Sint32 y = startY; y <= endY; ++y ) {
			LinkedList<Entity*>& population = chunks[y + x * cH].getEPopulation();
			for( auto entity : population ) {
				Vector diff = origin - entity->getPos();
				float dist = flat ?
					sqrt( diff.x * diff.x + diff.y * diff.y ) :
					sqrt( diff.x * diff.x + diff.y * diff.y + diff.z * diff.z );
				if( dist <= radius ) {
					outList.addNodeLast(entity);
				}
			}
		}
	}
}

void TileWorld::optimizeChunks() {
	unsigned int w = calcChunksWidth();
	unsigned int h = calcChunksHeight();
	for( unsigned int x=0; x<w; ++x ) {
		for( unsigned int y=0; y<h; ++y ) {
			chunks[y+x*h].optimizeBuffers();
		}
	}
}

void TileWorld::rebuildChunks() {
	unsigned int w = calcChunksWidth();
	unsigned int h = calcChunksHeight();
	for( unsigned int x=0; x<w; ++x ) {
		for( unsigned int y=0; y<h; ++y ) {
			chunks[y+x*h].buildBuffers();
			chunks[y+x*h].optimizeBuffers();
		}
	}
}

bool TileWorld::saveFile(const char* _filename, bool updateFilename ) {
	const String& path = (_filename == nullptr || _filename[0] == '\0') ? filename : StringBuf<256>(_filename);
	if( updateFilename ) {
		changeFilename(path.get());
	}

	// open file for saving
	if( path.empty() ) {
		mainEngine->fmsg(Engine::MSG_WARN,"attempted to save world without filename");
		return false;
	}
	if( _filename == nullptr || _filename[0] == '\0' ) {
		mainEngine->fmsg(Engine::MSG_INFO,"saving world file '%s'...",shortname.get());
	} else {
		mainEngine->fmsg(Engine::MSG_INFO,"saving world file '%s'...",_filename);
	}

	if (filetype == FILE_BINARY) {
		return FileHelper::writeObject(path.get(), EFileFormat::Binary, *this);
	} else {
		return FileHelper::writeObject(path.get(), EFileFormat::Json, *this);
	}
}

void TileWorld::serialize(FileInterface * file) {
	Uint32 version = 0;

	file->property("TileWorld::version", version);
	file->property("nameStr", nameStr);
	file->property("width", width);
	file->property("height", height);
	file->property("tiles", tiles, width * height);

	file->propertyName("entities");
	if (file->isReading()) {
		Uint32 numEntities = 0;
		file->beginArray(numEntities);

		for (Uint32 index = 0; index < numEntities; ++index) {
			Entity * entity = new Entity(this);
			file->value(*entity);
		}

		file->endArray();
	}
	else {
		// write number of entities
		Uint32 numEntities = 0;
		for (Uint32 c = 0; c<numBuckets; ++c) {
			for (Node<Entity*>* node = entities[c].getFirst(); node != nullptr; node = node->getNext()) {
				Entity* entity = node->getData();
				if (entity->isToBeDeleted() || !entity->isShouldSave()) {
					continue;
				}

				++numEntities;
			}
		}

		file->beginArray(numEntities);

		for (Uint32 c = 0; c<numBuckets; ++c) {
			for (Node<Entity*>* node = entities[c].getFirst(); node != nullptr; node = node->getNext()) {
				Entity* entity = node->getData();
				if (entity->isToBeDeleted() || !entity->isShouldSave()) {
					continue;
				}

				file->value(*entity);
			}
		}

		file->endArray();
	}
}

void TileWorld::resize(int left, int right, int up, int down) {
	if( left==0 && right==0 && up==0 && down==0 ) {
		// no resizing needed
		return;
	}

	// delete occlusion data for all entities
	for( Uint32 c = 0; c < numBuckets; ++c ) {
		for( Node<Entity*>* node = entities[c].getFirst(); node != nullptr; node = node->getNext() ) {
			Entity* entity = node->getData();
			entity->deleteAllVisMaps();
			entity->clearAllChunkNodes();
		}
	}

	// create new tile array
	int newWidth = width + left + right;
	int newHeight = height + up + down;
	if( newWidth<=0 || newHeight<=0 ) {
		// bad size!
		return;
	}
	ArrayList<Tile> newTiles;
	newTiles.resize(newWidth*newHeight);

	// create new chunk array
	int newChunkWidth = newWidth/Chunk::size + ((newWidth%Chunk::size)>0 ? 1 : 0);
	int newChunkHeight = newHeight/Chunk::size + ((newHeight%Chunk::size)>0 ? 1 : 0);
	Chunk* newChunks = new Chunk[newChunkWidth*newChunkHeight];

	// initialize new tile array
	for( int x=0; x<newWidth; ++x ) {
		for( int y=0; y<newHeight; ++y ) {
			Tile& tile = newTiles[y+x*newHeight];

			tile.setWorld(*this);
			tile.setX(x*Tile::size);
			tile.setY(y*Tile::size);
			tile.setDynamicsWorld(*bulletDynamicsWorld);

			// assign chunks to tiles and vice versa
			Uint32 cX = x/Chunk::size;
			Uint32 cY = y/Chunk::size;
			Chunk& chunk = newChunks[cY+cX*newChunkHeight];
			chunk.setTile((y%Chunk::size)+(x%Chunk::size)*Chunk::size,&tile);
			tile.setChunk(chunk);
		}
	}

	// copy tile data from old tiles array to the new
	int newX;
	int newY;
	int newStartX = max(0,left);
	int newStartY = max(0,up);
	int newEndX = newWidth - max(0,right);
	int newEndY = newHeight - max(0,down);

	int oldX;
	int oldY;
	int oldStartX = max(0,-left);
	int oldStartY = max(0,-up);
	int oldEndX = width - max(0,-right);
	int oldEndY = height - max(0,-down);
	for( oldX=oldStartX, newX=newStartX; oldX<oldEndX && newX<newEndX; ++oldX, ++newX ) {
		for( oldY=oldStartY, newY=newStartY; oldY<oldEndY && newY<newEndY; ++oldY, ++newY ) {
			Tile& newTile = newTiles[newY+newX*newHeight];
			Tile& oldTile = tiles[oldY+oldX*height];

			newTile.setCeilingHeight(oldTile.getCeilingHeight());
			newTile.setFloorHeight(oldTile.getFloorHeight());
			
			newTile.setUpperTexture(Tile::SIDE_EAST,oldTile.getUpperTexture(Tile::SIDE_EAST));
			newTile.setLowerTexture(Tile::SIDE_EAST,oldTile.getLowerTexture(Tile::SIDE_EAST));
			newTile.setUpperTexture(Tile::SIDE_SOUTH,oldTile.getUpperTexture(Tile::SIDE_SOUTH));
			newTile.setLowerTexture(Tile::SIDE_SOUTH,oldTile.getLowerTexture(Tile::SIDE_SOUTH));
			newTile.setUpperTexture(Tile::SIDE_WEST,oldTile.getUpperTexture(Tile::SIDE_WEST));
			newTile.setLowerTexture(Tile::SIDE_WEST,oldTile.getLowerTexture(Tile::SIDE_WEST));
			newTile.setUpperTexture(Tile::SIDE_NORTH,oldTile.getUpperTexture(Tile::SIDE_NORTH));
			newTile.setLowerTexture(Tile::SIDE_NORTH,oldTile.getLowerTexture(Tile::SIDE_NORTH));

			newTile.setCeilingTexture(oldTile.getCeilingTexture());
			newTile.setFloorTexture(oldTile.getFloorTexture());

			newTile.setCeilingSlopeSide(oldTile.getCeilingSlopeSide());
			newTile.setCeilingSlopeSize(oldTile.getCeilingSlopeSize());
			newTile.setFloorSlopeSide(oldTile.getFloorSlopeSide());
			newTile.setFloorSlopeSize(oldTile.getFloorSlopeSize());
		}
	}

	// finalize copied tiles
	for( int x=0; x<newWidth; ++x ) {
		for( int y=0; y<newHeight; ++y ) {
			Tile& tile = newTiles[y+x*newHeight];

			tile.compileFloorVertices();
			tile.compileCeilingVertices();
			if( x<(int)newWidth-1 ) {
				Tile& neighbor = newTiles[y+(x+1)*newHeight];
				tile.compileLowerVertices(neighbor,Tile::SIDE_EAST);
				tile.compileUpperVertices(neighbor,Tile::SIDE_EAST);
			}
			if( y<(int)newHeight-1 ) {
				Tile& neighbor = newTiles[(y+1)+x*newHeight];
				tile.compileLowerVertices(neighbor,Tile::SIDE_SOUTH);
				tile.compileUpperVertices(neighbor,Tile::SIDE_SOUTH);
			}
			if( x>0 ) {
				Tile& neighbor = newTiles[y+(x-1)*newHeight];
				tile.compileLowerVertices(neighbor,Tile::SIDE_WEST);
				tile.compileUpperVertices(neighbor,Tile::SIDE_WEST);
			}
			if( y>0 ) {
				Tile& neighbor = newTiles[(y-1)+x*newHeight];
				tile.compileLowerVertices(neighbor,Tile::SIDE_NORTH);
				tile.compileUpperVertices(neighbor,Tile::SIDE_NORTH);
			}
			tile.buildBuffers();
			tile.compileBulletPhysicsMesh();
		}
	}

	// delete the old tiles
	tiles.clear();
	delete[] chunks;

	// copy final width and height
	width = newWidth;
	height = newHeight;

	// reassign pointers
	tiles = newTiles;
	chunks = newChunks;

	// initialize chunks
	for( int x=0; x<newChunkWidth; ++x ) {
		for( int y=0; y<newChunkHeight; ++y ) {
			Chunk& chunk = newChunks[y+x*newChunkHeight];
			chunk.setWorld(*this);
			chunk.buildBuffers();
			chunk.optimizeBuffers();
		}
	}

	// clear chunk pointers from lights
	LinkedList<Light*> lights;
	for( Uint32 c = 0; c < numBuckets; ++c ) {
		for( Node<Entity*>* node=entities[c].getFirst(); node!=nullptr; node=node->getNext() ) {
			Entity* entity = node->getData();
			entity->findAllComponents<Light>(Component::COMPONENT_LIGHT, lights);
		}
	}
	for (auto light : lights) {
		light->getChunksLit().clear();
		light->getChunksShadow().clear();
	}

	// move entities, if necessary
	if( left || up ) {
		for( int c=0; c<numBuckets; ++c ) {
			for( Node<Entity*>* node=entities[c].getFirst(); node!=nullptr; node=node->getNext() ) {
				Entity* entity = node->getData();

				Vector pos = entity->getPos();
				pos.x += left * Tile::size;
				pos.y += up * Tile::size;
				entity->setPos(pos);
				entity->update();
			}
		}
	}

	// resize grid
	destroyGrid();
	createGrid();
}

void TileWorld::drawGrid(Camera& camera, float z) {
	glLineWidth(2.f);

	// setup model matrix
	glm::mat4 modelMatrix = glm::translate(glm::mat4(1.f),glm::vec3(0,-z,0));

	// setup view matrix
	const glm::mat4 viewMatrix = camera.getProjViewMatrix() * modelMatrix;

	// load shader
	Material* mat = mainEngine->getMaterialResource().dataForString("shaders/basic/grid.json");
	if( mat ) {
		const ShaderProgram& shader = mat->getShader();
		if( &shader != ShaderProgram::getCurrentShader() )
			shader.mount();

		// upload uniform variables
		glUniformMatrix4fv(shader.getUniformLocation("gView"),1,GL_FALSE,glm::value_ptr(viewMatrix));

		// draw elements
		glBindVertexArray(vao);
		glDrawElements(GL_LINES, (width+1+height+1)*2, GL_UNSIGNED_INT, NULL);
		glBindVertexArray(0);

		shader.unmount();
	}
}

static Cvar cvar_depthOffset("render.depthoffset","depth buffer adjustment","1");
static Cvar cvar_shadowsEnabled("render.shadows", "enables shadow rendering", "2");
static Cvar cvar_renderCull("render.cull", "accuracy for occlusion culling", "7");

void TileWorld::drawSceneObjects(Camera& camera, Light* light, const ArrayList<Chunk*>& chunkDrawList) {
	Client* client = mainEngine->getLocalClient();
	if( !client )
		return;

	// editor variables
	bool editorActive = false;
	bool ceilingMode = false;
	Editor::editingmode_t editingMode = Editor::editingmode_t::TILES;
	Uint32 highlightedObj = nuid;
	if( client->isEditorActive() ) {
		editorActive = true;
		Editor* editor = client->getEditor();
		ceilingMode = editor->isCeilingMode();
		highlightedObj = editor->getHighlightedObj();
		editingMode = static_cast<Editor::editingmode_t>(editor->getEditingMode());
	}

	if( camera.getDrawMode() != Camera::DRAW_STENCIL || cvar_shadowsEnabled.toInt()&2 ) {
		if( camera.getDrawMode() <= Camera::DRAW_GLOW ||
			camera.getDrawMode() == Camera::DRAW_TRIANGLES ||
			(camera.getDrawMode() == Camera::DRAW_SILHOUETTE && cvar_showEdges.toInt()) ) {
			// draw editor selector cube things
			if( camera.getDrawMode() != Camera::DRAW_DEPTH ) {
				if( client->isEditorActive() && !camera.isOrtho() && showTools && selectedRect.x!=-1 && selectedRect.y!=-1 ) {
					Sint32 startX = min( max( 0, selectedRect.x + min(0,selectedRect.w) ), (int)width );
					Sint32 endX = min( max( 0, selectedRect.x + max(0,selectedRect.w) ), (int)width );
					Sint32 startY = min( max( 0, selectedRect.y + min(0,selectedRect.h) ), (int)height );
					Sint32 endY = min( max( 0, selectedRect.y + max(0,selectedRect.h) ), (int)height );
					for( Sint32 x=startX; x<=endX; ++x ) {
						for( Sint32 y=startY; y<=endY; ++y ) {
							Tile& tile = tiles[y + x * height];

							glm::vec3 pos;
							pos.x = x*Tile::size + Tile::size/2;
							pos.y = y*Tile::size + Tile::size/2;
							if( ceilingMode ) {
								pos.z = tile.getCeilingHeight() + tile.getCeilingSlopeSize();
							} else {
								pos.z = tile.getFloorHeight();
							}
							glm::mat4 cubeM = glm::translate(glm::mat4(1.f),glm::vec3(pos.x,-pos.z,pos.y));
							cubeM = glm::scale(cubeM,glm::vec3(Tile::size, 8.f, Tile::size));
							glm::vec4 color = (editingMode == Editor::editingmode_t::TILES) ?
								glm::vec4(0.f,0.f,1.f,.25f):
								glm::vec4(1.f,0.f,1.f,.25f);
							camera.drawCube(cubeM,color);
						}
					}
				}
			}

			// draw chunks
			Tile::loadShader(*this,camera,light);
			for( auto chunk : chunkDrawList ) {
				chunk->draw(camera);
			}
		}
	}
	
	float offset = cvar_depthOffset.toFloat();
	if( camera.getDrawMode() == Camera::DRAW_DEPTH && offset ) {
		glPolygonOffset(1.f, offset);
		glEnable(GL_POLYGON_OFFSET_FILL);
	}

	// draw entities
	if( camera.getDrawMode() != Camera::DRAW_GLOW || !editorActive || !showTools ) {
		for( auto chunk : chunkDrawList ) {
			for( auto entity : chunk->getEPopulation() ) {
				// in silhouette mode, skip unhighlighted or unselected actors
				if( camera.getDrawMode()==Camera::DRAW_SILHOUETTE ) {
					if( !entity->isHighlighted() && entity->getUID() != highlightedObj ) {
						continue;
					}
				}

				// draw the entity
				entity->draw(camera,light);
			}
		}
	}

	if( camera.getDrawMode() == Camera::DRAW_DEPTH && offset ) {
		glPolygonOffset(1.f, 0.f);
		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	// reset some gl state
	glActiveTexture(GL_TEXTURE0);
	ShaderProgram::unmount();
}

void TileWorld::draw() {
	Client* client = mainEngine->getLocalClient();
	if( !client )
		return;
	Renderer* renderer = client->getRenderer();
	if( !renderer || !renderer->isInitialized() )
		return;
	Editor* editor = client->getEditor();

	// build camera list
	LinkedList<Camera*> cameras;
	for( Uint32 c = 0; c < numBuckets; ++c ) {
		for( Node<Entity*>* node=entities[c].getFirst(); node!=nullptr; node=node->getNext() ) {
			Entity* entity = node->getData();
			entity->findAllComponents<Camera>(Component::COMPONENT_CAMERA, cameras);
		}
	}

	// build light list
	LinkedList<Light*> lights;
	for( Uint32 c = 0; c < numBuckets; ++c ) {
		for( Node<Entity*>* node=entities[c].getFirst(); node!=nullptr; node=node->getNext() ) {
			Entity* entity = node->getData();
			entity->findAllComponents<Light>(Component::COMPONENT_LIGHT, lights);
		}
	}

	// cull unselected cameras (editor)
	if( editor && editor->isInitialized() ) {
		Node<Camera*>* nextNode = nullptr;
		for( Node<Camera*>* node=cameras.getFirst(); node!=nullptr; node=nextNode ) {
			Camera* camera = node->getData();
			nextNode = node->getNext();

			// in editor, skip all but selected cams and the main cams
			if( camera != editor->getEditingCamera() && 
				camera != editor->getMinimapCamera() &&
				!camera->getEntity()->isSelected() ) {
				cameras.removeNode(node);
			}
		}
	}
	
	// iterate cameras
	for( Node<Camera*>* node=cameras.getFirst(); node!=nullptr; node=node->getNext() ) {
		Camera* camera = node->getData();

		// in editor, skip minimap if any other cameras are selected
		// replace it with our selected camera(s)
		Rect<Sint32> oldWin;
		if( editor && editor->isInitialized() ) {
			if( camera != editor->getEditingCamera() && 
				camera != editor->getMinimapCamera() ) {
				oldWin = camera->getWin();
				camera->setWin(editor->getMinimapCamera()->getWin());
			} else if( camera == editor->getMinimapCamera() && cameras.getSize() > 2 ) {
				continue;
			}
		}

		// skip deactivated cameras
		if( !camera->getEntity()->isFlag(Entity::flag_t::FLAG_VISIBLE) ) {
			continue;
		}

		// skip cameras whose window is too small
		if( camera->getWin().w <= 0 || camera->getWin().h <= 0 ) {
			continue;
		}

		// clear the window area
		glClear(GL_DEPTH_BUFFER_BIT);
		Rect<int> backgroundRect = camera->getWin();
		renderer->drawRect(&backgroundRect,glm::vec4(0.f,0.f,0.f,1.f));

		// set proper light blending function
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);

		// setup projection
		camera->setupProjection();

		// occlusion test
		if( !camera->getChunksVisible() ) {
			camera->occlusionTest(camera->getClipFar(), cvar_renderCull.toInt());
		}

		Sint32 w = calcChunksWidth();
		Sint32 h = calcChunksHeight();
		Sint32 chunkSize = Tile::size * Chunk::size;

		ArrayList<Light*> cameraLightList;

		// build relevant light list
		// this could be done better
		for( Node<Light*>* node=lights.getFirst(); node!=nullptr; node=node->getNext() ) {
			Light* light = node->getData();

			light->setChosen(false);
			light->getChunksLit().clear();

			// don't render invisible lights
			if( !light->getEntity()->isFlag(Entity::flag_t::FLAG_VISIBLE) ) {
				continue;
			}

			for( auto lightChunk : light->getVisibleChunks() ) {
				Sint32 chunkX = lightChunk->getTile(0)->getX() / chunkSize;
				Sint32 chunkY = lightChunk->getTile(0)->getY() / chunkSize;

				// add to list of lit chunks
				if( camera->getChunksVisible()[chunkY + chunkX * h] ) {
					light->getChunksLit().push(lightChunk);
					if( !light->isChosen() ) {
						light->setChosen(true);
						cameraLightList.push(light);
					}
				}
			}
		}

		// render scene into depth buffer
		camera->setDrawMode(Camera::DRAW_DEPTH);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		glDrawBuffer(GL_NONE);
		drawSceneObjects(*camera,nullptr,camera->getVisibleChunks());

		if( client->isEditorActive() && showTools ) {
			// render fullbright scene
			camera->setDrawMode(Camera::DRAW_STANDARD);
			glDrawBuffer(GL_BACK);
			glDisable(GL_STENCIL_TEST);
			glEnable(GL_DEPTH_TEST);
			glDepthMask(GL_FALSE);
			drawSceneObjects(*camera,nullptr,camera->getVisibleChunks());
		} else {
			for( auto light : cameraLightList ) {
				// render stencil shadows
				glEnable(GL_STENCIL_TEST);
				glDepthMask(GL_FALSE);
				glEnable(GL_DEPTH_CLAMP);
				glDisable(GL_CULL_FACE);
				glStencilFunc(GL_ALWAYS, 0x00, 0xFF);
				glStencilOpSeparate(GL_BACK, GL_KEEP, GL_DECR_WRAP, GL_KEEP);
				glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_INCR_WRAP, GL_KEEP);
				glClear(GL_STENCIL_BUFFER_BIT);
				glDepthFunc(GL_LESS);
				glDrawBuffer(GL_NONE);
				if( cvar_shadowsEnabled.toInt() ) {
					if( light->getEntity()->isFlag(Entity::flag_t::FLAG_SHADOW) && light->isShadow() ) {
						camera->setDrawMode(Camera::DRAW_STENCIL);
						if( light->getChunksShadow().getSize() > 0 ) {
							drawSceneObjects(*camera,light,light->getChunksShadow());
						}
					}
				}
				glDisable(GL_DEPTH_CLAMP);
				glEnable(GL_CULL_FACE);

				// render shadowed scene
				camera->setDrawMode(Camera::DRAW_STANDARD);
				glDrawBuffer(GL_BACK);
				glStencilFunc(GL_EQUAL, 0x00, 0xFF);
				glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_KEEP);
				glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_KEEP);
				glDepthFunc(GL_LEQUAL);
				if( light->getChunksLit().getSize() > 0 ) {
					drawSceneObjects(*camera,light,light->getChunksLit());
				}
				glDisable(GL_STENCIL_TEST);
			}
		}

		// render scene with glow textures
		camera->setDrawMode(Camera::DRAW_GLOW);
		glDepthMask(GL_FALSE);
		glDrawBuffer(GL_BACK);
		glDepthFunc(GL_LEQUAL);
		drawSceneObjects(*camera,nullptr,camera->getVisibleChunks());

		// render lasers
		for (auto& laser : lasers) {
			if (laser.maxLife > 0.f) {
				glm::vec4 color = laser.color;
				color.a *= laser.life / laser.maxLife;
				camera->drawLaser(laser.size, laser.start, laser.end, color);
			} else {
				camera->drawLaser(laser.size, laser.start, laser.end, laser.color);
			}
		}

		// render triangle lines
		if( cvar_showVerts.toInt() ) {
			camera->setDrawMode(Camera::DRAW_TRIANGLES);
			drawSceneObjects(*camera,nullptr,camera->getVisibleChunks());
		}

		// render depth fail scene
		camera->setDrawMode(Camera::DRAW_DEPTHFAIL);
		glDepthFunc(GL_GREATER);
		drawSceneObjects(*camera,nullptr,camera->getVisibleChunks());
		glDepthFunc(GL_LEQUAL);

		if( camera->isOrtho() ) {
			// draw level mask
			camera->setDrawMode(Camera::DRAW_DEPTH);
			glEnable(GL_STENCIL_TEST);
			glDisable(GL_DEPTH_TEST);
			glDepthMask(GL_FALSE);
			glDrawBuffer(GL_NONE);
			glClear(GL_STENCIL_BUFFER_BIT);
			glStencilFunc(GL_ALWAYS, 0x00, 0xFF);
			glStencilOp(GL_INCR, GL_INCR, GL_INCR);
			drawSceneObjects(*camera,nullptr,camera->getVisibleChunks());
			glStencilFunc(GL_EQUAL, 0x00, 0xFF);
			glDrawBuffer(GL_BACK);
			Renderer* renderer = camera->getRenderer();
			if( renderer ) {
				renderer->drawRect( &camera->getWin(), glm::vec4(.25f,.25f,.25f,1.f) );
			}
			glStencilFunc(GL_ALWAYS, 0x00, 0xFF);

			// render state gets messed up after this, so reinit
			camera->setupProjection();
		} else {
			// render silhouettes
			camera->setDrawMode(Camera::DRAW_SILHOUETTE);
			drawSceneObjects(*camera,nullptr,camera->getVisibleChunks());
		}

		glDrawBuffer(GL_BACK);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		glDisable(GL_STENCIL_TEST);

		// draw editing grid
		if( client->isEditorActive() && showTools && gridVisible ) {
			if( camera->isOrtho() ) {
				drawGrid(*camera,camera->getClipFar()-1.f);
			} else {
				if( client->getEditor()->getEditingMode() != Editor::ENTITIES ) {
					float z = pointerActive ? pointerPos.z : 0;
					drawGrid(*camera,z);
				} else {
					drawGrid(*camera,0.f);
				}
			}
		}

		// reset selected cam in editor
		if( editor && editor->isInitialized() ) {
			if( camera != editor->getEditingCamera() &&
				camera != editor->getMinimapCamera() ) {
				camera->setWin(oldWin);
			}
		}

		// draw debug stuff
		camera->drawDebug();

		// reset GL state
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		int xres = renderer->getXres();
		int yres = renderer->getYres();
		glViewport( 0, 0, xres, yres );
		glScissor( 0, 0, xres, yres );
		glEnable( GL_SCISSOR_TEST );
		ShaderProgram::unmount();

		cameraLightList.clear();
	}
}

void TileWorld::findExits() {
	exit_t exit;
	bool buildingExit = false;

	// find north/south exits
	for( Uint32 c = 0; c < 2; ++c ) {
		for( Uint32 u = 0; u < width; ++u ) {
			Uint32 v = (c == 0) ? 0 : height - 1;
			Tile& tile = tiles[v + u * height];
			if( tile.hasVolume() ) {
				if( buildingExit ) {
					exit_t& exit = exits.getLast()->getData();
					++exit.size.w;
				} else {
					buildingExit = true;
					exit.floorHeight = tile.getFloorHeight();
					exit.side = (v == 0) ? Tile::SIDE_NORTH : Tile::SIDE_SOUTH;
					exit.size.x = u;
					exit.size.w = 1;
					exit.size.y = v;
					exit.size.h = 1;
					exit.id = (Uint32)exits.getSize();
					exits.addNodeLast(exit);
				}
			} else {
				buildingExit = false;
			}
		}
	}

	// find east/west exits
	buildingExit = false;
	for( Uint32 c = 0; c < 2; ++c ) {
		for( Uint32 v = 0; v < height; ++v ) {
			Uint32 u = (c == 0) ? 0 : width - 1;
			Tile& tile = tiles[v + u * height];
			if( tile.hasVolume() ) {
				if( buildingExit ) {
					exit_t& exit = exits.getLast()->getData();
					++exit.size.h;
				} else {
					buildingExit = true;
					exit.floorHeight = tile.getFloorHeight();
					exit.side = (u == 0) ? Tile::SIDE_WEST : Tile::SIDE_EAST;
					exit.size.x = u;
					exit.size.w = 1;
					exit.size.y = v;
					exit.size.h = 1;
					exit.id = (Uint32)exits.getSize();
					exits.addNodeLast(exit);
				}
			} else {
				buildingExit = false;
			}
		}
	}
}

void TileWorld::placeRoom(const TileWorld& world, Uint32 pickedExitIndex, Uint32 x, Uint32 y, Sint32 floorDiff) {
	// copy tiles
	for( Uint32 u = 0; u < world.getWidth(); ++u ) {
		for( Uint32 v = 0; v < world.getHeight(); ++v ) {
			Tile& tile0 = tiles[(v+y) + (u+x) * height];
			const Tile& tile1 = world.getTiles()[v + u * world.getHeight()];
			tile0 = tile1;
			tile0.setFloorHeight(tile0.getFloorHeight() + floorDiff);
			tile0.setCeilingHeight(tile0.getCeilingHeight() + floorDiff);
			tile0.setLocked(true);
		}
	}

	// copy exits
	for( const Node<exit_t>* node = world.getExits().getFirst(); node != nullptr; node = node->getNext() ) {
		const exit_t& exit = node->getData();
		if( exit.id == pickedExitIndex ) {
			continue;
		}
		exit_t newExit = exit;
		newExit.size.x += x;
		newExit.size.y += y;
		newExit.floorHeight += floorDiff;
		newExit.id = (Uint32)exits.getSize();
		exits.addNodeLast(newExit);
	}

	// copy entities
	for( int c=0; c<numBuckets; ++c ) {
		for( const Node<Entity*>* node=world.getEntities(c).getFirst(); node!=nullptr; node=node->getNext() ) {
			Entity* src = node->getData();

			Entity* entity = src->copy(this);

			Vector pos = entity->getPos();
			pos.x += x * Tile::size;
			pos.y += y * Tile::size;
			pos.z += (float)floorDiff;
			entity->setPos(pos);
			entity->setNewPos(pos);
		}
	}
}

void TileWorld::generateObstacleCache()
{
	//TODO:
}

std::future<PathFinder::Path*> TileWorld::findAPath(int startX, int startY, int endX, int endY) {
	return pathFinder.generateAStarPath(startX, startY, endX, endY);
}
