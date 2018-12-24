// BBox.cpp

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Main.hpp"
#include "Engine.hpp"
#include "BBox.hpp"
#include "Mesh.hpp"
#include "Model.hpp"
#include "Tile.hpp"
#include "World.hpp"
#include "TileWorld.hpp"
#include "Camera.hpp"

const float BBox::collisionEpsilon = .01f;

const char* BBox::meshCapsuleCylinderStr = "assets/editor/bbox/CapsuleCylinder.FBX";
const char* BBox::meshCapsuleHalfSphereStr = "assets/editor/bbox/CapsuleHalfSphere.FBX";
const char* BBox::meshConeStr = "assets/editor/bbox/Cone.FBX";
const char* BBox::meshCylinderStr = "assets/editor/bbox/Cylinder.FBX";
const char* BBox::meshSphereStr = "assets/editor/bbox/Sphere.FBX";
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
	createRigidBody();
}

void BBox::deleteRigidBody() {
	if( rigidBody!=nullptr ) {
		if( dynamicsWorld ) {
			dynamicsWorld->removeRigidBody(rigidBody);
		}
		delete rigidBody;
		rigidBody = nullptr;
	}
	if( motionState!=nullptr ) {
		delete motionState;
		motionState = nullptr;
	}
	if( collisionShapePtr!=nullptr ) {
		delete collisionShapePtr;
		collisionShapePtr = nullptr;
	}
	if( triMesh!=nullptr ) {
		delete triMesh;
		triMesh = nullptr;
	}
}

void BBox::conformToModel(const Model& model) {
	// this doesn't work for animated meshes, yet
	if( model.hasAnimations() ) {
		return;
	}

	meshName = model.getMesh();
	Mesh* mesh = mainEngine->getMeshResource().dataForString(model.getMesh());
	if( !mesh ) {
		return;
	}
	if( triMesh != nullptr ) {
		delete triMesh;
		triMesh = nullptr;
	}

	// iterate through every submesh
	for( const Node<Mesh::SubMesh*>* entryNode = mesh->getSubMeshes().getFirst(); entryNode!=nullptr; entryNode=entryNode->getNext() ) {
		const Mesh::SubMesh* entry = entryNode->getData();

		if (!entry->getVertices())
			continue;

		if (!triMesh)
			triMesh = new btTriangleMesh();

		// iterate through each index
		LinkedList<Vector> vlist;
		for( unsigned int i=0; i<entry->getNumIndices(); i+=2 ) {
			GLuint index = entry->getIndices()[i];

			const float& x = entry->getVertices()[index * 3];
			const float& y = entry->getVertices()[index * 3 + 1];
			const float& z = entry->getVertices()[index * 3 + 2];

			// create vertex for physics mesh
			Vector v(x,z,-y);
			vlist.addNodeLast(v);

			if( vlist.getSize()>=3 ) {
				// add the three vertices to the triangle mesh
				Vector& v0 = vlist.nodeForIndex(0)->getData();
				Vector& v1 = vlist.nodeForIndex(1)->getData();
				Vector& v2 = vlist.nodeForIndex(2)->getData();
				triMesh->addTriangle(v0,v1,v2,true);

				// clear the triangle list
				vlist.removeAll();
			}
		}

		// clear the vertex list (in case there are leftovers)
		vlist.removeAll();
	}
}

btTransform BBox::getPhysicsTransform() const {
	if (!motionState) {
		return btTransform();
	} else {
		btTransform transform;
		motionState->getWorldTransform(transform);
		return transform;
	}
}

void BBox::updateRigidBody(const Vector& oldGScale) {
	if (mainEngine->isEditorRunning() || mass <= 0.f) {
		dirty = true;
	} else {
		float epsilon = 0.1;
		if( fabs(oldGScale.lengthSquared() - gScale.lengthSquared()) > epsilon) {
			dirty = true;
			if( shape == SHAPE_MESH ) {
				if( parent && parent->getType() == Component::COMPONENT_MODEL ) {
					Model* model = static_cast<Model*>(parent);
					if( model ) {
						meshName = model->getMesh();
					}
				}
			}
		}
	}

	if( dirty ) {
		createRigidBody();
		dirty = false;
	} else {
		if (parent != nullptr || mass <= 0.f) {
			if (motionState) {
				if (shape == SHAPE_MESH) {
					btQuaternion btQuat;
					btQuat.setEulerZYX(gAng.yaw, -gAng.pitch, gAng.roll);
					collisionShapePtr->setLocalScaling(btVector3(fabs(gScale.x), fabs(gScale.y), fabs(gScale.z)));
					btTransform btTrans(btQuat, btVector3(gPos.x, gPos.y, gPos.z));
					motionState->setWorldTransform(btTrans);
				} else {
					btTransform btTrans(btQuaternion(0.f, 0.f, 0.f, 1.f), btVector3(gPos.x, gPos.y, gPos.z));
					motionState->setWorldTransform(btTrans);
				}
			}
		}
	}
}

