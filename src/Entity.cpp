// Entity.cpp

#include "Main.hpp"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <btBulletDynamicsCommon.h>

#include "LinkedList.hpp"
#include "Node.hpp"
#include "Engine.hpp"
#include "Client.hpp"
#include "World.hpp"
#include "TileWorld.hpp"
#include "Resource.hpp"
#include "Tile.hpp"
#include "Entity.hpp"
#include "Chunk.hpp"
#include "Script.hpp"
#include "Frame.hpp"

//Component headers.
#include "BBox.hpp"
#include "Model.hpp"
#include "Light.hpp"
#include "Camera.hpp"
#include "Speaker.hpp"
#include "Character.hpp"

const char* Entity::flagStr[static_cast<int>(Entity::flag_t::FLAG_NUM)] = {
	"VISIBLE",
	"PASSABLE",
	"LOCAL",
	"UPDATE",
	"GENIUS",
	"OVERDRAW",
	"SPRITE",
	"ALLOWTRACE",
	"SHADOW",
	"FULLYLIT",
	"GLOWING",
	"DEPTHFAIL",
	"OCCLUDE",
	"INTERACTABLE",
	"STATIC"
};

const char* Entity::flagDesc[static_cast<int>(Entity::flag_t::FLAG_NUM)] = {
	"Makes the entity visible for gameplay",
	"Disables the entity's collision volume",
	"Turns the entity into a client-side only object",
	"Allows network updates for the entity",
	"Causes the entity's cameras to not draw the entity's 1st model",
	"Causes the entity to be drawn over most objects",
	"Makes the entity render a sprite rather than a mesh",
	"Enables line tracing against the entity's physics volume",
	"Permits the entity to cast stencil shadows",
	"Causes the entity to be lit with 100% brightness",
	"Enables drawing in the glow pass",
	"Enables drawing in the depth fail pass",
	"Causes the entity to block occlusion tests",
	"Indicates to clients whether an entity is interactible",
	"Heavily optimize the entity but render it immobile"
};

const char* Entity::sortStr[SORT_MAX] = {
	"any",
	"char",
	"decor",
	"interact",
	"item",
	"light",
	"trigger"
};

Entity::Entity(World* _world, Uint32 _uid) {
	// insert the entity into the world
	world = _world;
	if( world ) {
		if( _uid == UINT32_MAX ) {
			uid = world->getNewUID();
		} else {
			uid = _uid;
		}
		node = world->getEntities(uid&(World::numBuckets-1)).addNodeLast(this);
	}

	// initialize a few things
	scale = Vector(1.f);
	flags = static_cast<int>(flag_t::FLAG_VISIBLE) | static_cast<int>(flag_t::FLAG_SHADOW) | static_cast<int>(flag_t::FLAG_ALLOWTRACE);

	// assign a default name
	char newName[32];
	snprintf(newName, 32, "Entity #%d", uid);
	name = newName;

	pathRequested = false;
	path = nullptr;
}

Entity::~Entity() {
	if( player ) {
		player->onEntityDeleted(this);
		player = nullptr;
	}
	if( listener ) {
		listener->onDeleted();
	}

	// delete components
	for( size_t c = 0; c < components.getSize(); ++c ) {
		if( components[c] ) {
			delete components[c];
			components[c] = nullptr;
		}
	}
	components.clear();

	// remove chunk node
	clearChunkNode();

	// delete script engine
	if( script )
	{
		delete script;
	}

	if (path)
	{
		delete path;
	}
}

void Entity::clearAllChunkNodes() {
	clearChunkNode();
	for( size_t c = 0; c < components.getSize(); ++c ) {
		components[c]->clearAllChunkNodes();
	}
}

void Entity::addToEditorList() {
	if( !shouldSave ) {
		return;
	}
	Client* client = mainEngine->getLocalClient();
	if( client ) {
		Frame* gui = client->getGUI();
		if( gui ) {
			Frame* levelList = gui->findFrame("editor_FrameLevelNavigatorList");
			if( levelList ) {

				Frame::entry_t* entry = levelList->addEntry("entity",true);
				entry->text = name.get();
				entry->params.addInt(uid);
				if( selected ) {
					entry->color = glm::vec4(1.f,0.f,0.f,1.f);
				} else if( highlighted ) {
					entry->color = glm::vec4(1.f,1.f,0.f,1.f);
				} else {
					entry->color = glm::vec4(1.f);
				}

				listener = std::make_shared<Entity::listener_t>((void*) entry);
				entry->listener = listener;
			}
		}
	}
}

