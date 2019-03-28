// Entity.hpp

// An Entity is any 3D object residing in a World which runs a script.
// Entities should generally not be used for particles.

#pragma once

#include "Main.hpp"

#include <btBulletDynamicsCommon.h>

#include "LinkedList.hpp"
#include "Node.hpp"
#include "Angle.hpp"
#include "Vector.hpp"
#include "Rect.hpp"
#include "String.hpp"
#include "Component.hpp"
#include "World.hpp"
#include "Map.hpp"
#include "Path.hpp"
#include "Tile.hpp"
#include "Script.hpp"

#include <memory>

class Engine;
class World;
class Light;
class ShaderProgram;
class Camera;
class Player;
class BBox;
class Packet;

class Entity {
public:
	// entity flags
	enum flag_t {
		FLAG_VISIBLE = 1 << 0,
		FLAG_PASSABLE = 1 << 1,
		FLAG_LOCAL = 1 << 2,
		FLAG_UPDATE = 1 << 3,
		FLAG_GENIUS = 1 << 4,
		FLAG_OVERDRAW = 1 << 5,
		FLAG_SPRITE = 1 << 6,
		FLAG_ALLOWTRACE = 1 << 7,
		FLAG_SHADOW = 1 << 8,
		FLAG_FULLYLIT = 1 << 9,
		FLAG_GLOWING = 1 << 10,
		FLAG_DEPTHFAIL = 1 << 11,
		FLAG_OCCLUDE = 1 << 12,
		FLAG_INTERACTABLE = 1 << 13,
		FLAG_STATIC = 1 << 14,
		FLAG_NUM = 15
	};
	static const char* flagStr[static_cast<int>(flag_t::FLAG_NUM)];
	static const char* flagDesc[static_cast<int>(flag_t::FLAG_NUM)];

	// entity sorting filter
	enum sort_t {
		SORT_ANY,
		SORT_CHAR,
		SORT_DECOR,
		SORT_INTERACT,
		SORT_ITEM,
		SORT_LIGHT,
		SORT_TRIGGER,
		SORT_MAX
	};
	static const char* sortStr[SORT_MAX];

	// editor definition
	struct def_t;

	struct listener_t {
		listener_t(void* _entry):
			entry(_entry) {}

		void onDeleted();
		void onChangeColor(bool selected, bool highlighted);
		void onChangeName(const char* name);

		// Frame::entry_t*
		void* entry = nullptr;
	};

	Entity(World* _world, Uint32 uid = UINT32_MAX);
	virtual ~Entity();

	// getters & setters
	const String&						getName() const						{ return name; }
	const Uint32&						getUID() const						{ return uid; }
	const Uint32&						getTicks() const					{ return ticks; }
	const Vector&						getOldPos() const					{ return oldPos; }
	const Vector&						getPos() const						{ return pos; }
	const Vector&						getNewPos() const					{ return newPos; }
	const Vector&						getVel() const						{ return vel; }
	const Angle&						getAng() const						{ return ang; }
	const Angle&						getNewAng() const					{ return newAng; }
	const Angle&						getRot() const						{ return rot; }
	const glm::mat4&					getMat() const						{ return mat; }
	const char*							getScriptStr() const				{ return scriptStr.get(); }
	const bool							isToBeDeleted() const				{ return toBeDeleted; }
	const Vector&						getScale() const					{ return scale; }
	const Uint32&						getFlags() const					{ return flags; }
	const Map<String, String>&			getKeyValues() const				{ return keyvalues; }
	const bool							isFlag(const flag_t flag) const		{ return ((flags&static_cast<Uint32>(flag))!=0); }
	const bool							isShouldSave() const				{ return shouldSave; }
	World*								getWorld()							{ return world; }
	const World*						getWorld() const					{ return world; }
	Player*								getPlayer()							{ return player; }
	const bool&							isFalling()							{ return falling; }
	Uint32								getLastUpdate()						{ return lastUpdate; }
	const char*							getDefName() const					{ return defName.get(); }
	Uint32								getDefIndex() const					{ return defIndex; }
	const ArrayList<Component*>&		getComponents() const				{ return components; }
	ArrayList<Component*>&				getComponents()						{ return components; }
	const sort_t&						getSort() const						{ return sort; }
	Sint32								getCurrentCX() const				{ return currentCX; }
	Sint32								getCurrentCY() const				{ return currentCY; }
	int									getCurrentTileX() const				{ return static_cast<int>(getPos().x) / Tile::size; }
	int									getCurrentTileY() const				{ return static_cast<int>(getPos().y) / Tile::size; }
	int									getCurrentTileZ() const				{ return static_cast<int>(getPos().z) / Tile::size; }
	bool								isPathRequested() const				{ return pathRequested; }
	const Vector&						getPathNodePosition() const			{ return pathNode; }
	const Vector&						getPathNodeDir() const				{ return pathDir; }
	bool								hasPath() const						{ return path != nullptr; }
	const Entity*						getAnchor() const					{ return anchor; }
	const Vector&						getOffset() const					{ return offset; }