void BBox::createRigidBody() {
	deleteRigidBody();

	// setup new collision volume
	switch( shape ) {
		case SHAPE_SPHERE:
			collisionShapePtr = new btSphereShape(max(max(gScale.x,gScale.y),gScale.z));
			break;
		case SHAPE_CAPSULE:
			collisionShapePtr = new btCapsuleShapeZ(max(gScale.x,gScale.y),gScale.z);
			break;
		case SHAPE_CYLINDER:
			collisionShapePtr = new btCylinderShapeZ(btVector3(gScale.x,gScale.y,gScale.z));
			break;
		case SHAPE_CONE:
			collisionShapePtr = new btConeShapeZ(max(gScale.x,gScale.y),gScale.z);
			break;
		case SHAPE_MESH:
			if( editorOnly && !mainEngine->isEditorRunning() ) {
				return;
			}
			if( triMesh ) {
				collisionShapePtr = new btBvhTriangleMeshShape(triMesh,true,true);
			} else {
				if( parent && parent->getType() == Component::COMPONENT_MODEL ) {
					Model* model = static_cast<Model*>(parent);
					conformToModel(*model);
				}
				if( triMesh && triMesh->getNumTriangles() > 0 ) {
					collisionShapePtr = new btBvhTriangleMeshShape(triMesh,true,true);
				} else {
					shape = SHAPE_CYLINDER;
					mainEngine->fmsg(Engine::MSG_DEBUG,"mesh shape assigned to bbox, but mesh is unavailable");
					collisionShapePtr = new btCylinderShapeZ(btVector3(gScale.x,gScale.y,gScale.z));
				}
			}
			break;
		default:
			collisionShapePtr = new btBoxShape(btVector3(gScale.x,gScale.y,gScale.z));
			break;
	}

	// create motion state
	if( shape == SHAPE_MESH ) {
		btQuaternion btQuat;
		btQuat.setEulerZYX(gAng.yaw, -gAng.pitch, gAng.roll);
		collisionShapePtr->setLocalScaling(btVector3(fabs(gScale.x), fabs(gScale.y), fabs(gScale.z)));
		motionState = new btDefaultMotionState(btTransform(btQuat, btVector3(gPos.x, gPos.y, gPos.z)));
	} else {
		motionState = new btDefaultMotionState(btTransform(btQuaternion(0.f, 0.f, 0.f, 1.f), btVector3(gPos.x, gPos.y, gPos.z)));
	}

	// create rigid body
	btVector3 inertia;
	collisionShapePtr->calculateLocalInertia(mass, inertia);
	btRigidBody::btRigidBodyConstructionInfo
		rigidBodyCI(mass, motionState, collisionShapePtr, inertia);
	rigidBody = new btRigidBody(rigidBodyCI);
	rigidBody->setUserIndex(entity->getUID());
	rigidBody->setUserIndex2(World::nuid);
	rigidBody->setUserPointer(nullptr);
	rigidBody->setSleepingThresholds(0.f, 0.f);

	// add a new rigid body to the simulation
	World* world = entity->getWorld();
	if( world ) {
		dynamicsWorld = world->getBulletDynamicsWorld();
		if( dynamicsWorld ) {
			dynamicsWorld->addRigidBody(rigidBody);
		}
	}
}

bool BBox::containsPoint(const Vector& point) const {
	if( point.x >= gPos.x - gScale.x && 
		point.x <= gPos.x + gScale.x ) {
		if( point.y >= gPos.y - gScale.y && 
			point.y <= gPos.y + gScale.y ) {
			if( point.z >= gPos.z - gScale.z &&
				point.z <= gPos.z + gScale.z ) {
				return true;
			}
		}
	}
	return false;
}

