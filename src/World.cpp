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
#include "Generator.hpp"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

const int World::tileSize = 32;
const Uint32 World::nuid = UINT32_MAX;
const char* World::fileExtensions[World::FILE_MAX] = {
	"wlb",
	"wlj",
	"wld"
};

Cvar cvar_showEdges("showedges", "highlight chunk silhouettes with visible lines", "0");
Cvar cvar_showVerts("showverts", "highlight all triangle edges with visible lines", "0");
Cvar cvar_renderFullbright("render.fullbright", "replaces all lights with camera-based illumination", "0");
Cvar cvar_depthOffset("render.depthoffset", "depth buffer adjustment", "-0.1");
Cvar cvar_renderCull("render.cull", "accuracy for occlusion culling", "7");

World::World(Game* _game)
{
	game = _game;
	script = new Script(*this);
}

World::~World() {
	if (!silent) {
		mainEngine->fmsg(Engine::MSG_INFO, "deleting world '%s'", nameStr.get());
	}

	// delete entities
	for (auto& pair : entities) {
		Entity* entity = pair.b;
		delete entity;
	}
	entities.clear();
	for (auto entity : entitiesToInsert) {
		delete entity;
	}
	entitiesToInsert.clear();

	// delete script engine
	if (script) {
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

void World::bulletCollisionCallback(btBroadphasePair& pair, btCollisionDispatcher& dispatcher, const btDispatcherInfo& info) {
	auto obj0 = static_cast<btCollisionObject*>(pair.m_pProxy0->m_clientObject);
	auto obj1 = static_cast<btCollisionObject*>(pair.m_pProxy1->m_clientObject);
	auto manifest0 = static_cast<World::physics_manifest_t*>(obj0->getUserPointer());
	auto manifest1 = static_cast<World::physics_manifest_t*>(obj1->getUserPointer());
	Entity* entity0 = manifest0 ? manifest0->entity : nullptr;
	Entity* entity1 = manifest1 ? manifest1->entity : nullptr;
	BBox* bbox0 = manifest0 ? manifest0->bbox : nullptr;
	BBox* bbox1 = manifest1 ? manifest1->bbox : nullptr;
	if (bbox0 && bbox1 && entity0 && entity1 && 0) {
		if (pair.m_algorithm) {
			btManifoldArray arr;
			pair.m_algorithm->getAllContactManifolds(arr);
			for (int c = 0; c < arr.size(); ++c) {
				auto contact = arr.at(c);
				for (int i = 0; i < contact->getNumContacts(); ++i) {
					auto& point = contact->getContactPoint(i);

					float mass0 = fabs(bbox0->getMass());
					float mass1 = fabs(bbox1->getMass());
					float mass0over1 = mass1 ? mass0 / mass1 : 2.f;
					float mass1over0 = mass0 ? mass1 / mass0 : 2.f;

					Vector vel = entity0->getVel() - entity1->getVel();

					// react entity0
					if (bbox0->getMass()) {
						const Vector point0 = Vector(point.m_positionWorldOnA.x(), point.m_positionWorldOnA.y(), point.m_positionWorldOnA.z());
						Vector diff = point0 - entity0->getPos();
						if (diff.dot(vel) > 0.f) {
							Vector dir = diff.normal();
							glm::vec3 r = glm::reflect(glm::vec3(vel.x, vel.y, vel.z), glm::vec3(dir.x, dir.y, dir.z));
							Vector bounce(r.x, r.y, r.z);
							Vector force = bounce * mass1over0;
							if (bbox0->getMass() < 0.f) {
								entity0->setVel(entity0->getVel() + force);
							} else {
								auto& v = point.m_localPointA;
								entity0->applyForce(force, Vector(0.f));
							}
						}
					}

					// react entity1
					if (bbox1->getMass()) {
						const Vector point1 = Vector(point.m_positionWorldOnB.x(), point.m_positionWorldOnB.y(), point.m_positionWorldOnB.z());
						Vector diff = point1 - entity1->getPos();
						if (diff.dot(vel) > 0.f) {
							Vector dir = diff.normal();
							glm::vec3 r = glm::reflect(glm::vec3(vel.x, vel.y, vel.z), glm::vec3(dir.x, dir.y, dir.z));
							Vector bounce(r.x, r.y, r.z);
							Vector force = bounce * mass0over1;
							if (bbox1->getMass() < 0.f) {
								entity1->setVel(entity1->getVel() + force);
							} else {
								auto& v = point.m_localPointB;
								entity1->applyForce(force, Vector(0.f));
							}
						}
					}
				}
			}
		}
	}
	dispatcher.defaultNearCallback(pair, dispatcher, info);
}

void World::initialize(bool empty) {
	mainEngine->fmsg(Engine::MSG_INFO, "creating physics simulation...");

	// build the broadphase
	bulletBroadphase = new btDbvtBroadphase();

	// set up the collision configuration and dispatcher
	btNearCallback collisionCallback = bulletCollisionCallback;
	bulletCollisionConfiguration = new btDefaultCollisionConfiguration();
	bulletDispatcher = new btCollisionDispatcher(bulletCollisionConfiguration);
	bulletDispatcher->setNearCallback(collisionCallback);

	// the actual physics solver
	bulletSolver = new btSequentialImpulseConstraintSolver;

	// the world
	bulletDynamicsWorld = new btDiscreteDynamicsWorld(bulletDispatcher, bulletBroadphase, bulletSolver, bulletCollisionConfiguration);
	bulletDynamicsWorld->setGravity(btVector3(0.f, 0.f, 9.81 * (World::tileSize / 2.f)));
	bulletDynamicsWorld->getPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());

	// create shadow camera + default shadow texture
	defaultShadow.init();
	const Entity::def_t* def = Entity::findDef("Shadow Camera"); assert(def);
	shadowCamera = Entity::spawnFromDef(this, *def, Vector(), Rotation());
	shadowCamera->setShouldSave(false);
}

void World::getSelectedEntities(LinkedList<Entity*>& outResult) {
	for (auto& pair : entities) {
		Entity* entity = pair.b;

		// skip editor entities
		if (!entity->isShouldSave())
			continue;

		if (entity->isSelected()) {
			outResult.addNodeLast(entity);
		}
	}
}

ArrayList<int> World::generateDungeon(const char* filename) const {
	Generator gen(clientObj);
	FileHelper::readObject(mainEngine->buildPath(filename).get(), gen);
	gen.createDungeon();
	ArrayList<int> result;
	result.resize(gen.getTiles().getSize());
	for (unsigned int c = 0; c < gen.getTiles().getSize(); ++c)
	{
		result[c] = (int)gen.getTiles()[c];
	}
	return result;
}

void World::changeFilename(const char* _filename) {
	if (_filename == nullptr) {
		return;
	}

	// set new filetype
	filetype = FILE_MAX;
	for (int c = 0; c < static_cast<int>(World::FILE_MAX); ++c) {
		StringBuf<16> fullExtension(".%s", 1, fileExtensions[static_cast<int>(c)]);
		filename.alloc((Uint32)strlen(_filename) + fullExtension.length() + 1);
		filename = _filename;

		// append filetype extension
		if (filename.length() >= fullExtension.length()) {
			String currExtension = filename.substr(filename.length() - fullExtension.length());
			if (currExtension == fullExtension.get()) {
				filetype = static_cast<filetype_t>(c);
				break;
			}
		}
	}
	if (filetype == FILE_MAX) {
		filetype = FILE_BINARY;
		filename.appendf(".%s", fileExtensions[static_cast<int>(filetype)]);
	}

	// create shortened filename
	shortname = filename;
	Uint32 i = 0;
	do {
		i = shortname.find('/', 0);
		if (i != UINT32_MAX) {
			shortname = shortname.substr(i + 1);
		}
	} while (i != UINT32_MAX);

#ifdef PLATFORM_WINDOWS
	// windows has to cut out their crazy backward slashes, too.
	i = 0;
	do {
		i = shortname.find('\\', 0);
		if (i != UINT32_MAX) {
			shortname = shortname.substr(i + 1);
		}
	} while (i != UINT32_MAX);
#endif
}

bool World::isShowTools() const {
	Client* client = mainEngine->getLocalClient();
	if (!client || !client->isEditorActive()) {
		return false;
	} else {
		return showTools;
	}
}

Entity* World::spawnEntity(const char* name, const Vector& pos, const Rotation& ang) {
	if (!name) {
		return nullptr;
	}
	const Entity::def_t* def = Entity::findDef(name);
	if (!def) {
		mainEngine->fmsg(Engine::MSG_ERROR, "tried to spawn entity of unknown type '%s'", name);
		return nullptr;
	}
	Entity* entity = Entity::spawnFromDef(nullptr, *def, pos, ang);
	entitiesToInsert.push(entity);
	return entity;
}

const bool World::selectEntity(const Uint32 uid, const bool b) {
	Entity* entity = uidToEntity(uid);
	if (entity) {
		entity->setSelected(b);
		entity->setHighlighted(b);

		if (b) {
			mainEngine->playSound("sounds/editor/message.wav");
		}
		return true;
	}
	return false;
}

void World::selectEntities(const bool b) {
	for (auto& pair : entities) {
		Entity* entity = pair.b;
		if (entity->isSelected()) {
			entity->setSelected(b);
			entity->setHighlighted(b);
		}
	}

	if (b) {
		mainEngine->playSound("sounds/editor/message.wav");
	}
}

const World::hit_t World::convexSweep(const btConvexShape* shape, const Vector& originPos, const Quaternion& originAng, const Vector& destPos, const Quaternion& destAng) {
	hit_t emptyResult;
	emptyResult.pos = destPos;

	LinkedList<hit_t> list;
	convexSweepList(shape, originPos, originAng, destPos, destAng, list);

	// return the first entry found
	if (list.getSize() > 0) {
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

void World::convexSweepList(const btConvexShape* shape, const Vector& originPos, const Quaternion& originAng, const Vector& destPos, const Quaternion& destAng, LinkedList<hit_t>& outResult) {
	btVector3 btOriginPos(originPos);
	btQuaternion btOriginQuat = btQuat(originAng);
	btTransform btOrigin(btOriginQuat, btOriginPos);

	btVector3 btDestPos(destPos);
	btQuaternion btDestQuat = btQuat(destAng);
	btTransform btDest(btDestQuat, btDestPos);

	// perform raycast
	AllHitsConvexResultCallback SweepCallback(btOriginPos, btDestPos);
	bulletDynamicsWorld->convexSweepTest(shape, btOrigin, btDest, SweepCallback);

	if (SweepCallback.hasHit()) {
		for (int num = 0; num < SweepCallback.m_hitPointWorld.size(); ++num) {
			hit_t hit;

			// determine properties of the hit
			hit.pos = Vector(SweepCallback.m_hitPointWorld[num].x(), SweepCallback.m_hitPointWorld[num].y(), SweepCallback.m_hitPointWorld[num].z());
			glm::vec3 normal = glm::normalize(glm::vec3(SweepCallback.m_hitNormalWorld[num].x(), SweepCallback.m_hitNormalWorld[num].y(), SweepCallback.m_hitNormalWorld[num].z()));
			hit.normal.x = normal.x;
			hit.normal.y = normal.y;
			hit.normal.z = normal.z;
			hit.manifest = static_cast<World::physics_manifest_t*>(SweepCallback.m_collisionObjects[num]->getUserPointer());

			// skip untraceable entities
			if (hit.manifest && hit.manifest->entity) {
				Entity* entity = hit.manifest->entity;
				if (!entity->isShouldSave()) {
					continue;
				}
			}

			// sort and insert the hit entry into the list of results
			int index = 0;
			Node<hit_t>* node = nullptr;
			for (node = outResult.getFirst(); node != nullptr; node = node->getNext()) {
				hit_t& current = node->getData();

				if ((originPos - hit.pos).lengthSquared() < (originPos - current.pos).lengthSquared()) {
					break;
				}

				++index;
			}
			outResult.addNode(index, hit);
		}
	}
}

World::hit_t World::lineTrace(const Vector& origin, const Vector& dest) {
	hit_t emptyResult;
	emptyResult.pos = dest;

	LinkedList<hit_t> list;
	lineTraceList(origin, dest, list);

	// return the first entry found
	if (list.getSize() > 0) {
		return list.getFirst()->getData();
	}

	return emptyResult;
}

void World::lineTraceList(const Vector& origin, const Vector& dest, LinkedList<World::hit_t>& outResult) {
	btVector3 btOrigin(origin);
	btVector3 btDest(dest);

	// perform raycast
	btCollisionWorld::AllHitsRayResultCallback RayCallback(btOrigin, btDest);
	bulletDynamicsWorld->rayTest(btOrigin, btDest, RayCallback);

	if (RayCallback.hasHit()) {
		for (int num = 0; num < RayCallback.m_hitPointWorld.size(); ++num) {
			hit_t hit;

			// determine properties of the hit
			hit.pos = Vector(RayCallback.m_hitPointWorld[num].x(), RayCallback.m_hitPointWorld[num].y(), RayCallback.m_hitPointWorld[num].z());
			glm::vec3 normal = glm::normalize(glm::vec3(RayCallback.m_hitNormalWorld[num].x(), RayCallback.m_hitNormalWorld[num].y(), RayCallback.m_hitNormalWorld[num].z()));
			hit.normal.x = normal.x;
			hit.normal.y = normal.y;
			hit.normal.z = normal.z;
			hit.manifest = static_cast<World::physics_manifest_t*>(RayCallback.m_collisionObjects[num]->getUserPointer());

			// skip untraceable entities
			if (hit.manifest && hit.manifest->entity) {
				Entity* entity = hit.manifest->entity;
				if (!entity->isFlag(Entity::flag_t::FLAG_ALLOWTRACE) && (!mainEngine->isEditorRunning() || !entity->isShouldSave())) {
					continue;
				}
				if (entity->getName() == "Shadow Camera") {
					continue;
				}
			}

			// sort and insert the hit entry into the list of results
			int index = 0;
			Node<hit_t>* node = nullptr;
			for (node = outResult.getFirst(); node != nullptr; node = node->getNext()) {
				hit_t& current = node->getData();

				if ((origin - hit.pos).lengthSquared() < (origin - current.pos).lengthSquared()) {
					break;
				}

				++index;
			}
			outResult.addNode(index, hit);
		}
	}
}

World::hit_t World::lineTraceNoEntities(const Vector& origin, const Vector& dest) {
	hit_t emptyResult;

	LinkedList<hit_t> list;
	lineTraceList(origin, dest, list);

	// return the first non-entity found
	for (Node<hit_t>* node = list.getFirst(); node != nullptr; node = node->getNext()) {
		hit_t& hit = node->getData();
		if (!hit.manifest || !hit.manifest->entity) {
			return hit;
		}
	}

	return emptyResult;
}

ArrayList<Entity*> World::getEntitiesByName(const char* name) {
	if (name == nullptr || name[0] == '\0') {
		return ArrayList<Entity*>();
	}
	ArrayList<Entity*> result;
	for (auto& pair : entities) {
		Entity* entity = pair.b;
		if (entity->getName() == name) {
			result.push(entity);
		}
	}
	return result;
}

Entity* World::uidToEntity(const Uint32 uid) {
	auto result = entities.find(uid);
	return result ? *result : nullptr;
}

void World::findSelectedEntities(LinkedList<Entity*>& outList) {
	outList.removeAll();
	for (auto& pair : entities) {
		Entity* entity = pair.b;
		if (entity->isSelected()) {
			outList.addNodeLast(entity);
		}
	}
}

void World::preProcess() {
	for (auto& pair : entities) {
		Entity* entity = pair.b;
		entity->preProcess();
	}
}

void World::process() {
	++ticks;

	// find out if the editor is running
	bool editorRunning = false;
	Client* client = mainEngine->getLocalClient();
	if (client) {
		if (client->isEditorActive()) {
			editorRunning = true;
		}
	}

	if (!editorRunning) {
		// run world script
		const String scriptname = filename.substr(0, filename.length() - 4);
		if (clientObj && mainEngine->isRunningClient()) {
			//script->run(StringBuf<128>("scripts/client/maps/%s.lua", 1, scriptname.get()).get());
		} else if (clientObj && mainEngine->isRunningServer()) {
			//script->run(StringBuf<128>("scripts/server/maps/%s.lua", 1, scriptname.get()).get());
		}
	}

	// update lasers
	for (Uint32 c = 0; c < lasers.getSize(); ++c) {
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
	if (!mainEngine->isEditorRunning()) {
		float step = 1.f / (float)mainEngine->getTicksPerSecond();
		bulletDynamicsWorld->stepSimulation(step, 1, step);
	}

	// iterate through entities
	for (auto& pair : entities) {
		Entity* entity = pair.b;
		entity->process();
	}

	// insert pending entities
	for (auto entity : entitiesToInsert) {
		entity->insertIntoWorld(this);
		entity->finishInsertIntoWorld();
	}
	entitiesToInsert.clear();

	// delete entities marked for removal and transfer entities marked for level change
	for (auto& it = entities.begin(); it != entities.end(); ++it) {
		Entity* entity = (*it).b;

		if (entity->isToBeDeleted()) {
			bool updateNeeded = entity->isFlag(Entity::flag_t::FLAG_UPDATE) && !entity->isFlag(Entity::flag_t::FLAG_LOCAL);
			Uint32 uid = entity->getUID();
			entities.remove((*it).a);
			delete entity;
			--it;

			// inform clients of entity deletion
			if (!clientObj && updateNeeded) {
				Server* server = mainEngine->getLocalServer();
				if (server) {
					Packet packet;
					packet.write32(uid);
					packet.write32(id);
					packet.write("ENTD");
					server->getNet()->signPacket(packet);
					server->getNet()->broadcastSafe(packet);
				}
			}
		} else {
			entity->finishInsertIntoWorld();
		}
	}
}

void World::postProcess() {
	for (auto& pair : entities) {
		Entity* entity = pair.b;
		entity->postProcess();
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