void Entity::insertIntoWorld(World* _world) {
	// signal components
	for( size_t c = 0; c < components.getSize(); ++c ) {
		components[c]->beforeWorldInsertion(_world);
	}

	// insert to new world
	if( node ) {
		if( world ) {
			world->getEntities(uid&(World::numBuckets-1)).removeNode(node);
		}
	}
	world = _world;
	if( world ) {
		uid = world->getNewUID();
		node = world->getEntities(uid&(World::numBuckets-1)).addNodeLast(this);
	} else {
		uid = World::nuid;
		node = nullptr;
	}

	// signal components again
	for( size_t c = 0; c < components.getSize(); ++c ) {
		components[c]->afterWorldInsertion(_world);
	}

	// run init script
	if( script && !scriptStr.empty() && world ) {
		StringBuf<64> path;
		if( world->isClientObj() && mainEngine->isRunningClient() ) {
			path.format("scripts/client/entities/%s.lua", scriptStr.get());
		} else if( world->isServerObj() && mainEngine->isRunningServer() ) {
			path.format("scripts/server/entities/%s.lua", scriptStr.get());
		}
		script->load(path.get());
		script->dispatch("init");
	}
}

void Entity::remove() {
	toBeDeleted=true;
}

void Entity::deleteAllVisMaps() {
	for( size_t c = 0; c < components.getSize(); ++c ) {
		components[c]->deleteAllVisMaps();
	}
}

bool Entity::isNearCharacter(float radius) const {
	LinkedList<Entity*> list;
	findEntitiesInRadius(radius, list);
	for( Node<Entity*>* node = list.getFirst(); node != nullptr; node = node->getNext() ) {
		Entity* entity = node->getData();
		if( entity->getPlayer() || entity->hasComponent(Component::COMPONENT_CHARACTER) ) {
			return true;
		}
	}
	return false;
}

bool Entity::isCrouching() const {
	if( player ) {
		return player->isCrouching();
	} else {
		return false;
	}
}

bool Entity::isMoving() const {
	if( vel.lengthSquared() > 0.01f ) {
		if( player ) {
			return player->isMoving();
		} else {
			return true;
		}
	} else {
		return false;
	}
}

bool Entity::hasJumped() const {
	if( player ) {
		return player->hasJumped();
	} else {
		return false;
	}
}

Angle Entity::getLookDir() const {
	if( player ) {
		return player->getLookDir();
	} else {
		return Angle();
	}
}

void Entity::findEntitiesInRadius(float radius, LinkedList<Entity*>& outList) const {
	if( !world ) {
		return;
	}
	world->findEntitiesInRadius( pos, radius, outList );
}