float BBox::distToFloor(float floorHeight) {
	float z = gPos.z + gScale.z;
	return max( 0.f, floorHeight - z );
}

void BBox::testAgainstComponentForFloor(Component* component, const Vector& start, const Vector& end, float& nearestFloor) const {
	if( component->getType() == COMPONENT_BBOX ) {
		BBox* bbox = static_cast<BBox*>(component);
		if( bbox->isEnabled() ) {
			const Vector& gPos2 = component->getGlobalPos();
			const Vector& gScale2 = component->getGlobalScale();
			float startX2 = gPos2.x - gScale2.x;
			float endX2 = gPos2.x + gScale2.x;
			float startY2 = gPos2.y - gScale2.y;
			float endY2 = gPos2.y + gScale2.y;
			float startZ2 = gPos2.z - gScale2.z;
			float endZ2 = gPos2.z + gScale2.z;

			if( start.x < endX2 && end.x > startX2 ) {
				if( start.y < endY2 && end.y > startY2 ) {
					if( start.z <= startZ2 ) {
						nearestFloor = min( startZ2, nearestFloor );
					}
				}
			}
		}
	}

	for( Uint32 c = 0; c < component->getComponents().getSize(); ++c ) {
		Component* sub = component->getComponents()[c];
		testAgainstComponentForFloor(sub, start, end, nearestFloor);
	}
}

float BBox::nearestFloor() {
	float nearestFloor = (float)(UINT32_MAX);
	World* world = entity->getWorld();
	if( !world ) {
		return nearestFloor;
	}

	Vector start( ( gPos.x - gScale.x ) + collisionEpsilon, ( gPos.y - gScale.y ) + collisionEpsilon, ( gPos.z - gScale.z ) + collisionEpsilon );
	Vector end( ( gPos.x + gScale.x ) - collisionEpsilon, ( gPos.y + gScale.y ) - collisionEpsilon, ( gPos.z + gScale.z ) - collisionEpsilon );
	float xInc = std::min( (float)Tile::size, (end.x - start.x) - collisionEpsilon );
	float yInc = std::min( (float)Tile::size, (end.y - start.y) - collisionEpsilon );

	// check against entities
	for( Uint32 c = 0; c < World::numBuckets; ++c ) {
		for( Node<Entity*>* node = world->getEntities(c).getFirst(); node != nullptr; node = node->getNext() ) {
			Entity* entity = node->getData();

			if( entity == this->entity || entity->isFlag(Entity::flag_t::FLAG_PASSABLE) ) {
				continue;
			}

			for( Uint32 c = 0; c < entity->getComponents().getSize(); ++c ) {
				Component* component = entity->getComponents()[c];
				testAgainstComponentForFloor(component, start, end, nearestFloor);
			}
		}
	}

	// check against tiles
	if( world->getType() == World::WORLD_TILES ) {
		TileWorld* tileworld = static_cast<TileWorld*>(world);
		ArrayList<Tile>& tiles = tileworld->getTiles();
		for( float x = start.x; x <= end.x; x += std::max(collisionEpsilon, std::min(xInc, end.x - x)) ) {
			for( float y = start.y; y <= end.y; y += std::max(collisionEpsilon, std::min(yInc, end.y - y)) ) {
				int sX = min( max( 0, (int)floor(x / (float)Tile::size) ), (int)tileworld->getWidth()-1 );
				int sY = min( max( 0, (int)floor(y / (float)Tile::size) ), (int)tileworld->getHeight()-1 );
				Tile& tile = tiles[sY + sX * tileworld->getHeight()];

				// test against floor
				glm::vec3 floorVec( x, y, (float)tile.getFloorHeight() );
				tile.setFloorSlopeHeightForVec(floorVec);
				nearestFloor = min( floorVec.z, nearestFloor);
			}
		}
	}

	// result
	return nearestFloor;
}

float BBox::distToCeiling(float ceilingHeight) {
	float z = gPos.z - gScale.z;
	return max( 0.f, z - ceilingHeight );
}