	void					setName(const char* _name)						{ name = _name; if(listener) listener->onChangeName(name); }
	void					setMat(const glm::mat4& _mat)					{ if( mat != _mat ) { mat = _mat; updateNeeded = true; matSet = true; } }
	void					setPos(const Vector& _pos)						{ if( pos != _pos ) { pos = _pos; updateNeeded = true; matSet = false; } }
	void					setOldPos(const Vector& _oldPos)				{ oldPos = _oldPos; }
	void					setVel(const Vector& _vel)						{ vel = _vel; }
	void					setNewPos(const Vector& _newPos)				{ newPos = _newPos; }
	void					setAng(const Angle& _ang)						{ if( ang != _ang ) { ang = _ang; updateNeeded = true; matSet = false; } }
	void					setRot(const Angle& _rot)						{ rot = _rot; }
	void					setNewAng(const Angle& _newAng)					{ newAng = _newAng; }
	void					setScriptStr(const char* _scriptStr);
	void					setScale(const Vector& _scale)					{ if( scale != _scale ) { scale = _scale; updateNeeded = true; matSet = false; } }
	void					setFlags(const Uint32 _flags)					{ flags = _flags; }
	void					setFlag(const Uint32 flag)						{ flags |= flag; }
	void					resetFlag(const Uint32 flag)					{ flags &= ~flag; }
	void					toggleFlag(const Uint32 flag)					{ flags ^= flag; }
	void					setShouldSave(const bool _shouldSave)			{ shouldSave = _shouldSave; }
	void					setPlayer(Player* _player)						{ player = _player; }
	void					setFalling(const bool b)						{ falling = b; }
	void					setLastUpdate(const Uint32 _lastUpdate)			{ lastUpdate = _lastUpdate; }
	void					setDefName(const char* _defName)				{ defName = _defName; }
	void					setDefIndex(Uint32 _defIndex)					{ defIndex = _defIndex; }
	void					setSort(sort_t _sort)							{ sort = _sort; }

	// editor properties

	const bool	isSelected() const							{ return selected; }
	const bool	isHighlighted() const						{ return highlighted; }

	void	setSelected(const bool _selected)				{ selected = _selected; if(listener) listener->onChangeColor(selected, highlighted); }
	void	setHighlighted(const bool _highlighted)			{ highlighted = _highlighted; if(listener) listener->onChangeColor(selected, highlighted); }

	// get game sim that we are living in (if any)
	// @return the Game* that we are in, or nullptr if we aren't in a game
	Game* getGame();

	// add to level navigator in editor
	void addToEditorList();

	// send a signal from server/client or vice versa requesting they run a function
	// @param funcName the name of the function to remote execute
	void remoteExecute(const char* funcName, const Script::Args& args);

