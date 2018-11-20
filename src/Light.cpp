// Light.cpp

#include "Main.hpp"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Engine.hpp"
#include "Client.hpp"
#include "World.hpp"
#include "Light.hpp"
#include "Camera.hpp"
#include "Script.hpp"
#include "BBox.hpp"
#include "Chunk.hpp"
#include "Tile.hpp"
#include "TileWorld.hpp"

const char* Light::meshStr = "assets/editor/light/light.FBX";
const char* Light::materialStr = "assets/editor/light/material.json";

const char* Light::shapeStr[SHAPE_NUM] = {
	"sphere",
	"box",
	"capsule",
	"cylinder",
	"cone",
	"pyramid"
};

Light::Light(Entity& _entity, Component* _parent) :
	Component(_entity, _parent) {
	name = typeStr[COMPONENT_LIGHT];

	// add a bbox for editor usage
	if( mainEngine->isEditorRunning() ) {
		BBox* bbox = addComponent<BBox>();
		bbox->setShape(BBox::SHAPE_SPHERE);
		bbox->setLocalPos(Vector(0.f, 0.f, -8.f));
		bbox->setLocalScale(Vector(8.f));
		bbox->setEditorOnly(true);
		bbox->update();
	}
}

Light::~Light() {
}

static Cvar cvar_lightCull("light.cull", "accuracy for lights' occlusion culling", "7");

void Light::update() {
	Component::update();

	World* world = entity->getWorld();
	if( world && world->isLoaded() ) {
		if( lastUpdate != entity->getTicks() || !chunksVisible ) {
			occlusionTest(radius, cvar_lightCull.toInt());
			lastUpdate = entity->getTicks();
			chunksShadow.clear();

			// get all chunks in a radius
			if( world->getType() == World::WORLD_TILES && entity->isFlag(Entity::FLAG_SHADOW) ) {
				TileWorld* tileworld = static_cast<TileWorld*>(world);

				Sint32 worldW = (Sint32)tileworld->getWidth() / Chunk::size;
				Sint32 worldH = (Sint32)tileworld->getHeight() / Chunk::size;
				Sint32 chunkSize = (Chunk::size * Tile::size);
				Sint32 chunkRadius = floor(radius / chunkSize);
				Sint32 midX = floor(gPos.x / chunkSize);
				Sint32 midY = floor(gPos.y / chunkSize);
				Sint32 startX = min( max( 0, midX - chunkRadius ), worldW - 1 );
				Sint32 startY = min( max( 0, midY - chunkRadius ), worldH - 1 );
				Sint32 endX = min( max( 0, midX + chunkRadius ), worldW - 1 );
				Sint32 endY = min( max( 0, midY + chunkRadius ), worldH - 1 );

				for( Sint32 x = startX; x <= endX; ++x ) {
					for( Sint32 y = startY; y <= endY; ++y ) {
						Chunk& chunk = tileworld->getChunks()[y + x * worldH];

						float lightX = gPos.x;
						float lightY = gPos.y;
						float chunkX = x * chunkSize;
						float chunkY = y * chunkSize;
						if( lightX>=chunkX && lightX<chunkX+chunkSize ) {
							chunkX = lightX;
						}
						if( lightY>=chunkY && lightY<chunkY+chunkSize ) {
							chunkY = lightY;
						}
						if( lightX != chunkX || lightY != chunkY ) {
							if( lightX>chunkX )
								chunkX += chunkSize;
							if( lightY>chunkY )
								chunkY += chunkSize;
							float diffX = (chunkX-lightX)*(chunkX-lightX);
							float diffY = (chunkY-lightY)*(chunkY-lightY);
							float dist = diffX+diffY;
							if( dist <= radius * radius ) {
								chunksShadow.push(&chunk);
							}
						} else {
							chunksShadow.push(&chunk);
						}
					}
				}
			}
		}
	}
}

void Light::draw(Camera& camera, Light* light) {
	// only render in the editor!
	if( !mainEngine->isEditorRunning() || !entity->getWorld()->isShowTools() || camera.isOrtho() ) {
		return;
	}

	// do not render for these fx passes
	if( camera.getDrawMode() >= Camera::DRAW_GLOW ) {
		return;
	}

	Mesh::shadervars_t shaderVars;
	shaderVars.customColorEnabled = GL_TRUE;
	shaderVars.customColorR = { color.x, color.y, color.z, 1.f };

	glm::mat4 matrix = glm::scale(glm::mat4(1.f), glm::vec3(.5f));
	Mesh* mesh = mainEngine->getMeshResource().dataForString(meshStr);
	Material* material = mainEngine->getMaterialResource().dataForString(materialStr);
	if( mesh && material ) {
		ShaderProgram* shader = mesh->loadShader(*this, camera, light, material, shaderVars, gMat * matrix);
		if( shader ) {
			mesh->draw(camera, this, shader);
		}
	}
}

void Light::load(FILE* fp) {
	Component::load(fp);

	Engine::freadl(&color, sizeof(Vector), 1, fp, nullptr, "Light::load()");
	Engine::freadl(&intensity, sizeof(float), 1, fp, nullptr, "Light::load()");
	Engine::freadl(&radius, sizeof(float), 1, fp, nullptr, "Light::load()");

	Uint32 reserved = 0;
	Engine::freadl(&reserved, sizeof(Uint32), 1, fp, nullptr, "Light::load()");

	loadSubComponents(fp);
}

void Light::serialize(FileInterface * file) {
	Component::serialize(file);

	Uint32 version = 2;
	file->property("Light::version", version);

	file->property("color", color);
	file->property("intensity", intensity);
	file->property("radius", radius);
	if (version >= 1) {
		file->property("shape", shape);
	}
	if (version >= 2) {
		file->property("shadow", shadow);
		file->property("arc", arc);
	}
}