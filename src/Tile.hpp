// Tile.hpp

// A tile is a grid space from a TileWorld structure.
// Walls are automatically defined between tiles of varying elevation.
// Each tile defines vertices and textures for any wall that points towards the tile
// For tiles with no neighboring tiles in any one of the cardinal directions,
// the upper texture is used to texture the wall separating the TileWorld from the void beyond.

// A sloped tile can be sloped in any one of the cardinal directions.
// The slope size determines how much lower the end of the slope descends from the other end.

#pragma once

#include "Main.hpp"

#ifdef PLATFORM_LINUX
#include <btBulletDynamicsCommon.h>
#else
#include <bullet3/btBulletDynamicsCommon.h>
#endif

#define GLM_FORCE_RADIANS
#include <glm/vec3.hpp>

#include "LinkedList.hpp"
#include "Rect.hpp"
#include "String.hpp"
#include "ArrayList.hpp"

class Camera;
class Light;
class TileWorld;
class Chunk;
class ShaderProgram;

class Tile {
public:
	Tile();
	~Tile();

	// the smallest possible size of a leaf tile (ie a single tile)
	static const int size = 128;

	// the floor and ceiling height of a completely solid tile
	static const int solidWallHeight = 0;

	// default texture
	static const char* defaultTexture;

	// four cardinal directions
	enum side_t {
		SIDE_EAST = 0,
		SIDE_SOUTH = 1,
		SIDE_WEST = 2,
		SIDE_NORTH = 3,
		SIDE_TYPE_LENGTH
	};

	// left hand corner, right hand corner (paired with side_t)
	enum corner_t {
		CORNER_LEFT = 0,
		CORNER_RIGHT = 1,
		CORNER_TYPE_LENGTH
	};

	// shader vars
	struct shadervars_t {
		ArrayList<GLfloat> customColorR = { 1.f, 0.f, 0.f };
		ArrayList<GLfloat> customColorG = { 0.f, 1.f, 0.f };
		ArrayList<GLfloat> customColorB = { 0.f, 0.f, 1.f };
	};

	// geometry vertex
	struct vertex_t {
		glm::vec3 pos;
	};

	// getters & setters
	Uint32					getX() const							{ return x; }
	Uint32					getY() const							{ return y; }
	Chunk*&					getChunk()								{ return chunk; }
	const Sint32&			getCeilingHeight() const				{ return ceilingHeight; }
	const Sint32&			getFloorHeight() const					{ return floorHeight; }
	Uint32					getUpperTexture(side_t side) const		{ return upperTextures[side]; }
	Uint32					getLowerTexture(side_t side) const		{ return lowerTextures[side]; }
	Uint32					getCeilingTexture() const				{ return ceilingTexture; }
	Uint32					getFloorTexture() const					{ return floorTexture; }
	const side_t&			getCeilingSlopeSide() const				{ return ceilingSlopeSide; }
	const Sint32&			getCeilingSlopeSize() const				{ return ceilingSlopeSize; }
	const side_t&			getFloorSlopeSide() const				{ return floorSlopeSide; }
	const Sint32&			getFloorSlopeSize() const				{ return floorSlopeSize; }
	LinkedList<vertex_t>&			getCeilingVertices()					{ return ceilingVertices; }
	LinkedList<vertex_t>&			getFloorVertices()						{ return floorVertices; }
	LinkedList<vertex_t>&			getLowerVertices(const side_t side)		{ return lowerVertices[side]; }
	LinkedList<vertex_t>&			getUpperVertices(const side_t side)		{ return upperVertices[side]; }
	const bool				isChanged() const						{ return changed; }
	const size_t			getNumVertices() const					{ return numVertices; }
	bool					isLocked() const						{ return locked; }
	const shadervars_t&		getShaderVars() const					{ return shaderVars; }

	void	setX(Uint32 _x)													{ x = _x; }
	void	setY(Uint32 _y)													{ y = _y; }
	void	setWorld(TileWorld& _world)										{ world = &_world; }
	void	setChunk(Chunk& _chunk)											{ chunk = &_chunk; }
	void	setCeilingHeight(Sint32 _ceilingHeight)							{ ceilingHeight = _ceilingHeight; }
	void	setFloorHeight(Sint32 _floorHeight)								{ floorHeight = _floorHeight; }
	void	setUpperTexture(side_t side, Uint32 texture)					{ upperTextures[side] = texture; }
	void	setLowerTexture(side_t side, Uint32 texture)					{ lowerTextures[side] = texture; }
	void	setCeilingTexture(Uint32 texture)								{ ceilingTexture = texture; }
	void	setFloorTexture(Uint32 texture)									{ floorTexture = texture; }
	void	setCeilingSlopeSide(side_t side)								{ ceilingSlopeSide = side; }
	void	setCeilingSlopeSize(Sint32 size)								{ ceilingSlopeSize = size; }
	void	setFloorSlopeSide(side_t side)									{ floorSlopeSide = side; }
	void	setFloorSlopeSize(Sint32 size)									{ floorSlopeSize = size; }
	void	setDynamicsWorld(btDiscreteDynamicsWorld& _dynamicsWorld)		{ dynamicsWorld = &_dynamicsWorld; }
	void	setChanged(bool _changed)										{ changed = _changed; }
	void	setLocked(bool _locked)											{ locked = _locked; }
	void	setShaderVars(const shadervars_t& src)							{ shaderVars = src; }

	// load the shader that we will use to draw the tile with
	// @param world: the world object that contains the scene
	// @param camera: the camera object that will be used to render the scene
	// @param light: the light object to illuminate the scene with, or nullptr for no light
	// @return the shader program that was loaded, or nullptr if the shader failed to load
	static ShaderProgram* loadShader(TileWorld& world, Camera& camera, Light* light);

