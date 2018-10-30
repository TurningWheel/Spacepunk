// Component.cpp

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Main.hpp"
#include "Engine.hpp"
#include "Chunk.hpp"
#include "Component.hpp"
#include "Entity.hpp"
#include "TileWorld.hpp"

//Component headers.
#include "BBox.hpp"
#include "Model.hpp"
#include "Light.hpp"
#include "Camera.hpp"
#include "Speaker.hpp"
#include "Character.hpp"

const char* Component::typeStr[COMPONENT_MAX] = {
	"basic",
	"bbox",
	"model",
	"light",
	"camera",
	"speaker",
	"emitter",
	"character"
};

const char* Component::typeIcon[COMPONENT_MAX] = {
	"images/gui/component.png",
	"images/gui/bbox.png",
	"images/gui/mesh.png",
	"images/gui/light.png",
	"images/gui/camera.png",
	"images/gui/speaker.png",
	"images/gui/emitter.png",
	"images/gui/character.png"
};

Component::Component(Entity& _entity, Component* _parent) {
	entity = &_entity;
	parent = _parent;

	uid = entity->getNewComponentID();

	name = typeStr[COMPONENT_BASIC];
	lPos = Vector(0.f, 0.f, 0.f);
	lAng = Angle(0.f, 0.f, 0.f);
	lScale = Vector(1.f, 1.f, 1.f);
	lMat = glm::mat4(1.f);

	update();
}

Component::~Component() {
	for( size_t c = 0; c < components.getSize(); ++c ) {
		if( components[c] ) {
			delete components[c];
			components[c] = nullptr;
		}
	}
	components.clear();

	// delete occlusion data
	deleteVisMaps();

	// remove chunk node
	clearChunkNode();
}

void Component::clearAllChunkNodes() {
	clearChunkNode();
	for( size_t c = 0; c < components.getSize(); ++c ) {
		components[c]->clearAllChunkNodes();
	}
}

bool Component::seesEntity(const Entity& target, float range, int accuracy) {
	if( !chunksVisible ) {
		occlusionTest(range, accuracy);
	}
	
	// for tile worlds
	if( entity->getWorld()->getType() == World::WORLD_TILES ) {
		const TileWorld* tileworld = static_cast<const TileWorld*>(target.getWorld());
		Sint32 chunkW = tileworld->calcChunksWidth();
		Sint32 chunkH = tileworld->calcChunksHeight();
		Sint32 chunkX = min( max( 0, target.getCurrentCX() ), chunkW - 1 );
		Sint32 chunkY = min( max( 0, target.getCurrentCY() ), chunkH - 1 );
		if( chunksVisible[chunkY + chunkX * chunkH] ) {
			return true;
		} else {
			return false;
		}
	}

	return true;
}

void Component::occlusionTest(float range, int accuracy) {
	World* world = entity->getWorld();
	if( !world ) {
		return;
	} else {
		switch( world->getType() ) {
		case World::WORLD_TILES:
			occlusionTestTiles(range, accuracy);
			break;
		case World::WORLD_SECTORS:
		case World::WORLD_INVALID:
		default:
			mainEngine->fmsg(Engine::MSG_WARN, "Component tried occlusion testing in an invalid world!");
			break;
		}
	}
}