void Entity::update() {
	updateNeeded = false;

	// static entities never update
	if (ticks && isFlag(Entity::FLAG_STATIC) && !mainEngine->isEditorRunning()) {
		return;
	}

	if (!matSet) {
		glm::mat4 translationM = glm::translate(glm::mat4(1.f),glm::vec3(pos.x,-pos.z,pos.y));
		glm::mat4 rotationM = glm::mat4( 1.f );
		rotationM = glm::rotate(rotationM, (float)(ang.radiansYaw()), glm::vec3(0.f, -1.f, 0.f));
		rotationM = glm::rotate(rotationM, (float)(ang.radiansPitch()), glm::vec3(0.f, 0.f, -1.f));
		rotationM = glm::rotate(rotationM, (float)(ang.radiansRoll()), glm::vec3(1.f, 0.f, 0.f));
		glm::mat4 scaleM = glm::scale(glm::mat4(1.f),glm::vec3(scale.x, scale.z, scale.y));
		mat = translationM * rotationM * scaleM;
	} else {
		pos = Vector( mat[3][0], mat[3][2], -mat[3][1] );
		scale = Vector( glm::length( mat[0] ), glm::length( mat[2] ), glm::length( mat[1] ) );
		ang.yaw = PI/2.f - atan2f(mat[0][0], mat[0][2]);
		ang.pitch = asinf(mat[1][1]) - PI/2.f;
		ang.roll = 0.f;
		ang.bindAngles();
		/*glm::extractEulerAngleXYZ(mat, ang.roll, ang.yaw, ang.pitch);
		ang.yaw *= -1.f;
		ang.pitch *= -.1f;
		ang.roll *= -.1f;
		ang.bindAngles();*/
	}

	// update the chunk node
	if( world && world->getType() == World::WORLD_TILES ) {
		TileWorld* tileworld = static_cast<TileWorld*>(world);
		if( tileworld->getChunks() ) {
			Sint32 cW = tileworld->calcChunksWidth();
			Sint32 cH = tileworld->calcChunksHeight();
			if( cW > 0 && cH > 0 ) {
				Sint32 cX = std::min( std::max( 0, (Sint32)floor((pos.x / Tile::size) / Chunk::size) ), cW - 1 );
				Sint32 cY = std::min( std::max( 0, (Sint32)floor((pos.y / Tile::size) / Chunk::size) ), cH - 1 );

				if( cX != currentCX || cY != currentCY ) {
					clearChunkNode();

					currentCX = cX;
					currentCY = cY;

					chunkNode = tileworld->getChunks()[cY + cX * cH].addEPopulation(this);
				}
			}
		}
	}

	for( size_t c = 0; c < components.getSize(); ++c ) {
		if( components[c]->isToBeDeleted() ) {
			delete components[c];
			components.remove(c);
			--c;
		} else {
			components[c]->update();
		}
	}
}

void Entity::process() {
	++ticks;

	// correct orientations
	ang.wrapAngles();

	// update path request
	if( path ) {
		pathRequested = false;
		if (path->getSize() == 0) {
			delete path;
			path = nullptr;
		} else {
			const PathFinder::PathWaypoint& node = path->getFirst()->getData();

			if (world->getType() != World::type_t::WORLD_TILES) {
				TileWorld* tileWorld = static_cast<TileWorld*>(world);

				// place dest coordinates in the middle of the tile
				pathNode.x = node.x * Tile::size + Tile::size / 2.f; float& x = pathNode.x;
				pathNode.y = node.y * Tile::size + Tile::size / 2.f; float& y = pathNode.y;
				pathNode.z = tileWorld->getTiles()[y + x * tileWorld->getHeight()].getFloorHeight(); float& z = pathNode.z;
				float epsilon = Tile::size / 4.f;

				if (pos.x >= x-epsilon && pos.x <= x-epsilon &&
					pos.y >= y-epsilon && pos.y <= y-epsilon) {
					path->removeNode(path->getFirst());

					//Finished pathfinding.
					if (path->getSize() == 0) {
						mainEngine->fmsg(Engine::MSG_DEBUG, "Entity '%s' reached goal tile (%d, %d)!", getName().get(), node.x, node.y);
						pathNode = Vector(0.f);
						pathDir = Vector(0.f);

						delete path;
						path = nullptr;
					}
				}
				else {
					//Move to the target tile.
					//Stop moving once reached destination.
					Vector pathDir = (pathNode - pos).normal();
				}
			}
		}
	}

	// run entity script
	bool processed = false;
	if( !mainEngine->isEditorRunning() || mainEngine->isPlayTest() ) {
		if( script && !scriptStr.empty() && world ) {
			StringBuf<128> path;
			if( world->isClientObj() && mainEngine->isRunningClient() ) {
				path.format("scripts/client/entities/%s.lua", scriptStr.get());
			} else if( world->isServerObj() && mainEngine->isRunningServer() ) {
				path.format("scripts/server/entities/%s.lua", scriptStr.get());
			}

			// first time
			if( !ranScript ) {
				ranScript = true;
				script->load(path.get());
				script->dispatch("init");
			} else {
				script->dispatch("process");
				processed = true;
			}
		}

		// interpolate between new and old positions
		if( ticks - lastUpdate <= mainEngine->getTicksPerSecond()/15 && !isFlag(flag_t::FLAG_LOCAL) ) {
			Vector vDiff = newPos - pos;
			pos += vDiff / 4.f;

			Angle aDiff;
			aDiff.yaw = newAng.yaw - ang.yaw;
			aDiff.pitch = newAng.pitch - ang.pitch;
			aDiff.roll = newAng.roll - ang.roll;
			aDiff.wrapAngles();
			ang.yaw += aDiff.yaw / 4.f;
			ang.pitch += aDiff.pitch / 4.f;
			ang.roll += aDiff.roll / 4.f;

			if( vDiff.lengthSquared() > 0.f ) {
				updateNeeded = true;
			}
		}
	}

	// move entity
	move();

	// correct orientation again
	ang.wrapAngles();

	// update component matrices
	if( updateNeeded ) {
		update();
	}

	// process components
	for( size_t c = 0; c < components.getSize(); ++c ) {
		components[c]->process();
	}

	// run post process script
	if( !mainEngine->isEditorRunning() || mainEngine->isPlayTest() ) {
		if( script && !scriptStr.empty() && world && processed ) {
			StringBuf<128> path;
			if( world->isClientObj() && mainEngine->isRunningClient() ) {
				path.format("scripts/client/entities/%s.lua", scriptStr.get());
			} else if( world->isServerObj() && mainEngine->isRunningServer() ) {
				path.format("scripts/server/entities/%s.lua", scriptStr.get());
			}

			script->dispatch("postprocess");
		}
	}
}

