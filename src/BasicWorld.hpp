//! @file BasicWorld.hpp

#pragma once

#include "World.hpp"

//! BasicWorld is the primary world implementation as of the latest version of the engine.
//! See World for more info.
class BasicWorld : public World {
public:
	BasicWorld() = delete;
	BasicWorld(Game* _game, bool _silent, Uint32 _id, const char* _name);
	BasicWorld(const BasicWorld&) = delete;
	BasicWorld(BasicWorld&&) = delete;
	BasicWorld& operator=(const BasicWorld&) = delete;
	BasicWorld& operator=(BasicWorld&&) = delete;
	virtual ~BasicWorld();

	//! const variables
	static const int fileMagicNumberLen = 16;
	static const char* fileMagicNumber;
	static const char* fileMagicNumberSaveOut;

	//! post-load world initialization
	//! @param empty if the world is empty
	virtual void initialize(bool empty) override;

	//! clears all geometry selection
	virtual void deselectGeometry();

	//! finds all entities within a given radius of the provided point
	//! @param origin position to search from
	//! @param radius the radius to search in
	//! @param outList the list to populate
	//! @param flat if true, the search radius is 2-dimensional
	virtual void findEntitiesInRadius(const Vector& origin, float radius, LinkedList<Entity*>& outList, bool flat = false);

	//! draws the world and its contents
	virtual void draw();

	//! draws entities in the world
	//! @param camera the camera through which to draw the scene
	//! @param light the light by which the scene should be illuminated (or nullptr for no illumination)
	//! @param entities the entities to draw
	void drawSceneObjects(Camera& camera, const ArrayList<Light*>& lights, const ArrayList<Entity*>& entities);

	//! fill an empty draw list with entities sorted by camera distance
	//! @param camera the camera in question
	//! @param maxLength if an entity is further away than this, it will be skipped
	//! @param entities the draw list to fill
	void fillDrawList(const Camera& camera, float maxLength, ArrayList<Entity*>& entities);

	//! writes the world contents to a file
	//! @param _filename the filename to write to, or blank to use our last filename
	//! @param updateFilename if true, our current filename is changed, otherwise, it is not
	//! @return true on success, false on failure
	virtual bool saveFile(const char* _filename = "", bool updateFilename = false);

	//! save/load this object to a file
	//! @param file interface to serialize with
	virtual void serialize(FileInterface * file);

	//! process world events
	virtual void process() override;

	//! pathfinding
	virtual std::future<PathFinder::Path*> findAPath(int startX, int startY, int endX, int endY) override;

	virtual type_t getType() const { return WORLD_BASIC; }

protected:
	//! when a new world is spawned, it generates an obstacle map/cache of all static obstacles.
	virtual void generateObstacleCache();

private:
	//! draws the editing grid
	//! @param camera the camera through which to draw the grid
	//! @param z the height of the grid in the world
	void drawGrid(Camera& camera);

	//! creates rendering objects for the world grid
	void createGrid();

	//! destroys rendering objects for the world grid
	void destroyGrid();

	//! grid data
	static const int largeGridSize = 8;
	static const Uint32 gridWidth = 256;
	static const Uint32 gridHeight = 256;
	static const Uint32 lines = gridWidth + 1 + gridHeight + 1;

	//! editing grid graphics data
	enum buffer_t {
		BUFFER_VERTEX,
		BUFFER_COLOR,
		BUFFER_INDEX,
		BUFFER_MAX
	};
	GLuint vbo[BUFFER_MAX];
	GLuint vao = 0;
};
