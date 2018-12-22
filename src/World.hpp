// TileWorld.hpp

#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#ifdef PLATFORM_LINUX
#include <btBulletDynamicsCommon.h>
#else
#include <bullet3/btBulletDynamicsCommon.h>
#endif

#include "LinkedList.hpp"
#include "Node.hpp"
#include "Vector.hpp"
#include "Console.hpp"
#include "Path.hpp"
#include "Shadow.hpp"

class Script;
class Entity;

class World {
public:
	World();
	virtual ~World();

	// world type
	enum type_t {
		WORLD_INVALID,
		WORLD_TILES,
		WORLD_SECTORS
	};

	// hit structure (for collisions)
	struct hit_t {
		Vector pos;
		Vector normal;
		Uint32 index = nuid;
		Uint32 index2 = nuid;
		void* pointer = nullptr;

		bool hitEntity = false;
		bool hitTile = false;
		bool hitSector = false;
		bool hitSectorVertex = false;
	};

	// file type
	enum filetype_t {
		FILE_BINARY,
		FILE_JSON,
		FILE_WLD,		// deprecated
		FILE_MAX
	};

	// laser type
	struct laser_t {
		Vector start, end;
		glm::vec4 color;
		float size;
		float life;
		float maxLife;
	};

	// const variables
	static const int numBuckets = 128;
	static const char* fileExtensions[FILE_MAX];

	// invalid uid for any entity
	static const Uint32 nuid;

	// post-load world initialization
	// @param empty: if the world is empty
	virtual void initialize(bool empty);

	// produces a list of all selected entities in the world
	void getSelectedEntities(LinkedList<Entity*>& outResult);

	// finds the entity with the given uid in this world
	// @param uid: The uid of the entity to be found
	// @return a pointer to the entity, or nullptr if the entity could not be found
	Entity* uidToEntity(const Uint32 uid);

	// selects or deselects the entity with the given uid
	// @param uid: the uid of the entity to select
	// @param b: if true, the entity is selected; if false, it is deselected
	// @return true if the entity could be found, false otherwise
	const bool selectEntity(const Uint32 uid, const bool b);

	// selects or deselects all entities in the world
	// @param b: if true, all entities are selected; if false, they are deselected
	void selectEntities(const bool b);

	// clears all geometry selection
	virtual void deselectGeometry() = 0;

	// perform a line test (raytrace) through the world which stops at first hit object
	// @param origin: the starting point of the ray
	// @param dest: the ending point of the ray
	// @return a hit_t structure containing information on the hit object
	const hit_t lineTrace( const Vector& origin, const Vector& dest );

	// perform a line test (raytrace) through the world and gets a list of all objects in the ray's path
	// @param origin: the starting point of the ray
	// @param dest: the ending point of the ray
	// @param outResult: the list which will contain all of the hit_t structures (sorted nearest to furthest)
	void lineTraceList( const Vector& origin, const Vector& dest, LinkedList<hit_t>& outResult );

	// perform a line test (raytrace) through the world, skipping entities
	// @param origin: the starting point of the ray
	// @param dest: the ending point of the ray
	// @return a hit_t structure containing information on the hit object
	const hit_t lineTraceNoEntities( const Vector& origin, const Vector& dest );

	// finds all entities within a given radius of the provided point
	// @param origin: position to search from
	// @param radius: the radius to search in
	// @param outList: the list to populate
	// @param flat: if true, the search radius is 2-dimensional
	virtual void findEntitiesInRadius( const Vector& origin, float radius, LinkedList<Entity*>& outList, bool flat = false ) = 0;

	// generate a new UID for a world object and advance the UID counter
	// @return the fresh UID
	Uint32 getNewUID() { ++uids; return uids; }

	// draws the world and its contents
	virtual void draw() = 0;

	// process world events
	virtual void process();

	// writes the world contents to a file
	// @param _filename: the filename to write to, or blank to use our last filename
	// @param updateFilename: if true, our current filename is changed, otherwise, it is not
	// @return true on success, false on failure
	virtual bool saveFile(const char* _filename="", bool updateFilename = false) = 0;

	// update filename and shortname members
	// @param _filename: the new full filename for the world
	void changeFilename(const char* _filename);

