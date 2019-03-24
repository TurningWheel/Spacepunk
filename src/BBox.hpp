// BBox.hpp

#pragma once

#include "Component.hpp"

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletDynamics/Character/btKinematicCharacterController.h>

class Model;
class World;

class BBox : public Component {
public:
	// collision shapes
	enum shape_t {
		SHAPE_BOX,
		SHAPE_SPHERE,
		SHAPE_CAPSULE,
		SHAPE_CYLINDER,
		SHAPE_CONE,
		SHAPE_MESH,
		SHAPE_MAX
	};
	static const char* shapeStr[SHAPE_MAX];

	BBox(Entity& _entity, Component* _parent);
	virtual ~BBox();

	static const float collisionEpsilon;

	// bbox models
	static const char* meshCapsuleCylinderStr;
	static const char* meshCapsuleHalfSphereStr;
	static const char* meshConeStr;
	static const char* meshCylinderStr;
	static const char* meshSphereStr;
	static const char* meshBoxStr;
	static const char* materialStr;

	// create the physics body
	void createRigidBody();

	// removes the physics object from the simulation
	void deleteRigidBody();

	// update the location, shape, and all other properties of the rigid body
	// @param oldGScale the old scale; if this changed, any triangle mesh is invalid
	void updateRigidBody(const Vector& oldGScale);

	// get the current transform of this bbox in the physics sim
	// @return The transform from the physics sim
	btTransform getPhysicsTransform() const;

	// move the bbox to the given location
	// @param v The new position
	// @param a The new rotation
	void setPhysicsTransform(const Vector& v, const Angle& a);

	// generate list of all entities whose bboxes overlap this bbox
	// @return list of entities overlapping this one
	ArrayList<Entity*> findAllOverlappingEntities() const;

	// check whether the component collides with anything at the current location
	// @return true if we collide, false if we do not
	virtual bool checkCollision() const override;

	// @param outEntity the entity we are standing on, if any
	// @return the nearest floor, if any
	float nearestFloor(Entity*& outEntity);

	// @return the nearest ceiling, if any
	float nearestCeiling();

	// get the distance to the floor
	// @param floorHeight floor height to test bbox against
	// @return the distance to the nearest floor tile under the entity
	float distToFloor(float floorHeight);

	// get the distance to the ceiling
	// @param ceilingHeight ceiling height to test bbox against
	// @return the distance to the nearest ceiling tile above the entity
	float distToCeiling(float ceilingHeight);

	// apply movement forces (velocity and rotation) to bbox's physics component
	void applyMoveForces(const Vector& vel, const Angle& rot);

	// apply a force to the bbox's physics component
	// @param force the force to apply in world coordinates
	// @param origin point of origin for the force in world space
	void applyForce(const Vector& force, const Vector& origin);

	// called just before the parent is inserted into a new world
	// @param world the world we will be placed into, if any
	virtual void beforeWorldInsertion(const World* world) override;

	// called just after the parent is inserted into a new world
	// @param world the world we have been placed into, if any
	virtual void afterWorldInsertion(const World* world) override;

	// updates matrices and rigid body
	virtual void update() override;
		
	// draws the component
	// @param camera the camera through which to draw the component
	// @param light the light by which the component should be illuminated (or nullptr for no illumination)
	virtual void draw(Camera& camera, const ArrayList<Light*>& lights) override;

	// load the component from a file
	// @param fp the file to read from
	virtual void load(FILE* fp) override;

	// save/load this object to a file
	// @param file interface to serialize with
	virtual void serialize(FileInterface* file) override;

	// getters & setters
	virtual type_t					getType() const override		{ return COMPONENT_BBOX; }
	shape_t							getShape() const				{ return shape; }
	bool							isEnabled() const				{ return enabled; }
	float							getMass() const					{ return mass; }
	const btCollisionShape*			getCollisionShapePtr() const	{ return collisionShapePtr; }

	void		setShape(shape_t _shape)			{ shape = _shape; dirty = true; updateNeeded = true; }
	void		setEnabled(bool _enabled)			{ enabled = _enabled; dirty = true; updateNeeded = true; }
	void		setMass(float _mass)				{ mass = _mass; dirty = true; updateNeeded = true; }

	BBox& operator=(const BBox& src) {
		enabled = src.enabled;
		shape = src.shape;
		meshName = src.meshName;
		mass = src.mass;
		updateNeeded = true;
		dirty = true;
		return *this;
	}

private:
	bool enabled = true;
	shape_t shape = SHAPE_BOX;
	float mass = 0.f;
	String meshName;

	bool dirty = false;

	// bullet physics objects
	btDiscreteDynamicsWorld* dynamicsWorld = nullptr;
	btCollisionShape* collisionShapePtr = nullptr;
	btDefaultMotionState* motionState = nullptr;
	btRigidBody* rigidBody = nullptr;
	btTriangleMesh* triMesh = nullptr;
	btPairCachingGhostObject* ghostObject = nullptr;
	btKinematicCharacterController* controller = nullptr;

	// update the bbox to match the given model
	// @param model the model to conform to
	void conformToModel(const Model& model);
};