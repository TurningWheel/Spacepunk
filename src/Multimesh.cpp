// Multimesh.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "Multimesh.hpp"
#include "Mesh.hpp"
#include "Camera.hpp"

Multimesh::Multimesh(Entity& _entity, Component* _parent) :
	Component(_entity, _parent) {
	name = typeStr[COMPONENT_MULTIMESH];

	meshStr = generateGUID();

	// exposed attributes
	attributes.push(new AttributeString("Material", materialStr));
	attributes.push(new AttributeString("Depth Fail Material", depthfailStr));
	attributes.push(new AttributeBool("Custom Color Enabled", shaderVars.customColorEnabled));
	attributes.push(new AttributeColor("Custom Red", shaderVars.customColorR));
	attributes.push(new AttributeColor("Custom Green", shaderVars.customColorG));
	attributes.push(new AttributeColor("Custom Blue", shaderVars.customColorB));
	attributes.push(new AttributeColor("Custom Glow", shaderVars.customColorA));
}

Multimesh::~Multimesh() {
	if (!meshStr.empty()) {
		mainEngine->getMeshResource().deleteData(meshStr.get());
	}
}

void Multimesh::draw(Camera& camera, const ArrayList<Light*>& lights) {
	// setup silhouette prepass
	bool silhouette = false;
	if (camera.getDrawMode() == Camera::DRAW_SILHOUETTE) {
		camera.setDrawMode(Camera::DRAW_DEPTH);
		glEnable(GL_STENCIL_TEST);
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		glDrawBuffer(GL_NONE);
		glClear(GL_STENCIL_BUFFER_BIT);
		glStencilFunc(GL_ALWAYS, 1, -1);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
		silhouette = true;
	}

	// prevents editor widgets from drawing for non-editor cameras
	if (camera.getEntity()->isShouldSave() && !entity->isShouldSave()) {
		return;
	}
	if (camera.getDrawMode() == Camera::DRAW_SHADOW && (!entity->isShouldSave() && entity->getScriptStr() == "")) {
		return;
	}

	// static lights only render static objects
	if (camera.getDrawMode() == Camera::DRAW_SHADOW && lights[0]->getEntity()->isFlag(Entity::FLAG_STATIC) && !entity->isFlag(Entity::FLAG_STATIC))
		return;

	// skip certain passes if necessary
	if (camera.getDrawMode() == Camera::DRAW_SHADOW && !(entity->isFlag(Entity::flag_t::FLAG_SHADOW)))
		return;
	if (camera.getDrawMode() == Camera::DRAW_STENCIL && !(entity->isFlag(Entity::flag_t::FLAG_SHADOW)))
		return;
	if (camera.getDrawMode() == Camera::DRAW_GLOW && !(entity->isFlag(Entity::flag_t::FLAG_GLOWING)))
		return;
	if (camera.getDrawMode() == Camera::DRAW_DEPTHFAIL && !(entity->isFlag(Entity::flag_t::FLAG_DEPTHFAIL)))
		return;

	// load assets
	Mesh* mesh = mainEngine->getMeshResource().dataForString(meshStr.get());
	Material* mat = mainEngine->getMaterialResource().dataForString(materialStr.get());
	Material* depthfailmat = mainEngine->getMaterialResource().dataForString(depthfailStr.get());

	if (mesh) {
		// skip models that aren't glowing in the "glow" pass...
		if (camera.getDrawMode() == Camera::DRAW_GLOW) {
			if (!mat) {
				return;
			}
			else if (!mat->isGlowing()) {
				return;
			}
		}

		// skip models that don't have depth fail materials in the depth fail pass...
		if (camera.getDrawMode() == Camera::DRAW_DEPTHFAIL) {
			if (!depthfailmat) {
				return;
			}
		}

		// load shader
		ShaderProgram* shader = nullptr;
		if (camera.getDrawMode() == Camera::DRAW_DEPTHFAIL) {
			shader = mesh->loadShader(*this, camera, lights, depthfailmat, shaderVars, gMat);
		}
		else {
			shader = mesh->loadShader(*this, camera, lights, mat, shaderVars, gMat);
		}

		// draw mesh
		if (shader) {
			mesh->draw(camera, this, shader);
		}

		// silhouette requires a second pass after the stencil op
		if (silhouette) {
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			glEnable(GL_DEPTH_TEST);
			glStencilFunc(GL_NOTEQUAL, 1, -1);
			glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
			camera.setDrawMode(Camera::DRAW_SILHOUETTE);
			ShaderProgram* shader = nullptr;
			shader = mesh->loadShader(*this, camera, lights, mat, shaderVars, gMat);
			if (shader) {
				mesh->draw(camera, this, shader);
			}
			glDepthMask(GL_TRUE);
			glDisable(GL_STENCIL_TEST);
			glStencilFunc(GL_ALWAYS, 0x00, 0xFF);
		}
	}
}

void Multimesh::update() {
	Component::update();
	Mesh* mesh = mainEngine->getMeshResource().dataForString(meshStr.get()); assert(mesh);
	mesh->clear();
	LinkedList<Model*> models;
	findAllComponents<Model>(Component::type_t::COMPONENT_MODEL, models);
	mesh->composeMesh(models, gMat);
}

void Multimesh::serialize(FileInterface * file) {
	Component::serialize(file);
	Uint32 version = 0;
	file->property("Multimesh::version", version);
	file->property("materialStr", materialStr);
	file->property("depthfailStr", depthfailStr);
	file->property("shaderVars", shaderVars);
}

String Multimesh::generateGUID() {
	if (!entity || !entity->getWorld()) {
		return String();
	}
	StringBuf<32> result("#w%ue%uc%u", 3, entity->getWorld()->getID(), entity->getUID(), uid);
	return result;
}

void Multimesh::afterWorldInsertion(const World* world) {
	if (!meshStr.empty()) {
		mainEngine->getMeshResource().deleteData(meshStr.get());
	}
	meshStr = generateGUID();
}