	// fill out an update packet for this entity
	// @param packet the packet to fill out
	void updatePacket(Packet& packet) const;

	// run a script function with the given name
	// @param funcName the name of the function to execute
	void dispatch(const char* funcName, Script::Args& args);

	// removes the entity from the current simulation
	void remove();

	// delete occlusion data for self and children
	void deleteAllVisMaps();

	// finds all entities within a given radius to this entity
	// @param radius the radius to search in
	// @param outList the list to populate
	void findEntitiesInRadius(float radius, LinkedList<Entity*>& outList) const;

	// set the specified key value pair
	// @param key the key to insert or modify
	// @param value the value to set
	void setKeyValue(const char* key, const char* value);

	// remove the specified key value pair
	// @param key the key to remove
	void deleteKeyValue(const char* key);

	// get the specified key value as a string
	// @param key the key to search for
	// @return the value associated with the key, or "" if it could not be found
	const char* getKeyValueAsString(const char* key) const;

	// get the specified key value as a float
	// @param key the key to search for
	// @return the value associated with the key, or 0.f if it could not be found
	float getKeyValueAsFloat(const char* key) const;

	// get the specified key value as an int
	// @param key the key to search for
	// @return the value associated with the key, or 0 if it could not be found
	int getKeyValueAsInt(const char* key) const;

	// same as getKeyValueAsString()
	// @param key the key to search for
	// @return the value associated with the key, or "" if it could not be found
	const char* getKeyValue(const char* key) const;

	// runs the entity's assigned script, if any
	virtual void process();

	// mark the object to be transfered into another world
	// @param _world the world to move the entity into
	// @param _anchor the entity we need to remain attached to during the move
	// @param _offset offset from the anchor if one exists
	void insertIntoWorld(World* _world, const Entity* _anchor = nullptr, const Vector& _offset = Vector(0.f));

	// finish transfering the object into another world
	void finishInsertIntoWorld();

	// draws the entity
	// @param camera the camera through which to draw the entity
	// @param light the light by which the entity should be illuminated (or nullptr for no illumination)
	virtual void draw(Camera& camera, const ArrayList<Light*>& lights) const;

	// animates all the entity's meshes in unison
	// @param name The name of the animation to play
	// @param blend If true, blend to the new animation
	void animate(const char* name, bool blend = true);

	// move the entity by its translational and rotational velocity
	// @return true if we moved without hitting any obstacles, otherwise false
	bool move();

	// apply a force to the entity's rigid body (if any)
	// @param force the force to apply in world coordinates
	// @param origin point of origin for the force in world space
	void applyForce(const Vector& force, const Vector& origin);

	// loads an entity definition from a json file
	// @param filename the file to load
	// @return a newly created def_t struct
	static def_t* loadDef(const char* filename);

	// saves an entity definition to a json file
	// @param filename the file to load
	// @return true on success, false on failure
	bool saveDef(const char* filename) const;

	// locates the entity definition with the given name
	// @param name the name of the entity to find
	// @return the entity def, or nullptr if it could not be found
	static const Entity::def_t* findDef(const char* name);

	// locates the entity definition with the given index
	// @param index the index of the definition to find
	// @return the entity def, or nullptr if it could not be found
	static const Entity::def_t* findDef(const Uint32 index);

	// spawns an entity using an entity def
	// @param world the world to spawn the entity into
	// @param def the definition to use
	// @param pos where to spawn the entity
	// @param ang the new entity's orientation
	// @param uid the uid of the entity to be created, if any
	// @return the entity, or nullptr if the associated def was not found
	static Entity* spawnFromDef(World* world, const Entity::def_t& def, const Vector& pos, const Angle& ang, Uint32 uid = UINT32_MAX);

	// determines if we are near a character or not
	// @param radius the search radius
	// @return true if we are near a character, otherwise false
	bool isNearCharacter(float radius) const;

	// only valid for player entities, determine if entity is crouching or not
	// @return true if crouching, otherwise false
	bool isCrouching() const;

