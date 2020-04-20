//! @file TileWorld.hpp

#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include <btBulletDynamicsCommon.h>

#include "LinkedList.hpp"
#include "Node.hpp"
#include "Vector.hpp"
#include "Console.hpp"
#include "Path.hpp"
#include "Shadow.hpp"
#include "Quaternion.hpp"

class Script;
class Entity;
class Game;
class BBox;

//! The World class basically represents a Level in the game. It is a self-contained universe predominantly filled with entities.
//! The World class itself is abstract and there are multiple world types; the current version of the engine uses BasicWorld
//! pretty much exclusively. Historically, TileWorld was the main world type.
class World {
public:
	World(Game* _game);
	virtual ~World();

	//! data owned by every physics object in a world
	struct physics_manifest_t {
		World* world = nullptr;
		Entity* entity = nullptr;
		BBox* bbox = nullptr;
	};

	//! world type
	enum type_t {
		WORLD_INVALID,
		WORLD_TILES,
		WORLD_SECTORS,
		WORLD_BASIC
	};

	//! hit structure (for collisions)
	struct hit_t {
		Vector pos;
		Vector normal;
		physics_manifest_t* manifest = nullptr;
	};

	//! file type
	enum filetype_t {
		FILE_BINARY,
		FILE_JSON,
		FILE_WLD,		//! deprecated
		FILE_MAX
	};

	//! laser type
	struct laser_t {
		Vector start, end;
		glm::vec4 color;
		float size;
		float life;
		float maxLife;
	};

	//! const variables
	static const char* fileExtensions[FILE_MAX];

	//! invalid uid for any entity
	static const Uint32 nuid;

	//! post-load world initialization
	//! @param empty if the world is empty
	virtual void initialize(bool empty);

	//! produces a list of all selected entities in the world
	void getSelectedEntities(LinkedList<Entity*>& outResult);

	//! finds the entity with the given uid in this world
	//! @param uid The uid of the entity to be found
	//! @return a pointer to the entity, or nullptr if the entity could not be found
	Entity* uidToEntity(const Uint32 uid);

	//! selects or deselects the entity with the given uid
	//! @param uid the uid of the entity to select
	//! @param b if true, the entity is selected; if false, it is deselected
	//! @return true if the entity could be found, false otherwise
	const bool selectEntity(const Uint32 uid, const bool b);

	//! spawn an entity at the given location/orientation
	//! @param name The entity's name as given in the editor (not the filename)
	//! @param pos Position in world space (x, y, z)
	//! @param ang Rotation in world space (yaw, pitch, roll)
	//! @return the newly spawned entity
	Entity* spawnEntity(const char* name, const Vector& pos, const Rotation& ang);

	//! selects or deselects all entities in the world
	//! @param b if true, all entities are selected; if false, they are deselected
	void selectEntities(const bool b);

	//! clears all geometry selection
	virtual void deselectGeometry() = 0;

	//! perform a line test (raytrace) through the world which stops at first hit object
	//! @param origin the starting point of the ray
	//! @param dest the ending point of the ray
	//! @return a hit_t structure containing information on the hit object
	hit_t lineTrace(const Vector& origin, const Vector& dest);

	//! perform a line test (raytrace) through the world and gets a list of all objects in the ray's path
	//! @param origin the starting point of the ray
	//! @param dest the ending point of the ray
	//! @param outResult the list which will contain all of the hit_t structures (sorted nearest to furthest)
	void lineTraceList(const Vector& origin, const Vector& dest, LinkedList<hit_t>& outResult);

	//! perform a line test (raytrace) through the world, skipping entities
	//! @param origin the starting point of the ray
	//! @param dest the ending point of the ray
	//! @return a hit_t structure containing information on the hit object
	hit_t lineTraceNoEntities(const Vector& origin, const Vector& dest);

	//! get a list of all the entities with the given name
	//! @param name The name of the entities
	//! @return a list of entities
	ArrayList<Entity*> getEntitiesByName(const char* name);

	//! perform a convex sweep test through the world which stops at first hit object
	//! @param shape The convex shape to sweep with
	//! @param originPos The starting position of the shape
	//! @param originAng The starting angle of the shape
	//! @param destPos The ending position of the shape
	//! @param destAng The ending angle of the shape
	//! @return a hit_t structure containing information on the hit object
	const hit_t convexSweep(const btConvexShape* shape, const Vector& originPos, const Quaternion& originAng, const Vector& destPos, const Quaternion& destAng);

	//! perform a convex sweep test through the world which gets a list of all overlapped objects
	//! @param shape The convex shape to sweep with
	//! @param originPos The starting position of the shape
	//! @param originAng The starting angle of the shape
	//! @param destPos The ending position of the shape
	//! @param destAng The ending angle of the shape
	//! @param outResult the list which will contain all of the hit_t structures (sorted nearest to furthest)
	void convexSweepList(const btConvexShape* shape, const Vector& originPos, const Quaternion& originAng, const Vector& destPos, const Quaternion& destAng, LinkedList<hit_t>& outResult);

	//! finds all entities within a given radius of the provided point
	//! @param origin position to search from
	//! @param radius the radius to search in
	//! @param outList the list to populate
	//! @param flat if true, the search radius is 2-dimensional
	virtual void findEntitiesInRadius(const Vector& origin, float radius, LinkedList<Entity*>& outList, bool flat = false) = 0;

	//! generates a tilemap using the dungeon generator
	//! @param filename The filename to the generator options json
	//! @return a tilemap
	ArrayList<int> generateDungeon(const char* filename) const;