void Component::occlusionTestTiles(float range, int accuracy) {
	TileWorld* world = static_cast<TileWorld*>(entity->getWorld());
	if( !world ) {
		return;
	}

	float fX = gPos.x / Tile::size;
	float fY = gPos.y / Tile::size;

	Sint32 tWidth = (Sint32)world->getWidth();
	Sint32 tHeight = (Sint32)world->getHeight();
	Sint32 cWidth = (Sint32)world->calcChunksWidth();
	Sint32 cHeight = (Sint32)world->calcChunksHeight();

	// clear old visible tile/chunk data
	visibleChunks.resize(0);
	if( tilesWidth != tWidth || tilesHeight != tHeight ) {
		tilesWidth = tWidth;
		tilesHeight = tHeight;
		if( tilesVisible ) {
			delete[] tilesVisible;
		}
		tilesVisible = new bool[tWidth*tHeight];
		if( chunksVisible ) {
			delete[] chunksVisible;
		}
		chunksVisible = new bool[cWidth*cHeight];
	}

	Sint32 tX = min( max( 0, (Sint32)floor(fX) ), (Sint32)world->getWidth() - 1 );
	Sint32 tY = min( max( 0, (Sint32)floor(fY) ), (Sint32)world->getHeight() - 1 );
	Sint32 cX = min( max( 0, (Sint32)floor(fX / Chunk::size) ), cWidth - 1 );
	Sint32 cY = min( max( 0, (Sint32)floor(fY / Chunk::size) ), cHeight - 1 );

	// don't do occlusion culling under these conditions
	if( fX < 0.f || fX >= tWidth || fY < 0.f || fY >= tHeight ||
		!world->getTiles()[tY+tX*tHeight].hasVolume() ||
		( world->isShowTools() && getType() == COMPONENT_CAMERA ) ) {
		for( Sint32 x = 0; x < tWidth; ++x ) {
			for( Sint32 y = 0; y< tHeight; ++y ) {
				tilesVisible[y+x*tHeight] = true;
			}
		}
		for( Sint32 x = 0; x < cWidth; ++x ) {
			for( Sint32 y = 0; y < cHeight; ++y ) {
				chunksVisible[y+x*cHeight] = true;
				visibleChunks.push(&world->getChunks()[y+x*cHeight]);
			}
		}
		return;
	}

	// initial conditions
	memset(tilesVisible, 0, sizeof(bool) * tWidth * tHeight);
	memset(chunksVisible, 0, sizeof(bool) * cWidth * cHeight);
	if( world->getTiles()[tY + tX * tHeight].hasVolume() ) {
		tilesVisible[tY + tX * tHeight] = true;
	}

	// find visible tiles (line tests)
	occlusionTestTilesStep(range, tX, tY, accuracy);
	if( accuracy&1 ) {
		if( tX > 0 ) {
			occlusionTestTilesStep(range, tX-1, tY, accuracy);
		}
		if( tX < tWidth-1 ) {
			occlusionTestTilesStep(range, tX+1, tY, accuracy);
		}
		if( tY > 0 ) {
			occlusionTestTilesStep(range, tX, tY-1, accuracy);
		}
		if( tY < tHeight-1 ) {
			occlusionTestTilesStep(range, tX, tY+1, accuracy);
		}
	}

	// neighbor tiles are always visible
	if( accuracy&2 ) {
		bool* visibleByExtension = new bool[tWidth * tHeight];
		memset(visibleByExtension, 0, sizeof(bool) * tWidth * tHeight);
		for( Sint32 u = 1; u < tWidth-1; ++u ) {
			for( Sint32 v = 1; v < tHeight-1; ++v ) {
				Uint32 index = v + u * tHeight;
				if( !tilesVisible[index] && world->getTiles()[index].hasVolume() ) {
					if( accuracy&8 ) {
						for( Sint32 u2 = u-1; u2 <= u+1; ++u2 ) {
							for( Sint32 v2 = v-1; v2 <= v+1; ++v2 ) {
								Uint32 index2 = v2 + u2 * tHeight;
								if( tilesVisible[index2] && !visibleByExtension[index2] ) {
									tilesVisible[index] = true;
									visibleByExtension[index] = true;
									goto next;
								}
							}
						}
					} else {
						Uint32 index2;
						index2 = (v + 1) + u * tHeight;
						if( tilesVisible[index2] && !visibleByExtension[index2] ) {
							tilesVisible[index] = true;
							visibleByExtension[index] = true;
							goto next;
						}
						index2 = (v - 1) + u * tHeight;
						if( tilesVisible[index2] && !visibleByExtension[index2] ) {
							tilesVisible[index] = true;
							visibleByExtension[index] = true;
							goto next;
						}
						index2 = v + (u + 1) * tHeight;
						if( tilesVisible[index2] && !visibleByExtension[index2] ) {
							tilesVisible[index] = true;
							visibleByExtension[index] = true;
							goto next;
						}
						index2 = v + (u - 1) * tHeight;
						if( tilesVisible[index2] && !visibleByExtension[index2] ) {
							tilesVisible[index] = true;
							visibleByExtension[index] = true;
							goto next;
						}
					}
				}
			next:;
			}
		}
		delete[] visibleByExtension;
	}

	Sint32 cRange = (range / Tile::size) / Chunk::size;
	Uint32 chunkDim = Chunk::size * Chunk::size;

	// find visible chunks
	for( Sint32 u = cX - cRange; u <= cX + cRange; ++u ) {
		for( Sint32 v = cY - cRange; v <= cY + cRange; ++v ) {
			if( u < 0 || v < 0 || u >= cWidth || v >= cHeight ) {
				continue;
			}
			Uint32 index = v + u * cHeight;

			// try to find a visible tile in the chunk
			for( Uint32 c = 0; c < chunkDim; ++c ) {
				Tile* tile = world->getChunks()[index].getTile(c);
				if( tile ) {
					Uint32 tX = tile->getX() / Tile::size;
					Uint32 tY = tile->getY() / Tile::size;
					if( tilesVisible[tY + tX * tHeight] ) {
						visibleChunks.push(&world->getChunks()[index]);
						chunksVisible[index] = true;
						break;
					}
				}
			}
		}
	}
}

