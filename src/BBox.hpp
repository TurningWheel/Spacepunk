// BBox.hpp

#pragma once

#include "Component.hpp"

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
	static const char* materialStr;

	// removes the physics object from the simulation
	void deleteRigidBody();

	// update the location, shape, and all other properties of the rigid body
	// @param oldGScale: the old scale; if this changed, any triangle mesh is invalid
	void updateRigidBody(const Vector& oldGScale);

	// check whether the given point is inside the entity
	// @param point: the point to test
	// @return true if the point is inside the entity, otherwise false
	bool containsPoint(const Vector& point) const;

	// generate list of all entities whose bboxes overlap this bbox
	// @return list of entities overlapping this one
	LinkedList<const Entity*> findAllOverlappingEntities() const;

	// check whether the component collides with anything at the current location
	// @return true if we collide, false if we do not
	virtual bool checkCollision() const override;

	// @return the height of the nearest floor tile
	float nearestFloor();

	// @return the height of the nearest ceiling tile
	float nearestCeiling();

	// get the distance to the floor
	// @param floorHeight: floor height to test bbox against
	// @return the distance to the nearest floor tile under the entity
	float distToFloor(float floorHeight);

	// get the distance to the ceiling
	// @param ceilingHeight: ceiling height to test bbox against
	// @return the distance to the nearest ceiling tile above the entity
	float distToCeiling(float ceilingHeight);

	// called just before the parent is inserted into a new world
	// @param world: the world we will be placed into, if any
	virtual void beforeWorldInsertion(const World* world) override;

	// called just after the parent is inserted into a new world
	// @param world: the world we have been placed into, if any
	virtual void afterWorldInsertion(const World* world) override;

	// updates matrices and rigid body
	virtual void update() override;
		
	// draws the component
	// @param camera: the camera through which to draw the component
	// @param light: the light by which the component should be illuminated (or nullptr for no illumination)
	virtual void draw(Camera& camera, Light* light) override;

	// load the component from a file
	// @param fp: the file to read from
	virtual void load(FILE* fp) override;

	// save/load this object to a file
	// @param file interface to serialize with
	virtual void serialize(FileInterface* file) override;

	// getters & setters
	virtual type_t					getType() const override	{ return COMPONENT_BBOX; }
	shape_t							getShape() const			{ return shape; }
	bool							isEnabled() const			{ return enabled; }

	void		setShape(shape_t _shape)			{ shape = _shape; }
	void		setEnabled(bool _enabled)			{ if( _enabled != enabled ) { enabled = _enabled; updateNeeded = true; } }

	BBox& operator=(const BBox& src) {
		enabled = src.enabled;
		shape = src.shape;
		meshName = src.meshName;
		updateNeeded = true;
		return *this;
	}

private:
	bool enabled = true;
	shape_t shape = SHAPE_BOX;
	String meshName;

	// bullet physics objects
	btDiscreteDynamicsWorld* dynamicsWorld = nullptr;
	btCollisionShape* collisionShapePtr = nullptr;
	btDefaultMotionState* motionState = nullptr;
	btRigidBody* rigidBody = nullptr;
	btTriangleMesh* triMesh = nullptr;

	// update the bbox to match the given model
	// @param model: the model to conform to
	void conformToModel(const Model& model);

	void testAgainstComponentForFloor(Component* component, const Vector& start, const Vector& end, float& nearestFloor) const;
	void testAgainstComponentForCeiling(Component* component, const Vector& start, const Vector& end, float& nearestCeiling) const;
	bool testAgainstComponentForObstacle(Component* component, const Vector& start, const Vector& end) const;
};