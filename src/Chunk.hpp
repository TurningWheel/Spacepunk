// Chunk.hpp

#pragma once

#define GLM_FORCE_RADIANS
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Main.hpp"
#include "Tile.hpp"
#include "Camera.hpp"
#include "World.hpp"
#include "ArrayList.hpp"
#include "Pair.hpp"

class Component;

class Chunk {
public:
	Chunk();
	~Chunk();

	// the size of a chunk in tiles
	static const unsigned int size = 2;

	// builds vertex buffers
	void buildBuffers();

	// consolidates vertex data among the tiles
	void optimizeBuffers();

	// draws the chunk
	// @param camera: the camera to render the chunk with
	void draw(Camera& camera) const;

	// calculates the number of vertices in the chunk
	// @return the number of vertices for all surfaces in the chunk
	size_t calculateVertices() const;

	// adds an entity component to our population list
	// @param component: the entity component to add to our list
	Node<Component*>* addCPopulation(Component* component);

	// adds an entity to our population list
	// @param entity: the entity to add to our list
	Node<Entity*>* addEPopulation(Entity* entity);

	// getters & setters
	bool							isChanged() const			{ return changed; }
	Tile*							getTile(int index)			{ return tiles[index]; }
	const LinkedList<Entity*>&		getEPopulation() const		{ return ePopulation; }
	LinkedList<Entity*>&			getEPopulation()			{ return ePopulation; }
	const LinkedList<Component*>&	getCPopulation() const		{ return cPopulation; }
	LinkedList<Component*>&			getCPopulation()			{ return cPopulation; }

	void		setWorld(World& _world)						{ world = &_world; }
	void		setChanged(bool _changed)					{ changed = _changed; }
	void		setTile(int index, Tile* _tile)				{ tiles[index] = _tile; }

private:
	World* world = nullptr;
	bool changed = false;
	Tile* tiles[size*size];
	LinkedList<Entity*> ePopulation;
	LinkedList<Component*> cPopulation;
	bool optimized = false;

	enum buffer_t {
		VERTEX_BUFFER,
		NORMAL_BUFFER,
		TANGENT_BUFFER,
		DIFFUSEMAP_BUFFER,
		NORMALMAP_BUFFER,
		EFFECTSMAP_BUFFER,
		INDEX_BUFFER,
		BUFFER_TYPE_LENGTH
	};

	size_t numVertices = 0;
	size_t numIndices = 0;
	GLuint vbo[BUFFER_TYPE_LENGTH];
	GLuint vao = 0;
		
	GLuint findAdjacentIndex(GLuint index1, GLuint index2, GLuint index3);

	void combineVertices();

	typedef UnorderedPair<GLuint, GLuint> Edge;
	typedef ArrayList<Edge> EdgeList;
	void findEdges(EdgeList& edges, bool all);
	void findEdge(const Edge&, size_t skip, EdgeList& edges, bool all);
	void combineEdges(EdgeList& edges);
	EdgeList edges;

	glm::mat3x3 colorChannels[size*size];
	ArrayList<glm::vec3,0> vertexBuffer;
	ArrayList<glm::vec3,0> normalBuffer;
	ArrayList<glm::vec3,0> tangentBuffer;
	ArrayList<glm::vec3,0> diffuseMapBuffer;
	ArrayList<glm::vec3,0> normalMapBuffer;
	ArrayList<glm::vec3,0> effectsMapBuffer;
	ArrayList<GLuint,0> indexBuffer;

	// build tangent buffers
	void buildTangents();

	// uploads vertex data to gpu
	void uploadBuffers();
};
