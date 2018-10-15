// SectorVertex.hpp

#pragma once

#ifdef PLATFORM_LINUX
#include <btBulletDynamicsCommon.h>
#else
#include <bullet3/btBulletDynamicsCommon.h>
#endif

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/vec3.hpp>

#include "ArrayList.hpp"
#include "Sector.hpp"

class SectorWorld;

class SectorVertex {
public:
	SectorVertex(SectorWorld& _world);
	~SectorVertex();

	friend class Sector;
	friend class SectorWorld;

	static const char* materialStr;

	// draws the vertex
	// @param camera: the camera through which to draw the vertex
	void draw(Camera& camera);

	// move all the vertices to the given location
	// @param pos: where to move them to
	void move(const glm::vec3& pos);

	// assign a vertex to us
	// @param vertex: vertex to assign
	void own(Sector::vertex_t& vertex);

	// getters & setters
	bool									isSelected() const			{ return selected; }
	bool									isHighlighted() const		{ return highlighted; }
	const ArrayList<Sector::vertex_t*>&		getVertices() const			{ return vertices; }
	ArrayList<Sector::vertex_t*>&			getVertices()				{ return vertices; }
	const Vector&							getPos() const				{ return pos; }
	const Vector&							getOldPos() const			{ return oPos; }

	void	setSelected(bool _b)		{ selected = _b; }
	void	setHighlighted(bool _b)		{ highlighted = _b; }
	void	setPos(const Vector& v)		{ pos = v; }
	void	setOldPos(const Vector& v)	{ oPos = v; }

private:
	SectorWorld* world = nullptr;
	bool selected = false;
	bool highlighted = false;
	size_t index = 0;

	ArrayList<Sector::vertex_t*> vertices;
	Vector pos;
	Vector oPos;

	// bullet physics objects
	btDiscreteDynamicsWorld* dynamicsWorld = nullptr;
	btCollisionShape* collisionShapePtr = nullptr;
	btDefaultMotionState* motionState = nullptr;
	btRigidBody* rigidBody = nullptr;

	void updateRigidBody();
	void deleteRigidBody();
};