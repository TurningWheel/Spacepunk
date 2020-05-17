// BBox.cpp

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Main.hpp"
#include "Engine.hpp"
#include "BBox.hpp"
#include "Mesh.hpp"
#include "Model.hpp"
#include "World.hpp"
#include "Camera.hpp"

btQuaternion btQuat(const Quaternion& q) {
	return btQuaternion(-q.x, -q.z, q.y, -q.w);
}

const char* BBox::meshCapsuleCylinderStr = "assets/editor/bbox/CapsuleCylinder.FBX";
const char* BBox::meshCapsuleHalfSphereStr = "assets/editor/bbox/CapsuleHalfSphere.FBX";
const char* BBox::meshConeStr = "assets/editor/bbox/Cone.FBX";
const char* BBox::meshCylinderStr = "assets/editor/bbox/Cylinder.FBX";
const char* BBox::meshSphereStr = "assets/editor/bbox/Sphere.nff";
const char* BBox::meshBoxStr = "assets/editor/bbox/Cube.obj";
const char* BBox::materialStr = "assets/editor/bbox/material.json";

const char* BBox::shapeStr[SHAPE_MAX] = {
	"box",
	"sphere",
	"capsule",
	"cylinder",
	"cone",
	"mesh"
};

BBox::BBox(Entity& _entity, Component* _parent) :
	Component(_entity, _parent) {
	name = typeStr[COMPONENT_BBOX];

	// exposed attributes
	attributes.push(new AttributeBool("Enabled", enabled));
	attributes.push(new AttributeEnum<shape_t>("Shape", shapeStr, shape_t::SHAPE_MAX, shape));
	attributes.push(new AttributeFloat("Mass", mass));
}

BBox::~BBox() {
	deleteRigidBody();
}

void BBox::beforeWorldInsertion(const World* world) {
	Component::beforeWorldInsertion(world);
	deleteRigidBody();
}

void BBox::afterWorldInsertion(const World* world) {
	Component::afterWorldInsertion(world);
	updateRigidBody(Vector(FLT_MAX, FLT_MAX, FLT_MAX));
}

void BBox::deleteRigidBody() {
	if (rigidBody != nullptr) {
		if (dynamicsWorld) {
			dynamicsWorld->removeRigidBody(rigidBody);
		}
		auto manifest = static_cast<World::physics_manifest_t*>(rigidBody->getUserPointer());
		if (manifest) {
			delete manifest;
			manifest = nullptr;
		}
		delete rigidBody;
		rigidBody = nullptr;
	}
	if (motionState != nullptr) {
		delete motionState;
		motionState = nullptr;
	}
	if (controller != nullptr) {
		if (dynamicsWorld) {
			dynamicsWorld->removeAction(controller);
		}
		delete controller;
		controller = nullptr;
	}
	if (ghostObject != nullptr) {
		if (dynamicsWorld) {
			dynamicsWorld->removeCollisionObject(ghostObject);
		}
		delete ghostObject;
		ghostObject = nullptr;
	}
	if (collisionShapePtr != nullptr) {
		delete collisionShapePtr;
		collisionShapePtr = nullptr;
	}
	if (triMesh != nullptr) {
		delete triMesh;
		triMesh = nullptr;
	}
}

