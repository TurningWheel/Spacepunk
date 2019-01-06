// World.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "World.hpp"
#include "Client.hpp"
#include "Server.hpp"
#include "Script.hpp"
#include "Path.hpp"
#include "Shadow.hpp"
#include "Entity.hpp"
#include "BBox.hpp"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

const Uint32 World::nuid = UINT32_MAX;
const char* World::fileExtensions[World::FILE_MAX] = {
	"wlb",
	"wlj",
	"wld"
};

Cvar cvar_showEdges("showedges", "highlight chunk silhouettes with visible lines", "0");
Cvar cvar_showVerts("showverts", "highlight all triangle edges with visible lines", "0");

World::World()
{
	script = new Script(*this);
}

World::~World() {
	if( !silent ) {
		mainEngine->fmsg(Engine::MSG_INFO,"deleting world '%s'",nameStr.get());
	}

	// delete entities
	for( Uint32 c=0; c<numBuckets; ++c ) {
		while( entities[c].getFirst() ) {
			delete entities[c].getFirst()->getData();
			entities[c].removeNode(entities[c].getFirst());
		}
	}

	// delete script engine
	if( script ) {
		delete script;
		script = nullptr;
	}

	// delete physics data
	delete bulletDynamicsWorld;
	delete bulletSolver;
	delete bulletDispatcher;
	delete bulletCollisionConfiguration;
	delete bulletBroadphase;
}

void World::initialize(bool empty) {
	mainEngine->fmsg(Engine::MSG_INFO,"creating physics simulation...");

	// build the broadphase
	bulletBroadphase = new btDbvtBroadphase();

	// set up the collision configuration and dispatcher
	bulletCollisionConfiguration = new btDefaultCollisionConfiguration();
	bulletDispatcher = new btCollisionDispatcher(bulletCollisionConfiguration);

	// the actual physics solver
	bulletSolver = new btSequentialImpulseConstraintSolver;

	// the world
	bulletDynamicsWorld = new btDiscreteDynamicsWorld(bulletDispatcher,bulletBroadphase,bulletSolver,bulletCollisionConfiguration);
	bulletDynamicsWorld->setGravity(btVector3(0.f,0.f,9.81 * (Tile::size / 2.f)));
	bulletDynamicsWorld->getPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());

	// create shadow camera
	const Entity::def_t* def = Entity::findDef("Shadow Camera");
	shadowCamera = Entity::spawnFromDef(this, *def, Vector(), Angle());
	shadowCamera->setShouldSave(false);
}

void World::getSelectedEntities(LinkedList<Entity*>& outResult) {
	for( int c=0; c<numBuckets; ++c ) {
		for( Node<Entity*>* node=entities[c].getFirst(); node!=nullptr; node=node->getNext() ) {
			Entity* entity = node->getData();

			// skip editor entities
			if( !entity->isShouldSave() )
				continue;

			if( entity->isSelected() ) {
				outResult.addNodeLast(entity);
			}
		}
	}
}

void World::changeFilename(const char* _filename) {	
	if( _filename == nullptr ) {
		return;
	}

	// set new filetype
	filetype = FILE_MAX;
	for( int c = 0; c < static_cast<int>(World::FILE_MAX); ++c ) {
		StringBuf<16> fullExtension(".%s", fileExtensions[static_cast<int>(c)]);
		filename.alloc(strlen(_filename) + fullExtension.length() + 1);
		filename = _filename;

		// append filetype extension
		if( filename.length() >= fullExtension.length() ) {
			String currExtension = filename.substr( filename.length() - fullExtension.length() );
			if( currExtension == fullExtension.get() ) {
				filetype = static_cast<filetype_t>(c);
				break;
			}
		}
	}
	if( filetype == FILE_MAX ) {
		filetype = FILE_BINARY;
		filename.appendf(".%s",fileExtensions[static_cast<int>(filetype)]);
	}

	// create shortened filename
	shortname = filename;
	size_t i = 0;
	do {
		i=shortname.find('/',i);
		if( i != UINT32_MAX ) {
			shortname = shortname.substr(i+1);
		} else {
			break;
		}
	} while( 1 );

#ifdef PLATFORM_WINDOWS
	// windows has to cut out their crazy backward slashes, too.
	i = 0;
	do {
		i=shortname.find('\\',i);
		if( i != UINT32_MAX ) {
			shortname = shortname.substr(i+1);
		} else {
			break;
		}
	} while( 1 );
#endif
}