void Entity::draw(Camera& camera, const ArrayList<Light*>& lights) const {
	bool editorRunning = mainEngine->isEditorRunning();
	if( !isFlag(flag_t::FLAG_VISIBLE) && (!editorRunning || !shouldSave) ) {
		return;
	}

	// skip editor only entities in orthographic (minimap) mode
	if( camera.isOrtho() && !shouldSave ) {
		return;
	}

	// in editor, skip entities at too great a distance
	if( editorRunning && world->isShowTools() ) {
		if( (camera.getGlobalPos() - pos).lengthSquared() > camera.getClipFar() * camera.getClipFar() ) {
			return;
		}
	}

	// setup overdraw characteristics
	if( isFlag(Entity::flag_t::FLAG_OVERDRAW) ) {
		glDepthRange(0.f, .01f);
	}

	// draw components
	for( Uint32 c = 0; c < components.getSize(); ++c ) {
		components[c]->draw(camera, lights);
	}

	// reset overdraw characteristics
	if( isFlag(Entity::flag_t::FLAG_OVERDRAW) ) {
		glDepthRange(0.f, 1.f);
	}
}

bool Entity::checkCollision(const Vector& newPos) {
	Vector oldPos = pos;
	pos = newPos;
	update();

	for( size_t c = 0; c < components.getSize(); ++c ) {
		if( components[c]->checkCollision() ) {
			pos = oldPos;
			update();
			return true;
		}
	}
	pos = oldPos;
	update();
	return false;
}

void Entity::animate(const char* name, bool blend) {
	LinkedList<Model*> models;
	findAllComponents<Model>(Component::COMPONENT_MODEL, models);
	for( auto& model : models ) {
		model->animate(name, blend);
	}
}

bool Entity::move() {
	if( !isFlag(Entity::FLAG_STATIC) ) {
		if( rot.yaw || rot.pitch || rot.roll || vel.lengthSquared()) {
			BBox* bbox = findComponentByName<BBox>("physics");
			if (bbox && bbox->getMass() < 0.f && !bbox->getParent()) {
				bbox->applyMoveForces(vel, rot);
				updateNeeded = true;
			} else {
				ang += rot;
				pos += vel;
				ang.wrapAngles();
				updateNeeded = true;
			}
		}
	}
	return true;
}

void Entity::applyForce(const Vector& force, const Vector& origin) {
	BBox* bbox = findComponentByName<BBox>("physics");
	if (bbox) {
		bbox->applyForce(force, origin);
	}
}

Entity::def_t* Entity::loadDef(const char* filename) {
	def_t* def = new def_t();
	def->index = (Uint32)mainEngine->getEntityDefs().getSize();
	FileHelper::readObject(filename, *def);
	return def;
}

bool Entity::saveDef(const char* filename) const {
	def_t def(*this);
	return FileHelper::writeObject(filename, EFileFormat::Json, def);
}

const Entity::def_t* Entity::findDef(const char* name) {
	for( const Node<Entity::def_t*>* node = mainEngine->getEntityDefs().getFirst(); node!=nullptr; node=node->getNext() ) {
		const Entity::def_t* def = node->getData();
		if( def->entity.getName() == name ) {
			return def;
		}
	}
	return nullptr;
}