void BBox::testAgainstComponentForCeiling(Component* component, const Vector& start, const Vector& end, float& nearestCeiling) const {
	if( component->getType() == COMPONENT_BBOX ) {
		BBox* bbox = static_cast<BBox*>(component);
		if( bbox->isEnabled() ) {
			const Vector& gPos2 = component->getGlobalPos();
			const Vector& gScale2 = component->getGlobalScale();
			float startX2 = gPos2.x - gScale2.x;
			float endX2 = gPos2.x + gScale2.x;
			float startY2 = gPos2.y - gScale2.y;
			float endY2 = gPos2.y + gScale2.y;
			float startZ2 = gPos2.z - gScale2.z;
			float endZ2 = gPos2.z + gScale2.z;

			if( start.x < endX2 && end.x > startX2 ) {
				if( start.y < endY2 && end.y > startY2 ) {
					if( end.z >= endZ2 ) {
						nearestCeiling = max( endZ2, nearestCeiling );
					}
				}
			}
		}
	}
	
	for( Uint32 c = 0; c < component->getComponents().getSize(); ++c ) {
		Component* sub = component->getComponents()[c];
		testAgainstComponentForCeiling(sub, start, end, nearestCeiling);
	}
}

float BBox::nearestCeiling() {
	float nearestCeiling = (float)(INT32_MIN);
	World* world = entity->getWorld();
	if( !world ) {
		return nearestCeiling;
	}

	Vector start( ( gPos.x - gScale.x ) + collisionEpsilon, ( gPos.y - gScale.y ) + collisionEpsilon, ( gPos.z - gScale.z ) + collisionEpsilon );
	Vector end( ( gPos.x + gScale.x ) - collisionEpsilon, ( gPos.y + gScale.y ) - collisionEpsilon, ( gPos.z + gScale.z ) - collisionEpsilon );
	float xInc = std::min( (float)Tile::size, (end.x - start.x) - collisionEpsilon );
	float yInc = std::min( (float)Tile::size, (end.y - start.y) - collisionEpsilon );

	// check against entities
	for( Uint32 c = 0; c < World::numBuckets; ++c ) {
		for( Node<Entity*>* node = world->getEntities(c).getFirst(); node != nullptr; node = node->getNext() ) {
			Entity* entity = node->getData();

			if( entity == this->entity || entity->isFlag(Entity::flag_t::FLAG_PASSABLE) ) {
				continue;
			}

			for( Uint32 c = 0; c < entity->getComponents().getSize(); ++c ) {
				Component* component = entity->getComponents()[c];
				testAgainstComponentForCeiling(component, start, end, nearestCeiling);
			}
		}
	}

	// check against tiles
	if( world->getType() == World::WORLD_TILES ) {
		TileWorld* tileworld = static_cast<TileWorld*>(world);
		ArrayList<Tile>& tiles = tileworld->getTiles();
		for( float x = start.x; x <= end.x; x += std::max(collisionEpsilon, std::min(xInc, end.x - x)) ) {
			for( float y = start.y; y <= end.y; y += std::max(collisionEpsilon, std::min(yInc, end.y - y)) ) {
				int sX = min( max( 0, (int)floor(x / (float)Tile::size) ), (int)tileworld->getWidth()-1 );
				int sY = min( max( 0, (int)floor(y / (float)Tile::size) ), (int)tileworld->getHeight()-1 );
				Tile& tile = tiles[sY + sX * tileworld->getHeight()];

				// test against floor
				glm::vec3 ceilingVec( x, y, (float)tile.getCeilingHeight() );
				tile.setCeilingSlopeHeightForVec(ceilingVec);
				nearestCeiling = max( ceilingVec.z, nearestCeiling );
			}
		}
	}

	// result
	return nearestCeiling;
}

bool BBox::testAgainstComponentForObstacle(Component* component, const Vector& start, const Vector& end) const {
	if( component->getType() == COMPONENT_BBOX ) {
		BBox* bbox = static_cast<BBox*>(component);
		if( bbox->isEnabled() ) {
			const Vector& gPos2 = component->getGlobalPos();
			const Vector& gScale2 = component->getGlobalScale();
			float startX2 = gPos2.x - gScale2.x;
			float endX2 = gPos2.x + gScale2.x;
			float startY2 = gPos2.y - gScale2.y;
			float endY2 = gPos2.y + gScale2.y;
			float startZ2 = gPos2.z - gScale2.z;
			float endZ2 = gPos2.z + gScale2.z;

			if( start.x < endX2 && end.x > startX2 ) {
				if( start.y < endY2 && end.y > startY2 ) {
					if( start.z < endZ2 && end.z > startZ2 ) {
						return true;
					}
				}
			}
		}
	}

	for( Uint32 c = 0; c < component->getComponents().getSize(); ++c ) {
		Component* sub = component->getComponents()[c];
		if( testAgainstComponentForObstacle(sub, start, end) ) {
			return true;
		}
	}
	return false;
}