bool World::isShowTools() const {
	Client* client = mainEngine->getLocalClient();
	if( !client || !client->isEditorActive() ) {
		return false;
	} else {
		return showTools;
	}
}

const bool World::selectEntity(const Uint32 uid, const bool b) {
	Entity* entity = uidToEntity(uid);
	if( entity ) {
		entity->setSelected(b);
		entity->setHighlighted(b);

		if( b ) {
			mainEngine->playSound("editor/message.wav");
		}
		return true;
	}
	return false;
}

void World::selectEntities(const bool b) {
	for( unsigned int c=0; c<numBuckets; ++c ) {
		for( Node<Entity*>* node=entities[c].getFirst(); node!=nullptr; node=node->getNext() ) {
			Entity* entity = node->getData();

			if( entity->isSelected() ) {
				entity->setSelected(b);
				entity->setHighlighted(b);
			}
		}
	}

	if( b ) {
		mainEngine->playSound("editor/message.wav");
	}
}

const World::hit_t World::convexSweep( const btConvexShape* shape, const Vector& originPos, const Angle& originAng, const Vector& destPos, const Angle& destAng ) {
	hit_t emptyResult;
	emptyResult.pos = destPos;

	LinkedList<hit_t> list;
	convexSweepList(shape, originPos, originAng, destPos, destAng, list);

	// return the first entry found
	if( list.getSize() > 0 ) {
		return list.getFirst()->getData();
	}

	return emptyResult;
}

struct AllHitsConvexResultCallback : public btCollisionWorld::ConvexResultCallback {
	AllHitsConvexResultCallback(const btVector3& fromWorld, const btVector3& toWorld) :
		m_convexFromWorld(fromWorld),
		m_convexToWorld(toWorld)
	{
	}

	btAlignedObjectArray<const btCollisionObject*> m_collisionObjects;

	btVector3 m_convexFromWorld; // used to calculate hitPointWorld from hitFraction
	btVector3 m_convexToWorld;

	btAlignedObjectArray<btVector3> m_hitNormalWorld;
	btAlignedObjectArray<btVector3> m_hitPointWorld;
	btAlignedObjectArray<btScalar> m_hitFractions;

	virtual btScalar addSingleResult(btCollisionWorld::LocalConvexResult& convexResult, bool normalInWorldSpace)
	{
		const btCollisionObject* collisionObject = convexResult.m_hitCollisionObject;
		m_collisionObjects.push_back(collisionObject);

		btVector3 hitNormalWorld;
		if (normalInWorldSpace) {
			hitNormalWorld = convexResult.m_hitNormalLocal;
		} else {
			hitNormalWorld = collisionObject->getWorldTransform().getBasis() * convexResult.m_hitNormalLocal;
		}
		m_hitNormalWorld.push_back(hitNormalWorld);

		btVector3 hitPointWorld;
		hitPointWorld.setInterpolate3(m_convexFromWorld, m_convexToWorld, convexResult.m_hitFraction);
		m_hitPointWorld.push_back(hitPointWorld);
		m_hitFractions.push_back(convexResult.m_hitFraction);
		return m_closestHitFraction;
	}

	virtual bool hasHit() const
	{
		return m_collisionObjects.size() > 0;
	}
};