	// finds all selected entities in the world and places them in a list
	// @param outList: the list to populate
	void findSelectedEntities(LinkedList<Entity*>& outList);

	// add a laser to this level
	// @param start The start point of the laser
	// @param end The end point of the laser
	// @param color The laser's color
	// @param size The width of the laser
	// @param life The lifespan of the laser in ticks (60 = 1sec)
	laser_t& addLaser(const Vector& start, const Vector& end, const glm::vec4& color, float size, float life);

	// getters & setters
	virtual const type_t		getType() const = 0;
	const Uint32				getTicks() const						{ return ticks; }
	const String&				getFilename() const						{ return filename; }
	const String&				getShortname() const					{ return shortname; }
	const String&				getNameStr() const						{ return nameStr; }
	LinkedList<Entity*>&		getEntities(const Uint32 index)			{ return entities[index]; }
	const LinkedList<Entity*>&	getEntities(const Uint32 index) const	{ return entities[index]; }
	btDiscreteDynamicsWorld*&	getBulletDynamicsWorld()				{ return bulletDynamicsWorld; }
	const bool					isClientObj() const						{ return clientObj; }
	const bool					isServerObj() const						{ return !clientObj; }
	const String&				getZone() const							{ return zone; }
	Uint32						getID() const							{ return id; }
	const filetype_t			getFiletype() const						{ return filetype; }
	Entity*						getShadowCamera()						{ return shadowCamera; }

	// editing properties
	bool				isPointerActive() const				{ return pointerActive; }
	const Vector&		getPointerPos() const				{ return pointerPos; }
	bool				isGenerated() const					{ return generated; }
	Uint32				getSeed() const						{ return seed; }
	bool				isLoaded() const					{ return loaded; }
	bool				isGridVisible() const				{ return gridVisible; }
	bool				isShowTools() const;

	void	setPointerActive(bool _pointerActive)			{ pointerActive = _pointerActive; }
	void	setPointerPos(const Vector& _pointerPos)		{ pointerPos = _pointerPos; }
	void	setShowTools(const bool _showTools)				{ showTools = _showTools; }
	void	setNameStr(const char* _nameStr)				{ nameStr = _nameStr; }
	void	setGridVisible(const bool _gridVisible)			{ gridVisible = _gridVisible; }

	virtual std::future<PathFinder::Path*> findAPath(int startX, int startY, int endX, int endY) = 0;

protected:
	Script* script = nullptr; // scripting engine

	bool silent = false;	// if true, disables some logging
	bool clientObj = false;	// if true, this world exists on the client, otherwise, it exists on the server
	bool generated = false;	// has seeded elements
	Uint32 id = 0;			// non-unique identifier
	Uint32 seed = 0;		// prng seed

	String zone;						// room family
	String filename;					// full path
	String shortname;					// shortened filename (without folder path)
	String nameStr;						// descriptive name (eg 'Hub World')
	filetype_t filetype = FILE_BINARY;	// filetype (binary vs json vs wld)

	Uint32 ticks=0;			// lifespan of the world

	// entities
	Uint32 uids=0;
	LinkedList<Entity*> entities[numBuckets];

	// lasers
	ArrayList<laser_t> lasers;

	// bullet physics handlers
	btBroadphaseInterface* bulletBroadphase = nullptr;
	btDefaultCollisionConfiguration* bulletCollisionConfiguration = nullptr;
	btCollisionDispatcher* bulletDispatcher = nullptr;
	btSequentialImpulseConstraintSolver* bulletSolver = nullptr;
	btDiscreteDynamicsWorld* bulletDynamicsWorld = nullptr;

	// editing variables
	bool loaded=false;			// if true, world successfully loaded from a file
	bool showTools=true;		// when true, then grid, cursor, lights, etc. are drawn; otherwise, they are not
	bool pointerActive=false;	// if true, pointer is visible/usable
	Vector pointerPos;			// pointer location
	bool gridVisible=true;		// if true, editing grid is visible

	ArrayList<uint32_t> pathMap;

	// when a new world is spawned, it generates an obstacle map/cache of all static obstacles.
	virtual void generateObstacleCache() = 0;

	// shadow map stuff
	Entity* shadowCamera = nullptr;
};

extern Cvar cvar_showEdges;
extern Cvar cvar_showVerts;
