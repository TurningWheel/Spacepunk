//! @file BBox.hpp

#pragma once

#include "Component.hpp"

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletDynamics/Character/btKinematicCharacterController.h>

class Model;
class World;

//! A BBox (short for bounding box) is an entity Component that adds collision to an entity.
//! A BBox at the root of an entity that is named "physics" will become a driver for the entity
//! A BBox with mass 0 is static and should not be moved.
//! A BBox with negative mass becomes kinematic and moves with "game-style" physics, but will still stop when encountering other bboxes.
//! A BBox with positive mass becomes a rigid body and will be automatically affected by gravity.
class BBox : public Component {
public:
	//! collision shapes
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

	//! bbox models
	static const char* meshCapsuleCylinderStr;
	static const char* meshCapsuleHalfSphereStr;
	static const char* meshConeStr;
	static const char* meshCylinderStr;
	static const char* meshSphereStr;
	static const char* meshBoxStr;
	static const char* materialStr;

	//! create the physics body
	void createRigidBody();

	//! removes the physics object from the simulation
	void deleteRigidBody();

	//! update the location, shape, and all other properties of the rigid body
	//! @param oldGScale the old scale; if this changed, any triangle mesh is invalid
	void updateRigidBody(const Vector& oldGScale);

	//! get the current transform of this bbox in the physics sim
	//! @return The transform from the physics sim
	btTransform getPhysicsTransform() const;

	//! move the bbox to the given location
	//! @param v The new position
	//! @param a The new orientation
	void setPhysicsTransform(const Vector& v, const Quaternion& a);

	//! generate list of all entities whose bboxes overlap this bbox
	//! @return list of entities overlapping this one
	ArrayList<Entity*> findAllOverlappingEntities() const;

	//! check whether the component collides with anything at the current location
	//! @return true if we collide, false if we do not
	virtual bool checkCollision() const override;

	//! apply movement forces (velocity and rotation) to bbox's physics component
	void applyMoveForces(const Vector& vel, const Rotation& rot);

	//! apply a force to the bbox's physics component
	//! @param force the force to apply in world coordinates
	//! @param origin point of origin for the force in world space
	void applyForce(const Vector& force, const Vector& origin);

	//! called just before the parent is inserted into a new world
	//! @param world the world we will be placed into, if any
	virtual void beforeWorldInsertion(const World* world) override;

	//! called just after the parent is inserted into a new world
	//! @param world the world we have been placed into, if any
	virtual void afterWorldInsertion(const World* world) override;

	//! updates matrices and rigid body
	virtual void update() override;

	//! draws the component
	//! @param camera the camera through which to draw the component
	//! @param light the light by which the component should be illuminated (or nullptr for no illumination)
	virtual void draw(Camera& camera, const ArrayList<Light*>& lights) override;

	//! load the component from a file
	//! @param fp the file to read from
	virtual void load(FILE* fp) override;

	//! save/load this object to a file
	//! @param file interface to serialize with
	virtual void serialize(FileInterface* file) override;

	virtual type_t					getType() const override { return COMPONENT_BBOX; }
	shape_t							getShape() const { return shape; }
	bool							isEnabled() const { return enabled; }
	float							getMass() const { return mass; }
	const btCollisionShape*			getCollisionShapePtr() const { return collisionShapePtr; }

	void		setShape(Uint32 _shape) { shape = (shape_t)_shape; dirty = true; updateNeeded = true; }
	void		setEnabled(bool _enabled) { enabled = _enabled; dirty = true; updateNeeded = true; }
	void		setMass(float _mass) { mass = _mass; dirty = true; updateNeeded = true; }

	BBox& operator=(const BBox& src) {
		enabled = src.enabled;
		shape = src.shape;
		mass = src.mass;
		updateNeeded = true;
		dirty = true;
		return *this;
	}

private:
	bool enabled = true;
	shape_t shape = SHAPE_BOX;
	float mass = 0.f;

	bool dirty = false;
	bool meshDirty = false;

	//! bullet physics objects
	btDiscreteDynamicsWorld* dynamicsWorld = nullptr;
	btCollisionShape* collisionShapePtr = nullptr;
	btDefaultMotionState* motionState = nullptr;
	btRigidBody* rigidBody = nullptr;
	btTriangleMesh* triMesh = nullptr;
	btPairCachingGhostObject* ghostObject = nullptr;
	btKinematicCharacterController* controller = nullptr;

	//! update the bbox to match the given model
	//! @param model the model to conform to
	void conformToModel(const Model& model);

	btVector3 convertScaleBasedOnShape(const Vector& scale);
};