void World::convexSweepList( const btConvexShape* shape, const Vector& originPos, const Angle& originAng, const Vector& destPos, const Angle& destAng, LinkedList<hit_t>& outResult ) {
	btVector3 btOriginPos(originPos);
	btQuaternion btOriginQuat;
	btOriginQuat.setEulerZYX(originAng.yaw, -originAng.pitch, -originAng.roll);
	btTransform btOrigin(btOriginQuat, btOriginPos);

	btVector3 btDestPos(destPos);
	btQuaternion btDestQuat;
	btDestQuat.setEulerZYX(destAng.yaw, -destAng.pitch, -destAng.roll);
	btTransform btDest(btDestQuat, btDestPos);

	// perform raycast
	AllHitsConvexResultCallback SweepCallback(btOriginPos, btDestPos);
	bulletDynamicsWorld->convexSweepTest(shape, btOrigin, btDest, SweepCallback);

	if( SweepCallback.hasHit() ) {
		for( int num=0; num<SweepCallback.m_hitPointWorld.size(); ++num ) {
			hit_t hit;

			// determine properties of the hit
			hit.pos = Vector(SweepCallback.m_hitPointWorld[num].x(),SweepCallback.m_hitPointWorld[num].y(),SweepCallback.m_hitPointWorld[num].z());
			glm::vec3 normal = glm::normalize(glm::vec3(SweepCallback.m_hitNormalWorld[num].x(),SweepCallback.m_hitNormalWorld[num].y(),SweepCallback.m_hitNormalWorld[num].z()));
			hit.normal.x = normal.x;
			hit.normal.y = normal.y;
			hit.normal.z = normal.z;
			hit.index = SweepCallback.m_collisionObjects[num]->getUserIndex();
			hit.index2 = SweepCallback.m_collisionObjects[num]->getUserIndex2();
			hit.pointer = SweepCallback.m_collisionObjects[num]->getUserPointer();

			// determine if we hit an entity or a tile
			hit.hitEntity = false;
			hit.hitTile = false;
			if( hit.index <= uids ) {
				hit.hitEntity = true;
			} else if( hit.index2 != World::nuid ) {
				hit.hitSectorVertex = true;
			} else if( hit.index == World::nuid ) {
				if( getType() == WORLD_TILES ) {
					hit.hitTile = true;
				} else if( getType() == WORLD_SECTORS ) {
					hit.hitSector = true;
				}
			}

			// skip invalid "hits"
			if( !hit.hitEntity && !hit.hitTile && !hit.hitSector && !hit.hitSectorVertex ) {
				continue;
			}

			// skip untraceable entities
			if( hit.hitEntity ) {
				Entity* entity;
				if( (entity=uidToEntity(hit.index)) != nullptr ) {
					if( !entity->isShouldSave() ) {
						continue;
					}
					if( entity==shadowCamera ) {
						continue;
					}
				}
			}

			// sort and insert the hit entry into the list of results
			int index = 0;
			Node<hit_t>* node = nullptr;
			for( node=outResult.getFirst(); node!=nullptr; node=node->getNext() ) {
				hit_t& current = node->getData();

				if( (originPos - hit.pos).lengthSquared() < (originPos - current.pos).lengthSquared() ) {
					break;
				}

				++index;
			}
			outResult.addNode(index,hit);
		}
	}
}

const World::hit_t World::lineTrace( const Vector& origin, const Vector& dest ) {
	hit_t emptyResult;
	emptyResult.pos = dest;

	LinkedList<hit_t> list;
	lineTraceList(origin, dest, list);

	// return the first entry found
	if( list.getSize() > 0 ) {
		return list.getFirst()->getData();
	}

	return emptyResult;
}