	//! generate a new UID for a world object and advance the UID counter
	//! @return the fresh UID
	Uint32 getNewUID() { ++uids; return uids; }

	//! draws the world and its contents
	virtual void draw() = 0;

	//! preprocess world events
	virtual void preProcess();

	//! process world events
	virtual void process();

	//! postprocess world events
	virtual void postProcess();

	//! writes the world contents to a file
	//! @param _filename the filename to write to, or blank to use our last filename
	//! @param updateFilename if true, our current filename is changed, otherwise, it is not
	//! @return true on success, false on failure
	virtual bool saveFile(const char* _filename = "", bool updateFilename = false) = 0;

	//! update filename and shortname members
	//! @param _filename the new full filename for the world
	void changeFilename(const char* _filename);

	//! finds all selected entities in the world and places them in a list
	//! @param outList the list to populate
	void findSelectedEntities(LinkedList<Entity*>& outList);

	//! add a laser to this level
	//! @param start The start point of the laser
	//! @param end The end point of the laser
	//! @param color The laser's color
	//! @param size The width of the laser
	//! @param life The lifespan of the laser in ticks (60 = 1sec)
	laser_t& addLaser(const Vector& start, const Vector& end, const glm::vec4& color, float size, float life);

	virtual const type_t		getType() const = 0;
	Game*						getGame() { return game; }
	const Uint32				getTicks() const { return ticks; }
	const String&				getFilename() const { return filename; }
	const String&				getShortname() const { return shortname; }
	const String&				getNameStr() const { return nameStr; }
	Map<Uint32, Entity*>&		getEntities() { return entities; }
	const Map<Uint32, Entity*>&	getEntities() const { return entities; }
	btDiscreteDynamicsWorld*&	getBulletDynamicsWorld() { return bulletDynamicsWorld; }
	const bool					isClientObj() const { return clientObj; }
	const bool					isServerObj() const { return !clientObj; }
	const String&				getZone() const { return zone; }
	Uint32						getID() const { return id; }
	const filetype_t			getFiletype() const { return filetype; }
	Shadow&						getDefaultShadow() { return defaultShadow; }
	const Shadow&				getDefaultShadow() const { return defaultShadow; }
	void						setMaxUID(Uint32 uid) { uids = std::max(uids, uid); }

	bool				isPointerActive() const { return pointerActive; }
	const Vector&		getPointerPos() const { return pointerPos; }
	bool				isGenerated() const { return generated; }
	Uint32				getSeed() const { return seed; }
	bool				isLoaded() const { return loaded; }
	bool				isGridVisible() const { return gridVisible; }
	bool				isShowTools() const;

	void	setPointerActive(bool _pointerActive) { pointerActive = _pointerActive; }
	void	setPointerPos(const Vector& _pointerPos) { pointerPos = _pointerPos; }
	void	setShowTools(const bool _showTools) { showTools = _showTools; }
	void	setNameStr(const char* _nameStr) { nameStr = _nameStr; }
	void	setGridVisible(const bool _gridVisible) { gridVisible = _gridVisible; }

	virtual std::future<PathFinder::Path*> findAPath(int startX, int startY, int endX, int endY) = 0;

protected:
	Game* game = nullptr;     //!< the game sim we belong to (if any)
	Script* script = nullptr; //!< scripting engine

	bool silent = false;	//!< if true, disables some logging
	bool clientObj = false;	//!< if true, this world exists on the client, otherwise, it exists on the server
	bool generated = false;	//!< has seeded elements
	Uint32 id = 0;			//!< non-unique identifier
	Uint32 seed = 0;		//!< prng seed

	String zone;						//!< room family
	String filename;					//!< full path
	String shortname;					//!< shortened filename (without folder path)
	String nameStr;						//!< descriptive name (eg 'Hub World')
	filetype_t filetype = FILE_BINARY;	//!< filetype (binary vs json vs wld)

	Uint32 ticks = 0;			//!< lifespan of the world

	//! entities
	Uint32 uids = 0;
	Map<Uint32, Entity*> entities;
	ArrayList<Entity*> entitiesToInsert;

	//! lasers
	ArrayList<laser_t> lasers;

	//! bullet physics handlers
	btBroadphaseInterface* bulletBroadphase = nullptr;
	btDefaultCollisionConfiguration* bulletCollisionConfiguration = nullptr;
	btCollisionDispatcher* bulletDispatcher = nullptr;
	btSequentialImpulseConstraintSolver* bulletSolver = nullptr;
	btDiscreteDynamicsWorld* bulletDynamicsWorld = nullptr;

	//! editing variables
	bool loaded = false;			//!< if true, world successfully loaded from a file
	bool showTools = true;			//!< when true, then grid, cursor, lights, etc. are drawn; otherwise, they are not
	bool pointerActive = false;		//!< if true, pointer is visible/usable
	Vector pointerPos;				//!< pointer location
	bool gridVisible = true;		//!< if true, editing grid is visible

	ArrayList<uint32_t> pathMap;

	//! when a new world is spawned, it generates an obstacle map/cache of all static obstacles.
	virtual void generateObstacleCache() = 0;

	static void bulletCollisionCallback(btBroadphasePair& pair, btCollisionDispatcher& dispatcher, const btDispatcherInfo& info);

	//! shadow map stuff
	Shadow defaultShadow;
};

extern Cvar cvar_showEdges;
extern Cvar cvar_showVerts;
extern Cvar cvar_renderFullbright;
extern Cvar cvar_depthOffset;
extern Cvar cvar_renderCull;