void BBox::conformToModel(const Model& model) {
	// this doesn't work for animated meshes, yet
	if (model.hasAnimations()) {
		return;
	}

	if (triMesh != nullptr) {
		delete triMesh;
		triMesh = nullptr;
	}

	Mesh* mesh = mainEngine->getMeshResource().dataForString(model.getMesh());
	if (!mesh) {
		if (mainEngine->getMeshResource().getError() == resource_error_t::ERROR_CACHEFAILED) {
			meshDirty = false;
		}
		if (mainEngine->getMeshResource().getError() == resource_error_t::ERROR_CACHEINPROGRESS) {
			meshDirty = true;
		}
		return;
	}

	// iterate through every submesh
	for (const Node<Mesh::SubMesh*>* entryNode = mesh->getSubMeshes().getFirst(); entryNode != nullptr; entryNode = entryNode->getNext()) {
		const Mesh::SubMesh* entry = entryNode->getData();

		if (!entry->getVertices()) {
			continue;
		} else if (entry->getNumVertices() > 10000) {
			if (meshDirty) {
				mainEngine->fmsg(Engine::MSG_WARN, "submesh has an excessive number of faces, no physics mesh has been generated");
			}
			continue;
		}

		if (!triMesh) {
			triMesh = new btTriangleMesh();
		}

		// iterate through each index
		LinkedList<Vector> vlist;
		for (unsigned int i = 0; i < entry->getNumIndices(); i += 2) {
			GLuint index = entry->getIndices()[i];

			const float& x = entry->getVertices()[index * 3];
			const float& y = entry->getVertices()[index * 3 + 1];
			const float& z = entry->getVertices()[index * 3 + 2];

			// create vertex for physics mesh
			Vector v(x, z, -y);
			vlist.addNodeLast(v);

			if (vlist.getSize() >= 3) {
				// add the three vertices to the triangle mesh
				Vector& v0 = vlist.nodeForIndex(0)->getData();
				Vector& v1 = vlist.nodeForIndex(1)->getData();
				Vector& v2 = vlist.nodeForIndex(2)->getData();
				triMesh->addTriangle(v0, v1, v2, true);

				// clear the triangle list
				vlist.removeAll();
			}
		}

		// clear the vertex list (in case there are leftovers)
		vlist.removeAll();
	}

	meshDirty = false;
}

btTransform BBox::getPhysicsTransform() const {
	btTransform transform = btTransform::getIdentity();
	if (ghostObject) {
		transform = ghostObject->getWorldTransform();
	} else if (motionState) {
		motionState->getWorldTransform(transform);
	}
	return transform;
}

void BBox::setPhysicsTransform(const Vector& v, const Quaternion& a) {
	btTransform btTrans(btQuat(a), btVector3(v.x, v.y, v.z));
	if (ghostObject) {
		ghostObject->setWorldTransform(btTrans);
	} else if (motionState) {
		motionState->setWorldTransform(btTrans);
		rigidBody->setWorldTransform(btTrans);
	}
}

btVector3 BBox::convertScaleBasedOnShape(const Vector& _scale) {
	Vector scale(fabs(_scale.x), fabs(_scale.y), fabs(_scale.z));
	float maximum;
	btVector3 result;
	switch (shape) {
	default:
	case SHAPE_BOX:
		result = btVector3(scale.x, scale.y, scale.z);
		break;
	case SHAPE_SPHERE:
		maximum = max(max(scale.x, scale.y), scale.z);
		result = btVector3(maximum, maximum, maximum);
		break;
	case SHAPE_CAPSULE:
		maximum = max(scale.x, scale.y);
		result = btVector3(maximum, maximum, scale.z);
		break;
	case SHAPE_CYLINDER:
		result = btVector3(scale.x, scale.y, scale.z);
		break;
	case SHAPE_CONE:
		maximum = max(scale.x, scale.y);
		result = btVector3(maximum, maximum, scale.z);
		break;
	case SHAPE_MESH:
		result = btVector3(scale.x, scale.y, scale.z);
		break;
	}
	return result;
}

void BBox::updateRigidBody(const Vector& oldGScale) {
	if (mass == 0.f || mainEngine->isEditorRunning()) {
		dirty = true;
	}
	if (mass > 0.f && !gScale.close(oldGScale)) {
		dirty = true;
	}
	if (!mainEngine->isEditorRunning() && mass < 0.f && (ghostObject == nullptr || controller == nullptr)) {
		dirty = true;
	}
	if (dirty || meshDirty) {
		createRigidBody();
		dirty = false;
	}

	// scale collision shape
	if (collisionShapePtr && !gScale.close(oldGScale)) {
		btVector3 scale = convertScaleBasedOnShape(gScale);
		collisionShapePtr->setLocalScaling(scale);
	}

	if (parent != nullptr || mass == 0.f) {
		if (ghostObject) {
			btTransform btTrans(btQuat(gAng), btVector3(gPos.x, gPos.y, gPos.z));
			ghostObject->setWorldTransform(btTrans);
		} else if (motionState) {
			btTransform btTrans(btQuat(gAng), btVector3(gPos.x, gPos.y, gPos.z));
			motionState->setWorldTransform(btTrans);
		}
	}
}