void World::lineTraceList( const Vector& origin, const Vector& dest, LinkedList<World::hit_t>& outResult ) {
	btVector3 btOrigin(origin);
	btVector3 btDest(dest);

	// perform raycast
	btCollisionWorld::AllHitsRayResultCallback RayCallback(btOrigin, btDest);
	bulletDynamicsWorld->rayTest(btOrigin, btDest, RayCallback);

	if( RayCallback.hasHit() ) {
		for( int num=0; num<RayCallback.m_hitPointWorld.size(); ++num ) {
			hit_t hit;

			// determine properties of the hit
			hit.pos = Vector(RayCallback.m_hitPointWorld[num].x(),RayCallback.m_hitPointWorld[num].y(),RayCallback.m_hitPointWorld[num].z());
			glm::vec3 normal = glm::normalize(glm::vec3(RayCallback.m_hitNormalWorld[num].x(),RayCallback.m_hitNormalWorld[num].y(),RayCallback.m_hitNormalWorld[num].z()));
			hit.normal.x = normal.x;
			hit.normal.y = normal.y;
			hit.normal.z = normal.z;
			hit.index = RayCallback.m_collisionObjects[num]->getUserIndex();
			hit.index2 = RayCallback.m_collisionObjects[num]->getUserIndex2();
			hit.pointer = RayCallback.m_collisionObjects[num]->getUserPointer();

			// determine if we hit an entity or a tile
			hit.hitEntity = false;
			hit.hitTile = false;
			if( hit.index <= uids ) {
				hit.hitEntity = true;
			} else if( hit.index2 != World::nuid ) {
				hit.hitSectorVertex = true;
			} else if( hit.index == World::nuid ) {
				if( getType() == WORLD_TILES ) {
					hit.hitTile = true;
				} else if( getType() == WORLD_SECTORS ) {
					hit.hitSector = true;
				}
			}

			// skip invalid "hits"
			if( !hit.hitEntity && !hit.hitTile && !hit.hitSector && !hit.hitSectorVertex ) {
				continue;
			}

			// skip untraceable entities
			if( hit.hitEntity ) {
				Entity* entity;
				if( (entity=uidToEntity(hit.index)) != nullptr ) {
					if( !entity->isFlag(Entity::flag_t::FLAG_ALLOWTRACE) && (!mainEngine->isEditorRunning() || !entity->isShouldSave()) ) {
						continue;
					}
					if( entity==shadowCamera ) {
						continue;
					}
				}
			}

			// sort and insert the hit entry into the list of results
			int index = 0;
			Node<hit_t>* node = nullptr;
			for( node=outResult.getFirst(); node!=nullptr; node=node->getNext() ) {
				hit_t& current = node->getData();

				if( (origin - hit.pos).lengthSquared() < (origin - current.pos).lengthSquared() ) {
					break;
				}

				++index;
			}
			outResult.addNode(index,hit);
		}
	}
}

const World::hit_t World::lineTraceNoEntities( const Vector& origin, const Vector& dest ) {
	hit_t emptyResult;

	LinkedList<hit_t> list;
	lineTraceList(origin, dest, list);

	// return the first non-entity found
	for( Node<hit_t>* node = list.getFirst(); node != nullptr; node = node->getNext() ) {
		hit_t& hit = node->getData();
		if( hit.hitTile || hit.hitSector || hit.hitSectorVertex ) {
			return hit;
		}
	}

	return emptyResult;
}

Entity* World::uidToEntity(const Uint32 uid) {
	if( uid==nuid ) {
		return nullptr;
	}
	for( Node<Entity*>* node=entities[uid&(World::numBuckets-1)].getFirst(); node!=nullptr; node=node->getNext() ) {
		Entity* entity = node->getData();
		if( entity->getUID()==uid ) {
			return entity;
		}
	}
	return nullptr;
}

void World::findSelectedEntities(LinkedList<Entity*>& outList) {
	outList.removeAll();
	for( Uint32 c=0; c<World::numBuckets; ++c ) {
		for( Node<Entity*>* node=entities[c].getFirst(); node!=nullptr; node=node->getNext() ) {
			Entity* entity = node->getData();

			if( entity->isSelected() ) {
				outList.addNodeLast(entity);
			}
		}
	}
}