LinkedList<const Entity*> BBox::findAllOverlappingEntities() const {
	LinkedList<const Entity*> list;

	World* world = entity->getWorld();
	if( world ) {
		Vector start( ( gPos.x - gScale.x ) + collisionEpsilon, ( gPos.y - gScale.y ) + collisionEpsilon, ( gPos.z - gScale.z ) + collisionEpsilon );
		Vector end( ( gPos.x + gScale.x ) - collisionEpsilon, ( gPos.y + gScale.y ) - collisionEpsilon, ( gPos.z + gScale.z ) - collisionEpsilon );

		// check against entities
		for( Uint32 c = 0; c < World::numBuckets; ++c ) {
			for( Node<Entity*>* node = world->getEntities(c).getFirst(); node != nullptr; node = node->getNext() ) {
				Entity* entity = node->getData();

				if( entity == this->entity || entity->isFlag(Entity::flag_t::FLAG_PASSABLE) ) {
					continue;
				}

				bool overlaps = false;
				for( Uint32 c = 0; c < entity->getComponents().getSize(); ++c ) {
					Component* component = entity->getComponents()[c];
					if( testAgainstComponentForObstacle(component, start, end) ) {
						overlaps = true;
						break;
					}
				}

				if( overlaps ) {
					list.addNodeLast(entity);
					continue;
				}
			}
		}
	}

	return list;
}

bool BBox::checkCollision() const {
	if( Component::checkCollision() ) {
		return true;
	}

	if( editorOnly && !mainEngine->isEditorRunning() ) {
		return false;
	}

	if( !enabled ) {
		return false;
	}

	World* world = entity->getWorld();
	if( !world ) {
		return false;
	}

	Vector start( ( gPos.x - gScale.x ) + collisionEpsilon, ( gPos.y - gScale.y ) + collisionEpsilon, ( gPos.z - gScale.z ) + collisionEpsilon );
	Vector end( ( gPos.x + gScale.x ) - collisionEpsilon, ( gPos.y + gScale.y ) - collisionEpsilon, ( gPos.z + gScale.z ) - collisionEpsilon );
	float xInc = std::min( (float)Tile::size, (end.x - start.x) - collisionEpsilon );
	float yInc = std::min( (float)Tile::size, (end.y - start.y) - collisionEpsilon );
	float zInc = (end.z - start.z) - collisionEpsilon;
	
	// check against entities
	for( Uint32 c = 0; c < World::numBuckets; ++c ) {
		for( Node<Entity*>* node = world->getEntities(c).getFirst(); node != nullptr; node = node->getNext() ) {
			Entity* entity = node->getData();

			if( entity == this->entity || entity->isFlag(Entity::flag_t::FLAG_PASSABLE) ) {
				continue;
			}

			for( Uint32 c = 0; c < entity->getComponents().getSize(); ++c ) {
				Component* component = entity->getComponents()[c];
				if( testAgainstComponentForObstacle(component, start, end) ) {
					return true;
				}
			}
		}
	}

	// check against tiles
	if( world->getType() == World::WORLD_TILES ) {
		TileWorld* tileworld = static_cast<TileWorld*>(world);
		ArrayList<Tile>& tiles = tileworld->getTiles();
		for( float x = start.x; x <= end.x; x += std::max(collisionEpsilon, std::min(xInc, end.x - x)) ) {
			for( float y = start.y; y <= end.y; y += std::max(collisionEpsilon, std::min(yInc, end.y - y)) ) {
				for( float z = start.z; z <= end.z; z += zInc ) {
					int sX = min( max( 0, (int)floor(x / (float)Tile::size) ), (int)tileworld->getWidth()-1 );
					int sY = min( max( 0, (int)floor(y / (float)Tile::size) ), (int)tileworld->getHeight()-1 );
					Tile& tile = tiles[sY + sX * tileworld->getHeight()];

					// test against floor
					glm::vec3 floorVec( x, y, (float)tile.getFloorHeight() );
					tile.setFloorSlopeHeightForVec(floorVec);
					if( z > floorVec.z ) {
						return true;
					}

					// test against ceiling
					glm::vec3 ceilingVec( x, y, (float)tile.getCeilingHeight() );
					tile.setCeilingSlopeHeightForVec(ceilingVec);
					if( z < ceilingVec.z ) {
						return true;
					}
				}
			}
		}
	}

	return false;
}