class KinematicCharacterController : public btKinematicCharacterController {
public:
	KinematicCharacterController(btPairCachingGhostObject* ghostObject, btConvexShape* convexShape, btScalar stepHeight, const btVector3& up) :
		btKinematicCharacterController(ghostObject, convexShape, stepHeight, up)
	{}

	void setLinearVelocity(const btVector3& velocity) {
		m_walkDirection = velocity;
	}
	btVector3 getLinearVelocity() const {
		return m_walkDirection;
	}
};

void BBox::createRigidBody() {
	deleteRigidBody();

	// setup new collision volume
	switch (shape) {
	default:
	case SHAPE_BOX:
		collisionShapePtr = new btBoxShape(btVector3(1.f, 1.f, 1.f));
		break;
	case SHAPE_SPHERE:
		collisionShapePtr = new btSphereShape(1.f);
		break;
	case SHAPE_CAPSULE:
		collisionShapePtr = new btCapsuleShapeZ(1.f, 1.f);
		break;
	case SHAPE_CYLINDER:
		collisionShapePtr = new btCylinderShapeZ(btVector3(1.f, 1.f, 1.f));
		break;
	case SHAPE_CONE:
		collisionShapePtr = new btConeShapeZ(1.f, 1.f);
		break;
	case SHAPE_MESH:
		if (editorOnly && !mainEngine->isEditorRunning()) {
			return;
		}
		if (parent && parent->getType() == Component::COMPONENT_MODEL) {
			Model* model = static_cast<Model*>(parent);
			conformToModel(*model);
		}
		if (triMesh && triMesh->getNumTriangles() > 0) {
			collisionShapePtr = new btBvhTriangleMeshShape(triMesh, true, true);
		} else {
			return;
		}
		break;
	}

	if (!mainEngine->isEditorRunning()) {
		if (entity->isFlag(Entity::FLAG_PASSABLE) || !enabled) {
			return;
		}
	}

	World* world = entity->getWorld();
	if (world) {
		dynamicsWorld = world->getBulletDynamicsWorld();
		if (dynamicsWorld) {
			// create physics manifest
			auto manifest = new World::physics_manifest_t();
			manifest->world = world;
			manifest->entity = entity;
			manifest->bbox = this;

			if (mainEngine->isEditorRunning() || mass >= 0.f) {
				// create motion state
				auto scale = convertScaleBasedOnShape(gScale);
				collisionShapePtr->setLocalScaling(scale);
				btTransform btTrans(btQuat(gAng), btVector3(gPos.x, gPos.y, gPos.z));
				motionState = new btDefaultMotionState(btTrans);

				// create rigid body
				btVector3 inertia;
				collisionShapePtr->calculateLocalInertia(mass, inertia);
				btRigidBody::btRigidBodyConstructionInfo
					rigidBodyCI(mass, motionState, collisionShapePtr, inertia);
				rigidBody = new btRigidBody(rigidBodyCI);
				rigidBody->setUserPointer(manifest);
				if (mass > 0.f) {
					rigidBody->setActivationState(DISABLE_DEACTIVATION);
					rigidBody->setSleepingThresholds(0.f, 0.f);
				}

				// add a new rigid body to the simulation
				dynamicsWorld->addRigidBody(rigidBody);
			} else if (mass < 0.f) {
				// create kinematic body
				ghostObject = new btPairCachingGhostObject();
				btTransform btTrans(btQuat(gAng), btVector3(gPos.x, gPos.y, gPos.z));
				ghostObject->setWorldTransform(btTrans);
				ghostObject->setUserPointer(manifest);
				ghostObject->setActivationState(DISABLE_DEACTIVATION);
				ghostObject->setCollisionShape(collisionShapePtr);
				ghostObject->setCollisionFlags(btCollisionObject::CollisionFlags::CF_CHARACTER_OBJECT);
				dynamicsWorld->addCollisionObject(ghostObject, btBroadphaseProxy::CharacterFilter, btBroadphaseProxy::AllFilter);

				auto convexShape = static_cast<btConvexShape*>(collisionShapePtr);
				if (convexShape) {
					controller = new KinematicCharacterController(
						ghostObject, convexShape, 0.f,
						btVector3(0.f, 0.f, 1.f));
					controller->setGravity(btVector3(0.f, 0.f, 0.f));
					dynamicsWorld->addAction(controller);
				}
			}
		}
	}
}