void World::process() {
	++ticks;

	// find out if the editor is running
	bool editorRunning = false;
	Client* client = mainEngine->getLocalClient();
	if( client ) {
		if( client->isEditorActive() ) {
			editorRunning = true;
		}
	}

	if( !editorRunning ) {
		// run world script
		const String scriptname = filename.substr(0,filename.length()-4);
		if( clientObj && mainEngine->isRunningClient() ) {
			//script->run(StringBuf<128>("scripts/client/maps/%s.lua", scriptname.get()).get());
		} else if( clientObj && mainEngine->isRunningServer() ) {
			//script->run(StringBuf<128>("scripts/server/maps/%s.lua", scriptname.get()).get());
		}
	}

	// update lasers
	for (size_t c = 0; c < lasers.getSize(); ++c) {
		auto& laser = lasers[c];
		if (laser.life > 0.f) {
			laser.life -= 1.f;
			if (laser.life <= 0.f) {
				lasers.remove(c);
				--c;
			}
		}
	}

	// step physics
	if( !mainEngine->isEditorRunning() ) {
		float step = 1.f / (float)mainEngine->getTicksPerSecond();
		bulletDynamicsWorld->stepSimulation(step, 1, step);

		LinkedList<BBox*> bboxes;
		for( Uint32 c = 0; c < numBuckets; ++c ) {
			for( Node<Entity*>* node=entities[c].getFirst(); node!=nullptr; node=node->getNext() ) {
				Entity* entity = node->getData();
				entity->findAllComponents<BBox>(Component::COMPONENT_BBOX, bboxes);
			}
		}
		for (auto bbox : bboxes) {
			if (!bbox->getParent() && bbox->getMass() != 0.f) {
				btTransform transform = bbox->getPhysicsTransform();

				Vector pos = -bbox->getLocalPos();
				pos.x += transform.getOrigin().x();
				pos.y += transform.getOrigin().y();
				pos.z += transform.getOrigin().z();

				const btQuaternion& q = transform.getRotation();
				const Vector& scale = bbox->getEntity()->getScale();

				glm::mat4 translationM = glm::translate(glm::mat4(1.f),glm::vec3(pos.x,-pos.z,pos.y));
				glm::mat4 rotationM = glm::mat4_cast(glm::quat(q.w(), q.x(), -q.z(), q.y()));
				glm::mat4 scaleM = glm::scale(glm::mat4(1.f),glm::vec3(scale.x, scale.z, scale.y));
				glm::mat4 mat = translationM * rotationM * scaleM;
				bbox->getEntity()->setMat(mat);
			}
		}
	}

	// iterate through entities
	for( Uint32 c=0; c<World::numBuckets; ++c ) {
		for( Node<Entity*>* node=entities[c].getFirst(); node!=nullptr; node=node->getNext() ) {
			Entity* entity = node->getData();
			entity->process();
		}
	}

	// delete removed entities
	for( Uint32 c=0; c<World::numBuckets; ++c ) {
		Node<Entity*>* nextnode = nullptr;
		for( Node<Entity*>* node=entities[c].getFirst(); node!=nullptr; node=nextnode ) {
			Entity* entity = node->getData();
			nextnode = node->getNext();

			if( entity->isToBeDeleted() ) {
				bool updateNeeded = entity->isFlag(Entity::flag_t::FLAG_UPDATE) && !entity->isFlag(Entity::flag_t::FLAG_LOCAL);
				Uint32 uid = entity->getUID();
				entities[c].removeNode(node);
				delete entity;

				// inform clients of entity deletion
				if( !clientObj && updateNeeded ) {
					Server* server = mainEngine->getLocalServer();
					if( server ) {
						Packet packet;
						packet.write32(uid);
						packet.write32(id);
						packet.write("ENTD");
						server->getNet()->signPacket(packet);
						server->getNet()->broadcastSafe(packet);
					}
				}
			}
		}
	}
}

World::laser_t& World::addLaser(const Vector& start, const Vector& end, const glm::vec4& color, float size, float life) {
	laser_t laser;
	laser.start = start;
	laser.end = end;
	laser.color = color;
	laser.size = size;
	laser.life = life;
	laser.maxLife = life;
	lasers.push(laser);
	return lasers.peek();
}