	// cleans out our vertex lists
	void cleanVertexBuffers();

	// rebuild the list of vertices for the ceiling of this tile
	void compileCeilingVertices();

	// rebuild the list of vertices for the floor of this tile
	void compileFloorVertices();

	// rebuild the list of vertices for the given lower wall of this tile
	// @param neighbor: the neighboring tile to build the wall against
	// @param side: the vertices for which wall to build
	void compileUpperVertices(Tile& neighbor, const side_t side);

	// rebuild the list of vertices for the given lower wall of this tile
	// @param neighbor: the neighboring tile to build the wall against
	// @param side: the vertices for which wall to build
	void compileLowerVertices(Tile& neighbor, const side_t side);

	// calculates the number of vertices in the tile
	// @return the number of vertices for all surfaces in the tile
	size_t calculateVertices() const;

	// determines if the tile has any space or not
	// @return true if the tile has any visible surfaces, false otherwise
	bool hasVolume() const;

	// rebuilds the vertex buffers for the tile
	void buildBuffers();

	// modify the z value of the given vector by the amount of slope exhibited by the ceiling
	// @param vec: the vector to modify
	void setCeilingSlopeHeightForVec(glm::vec3& vec);

	// modify the z value of the given vector by the amount of slope exhibited by the floor
	// @param vec: the vector to modify
	void setFloorSlopeHeightForVec(glm::vec3& vec);

	// get the height of the given upper wall
	// @param neighbor: the neighboring tile to test against
	// @param side: the height of which wall to retrieve
	// @param corner: the corner of the wall to calculate height for
	// @return the height of the wall corner in map units
	Sint32 upperWallHeight(const Tile& neighbor, const side_t side, const corner_t corner);

	// get the height of the given lower wall
	// @param neighbor: the neighboring tile to test against
	// @param side: the height of which wall to retrieve
	// @param corner: the corner of the wall to calculate height for
	// @return the height of the wall corner in map units
	Sint32 lowerWallHeight(const Tile& neighbor, const side_t side, const corner_t corner);

	// recompile the physics mesh for this tile
	void compileBulletPhysicsMesh();

	// finds the tile neighboring this one, if one exists
	// @param side: the particular neighbor we are looking for
	// @return a pointer to the tile, or nullptr if the tile does not exist
	Tile* findNeighbor(const side_t side);

	// converts an int to one of the tile's several vertex lists
	// @param i: the int to convert. accepted values:
	// 0 = ceilingVertices
	// 1 = floorVertices
	// 2-5 = upperVertices(SIDE_EAST) ... upperVertices(SIDE_NORTH)
	// 6-9 = lowerVertices(SIDE_SOUTH) ... lowerVertices(SIDE_NORTH)
	// @return a pointer to the specified list of vertices, or nullptr if the given int doesn't match with a list
	LinkedList<vertex_t>* intForVertices( const int i );

	// determines if the tile is selected by testing it against the selection rectangle in its parent world
	// @return true if the tile is selected, false otherwise
	bool selected() const;

	// conversion to const char*
	void operator=(const Tile& src) {
		// ceiling data
		ceilingHeight = src.getCeilingHeight();
		ceilingTexture = src.getCeilingTexture();
		ceilingSlopeSide = src.getCeilingSlopeSide();
		ceilingSlopeSize = src.getCeilingSlopeSize();

		// floor data
		floorHeight = src.getFloorHeight();
		floorTexture = src.getFloorTexture();
		floorSlopeSide = src.getFloorSlopeSide();
		floorSlopeSize = src.getFloorSlopeSize();

		// wall data
		for( Uint32 c = 0; c < SIDE_TYPE_LENGTH; ++c ) {
			side_t side = static_cast<side_t>(c);
			upperTextures[side] = src.getUpperTexture(side);
			lowerTextures[side] = src.getLowerTexture(side);
		}
	}

	// save/load this object to a file
	// @param file interface to serialize with
	void serialize(FileInterface * file);

private:
	TileWorld* world = nullptr; // the parent world object
	Chunk* chunk = nullptr; // the parent chunk object

	// the x/y index in the parent world's tile tree
	Uint32 x = 0;
	Uint32 y = 0;

	// floor/ceiling height
	Sint32 ceilingHeight = 0;
	Sint32 floorHeight = 0;

	// textures
	Uint32 upperTextures[SIDE_TYPE_LENGTH];
	Uint32 lowerTextures[SIDE_TYPE_LENGTH];
	Uint32 ceilingTexture;
	Uint32 floorTexture;

	// shader vars
	shadervars_t shaderVars;

	// slope data
	side_t ceilingSlopeSide = SIDE_EAST;
	Sint32 ceilingSlopeSize = 0;
	side_t floorSlopeSide = SIDE_EAST;
	Sint32 floorSlopeSize = 0;

	// editing data
	bool changed = false;

	// generation data
	bool locked = false;

	// bullet physics objects
	btDiscreteDynamicsWorld* dynamicsWorld = nullptr;
	btTriangleMesh* triMesh = nullptr;
	btCollisionShape* triMeshShape = nullptr;
	btDefaultMotionState* motionState = nullptr;
	btRigidBody* rigidBody = nullptr;

	// vertex lists
	size_t numVertices = 0;
	LinkedList<vertex_t> ceilingVertices;
	LinkedList<vertex_t> floorVertices;
	LinkedList<vertex_t> upperVertices[SIDE_TYPE_LENGTH];
	LinkedList<vertex_t> lowerVertices[SIDE_TYPE_LENGTH];
};