static Cvar cvar_showBBoxes("showbboxes", "Makes bboxes visible", "0");

void BBox::draw(Camera& camera, const ArrayList<Light*>& lights) {
	Component::draw(camera,lights);

	if( editorOnly ) {
		// don't draw BBoxes that exist only for the editor
		return;
	}
	if( shape == SHAPE_MESH ) {
		// the mesh shape adopts the form of its parent model
		return;
	}

	// only render in the editor!
	if( !cvar_showBBoxes.toInt() ) {
		if( !mainEngine->isEditorRunning() || !entity->getWorld()->isShowTools() || camera.isOrtho() ) {
			return;
		}
	}

	// do not render for these fx passes
	if( camera.getDrawMode() >= Camera::DRAW_GLOW ) {
		return;
	}

	GLboolean data = GL_FALSE;
	glGetBooleanv(GL_DEPTH_WRITEMASK, &data);
	glDepthMask(GL_FALSE);

	if( shape == SHAPE_BOX ) {
		glm::mat4 cubeM = glm::translate(glm::mat4(1.f),glm::vec3(gPos.x,-gPos.z,gPos.y));
		cubeM = glm::scale(cubeM,glm::vec3(gScale.x*2, gScale.z*2, gScale.y*2));

		if( entity->isSelected() ) {
			if( entity->isHighlighted() ) {
				camera.drawCube(cubeM,glm::vec4(1.f,0.f,0.f,1.f));
			} else {
				camera.drawCube(cubeM,glm::vec4(1.f,0.f,0.f,.5f));
			}
		} else {
			if( entity->isHighlighted() ) {
				camera.drawCube(cubeM,glm::vec4(0.f,1.f,0.f,1.f));
			} else {
				camera.drawCube(cubeM,glm::vec4(0.f,1.f,0.f,.5f));
			}
		}
	} else {
		// build shader vars
		Mesh::shadervars_t shaderVars;
		shaderVars.lineWidth = 0;
		shaderVars.customColorEnabled = GL_TRUE;
		shaderVars.customColorR = { 0.f, 0.f, 0.f, 0.f };
		shaderVars.customColorB = { 0.f, 0.f, 0.f, 0.f };

		if( entity->isSelected() ) {
			if( entity->isHighlighted() ) {
				shaderVars.customColorA = { 1.f, 0.f, 0.f, 1.f };
			} else {
				shaderVars.customColorA = { .5f, 0.f, 0.f, 1.f };
			}
		} else {
			if( entity->isHighlighted() ) {
				shaderVars.customColorA = { 0.f, 1.f, 0.f, 1.f };
			} else {
				shaderVars.customColorA = { 0.f, .5f, 0.f, 1.f };
			}
		}

		if( shape == SHAPE_CAPSULE && lScale.z > lScale.x && lScale.z > lScale.y ) {
			float radius = max(gScale.x,gScale.y);

			Vector pos(0.f);
			Vector scale(radius * 2.f, radius * 2.f, gScale.z * 2.f);

			Material* material = mainEngine->getMaterialResource().dataForString(materialStr);
			if( material ) {
				Mesh* mesh = nullptr;

				scale.z -= radius * 2.f;

				// draw capsule part
				mesh = mainEngine->getMeshResource().dataForString(meshCapsuleCylinderStr);
				if( mesh ) {
					glm::mat4 translationM = glm::translate(glm::mat4(1.f),glm::vec3(gPos.x, -gPos.z, gPos.y));
					glm::mat4 rotationM = glm::mat4( 1.f );
					rotationM = glm::rotate(rotationM, (float)(gAng.radiansPitch()), glm::vec3(1.f, 0.f, 0.f));
					rotationM = glm::rotate(rotationM, (float)(gAng.radiansYaw()), glm::vec3(0.f, 1.f, 0.f));
					rotationM = glm::rotate(rotationM, (float)(gAng.radiansRoll()), glm::vec3(cos(gAng.yaw), 0.f, sin(gAng.yaw)));
					glm::mat4 scaleM = glm::scale(glm::mat4(1.f), glm::vec3(scale.x, scale.z, scale.y));
					glm::mat4 matrix = translationM * rotationM * scaleM;
					ShaderProgram* shader = mesh->loadShader(*this, camera, lights, material, shaderVars, matrix);
					if( shader ) {
						mesh->draw(camera, this, shader);
					}
				}

				float cylinderHeight = lScale.z;
				scale.z = radius * 2.f;
				pos.z -= cylinderHeight - radius;

				// draw first half-sphere
				mesh = mainEngine->getMeshResource().dataForString(meshCapsuleHalfSphereStr);
				if( mesh ) {
					glm::mat4 translationM = glm::translate(glm::mat4(1.f),glm::vec3(gPos.x + pos.x, -gPos.z - pos.z, gPos.y + pos.y));
					glm::mat4 rotationM = glm::mat4( 1.f );
					rotationM = glm::rotate(rotationM, (float)(gAng.radiansPitch()), glm::vec3(1.f, 0.f, 0.f));
					rotationM = glm::rotate(rotationM, (float)(gAng.radiansYaw()), glm::vec3(0.f, 1.f, 0.f));
					rotationM = glm::rotate(rotationM, (float)(gAng.radiansRoll()), glm::vec3(cos(gAng.yaw), 0.f, sin(gAng.yaw)));
					glm::mat4 scaleM = glm::scale(glm::mat4(1.f), glm::vec3(scale.x, scale.z, scale.y));
					glm::mat4 matrix = translationM * rotationM * scaleM;
					ShaderProgram* shader = mesh->loadShader(*this, camera, lights, material, shaderVars, matrix);
					if( shader ) {
						mesh->draw(camera, this, shader);
					}
				}

				pos.z += (cylinderHeight - radius) * 2;

				// draw second half-sphere
				mesh = mainEngine->getMeshResource().dataForString(meshCapsuleHalfSphereStr);
				if( mesh ) {
					glm::mat4 translationM = glm::translate(glm::mat4(1.f),glm::vec3(gPos.x + pos.x, -gPos.z - pos.z, gPos.y + pos.y));
					glm::mat4 rotationM = glm::mat4( 1.f );
					rotationM = glm::rotate(rotationM, (float)(gAng.radiansPitch() + PI), glm::vec3(1.f, 0.f, 0.f));
					rotationM = glm::rotate(rotationM, (float)(gAng.radiansYaw()), glm::vec3(0.f, 1.f, 0.f));
					rotationM = glm::rotate(rotationM, (float)(gAng.radiansRoll()), glm::vec3(cos(gAng.yaw), 0.f, sin(gAng.yaw)));
					glm::mat4 scaleM = glm::scale(glm::mat4(1.f), glm::vec3(scale.x, scale.z, scale.y));
					glm::mat4 matrix = translationM * rotationM * scaleM;
					ShaderProgram* shader = mesh->loadShader(*this, camera, lights, material, shaderVars, matrix);
					if( shader ) {
						mesh->draw(camera, this, shader);
					}
				}
			}
		} else {
			glm::mat4 translationM = glm::translate(glm::mat4(1.f),glm::vec3(gPos.x,-gPos.z,gPos.y));
			glm::mat4 rotationM = glm::mat4( 1.f );
			rotationM = glm::rotate(rotationM, (float)(gAng.radiansPitch()), glm::vec3(1.f, 0.f, 0.f));
			rotationM = glm::rotate(rotationM, (float)(gAng.radiansYaw()), glm::vec3(0.f, 1.f, 0.f));
			rotationM = glm::rotate(rotationM, (float)(gAng.radiansRoll()), glm::vec3(cos(gAng.yaw), 0.f, sin(gAng.yaw)));

			glm::mat4 matrix;
			Mesh* mesh = nullptr;
			if( shape == SHAPE_SPHERE || shape == SHAPE_CAPSULE ) {
				mesh = mainEngine->getMeshResource().dataForString(meshSphereStr);
				
				float size = max(max(lScale.x,lScale.y),lScale.z) * 2.f;
				glm::mat4 scaleM = glm::scale(glm::mat4(1.f),glm::vec3(size, size, size));
				matrix = translationM * rotationM * scaleM;
			} else if ( shape == SHAPE_CYLINDER ) {
				mesh = mainEngine->getMeshResource().dataForString(meshCylinderStr);

				float size = max(lScale.x,lScale.y) * 2.f;
				glm::mat4 scaleM = glm::scale(glm::mat4(1.f),glm::vec3(size, gScale.z * 2.f, size));
				matrix = translationM * rotationM * scaleM;
			} else if ( shape == SHAPE_CONE ) {
				mesh = mainEngine->getMeshResource().dataForString(meshConeStr);

				float size = max(lScale.x,lScale.y) * 2.f;
				glm::mat4 scaleM = glm::scale(glm::mat4(1.f),glm::vec3(size, gScale.z * 2.f, size));
				matrix = translationM * rotationM * scaleM;
			}

			Material* material = mainEngine->getMaterialResource().dataForString(materialStr);
			if( mesh && material ) {
				ShaderProgram* shader = mesh->loadShader(*this, camera, lights, material, shaderVars, matrix);
				if( shader ) {
					mesh->draw(camera, this, shader);
				}
			}
		}
	}

	glDepthMask(data);
}

