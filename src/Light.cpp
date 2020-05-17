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
#include "BasicWorld.hpp"
#include "Renderer.hpp"

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

Cvar cvar_shadowsEnabled("render.shadow.enabled", "enables shadow rendering", "1");
Cvar cvar_shadowsStaticOnly("render.shadow.static", "render only static shadow maps", "0");

Light::Light(Entity& _entity, Component* _parent) :
	Component(_entity, _parent) {
	name = typeStr[COMPONENT_LIGHT];

	// add a bbox for editor usage
	if (mainEngine->isEditorRunning()) {
		BBox* bbox = addComponent<BBox>();
		bbox->setShape(BBox::SHAPE_SPHERE);
		bbox->setLocalScale(Vector(8.f));
		bbox->setEditorOnly(true);
		bbox->update();
	}

	// exposed attributes
	attributes.push(new AttributeVector("Color", color));
	attributes.push(new AttributeFloat("Intensity", intensity));
	attributes.push(new AttributeFloat("Radius", radius));
	attributes.push(new AttributeFloat("Arc", arc));
	attributes.push(new AttributeBool("Shadow-casting", shadow));
	attributes.push(new AttributeEnum<shape_t>("Shape", shapeStr, shape_t::SHAPE_NUM, shape));
}

Light::~Light() {
}

static Cvar cvar_lightCull("light.cull", "accuracy for lights' occlusion culling", "7");

void Light::update() {
	Component::update();

	// occlusion test
	World* world = entity->getWorld();
	if (world && world->isLoaded()) {
		if (lastUpdate != entity->getTicks()) {
			lastUpdate = entity->getTicks();
		}
	}
}

void Light::draw(Camera& camera, const ArrayList<Light*>& lights) {
	// only render in the editor!
	if (!mainEngine->isEditorRunning() || !entity->getWorld()->isShowTools() || camera.isOrtho()) {
		return;
	}

	// do not render for these fx passes
	if (camera.getDrawMode() >= Camera::DRAW_GLOW) {
		return;
	}

	Mesh::shadervars_t shaderVars;
	shaderVars.customColorEnabled = true;
	shaderVars.customColorR = { color.x, color.y, color.z, 1.f };

	glm::mat4 matrix = glm::translate(glm::mat4(1.f), glm::vec3(0, -8.f, 0.f));
	matrix = glm::scale(matrix, glm::vec3(.5f));
	Mesh* mesh = mainEngine->getMeshResource().dataForString(meshStr);
	Material* material = mainEngine->getMaterialResource().dataForString(materialStr);
	if (mesh && material) {
		ShaderProgram* shader = mesh->loadShader(*this, camera, lights, material, shaderVars, gMat * matrix);
		if (shader) {
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

static Cvar cvar_shadowDepthOffset("render.shadow.depthoffset", "shadow depth buffer adjustment", "0");

bool Light::isOccluded(Entity& entity) {
	World* world = this->entity->getWorld();
	if (!world) {
		return true;
	}
	Entity* shadowCamera = world->getShadowCamera(); assert(shadowCamera);
	Camera* camera = shadowCamera->findComponentByUID<Camera>(1); assert(camera);
	for (int c = 0; c < 6; ++c) {
		camera->setOcclusionIndex(this->entity->getUID() * 6 + c);
		auto& query = camera->getOcclusionQuery(&entity);
		if (query.result == false) {
			return false;
		}
	}
	return true;
}

int Light::createShadowMap() {
	int result = 0;

	if (!entity || !entity->getWorld()) {
		return result;
	}
	World* world = entity->getWorld();
	if (!world) {
		return result;
	}
	if (entity->getTicks() == shadowTicks && shadowMap.isInitialized()) {
		return result;
	} else {
		shadowTicks = entity->getTicks();
	}

	if (shadowMap.isInitialized() && entity->isFlag(Entity::FLAG_STATIC)) {
		return result;
	}

	Entity* shadowCamera = world->getShadowCamera(); assert(shadowCamera);
	Camera* camera = shadowCamera->findComponentByUID<Camera>(1); assert(camera);
	shadowCamera->setPos(gPos);

	glPolygonOffset(cvar_shadowDepthOffset.toFloat(), 0.f);
	glEnable(GL_DEPTH_TEST);
	shadowMap.init();
	for (Uint32 c = 0; c < 6; ++c) {
		shadowMap.bindForWriting(Shadow::cameraInfo[c].face);
		Quaternion q;
		q = q.rotate(Shadow::cameraInfo[c].dir);
		shadowCamera->setAng(q);
		shadowCamera->update();
		int resolution = cvar_shadowResolution.toInt();
		camera->setWin(Rect<Sint32>(0, 0, resolution, resolution));
		camera->setClipNear(1.f);
		camera->setClipFar(radius);
		camera->setupProjection(false);
		auto bw = static_cast<BasicWorld*>(world); assert(bw);
		ArrayList<Entity*> drawList;
		bw->fillDrawList(*camera, radius * radius, drawList);

		// perform occlusion queries
		camera->setOcclusionIndex(entity->getUID() * 6 + c);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		camera->setDrawMode(Camera::DRAW_BOUNDS);
		bw->drawSceneObjects(*camera, ArrayList<Light*>(), drawList);

		// create reduced draw list
		ArrayList<Entity*> reducedDrawList;
		for (auto entity : drawList) {
			if (!entity->isOccluded(*camera) && entity->isFlag(Entity::FLAG_VISIBLE)) {
				reducedDrawList.push(entity);
			}
		}
		result += reducedDrawList.getSize();

		// draw shadow buffer
		glDepthMask(GL_TRUE);
		glDisable(GL_CULL_FACE);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		camera->setDrawMode(Camera::DRAW_SHADOW);
		bw->drawSceneObjects(*camera, ArrayList<Light*>({ this }), reducedDrawList);
		glEnable(GL_CULL_FACE);
	}
	glPolygonOffset(1.f, 0.f);

	Client* client = mainEngine->getLocalClient(); assert(client);
	Renderer* renderer = client->getRenderer(); assert(renderer);
	Framebuffer* fbo = renderer->getFramebuffer(); assert(fbo);
	fbo->bindForWriting();
	//glBindFramebuffer(GL_FRAMEBUFFER, fbo->getFBO());
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);

	shadowMapDrawn = true;
	return result;
}

void Light::deleteShadowMap() {
	shadowMap.term();
}