void Component::occlusionTestTilesStep(float range, Sint32 tX, Sint32 tY, int accuracy) {
	TileWorld* world = static_cast<TileWorld*>(entity->getWorld());

	Sint32 tRange = range / Tile::size;
	Sint32 tWidth = (Sint32)world->getWidth();
	Sint32 tHeight = (Sint32)world->getHeight();

	if( !world->getTiles()[tY + tX * tHeight].hasVolume() ) {
		return;
	}

	for( Sint32 u = tX - tRange; u <= tX + tRange; ++u ) {
		for( Sint32 v = tY - tRange; v <= tY + tRange; ++v ) {
			if( u < 0 || v < 0 || u >= tWidth || v >= tHeight ) {
				continue;
			}

			Uint32 index = v + u * tHeight;

			// skip tracing solid tiles
			if( !world->getTiles()[index].hasVolume() ) {
				continue;
			}
			if( accuracy&4 ) {
				Chunk* chunk = world->getTiles()[index].getChunk();
				if( chunk ) {
					for( Component* component : chunk->getCPopulation() ) {
						Entity* entity = component->getEntity();
						if( entity->isFlag(Entity::FLAG_OCCLUDE) ) {
							continue;
						}
					}
				}
			}

			// trace both ways
			if( occlusionTestTilesLine(tX, tY, u, v, (accuracy&4) != 0) ||
				occlusionTestTilesLine(u, v, tX, tY, (accuracy&4) != 0) ) {
				tilesVisible[index] = true;
			}
		}
	}
}