static void toEulerAngle(const btQuaternion& q, float& yaw, float& pitch, float& roll)
{
	// roll (x-axis rotation)
	float sinr = 2.f * (q.w() * q.x() + q.y() * q.z());
	float cosr = 1.f - 2.f * (q.x() * q.x() + q.y() * q.y());
	roll = atan2(sinr, cosr);

	// pitch (y-axis rotation)
	float sinp = 2.f * (q.w() * q.y() - q.z() * q.x());
	if (fabs(sinp) >= 1.f) {
		pitch = copysign(PI / 2.f, sinp); // use 90 degrees if out of range
	} else {
		pitch = asin(sinp);
	}

	// yaw (z-axis rotation)
	float siny = +2.f * (q.w() * q.z() + q.x() * q.y());
	float cosy = +1.f - 2.f * (q.y() * q.y() + q.z() * q.z());  
	yaw = atan2(siny, cosy);
}

/*void BBox::process() {
	if( motionState ) {
		btTransform btMat;
		motionState->getWorldTransform(btMat);
		btQuaternion btQuat = btMat.getRotation();
		btVector3 btVec = btMat.getOrigin();

		Vector pos = btVec;
		Angle ang;
		toEulerAngle(btQuat, ang.yaw, ang.pitch, ang.roll);
	}
}*/