	// only valid for player entities, determine if entity is moving or not
	// @return true if moving, otherwise false
	bool isMoving() const;

	// only valid for player entities, determine if entity is jumping or not
	// @return true if jumping, otherwise false
	bool hasJumped() const;

	// only valid for player entities, get the looking direction offset from the entity
	// @return the look direction of the entity
	Angle getLookDir() const;

	// check whether the entity collides with anything at the given location
	// @param newPos the position to test
	// @return true if we collide, false if we do not
	bool checkCollision(const Vector& newPos);

	// builds a copy of this entity
	// @param world the world to place the new entity into
	// @param entity a pre-made entity object, or nullptr to make a new one
	// @return the copied entity
	Entity* copy(World* world, Entity* entity = nullptr) const;

	// updates matrices
	void update();

	// clears the node pointing to us in the chunk we are occupying
	void clearChunkNode() { if( chunkNode ) { chunkNode->getList()->removeNode(chunkNode); chunkNode = nullptr; } }

	// clears the chunk nodes of all components
	void clearAllChunkNodes();

	// generate a new ID for a component and advance the component ID counter
	// @return the fresh ID
	Uint32 getNewComponentID() { ++componentIDs; return componentIDs; }

	// checks the entity for any components with the given type
	// @param type the type to look for
	// @return true if the component was found, false otherwise
	bool hasComponent(Component::type_t type) const;

	// find the component with the given name
	// @param name the name of the component
	// @return the component, or nullptr if it could not be found
	template <typename T>
	T* findComponentByName(const char* name) {
		if( name == nullptr || strcmp(name,"")==0 ) {
			return nullptr;
		}
		for( Uint32 c = 0; c < components.getSize(); ++c ) {
			if( strcmp( components[c]->getName(), name ) == 0 ) {
				return static_cast<T*>(components[c]);
			} else {
				T* result = components[c]->findComponentByName<T>(name);
				if( result ) {
					return static_cast<T*>(result);
				}
			}
		}
		return nullptr;
	}

	// find the component with the given uid
	// @param uid the uid of the component
	// @return the component, or nullptr if it could not be found
	template <typename T>
	T* findComponentByUID(const Uint32 uid) {
		if( uid == Component::nuid ) {
			return nullptr;
		}
		for( Uint32 c = 0; c < components.getSize(); ++c ) {
			if( components[c]->getUID() == uid ) {
				return static_cast<T*>(components[c]);
			} else {
				T* result = components[c]->findComponentByUID<T>(uid);
				if( result ) {
					return static_cast<T*>(result);
				}
			}
		}
		return nullptr;
	}

	// find the component with the given name and remove it
	// @param uid the name of the component
	// @return true if the component was removed, otherwise false
	bool removeComponentByName(const char* name);

	// find the component with the given uid and remove it
	// @param uid the uid of the component
	// @return true if the component was removed, otherwise false
	bool removeComponentByUID(const Uint32 uid);

	Component* addComponent(Component::type_t type);

	// adds a new component to our list of components
	// @return a pointer to our new component
	template <typename T>
	T* addComponent() {
		T* component = new T(*this, nullptr);
		components.push(component);
		return component;
	}

	// find all components of a given type
	// @param type the type of component to search for
	// @param list list to populate
	template <typename T>
	void findAllComponents(Component::type_t type, LinkedList<T*>& list) const {
		for( Uint32 c = 0; c < components.getSize(); ++c ) {
			if( components[c]->getType() == type ) {
				list.addNodeLast(dynamic_cast<T*>(components[c]));
			}
			components[c]->findAllComponents(type,list);
		}
	}

	// perform a line test (raytrace) through the entity's parent world which stops at first hit object
	// @param origin the starting point of the ray
	// @param dest the ending point of the ray
	// @return a hit_t structure containing information on the hit object
	const World::hit_t lineTrace( const Vector& origin, const Vector& dest );

