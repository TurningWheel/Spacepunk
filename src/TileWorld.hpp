// TileWorld.hpp

#pragma once

#include "World.hpp"
#include "Tile.hpp"
#include "Rect.hpp"

class Chunk;
class Generator;

class TileWorld : public World {
public:
	TileWorld(bool _clientObj, Uint32 _id, const char* _zone, Uint32 _seed, Uint32 _width, Uint32 _height, const char* _nameStr);
	TileWorld(bool _silent, bool _clientObj, Uint32 _id, Tile::side_t orientation, const char* _filename, Uint32 _width=32, Uint32 _height=32, const char* _nameStr="Untitled World");
	TileWorld(bool _clientObj, Uint32 _id, const char* _zone, const Generator& gen);
	virtual ~TileWorld();

	// exit structure
	struct exit_t {
		Rect<Uint32> size;
		Sint32 floorHeight;
		Tile::side_t side = Tile::SIDE_NORTH;
		Uint32 id = 0;
	};

	// candidate room structure
	struct candidate_t {
		TileWorld* world = nullptr;
		ArrayList<exit_t> exits;
	};

	// const variables
	static const int fileMagicNumberLen = 16;
	static const char* fileMagicNumber;
	static const char* fileMagicNumberSaveOut;

	// post-load world initialization
	// @param empty if the world is empty
	virtual void initialize(bool empty) override;

	// rotate the whole world. orientation is always assumed to be SIDE_EAST
	// @param orientation the direction to rotate it to
	void rotate(Tile::side_t orientation);

	// calculate the width (columns) in the chunks array
	int calcChunksWidth() const;

	// calculate the height (rows) in the chunks array
	int calcChunksHeight() const;

	// clears all geometry selection
	virtual void deselectGeometry();

	// finds all entities within a given radius of the provided point
	// @param origin position to search from
	// @param radius the radius to search in
	// @param outList the list to populate
	// @param flat if true, the search radius is 2-dimensional
	virtual void findEntitiesInRadius( const Vector& origin, float radius, LinkedList<Entity*>& outList, bool flat = false );

	// optimizes all the chunks in the world
	void optimizeChunks();

	// completely rebuilds all the chunks in the world
	void rebuildChunks();

	// draws the world and its contents
	virtual void draw();

	// writes the world contents to a file
	// @param _filename the filename to write to, or blank to use our last filename
	// @param updateFilename if true, our current filename is changed, otherwise, it is not
	// @return true on success, false on failure
	virtual bool saveFile(const char* _filename="", bool updateFilename = false);

	// save/load this object to a file
	// @param file interface to serialize with
	virtual void serialize(FileInterface * file);

	// change the map's dimensions
	// @param left amount of tiles to increase (+) or decrease (-) to the west/left
	// @param right amount of tiles to increase (+) or decrease (-) to the east/right
	// @param up amount of tiles to increase (+) or decrease (-) to the north/up
	// @param down amount of tiles to increase (+) or decrease (-) to the south/down
	void resize(int left, int right, int up, int down);

	// copies a room (world) into this world
	// @param world the world to copy
	// @param pickedExitIndex index of the exit we're joining
	// @param x x-coord to place the room
	// @param y y-coord to place the room
	// @param floorDiff the floor height difference between the two rooms, if any
	void placeRoom(const TileWorld& world, Uint32 pickedExitIndex, Uint32 x, Uint32 y, Sint32 floorDiff = 0);

	// draws all tiles and entities in the world
	// @param camera the camera through which to draw the scene
	// @param light the light by which the scene should be illuminated (or nullptr for no illumination)
	// @param chunkDrawList a list of chunks to draw
	void drawSceneObjects(Camera& camera, const ArrayList<Light*>& lights, const ArrayList<Chunk*>& chunkDrawList);

	// find a random traversible tile. if none are found, outX and outY remain unchanged
	// @param outX x coordinate of random tile (in tiles)
	// @param outY y coordinate of random tile (in tiles)
	// @param height the minimum height of a traversible tile (floor to ceiling)
	void findRandomTile(float height, int& outX, int& outY);

	// getters & setters
	virtual const type_t		getType() const						{ return WORLD_TILES; }
	const Uint32				getWidth() const					{ return width; }
	const Uint32				getHeight() const					{ return height; }
	ArrayList<Tile>&			getTiles()							{ return tiles; }
	const ArrayList<Tile>&		getTiles() const					{ return tiles; }
	Chunk*&						getChunks()							{ return chunks; }
	const LinkedList<exit_t>&	getExits() const					{ return exits; }

	// editing properties
	bool				isSelecting() const					{ return selecting; }
	const Rect<int>&	getSelectedRect() const				{ return selectedRect; }

	void	setSelectedRect(const Rect<int> _selectedRect)	{ selectedRect = _selectedRect; }
	void	setSelecting(const bool _selecting)				{ selecting = _selecting; }

	virtual std::future<PathFinder::Path*> findAPath(int startX, int startY, int endX, int endY) override;

protected:
	// when a new world is spawned, it generates an obstacle map/cache of all static obstacles.
	virtual void generateObstacleCache();

private:
	// draws the editing grid
	// @param camera the camera through which to draw the grid
	// @param z the height of the grid in the world
	void drawGrid(Camera& camera, float z);

	// creates rendering objects for the world grid
	void createGrid();

	// destroys rendering objects for the world grid
	void destroyGrid();
		
	// populate list of exits
	void findExits();

	// generation variables
	LinkedList<exit_t> exits;

	// world dimensions
	Uint32 width=0;
	Uint32 height=0;

	// tiles (world geometry)
	ArrayList<Tile> tiles;
	Chunk* chunks = nullptr;

	// editing variables
	bool selecting=false;		// selecting tiles
	Rect<int> selectedRect;		// tile selection rectangle

	// how many tiles a "large" grid section is
	static const int largeGridSize = 8;

	// editing grid graphics data
	enum buffer_t {
		BUFFER_VERTEX,
		BUFFER_COLOR,
		BUFFER_INDEX,
		BUFFER_MAX
	};
	GLuint vbo[BUFFER_MAX];
	GLuint vao = 0;

	PathFinder& pathFinder;
};