void BBox::update() {
	Vector oldGScale = gScale;
	Component::update();

	// hack to "rotate" aabb
	if( shape == SHAPE_BOX && lScale.x != lScale.y ) {
		glm::mat4 m = glm::mat4( 1.f );
		m = glm::rotate(m, (float)(gAng.radiansPitch()), glm::vec3(1.f, 0.f, 0.f));
		m = glm::rotate(m, (float)(gAng.radiansYaw()), glm::vec3(0.f, 1.f, 0.f));
		//m = glm::rotate(m, (float)(gAng.radiansRoll()), glm::vec3(cos(gAng.yaw), 0.f, sin(gAng.yaw)));
		glm::vec4 s = glm::vec4(gScale.x, gScale.z, gScale.y, 1.f) * m;

		float min = std::min(gScale.x, std::min(gScale.y, gScale.z));
		gScale.x = std::max(fabs(s.x), min);
		gScale.y = std::max(fabs(s.z), min);
		gScale.z = std::max(fabs(s.y), min);
	}

	updateRigidBody(oldGScale);
}

void BBox::load(FILE* fp) {
	Component::load(fp);

	Engine::freadl(&shape, sizeof(shape_t), 1, fp, nullptr, "BBox::load()");

	Uint32 reserved = 0;
	Engine::freadl(&reserved, sizeof(Uint32), 1, fp, nullptr, "BBox::load()");

	// new data 2017/11/29
	if( reserved == 1 ) {
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
