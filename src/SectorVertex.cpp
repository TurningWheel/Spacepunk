// SectorVertex.cpp

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Main.hpp"
#include "Engine.hpp"
#include "SectorVertex.hpp"
#include "Sector.hpp"
#include "SectorWorld.hpp"
#include "Camera.hpp"

SectorVertex::SectorVertex(SectorWorld& _world) {
	world = &_world;
	index = world->getVertices().getSize();
}

SectorVertex::~SectorVertex() {
	deleteRigidBody();
}

void SectorVertex::own(Sector::vertex_t& vertex) {
	if( vertex.joined ) {
		ArrayList<Sector::vertex_t*> theirVertices = vertex.joined->getVertices();
		for( Uint32 c = 0; c < theirVertices.getSize(); ++c ) {
			if( theirVertices[c] == &vertex ) {
				theirVertices.remove(c);
				--c;
			}
		}
	}
	vertex.joined = this;
	vertices.push(&vertex);
}

void SectorVertex::deleteRigidBody() {
	// delete old rigid body
	if( rigidBody!=nullptr ) {
		if( dynamicsWorld ) {
			dynamicsWorld->removeRigidBody(rigidBody);
		}
		delete rigidBody;
		rigidBody = nullptr;
	}

	// delete motion state
	if( motionState!=nullptr ) {
		delete motionState;
		motionState = nullptr;
	}

	// delete collision volume
	if( collisionShapePtr!=nullptr ) {
		delete collisionShapePtr;
		collisionShapePtr = nullptr;
	}
}

void SectorVertex::updateRigidBody() {
	deleteRigidBody();
	if( vertices.getSize() <= 0 || !mainEngine->isEditorRunning() ) {
		return;
	}

	// setup new collision volume
	collisionShapePtr = new btBoxShape(btVector3(4.f, 4.f, 4.f));

	// create motion state
	Sector::vertex_t& v = *vertices[0];
	btVector3 pos(v.position.x, v.position.y, v.position.z);
	motionState = new btDefaultMotionState(btTransform(btQuaternion(0.f, 0.f, 0.f, 1.f), pos));

	// create rigid body
	btRigidBody::btRigidBodyConstructionInfo
		rigidBodyCI(0, motionState, collisionShapePtr, btVector3(0.f, 0.f, 0.f));
	rigidBody = new btRigidBody(rigidBodyCI);

	// add a new rigid body to the simulation
	if( world ) {
		dynamicsWorld = world->getBulletDynamicsWorld();
		if( dynamicsWorld ) {
			dynamicsWorld->addRigidBody(rigidBody);
		}
	}
}

void SectorVertex::move(const glm::vec3& pos) {
	for( Uint32 c = 0; c < vertices.getSize(); ++c ) {
		vertices[c]->position = pos;
		vertices[c]->face->sector->setUpdateNeeded(true);
	}
	setPos(Vector(pos.x, pos.y, pos.z));
	updateRigidBody();
}

void SectorVertex::draw(Camera& camera) {
	if( !mainEngine->isEditorRunning() ) {
		return;
	}

	glm::mat4 mat = glm::translate( glm::mat4(1.f), glm::vec3(pos.x, -pos.z, pos.y) );
	mat = glm::scale(mat, glm::vec3(4.f));
	if( selected ) {
		camera.drawCube(mat, glm::vec4(1.f, 0.f, 0.f, 1.f));
	} else {
		if( highlighted ) {
			camera.drawCube(mat, glm::vec4(1.f, 1.f, 0.f, 1.f));
		} else {
			camera.drawCube(mat, glm::vec4(0.f, 1.f, 0.f, 1.f));
		}
	}
}