void BBox::applyMoveForces(const Vector& vel, const Rotation& ang) {
	if (ghostObject && controller) {
		ghostObject->activate();
		Vector newVel = vel;
		controller->setLinearVelocity(newVel);

		float radians = 1.f / PI;
		btTransform trans = ghostObject->getWorldTransform();
		btQuaternion transrot = trans.getRotation();
		btQuaternion rotquat;
		rotquat = rotquat.getIdentity();
		rotquat.setX(ang.roll * radians);
		rotquat.setY(ang.pitch * radians);
		rotquat.setZ(ang.yaw * radians);
		transrot = transrot * rotquat;
		trans.setRotation(transrot);
		ghostObject->setWorldTransform(trans);

		//float degrees = 180.f / PI;
		//controller->setAngularVelocity(btVector3(ang.roll * degrees, ang.pitch * degrees, ang.yaw * degrees));
	}
}

void BBox::applyForce(const Vector& force, const Vector& origin) {
	if (rigidBody) {
		rigidBody->activate(true);
		rigidBody->applyForce(force, origin - gPos);
	}
}

ArrayList<Entity*> BBox::findAllOverlappingEntities() const {
	ArrayList<Entity*> outList;

	World* world = entity->getWorld();
	if (!world || !collisionShapePtr) {
		return outList;
	}

	auto dynamicsWorld = world->getBulletDynamicsWorld();
	btPairCachingGhostObject* ghost = new btPairCachingGhostObject();
	ghost->setWorldTransform(btTransform(btQuat(gAng), btVector3(gPos.x, gPos.y, gPos.z)));
	ghost->setCollisionShape(collisionShapePtr);
	ghost->setCollisionFlags(btCollisionObject::CollisionFlags::CF_NO_CONTACT_RESPONSE);
	dynamicsWorld->addCollisionObject(ghost, btBroadphaseProxy::SensorTrigger, btBroadphaseProxy::AllFilter);

	for (int i = 0; i < ghost->getNumOverlappingObjects(); i++) {
		btCollisionObject* obj = ghost->getOverlappingObject(i);
		auto manifest = static_cast<World::physics_manifest_t*>(obj->getUserPointer());
		if (manifest) {
			auto overlap = manifest->entity;
			if (overlap && overlap != entity) {
				bool found = false;
				for (auto entity : outList) {
					if (entity == overlap) {
						found = true;
						break;
					}
				}
				if (!found) {
					outList.push(overlap);
				}
			}
		}
	}

	dynamicsWorld->removeCollisionObject(ghost);
	delete ghost;

	return outList;
}

bool BBox::checkCollision() const {
	if (Component::checkCollision()) {
		return true;
	}

	if (editorOnly && !mainEngine->isEditorRunning()) {
		return false;
	}

	if (!enabled || entity->isFlag(Entity::FLAG_PASSABLE)) {
		return false;
	}

	World* world = entity->getWorld();
	if (!world) {
		return false;
	}

	auto convexShape = static_cast<btConvexShape*>(collisionShapePtr);
	if (!convexShape) {
		return false;
	}

	auto list = findAllOverlappingEntities();
	for (auto entity : list) {
		if (entity->isFlag(Entity::FLAG_PASSABLE)) {
			continue;
		} else {
			return true;
		}
	}

	return false;
}

static Cvar cvar_showBBoxes("showbboxes", "Makes bboxes visible", "0");