bool Component::occlusionTestTilesLine(Sint32 sX, Sint32 sY, Sint32 eX, Sint32 eY, bool entities)
{
	TileWorld* world = static_cast<TileWorld*>(entity->getWorld());

	Sint32 tWidth = (Sint32)world->getWidth();
	Sint32 tHeight = (Sint32)world->getHeight();

	Sint32 dx = eX - sX;
	Sint32 dy = eY - sY;
	Sint32 dxabs = abs(dx);
	Sint32 dyabs = abs(dy);
	Sint32 a = static_cast<Sint32>(floor(dyabs * .5f));
	Sint32 b = static_cast<Sint32>(floor(dxabs * .5f));
	Sint32 u2 = sX;
	Sint32 v2 = sY;

	if( dxabs > dyabs ) { // the line is more horizontal than vertical
		for( Sint32 i = 0; i < dxabs; ++i ) {
			u2 += sgn(dx);
			b += dyabs;
			if( b >= dxabs ) {
				b -= dxabs;
				v2 += sgn(dy);
			}
			if( u2 >= 0 && u2 < tWidth && v2 >= 0 && v2 < tHeight ) {
				Uint32 index = v2 + u2 * tHeight;
				if( entities ) {
					Chunk* chunk = world->getTiles()[index].getChunk();
					if( chunk ) {
						for( Component* component : chunk->getCPopulation() ) {
							Entity* entity = component->getEntity();
							if( entity->isFlag(Entity::FLAG_OCCLUDE) ) {
								return false;
							}
						}
					}
				}
				if( !world->getTiles()[index].hasVolume() ) {
					return false;
				}
			}
		}
	} else { // the line is more vertical than horizontal
		for( Sint32 i = 0; i < dyabs; ++i ) {
			v2 += sgn(dy);
			a += dxabs;
			if( a >= dyabs ) {
				a -= dyabs;
				u2 += sgn(dx);
			}
			if( u2 >= 0 && u2 < tWidth && v2 >= 0 && v2 < tHeight ) {
				Uint32 index = v2 + u2 * tHeight;
				if( entities ) {
					Chunk* chunk = world->getTiles()[index].getChunk();
					if( chunk ) {
						for( Component* component : chunk->getCPopulation() ) {
							Entity* entity = component->getEntity();
							if( entity->isFlag(Entity::FLAG_OCCLUDE) ) {
								return false;
							}
						}
					}
				}
				if( !world->getTiles()[index].hasVolume() ) {
					return false;
				}
			}
		}
	}

	return true;
}

void Component::deleteVisMaps() {
	tilesWidth = 0;
	tilesHeight = 0;
	if( tilesVisible ) {
		delete[] tilesVisible;
		tilesVisible = nullptr;
	}
	if( chunksVisible ) {
		delete[] chunksVisible;
		chunksVisible = nullptr;
	}
	visibleChunks.clear();
}

void Component::deleteAllVisMaps() {
	for( size_t c = 0; c < components.getSize(); ++c ) {
		components[c]->deleteAllVisMaps();
	}
	deleteVisMaps();
}

void Component::process() {
	if( updateNeeded ) {
		update();
	}

	for( size_t c = 0; c < components.getSize(); ++c ) {
		components[c]->process();
	}
}

void Component::beforeWorldInsertion(const World* world) {
	for( size_t c = 0; c < components.getSize(); ++c ) {
		components[c]->beforeWorldInsertion(world);
	}
}

void Component::afterWorldInsertion(const World* world) {
	for( size_t c = 0; c < components.getSize(); ++c ) {
		components[c]->afterWorldInsertion(world);
	}
}

bool Component::checkCollision() const {
	for( size_t c = 0; c < components.getSize(); ++c ) {
		if( components[c]->checkCollision() ) {
			return true;
		}
	}
	return false;
}

void Component::draw(Camera& camera, Light* light) {
	for( size_t c = 0; c < components.getSize(); ++c ) {
		components[c]->draw(camera, light);
	}
}

