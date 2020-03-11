//! @file SectorWorld.hpp

#pragma once

#include "World.hpp"

class Sector;
class SectorVertex;

//! Implements the World class for a world full of Sectors as well as Entities.
class SectorWorld : public World {
public:
	SectorWorld(Game* _game, bool _silent, Uint32 _id, const char* _name);
	virtual ~SectorWorld();

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

	//! writes the world contents to a file
	//! @param _filename the filename to write to, or blank to use our last filename
	//! @param updateFilename if true, our current filename is changed, otherwise, it is not
	//! @return true on success, false on failure
	virtual bool saveFile(const char* _filename = "", bool updateFilename = false);

	//! remove a sector from the world
	//! @param index the index of the sector to remove
	void removeSector(Uint32 index);

	//! selects or deselects the vertex with the given index
	//! @param index the index of the vertex to select
	//! @param b if true, the vertex is selected; if false, it is deselected
	//! @return true if the vertex could be found, false otherwise
	const bool selectVertex(const Uint32 index, const bool b);

	//! selects or deselects all vertices in the world
	//! @param b if true, all vertices are selected; if false, they are deselected
	void selectVertices(const bool b);

	//! process world events
	virtual void process() override;

	//! getters & setters
	virtual const type_t				getType() const { return WORLD_SECTORS; }
	const ArrayList<Sector*>&			getSectors() const { return sectors; }
	const ArrayList<SectorVertex*>&		getVertices() const { return vertices; }
	ArrayList<Sector*>&					getSectors() { return sectors; }
	ArrayList<SectorVertex*>&			getVertices() { return vertices; }

	virtual std::future<PathFinder::Path*> findAPath(int startX, int startY, int endX, int endY) override;

protected:
	//! when a new world is spawned, it generates an obstacle map/cache of all static obstacles.
	virtual void generateObstacleCache();

private:
	//! draws the editing grid
	//! @param camera the camera through which to draw the grid
	//! @param z the height of the grid in the world
	void drawGrid(Camera& camera, float z);

	//! creates rendering objects for the world grid
	void createGrid();

	//! destroys rendering objects for the world grid
	void destroyGrid();

	//! draws all sectors and entities in the world
	//! @param camera the camera through which to draw the scene
	//! @param light the light by which the scene should be illuminated (or nullptr for no illumination)
	//! @param sectorDrawList a list of sectors to draw
	void drawSceneObjects(Camera& camera, Light* light);

	ArrayList<Sector*> sectors;
	ArrayList<SectorVertex*> vertices;

	//! how many tiles a "large" grid section is
	static const int largeGridSize = 8;
	float tileSize = 0;

	//! editing grid graphics data
	enum buffer_t {
		BUFFER_VERTEX,
		BUFFER_COLOR,
		BUFFER_INDEX,
		BUFFER_MAX
	};
	GLuint vbo[BUFFER_MAX];
	GLuint vao = 0;

	//! grid rigid body
	void updateGridRigidBody(float z);
	void deleteGridRigidBody();
	btCollisionShape* collisionShapePtr = nullptr;
	btDefaultMotionState* motionState = nullptr;
	btRigidBody* rigidBody = nullptr;

	//PathFinder& pathFinder;
};