void BBox::draw(Camera& camera, const ArrayList<Light*>& lights) {
	Component::draw(camera, lights);

	if (!cvar_showBBoxes.toInt() && editorOnly) {
		// don't draw BBoxes that exist only for the editor
		return;
	}
	if (shape == SHAPE_MESH) {
		// the mesh shape adopts the form of its parent model
		return;
	}

	// only render in the editor!
	if (!cvar_showBBoxes.toInt()) {
		if (!mainEngine->isEditorRunning() || !entity->getWorld()->isShowTools() || camera.isOrtho()) {
			return;
		}
	}

	// don't draw unselected bboxes - it makes things very ugly
	if (!cvar_showBBoxes.toInt()) {
		if (mainEngine->isEditorRunning()) {
			if (!entity->isSelected() && parent != nullptr) {
				return;
			}
		}
	}

	// do not render for these fx passes
	if (camera.getDrawMode() >= Camera::DRAW_GLOW) {
		return;
	}

	GLboolean data = GL_FALSE;
	glGetBooleanv(GL_DEPTH_WRITEMASK, &data);
	glDepthMask(GL_FALSE);

	// build shader vars
	Mesh::shadervars_t shaderVars;
	shaderVars.lineWidth = 0;
	shaderVars.customColorEnabled = true;
	shaderVars.customColorR = { 0.f, 0.f, 0.f, 0.f };
	shaderVars.customColorB = { 0.f, 0.f, 0.f, 0.f };

	if (entity->isSelected()) {
		if (entity->isHighlighted()) {
			shaderVars.customColorA = { 1.f, 0.f, 0.f, 1.f };
		} else {
			shaderVars.customColorA = { .5f, 0.f, 0.f, 1.f };
		}
	} else {
		if (entity->isHighlighted()) {
			shaderVars.customColorA = { 0.f, 1.f, 0.f, 1.f };
		} else {
			shaderVars.customColorA = { 0.f, .5f, 0.f, 1.f };
		}
	}

	glm::mat4 undoScale = glm::scale(glm::mat4(1.f), glm::vec3(1.f / gScale.x, 1.f / gScale.z, 1.f / gScale.y));

	if (shape == SHAPE_CAPSULE && lScale.z > lScale.x && lScale.z > lScale.y) {
		float radius = max(gScale.x, gScale.y);

		Vector pos(0.f);
		Vector scale(radius * 2.f, radius * 2.f, gScale.z * 2.f);

		Material* material = mainEngine->getMaterialResource().dataForString(materialStr);
		if (material) {
			Mesh* mesh = nullptr;

			// draw capsule part
			mesh = mainEngine->getMeshResource().dataForString(meshCapsuleCylinderStr);
			if (mesh) {
				glm::mat4 scaleM = glm::scale(glm::mat4(1.f), glm::vec3(scale.x, scale.z - radius * 2.f, scale.y));
				glm::mat4 matrix = gMat * undoScale * scaleM;
				ShaderProgram* shader = mesh->loadShader(*this, camera, lights, material, shaderVars, matrix);
				if (shader) {
					mesh->draw(camera, this, shader);
				}
			}

			float cylinderHeight = scale.z - radius * 2.f;

			// draw first half-sphere
			mesh = mainEngine->getMeshResource().dataForString(meshCapsuleHalfSphereStr);
			if (mesh) {
				glm::mat4 scaleM = glm::scale(glm::mat4(1.f), glm::vec3(scale.x, radius * 2.f, scale.y));
				glm::mat4 matrix = gMat * undoScale * glm::translate(glm::mat4(), glm::vec3(0.f, cylinderHeight * .5f, 0.f)) * scaleM;
				ShaderProgram* shader = mesh->loadShader(*this, camera, lights, material, shaderVars, matrix);
				if (shader) {
					mesh->draw(camera, this, shader);
				}
			}

			// draw second half-sphere
			mesh = mainEngine->getMeshResource().dataForString(meshCapsuleHalfSphereStr);
			if (mesh) {
				glm::mat4 scaleM = glm::scale(glm::mat4(1.f), glm::vec3(scale.x, radius * 2.f, scale.y));
				glm::mat4 matrix = gMat * undoScale * glm::translate(glm::mat4(), glm::vec3(0.f, cylinderHeight * -.5f, 0.f)) * glm::rotate(glm::mat4(), PI, glm::vec3(0.f, 0.f, 1.f)) * scaleM;
				ShaderProgram* shader = mesh->loadShader(*this, camera, lights, material, shaderVars, matrix);
				if (shader) {
					mesh->draw(camera, this, shader);
				}
			}
		}
	} else {
		glm::mat4 matrix;
		Mesh* mesh = nullptr;
		if (shape == SHAPE_BOX) {
			mesh = mainEngine->getMeshResource().dataForString(meshBoxStr);

			glm::mat4 scaleM = glm::scale(glm::mat4(1.f), glm::vec3(gScale.x, gScale.z, gScale.y));
			matrix = gMat * undoScale * scaleM;
		} else if (shape == SHAPE_SPHERE || shape == SHAPE_CAPSULE) {
			mesh = mainEngine->getMeshResource().dataForString(meshSphereStr);

			float size = max(max(gScale.x, gScale.y), gScale.z) * 2.f;
			glm::mat4 scaleM = glm::scale(glm::mat4(1.f), glm::vec3(size, size, size));
			matrix = gMat * undoScale * scaleM;
		} else if (shape == SHAPE_CYLINDER) {
			mesh = mainEngine->getMeshResource().dataForString(meshCylinderStr);

			float size = max(gScale.x, gScale.y) * 2.f;
			glm::mat4 scaleM = glm::scale(glm::mat4(1.f), glm::vec3(size, gScale.z * 2.f, size));
			matrix = gMat * undoScale * scaleM;
		} else if (shape == SHAPE_CONE) {
			mesh = mainEngine->getMeshResource().dataForString(meshConeStr);

			float size = max(gScale.x, gScale.y) * 2.f;
			glm::mat4 scaleM = glm::scale(glm::mat4(1.f), glm::vec3(size, gScale.z * 2.f, size));
			matrix = gMat * undoScale * scaleM;
		}

		Material* material = mainEngine->getMaterialResource().dataForString(materialStr);
		if (mesh && material) {
			ShaderProgram* shader = mesh->loadShader(*this, camera, lights, material, shaderVars, matrix);
			if (shader) {
				mesh->draw(camera, this, shader);
			}
		}
	}

	glDepthMask(data);
}