bool Component::hasComponent(type_t type) const {
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

void Component::rotate(const Angle& ang) {
	lMat *= glm::eulerAngleYXZ(ang.yaw, ang.pitch, ang.roll);
	updateNeeded = true;
	lMatSet = true;
}

void Component::translate(const Vector& vec) {
	lMat = glm::translate(lMat, glm::vec3(vec.x, vec.y, vec.z));
	updateNeeded = true;
	lMatSet = true;
}

void Component::scale(const Vector& vec) {
	lMat = glm::scale(lMat, glm::vec3(vec.x, vec.y, vec.z));
	updateNeeded = true;
	lMatSet = true;
}

void Component::revertRotation() {
	lMat[0] = { 1.f, 0.f, 0.f, 0.f };
	lMat[1] = { 0.f, 1.f, 0.f, 0.f };
	lMat[2] = { 0.f, 0.f, 2.f, 0.f };
}

void Component::revertTranslation() {
	lMat[3] = { 0.f, 0.f, 0.f, 1.f };
}

void Component::revertScale() {
	lMat[0] = glm::normalize(lMat[0]);
	lMat[1] = glm::normalize(lMat[1]);
	lMat[2] = glm::normalize(lMat[2]);
}

void Component::revertToIdentity() {
	lMat = glm::mat4();
}

void Component::update() {
	updateNeeded = false;
	
	deleteVisMaps();

	if (!lMatSet) {
		glm::mat4 translationM = glm::translate(glm::mat4(1.f),glm::vec3(lPos.x,-lPos.z,lPos.y));
		glm::mat4 rotationM = glm::mat4( 1.f );
		rotationM = glm::rotate(rotationM, (float)(lAng.radiansYaw()), glm::vec3(0.f, -1.f, 0.f));
		rotationM = glm::rotate(rotationM, (float)(lAng.radiansRoll()), glm::vec3(1.f, 0.f, 0.f));
		rotationM = glm::rotate(rotationM, (float)(lAng.radiansPitch()), glm::vec3(0.f, 0.f, -1.f));
		glm::mat4 scaleM = glm::scale(glm::mat4(1.f),glm::vec3(lScale.x, lScale.z, lScale.y));
		lMat = translationM * rotationM * scaleM;
	}

	if( parent ) {
		gMat = parent->getGlobalMat() * lMat;
		gAng.yaw = lAng.yaw + parent->getGlobalAng().yaw;
		gAng.pitch = lAng.pitch + parent->getGlobalAng().pitch;
		gAng.roll = -(lAng.roll + parent->getGlobalAng().roll);
		gAng.wrapAngles();
	} else {
		gMat = entity->getMat() * lMat;
		gAng.yaw = lAng.yaw + entity->getAng().yaw;
		gAng.pitch = lAng.pitch + entity->getAng().pitch;
		gAng.roll = -(lAng.roll + entity->getAng().roll);
		gAng.wrapAngles();
	}
	gPos = Vector( gMat[3][0], gMat[3][2], -gMat[3][1] );
	gScale = Vector( glm::length( gMat[0] ), glm::length( gMat[2] ), glm::length( gMat[1] ) );

	// update the chunk node
	World* world = entity->getWorld();
	if( world && world->getType() == World::WORLD_TILES ) {
		TileWorld* tileworld = static_cast<TileWorld*>(world);
		if( tileworld->getChunks() ) {
			Sint32 cW = tileworld->calcChunksWidth();
			Sint32 cH = tileworld->calcChunksHeight();
			if( cW > 0 && cH > 0 ) {
				Sint32 cX = std::min( std::max( 0, (Sint32)floor((gPos.x / Tile::size) / Chunk::size) ), cW - 1 );
				Sint32 cY = std::min( std::max( 0, (Sint32)floor((gPos.y / Tile::size) / Chunk::size) ), cH - 1 );

				if( cX != currentCX || cY != currentCY ) {
					clearChunkNode();

					currentCX = cX;
					currentCY = cY;

					if (entity->isFlag(Entity::FLAG_OCCLUDE)) {
						chunkNode = tileworld->getChunks()[cY + cX * cH].addCPopulation(this);
					}
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

void Component::copyComponents(Component& dest) {
	Component* component = nullptr;
	for( size_t c = 0; c < components.getSize(); ++c ) {
		switch( components[c]->getType() ) {
			case Component::COMPONENT_BASIC:
			{
				component = dest.addComponent<Component>();
				break;
			}
			case Component::COMPONENT_BBOX:
			{
				component = dest.addComponent<BBox>();
				BBox* bbox0 = static_cast<BBox*>(components[c]);
				BBox* bbox1 = static_cast<BBox*>(component);
				*bbox1 = *bbox0;
				break;
			}
			case Component::COMPONENT_MODEL:
			{
				component = dest.addComponent<Model>();
				Model* model0 = static_cast<Model*>(components[c]);
				Model* model1 = static_cast<Model*>(component);
				*model1 = *model0;
				break;
			}
			case Component::COMPONENT_LIGHT:
			{
				component = dest.addComponent<Light>();
				Light* light0 = static_cast<Light*>(components[c]);
				Light* light1 = static_cast<Light*>(component);
				*light1 = *light0;
				break;
			}
			case Component::COMPONENT_CAMERA:
			{
				component = dest.addComponent<Camera>();
				Camera* camera0 = static_cast<Camera*>(components[c]);
				Camera* camera1 = static_cast<Camera*>(component);
				*camera1 = *camera0;
				break;
			}
			case Component::COMPONENT_SPEAKER:
			{
				component = dest.addComponent<Speaker>();
				Speaker* speaker0 = static_cast<Speaker*>(components[c]);
				Speaker* speaker1 = static_cast<Speaker*>(component);
				*speaker1 = *speaker0;
				break;
			}
			case Component::COMPONENT_CHARACTER:
			{
				component = dest.addComponent<Character>();
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
}

bool Component::removeComponentByName(const char* name) {
	Component* component = findComponentByName<Component>(name);
	if( component ) {
		component->remove();
		return true;
	} else {
		return false;
	}
}

bool Component::removeComponentByUID(const Uint32 uid) {
	Component* component = findComponentByUID<Component>(uid);
	if( component ) {
		component->remove();
		return true;
	} else {
		return false;
	}
}

void Component::remove() {
	toBeDeleted = true;
}

void Component::load(FILE* fp) {
	Uint32 reserved = 0;
	Uint32 len = 0;

	char* nameStr = nullptr;
	Engine::freadl(&len, sizeof(Uint32), 1, fp, nullptr, "Component::load()");
	if( len > 0 && len < 128 ) {
		nameStr = (char *) calloc( len+1, sizeof(char));
		Engine::freadl(nameStr, sizeof(char), len, fp, nullptr, "Component::load()");
	} else if( len >= 128 ) {
		assert(0);
	}

	Vector pos;
	Engine::freadl(&pos, sizeof(Vector), 1, fp, nullptr, "Component::load()");

	Angle ang;
	Engine::freadl(&ang, sizeof(Angle), 1, fp, nullptr, "Component::load()");

	Vector scale;
	Engine::freadl(&scale, sizeof(Vector), 1, fp, nullptr, "Component::load()");

	// reserved 4 bytes
	Engine::freadl(&reserved, sizeof(Uint32), 1, fp, nullptr, "Component::load()");

	name = nameStr;
	lPos = pos;
	lAng = ang;
	lScale = scale;

	free(nameStr);

	if( getType() == COMPONENT_BASIC ) {
		loadSubComponents(fp);
	}
}

void Component::loadSubComponents(FILE* fp) {
	Uint32 numComponents = 0;
	Engine::freadl(&numComponents, sizeof(Uint32), 1, fp, nullptr, "Component::loadSubComponents()");
	for( Uint32 c = 0; c < numComponents; ++c ) {
		Component::type_t type = Component::type_t::COMPONENT_BASIC;
		Engine::freadl(&type, sizeof(Component::type_t), 1, fp, nullptr, "Component::loadSubComponents()");

		Component* component = addComponent(type);
		if( !component ) {
			mainEngine->fmsg(Engine::MSG_ERROR,"failed to load component for entity '%s'",entity->getName().get());
		} else {
			component->load(fp);
		}
	}
}

Component* Component::addComponent(Component::type_t type) {
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

void Component::serialize(FileInterface * file) {
	Uint32 version = 0;
	file->property("Component::version", version);
	file->property("name", name);
	file->property("lPos", lPos);
	file->property("lAng", lAng);
	file->property("lScale", lScale);
	serializeComponents(file);
}	

void Component::serializeComponents(FileInterface* file) {
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