const Entity::def_t* Entity::findDef(const Uint32 index) {
	Uint32 c;
	const Node<Entity::def_t*>* node;
	for( c = 0, node = mainEngine->getEntityDefs().getFirst(); node!=nullptr; node=node->getNext(), ++c ) {
		const Entity::def_t* def = node->getData();
		if( c == index ) {
			return def;
		}
	}
	return nullptr;
}

Entity* Entity::spawnFromDef(World* world, const Entity::def_t& def, const Vector& pos, const Angle& ang, Uint32 uid) {
	Entity* entity = new Entity(world, uid);
	def.entity.copy(world, entity);
	entity->defIndex = def.index;
	entity->animate("idle", false);

	entity->setPos(pos);
	entity->setNewPos(pos);
	entity->setAng(ang);
	entity->setNewAng(ang);

	BBox* bbox = entity->findComponentByName<BBox>("physics");
	if (bbox && bbox->getMass() != 0.f && !bbox->getParent()) {
		bbox->setPhysicsTransform(pos + bbox->getLocalPos(), ang - bbox->getLocalAng());
	}

	mainEngine->fmsg(Engine::MSG_DEBUG, "spawned entity '%s'", entity->getName().get());
	return entity;
}

bool Entity::hasComponent(Component::type_t type) const {
	for( size_t c = 0; c < components.getSize(); ++c ) {
		if( components[c]->getType() == type ) {
			return true;
		}
		if( components[c]->hasComponent(type) ) {
			return true;
		}
	}
	return false;
}

Entity* Entity::copy(World* world, Entity* entity) const {
	if( !entity ) {
		entity = new Entity(world);
	}

	entity->setPlayer(player);
	entity->setFlags(flags);
	entity->setPos(pos);
	entity->setVel(vel);
	entity->setNewPos(newPos);
	entity->setAng(ang);
	entity->setRot(rot);
	entity->setNewAng(newAng);
	entity->setScale(scale);
	entity->setDefIndex(defIndex);
	entity->setDefName(defName);
	entity->setName(name);
	entity->setScriptStr(scriptStr);
	entity->setFalling(falling);
	entity->setSort(sort);
	entity->keyvalues.copy(keyvalues);

	Component* component = nullptr;
	for( size_t c = 0; c < components.getSize(); ++c ) {
		switch( components[c]->getType() ) {
			case Component::COMPONENT_BASIC:
			{
				component = entity->addComponent<Component>();
				break;
			}
			case Component::COMPONENT_BBOX:
			{
				component = entity->addComponent<BBox>();
				BBox* bbox0 = static_cast<BBox*>(components[c]);
				BBox* bbox1 = static_cast<BBox*>(component);
				*bbox1 = *bbox0;
				break;
			}
			case Component::COMPONENT_MODEL:
			{
				component = entity->addComponent<Model>();
				Model* model0 = static_cast<Model*>(components[c]);
				Model* model1 = static_cast<Model*>(component);
				*model1 = *model0;
				break;
			}
			case Component::COMPONENT_LIGHT:
			{
				component = entity->addComponent<Light>();
				Light* light0 = static_cast<Light*>(components[c]);
				Light* light1 = static_cast<Light*>(component);
				*light1 = *light0;
				break;
			}
			case Component::COMPONENT_CAMERA:
			{
				component = entity->addComponent<Camera>();
				Camera* camera0 = static_cast<Camera*>(components[c]);
				Camera* camera1 = static_cast<Camera*>(component);
				*camera1 = *camera0;
				break;
			}
			case Component::COMPONENT_SPEAKER:
			{
				component = entity->addComponent<Speaker>();
				Speaker* speaker0 = static_cast<Speaker*>(components[c]);
				Speaker* speaker1 = static_cast<Speaker*>(component);
				*speaker1 = *speaker0;
				break;
			}
			case Component::COMPONENT_CHARACTER:
			{
				component = entity->addComponent<Character>();
				Character* character0 = static_cast<Character*>(components[c]);
				Character* character1 = static_cast<Character*>(component);
				*character1 = *character0;
				break;
			}
			default:
			{
				mainEngine->fmsg(Engine::MSG_WARN,"failed to copy component from '%s' with unknown type", entity->getName().get());
				break;
			}
		}
		if( !component ) {
			mainEngine->fmsg(Engine::MSG_WARN,"failed to copy component from entity '%s'", entity->getName());
		} else {
			*component = *components[c];
			components[c]->copyComponents(*component);
			mainEngine->fmsg(Engine::MSG_DEBUG,"copied %s component from '%s'", Component::typeStr[(int)component->getType()], entity->getName().get());
		}
	}

	entity->update();
	entity->animate("idle", false);

	return entity;
}