void BBox::update() {
	Vector oldGScale = gScale;
	Component::update();
	updateRigidBody(oldGScale);
}

void BBox::updateBounds() {
	Component::updateBounds();
	glm::vec3 bScale = glm::vec3(gScale.x, gScale.z, gScale.y);
	glm::mat4 mat = glm::mat4(glm::quat(gAng.w, gAng.x, gAng.y, gAng.z));
	for (float x = -1.f; x <= 1.f; x += 2.f) {
		for (float y = -1.f; y <= 1.f; y += 2.f) {
			for (float z = -1.f; z <= 1.f; z += 2.f) {
				glm::vec3 mScale = mat * glm::vec4(bScale * glm::vec3(x, y, z), 1.f);
				Vector scale = Vector(mScale.x, mScale.z, -mScale.y);
				boundsMax.x = std::max(boundsMax.x, scale.x);
				boundsMax.y = std::max(boundsMax.y, scale.y);
				boundsMax.z = std::max(boundsMax.z, scale.z);
				boundsMin.x = std::min(boundsMin.x, scale.x);
				boundsMin.y = std::min(boundsMin.y, scale.y);
				boundsMin.z = std::min(boundsMin.z, scale.z);
			}
		}
	}
	boundsMax += gPos - entity->getPos();
	boundsMin += gPos - entity->getPos();
}

void BBox::load(FILE* fp) {
	Component::load(fp);

	Engine::freadl(&shape, sizeof(shape_t), 1, fp, nullptr, "BBox::load()");

	Uint32 reserved = 0;
	Engine::freadl(&reserved, sizeof(Uint32), 1, fp, nullptr, "BBox::load()");

	// new data 2017/11/29
	if (reserved == 1) {
		Engine::freadl(&enabled, sizeof(bool), 1, fp, nullptr, "BBox::load()");

		reserved = 0;
		Engine::freadl(&reserved, sizeof(Uint32), 1, fp, nullptr, "BBox::load()");
	}

	loadSubComponents(fp);
}

// save/load this object to a file
// @param file interface to serialize with
void BBox::serialize(FileInterface* file) {
	Component::serialize(file);

	Uint32 version = 1;
	file->property("BBox::version", version);
	file->property("shape", shape);
	file->property("enabled", enabled);
	if (version >= 1) {
		file->property("mass", mass);
	}
}