	// dispatches the entity's interaction function in LUA.
	// @param user the entity that interacted with this entity
	// @param bbox the bbox that got clicked
	// @return true if the interaction was successful, false if object is uninteractable or if there were script errors
	bool interact(Entity& user, BBox& bbox);

	// save/load this object to a file
	// @param file interface to serialize with
	void serialize(FileInterface * file);

	// kicks off an async pathfinding task
	// @param goalX target x coordinate
	// @param goalY target y coordinate
	void findAPath(int endX, int endY);

	// kicks off an async pathfinding task to a random destination
	void findRandomPath();

	// checks if the async pathfinding task has finished. This MUST be referenced before making use of Entity::path
	// @return true if the async pathfinding task has finished yet, false if the pathfinding task has not finished yet
	bool pathFinished();

	// check if this entity is a player owned by this client
	// @return true if the entity is a player from this client, otherwise false
	bool isLocalPlayer();

	// updates the physics component of the entity to move it to the current location
	void warp();

protected:
	Node<Entity*>* node			= nullptr;	// node to the world entity list
	World* world				= nullptr;	// parent world object
	Script* script				= nullptr;	// scripting engine
	Player* player				= nullptr;	// player associated with this entity, if any
	Node<Entity*>* chunkNode	= nullptr;	// pointer to our node in the chunk we are occupying (if any)

	World* newWorld				= nullptr;  // world we are moving to, if any
	const Entity* anchor		= nullptr;	// entity we are attached to for the transition
	Vector offset;							// offset from the anchor

	Uint32 componentIDs = 0;
	ArrayList<Component*> components;		// component list

	Vector oldPos;							// mostly used by the editor and audio engines
	Vector pos;								// position
	Vector newPos;							// new position to interpolate towards (net)
	Vector vel;								// velocity
	Angle ang;								// angle
	Angle newAng;							// new angle to interpolate towards (net)
	Angle rot;								// angular velocity
	Vector scale;							// visual scale
	glm::mat4 mat;							// matrix (position * rotation * scale)
	bool matSet = false;
	bool updateNeeded = false;				// if true, matrix gets updated, and component matrices get updated

	Sint32 currentCX = INT32_MAX;			// X coord of the chunk we are currently occupying
	Sint32 currentCY = INT32_MAX;			// Y coord of the chunk we are currently occupying

	sort_t sort = SORT_ANY;					// editing filter
	Uint32 flags = 0;						// flags
	Uint32 defIndex = UINT32_MAX;			// index of the definition used to make the entity
	String defName;							// name gained from def file
	String name;							// the entity's name
	String scriptStr;						// entity script filename
	bool ranScript = false;					// is true if script has run at least once

	Uint32 uid=0;							// entity id number
	Uint32 ticks=0;							// lifespan of the entity
	Uint32 lastUpdate=0;					// time of last remote update of the entity (netplay)
	bool toBeDeleted = false;				// if true, the entity has been marked for deletion at the end of the current frame
	bool shouldSave = true;					// if true, the entity is saved when the world is saved to a file; if false, it is not
	bool falling = false;					// when true the entity is off the floor, otherwise they are on the floor

	Map<String, String> keyvalues;

	// editor variables
	bool selected = false;
	bool highlighted = false;
	std::shared_ptr<Entity::listener_t> listener;

	Vector pathNode;
	Vector pathDir;
	std::future<PathFinder::Path*> pathTask;
	PathFinder::Path* path = nullptr;
	bool pathRequested = false;
};
 
struct Entity::def_t {
	def_t() : entity(nullptr) {
	}
	def_t(const Entity& src) : entity(nullptr) {
		src.copy(nullptr, &entity);
	}

	// save/load this object to a file
	// @param file interface to serialize with
	void serialize(FileInterface * file);

	bool exposedInEditor = true;	// whether it is accessible by editor 
	Uint32 index = 0;				// uid for the definition, set by the engine
	Entity entity;					// entity info
};