bool Entity::removeComponentByName(const char* name) {
	Component* component = findComponentByName<Component>(name);
	if( component ) {
		component->remove();
		return true;
	} else {
		return false;
	}
}

bool Entity::removeComponentByUID(const Uint32 uid) {
	Component* component = findComponentByUID<Component>(uid);
	if( component ) {
		component->remove();
		return true;
	} else {
		return false;
	}
}

Component* Entity::addComponent(Component::type_t type) {
	Component* component = nullptr;
	switch (type) {
	case Component::COMPONENT_BASIC:
		return addComponent<Component>();
	case Component::COMPONENT_BBOX:
		return addComponent<BBox>();
	case Component::COMPONENT_MODEL:
		return addComponent<Model>();
	case Component::COMPONENT_LIGHT:
		return addComponent<Light>();
	case Component::COMPONENT_CAMERA:
		return addComponent<Camera>();
	case Component::COMPONENT_SPEAKER:
		return addComponent<Speaker>();
	case Component::COMPONENT_CHARACTER:
		return addComponent<Character>();
	default:
		mainEngine->fmsg(Engine::MSG_ERROR, "addComponent: Unknown entity type %u", (Uint32)type);
		return nullptr;
	}
}

const World::hit_t Entity::lineTrace( const Vector& origin, const Vector& dest )
{
	if ( !world )
	{
		World::hit_t emptyResult;
		return emptyResult;
	}

	return world->lineTrace(origin, dest);
}

bool Entity::interact(Entity& user, BBox& bbox)
{
	if ( !isFlag(flag_t::FLAG_INTERACTABLE) || !script )
	{
		return false;
	}

	Script::Args args;
	args.addInt(user.getUID());
	args.addString(bbox.getName());

	return (script->dispatch("interact", &args) == 0);
}

void Entity::serialize(FileInterface * file) {
	Uint32 version = 1;
	file->property("Entity::version", version);

	file->property("name", name);
	file->property("pos", pos);
	file->property("ang", ang);
	file->property("scale", scale);
	file->property("scriptStr", scriptStr);
	file->property("flags", flags);
	file->property("falling", falling);
	file->property("sort", sort);
	if( version >= 1 ) {
		file->property("keys", keyvalues);
	}

	if (file->isReading()) {
		Uint32 componentCount = 0;
		file->propertyName("components");
		file->beginArray(componentCount);
		for (Uint32 index = 0; index < componentCount; ++index) {
			file->beginObject();

			Component::type_t type = Component::type_t::COMPONENT_BASIC;
			file->property("type", type);

			Component* newComponent = addComponent(type);
			file->property("data", *newComponent);

			file->endObject();
		}
		file->endArray();

		// important to init script engine
		setScriptStr(scriptStr.get());
		
		// post
		setNewPos(pos);
		setNewAng(ang);
		updateNeeded = true;
	}
	else {
		Uint32 componentCount = 0;
		for (Uint32 index = 0; index < components.getSize(); ++index) {
			if (components[index]->isEditorOnly()) {
				continue;
			}
			++componentCount;
		}

		file->propertyName("components");
		file->beginArray(componentCount);
		for (Uint32 index = 0; index < components.getSize(); ++index) {
			if (components[index]->isEditorOnly()) {
				continue;
			}

			file->beginObject();

			Component::type_t type = components[index]->getType();
			file->property("type", type);

			file->property("data", *components[index]);

			file->endObject();
		}
		file->endArray();
	}
}

void Entity::setKeyValue(const char* key, const char* value)
{
	String* exists = keyvalues[key];
	if ( exists )
	{
		*exists = value;
	}
	else
	{
		keyvalues.insert(key, value);
	}
}

void Entity::deleteKeyValue(const char* key)
{
	keyvalues.remove(key);
}

