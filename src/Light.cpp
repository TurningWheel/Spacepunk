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

Cvar cvar_shadowsEnabled("render.shadows", "enables shadow rendering", "3");

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

	// occlusion test
	World* world = entity->getWorld();
	if( world && world->isLoaded() ) {
		if( lastUpdate != entity->getTicks() || !chunksVisible ) {
			occlusionTest(radius, cvar_lightCull.toInt());
			lastUpdate = entity->getTicks();
		}
	}
}

void Light::draw(Camera& camera, const ArrayList<Light*>& lights) {
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
		ShaderProgram* shader = mesh->loadShader(*this, camera, lights, material, shaderVars, gMat * matrix);
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

static Cvar cvar_shadowDepthOffset("render.shadowdepthoffset","shadow depth buffer adjustment","2");

void Light::createShadowMap() {
	if (!entity || !entity->getWorld()) {
		return;
	}
	TileWorld* world = static_cast<TileWorld*>(entity->getWorld());
	if (!world) {
		return;
	}
	if (entity->getTicks() == shadowTicks && shadowMap.isInitialized()) {
		return;
	} else {
		shadowTicks = entity->getTicks();
	}

	if (shadowMap.isInitialized() && entity->isFlag(Entity::FLAG_STATIC)) {
		return;
	}

	Entity* shadowCamera = world->getShadowCamera(); assert(shadowCamera);
	Camera* camera = shadowCamera->findComponentByUID<Camera>(1); assert(camera);
	camera->setDrawMode(Camera::DRAW_SHADOW);
	shadowCamera->setPos(gPos);

	glPolygonOffset(1.f, cvar_shadowDepthOffset.toFloat());
	glEnable(GL_DEPTH_TEST);
	shadowMap.init();
	for (Uint32 c = 0; c < 6; ++c) {
		shadowMap.bindForWriting(Shadow::cameraInfo[c].face);
		glClear(GL_DEPTH_BUFFER_BIT);
		shadowCamera->setAng(Shadow::cameraInfo[c].dir);
		shadowCamera->update();
		camera->setClipNear(1.f);
		camera->setClipFar(radius);
		camera->setupProjection(false);
		world->drawSceneObjects(*camera, ArrayList<Light*>({this}), visibleChunks);
	}
	glPolygonOffset(1.f, 0.f);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	shadowMapDrawn = true;
}

void Light::deleteShadowMap() {
	shadowMap.term();
}