const char* Entity::getKeyValueAsString(const char* key) const
{
	const String* value = keyvalues.find(key);
	if( value ) {
		return value->get();
	} else {
		return "";
	}
}

float Entity::getKeyValueAsFloat(const char* key) const
{
	const String* value = keyvalues.find(key);
	if( value ) {
		return strtof(value->get(), nullptr);
	} else {
		return 0.f;
	}
}

int Entity::getKeyValueAsInt(const char* key) const
{
	const String* value = keyvalues.find(key);
	if( value ) {
		return strtol(value->get(), nullptr, 10);
	} else {
		return 0;
	}
}

const char* Entity::getKeyValue(const char* key) const
{
	return getKeyValueAsString(key);
}

void Entity::listener_t::onDeleted() {
	if( !entry ) {
		return;
	}
	Frame::entry_t* entryCast = (Frame::entry_t *)entry;
	entryCast->suicide = true;
}

void Entity::listener_t::onChangeColor(bool selected, bool highlighted) {
	if( !entry ) {
		return;
	}
	Frame::entry_t* entryCast = (Frame::entry_t *)entry;
	if( selected ) {
		entryCast->color = glm::vec4(1.f,0.f,0.f,1.f);
	} else if( highlighted ) {
		entryCast->color = glm::vec4(1.f,1.f,0.f,1.f);
	} else {
		entryCast->color = glm::vec4(1.f);
	}
}

void Entity::listener_t::onChangeName(const char* name) {
	if( !entry ) {
		return;
	}
	Frame::entry_t* entryCast = (Frame::entry_t *)entry;
	entryCast->text = name;
}

void Entity::findAPath(int endX, int endY) {
	if (!world)
	{
		mainEngine->fmsg(Engine::MSG_WARN, "Entity '%s' attempted to path without world object.", getName().get());
		return;
	}
	if (pathRequested)
	{
		mainEngine->fmsg(Engine::MSG_DEBUG, "Entity '%s' cannot calculate more than one path at a time! Please wait for previous request to complete.", getName().get());
		return; //Can only do one path at a time!
	}
	mainEngine->fmsg(Engine::MSG_WARN, "Entity '%s' requested a path from (%d, %d) to (%d, %d)!", getName().get(), getCurrentTileX(), getCurrentTileY(), endX, endY);
	pathRequested = true;

	pathTask = world->findAPath(getCurrentTileX(), getCurrentTileY(), endX, endY);
}

void Entity::findRandomPath() {
	if (!world || world->getType() != World::WORLD_TILES) {
		return;
	}
	TileWorld* tileWorld = static_cast<TileWorld*>(world);

	Sint32 x = -1;
	Sint32 y = -1;
	tileWorld->findRandomTile(Tile::size, x, y);
	if (x >= 0 && y >= 0) {
		findAPath(x, y);
	}
}

bool Entity::pathFinished() {
	if (!pathRequested)
	{
		return false;
	}

	if (!pathTask.valid())
	{
		mainEngine->fmsg(Engine::MSG_WARN, "Entity '%s' received invalid future pathtask!", getName().get());
		if (nullptr != path)
		{
			delete path;
			path = nullptr;
		}
		return true;
	}
	if (pathTask.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
	{
		mainEngine->fmsg(Engine::MSG_DEBUG, "Entity '%s' finished pathing!", getName().get());
		path = pathTask.get();
		mainEngine->fmsg(Engine::MSG_DEBUG, "Path size is %d", path->getSize());
		return true;
	}

	//mainEngine->fmsg(Engine::MSG_WARN, "Entity %s still pathfinding!", getName().get());
	return false;
}

void Entity::def_t::serialize(FileInterface * file) {
	Uint32 version = 1;
	file->property("Entity::def_t::version", version);
	if (version >= 1)
		file->property("editor", exposedInEditor);
	Model::dontLoadMesh = true;
	file->property("entity", entity);
	Model::dontLoadMesh = false;
}

bool Entity::isLocalPlayer() {
	if (player) {
		if (player->getClientID() == Player::invalidID) {
			return true;
		}
	}
	return false;
}

void Entity::setScriptStr(const char* _scriptStr) {
	scriptStr = _scriptStr;
	if (script) {
		delete script;
	}
	if (!scriptStr.empty() && world) {
		script = new Script(*this);
	}
	ranScript = false;
}