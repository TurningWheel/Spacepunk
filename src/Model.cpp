// Model.cpp

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "Main.hpp"
#include "LinkedList.hpp"
#include "Node.hpp"
#include "Engine.hpp"
#include "Entity.hpp"
#include "Client.hpp"
#include "Camera.hpp"
#include "Mesh.hpp"
#include "ShaderProgram.hpp"
#include "Resource.hpp"
#include "Light.hpp"
#include "Model.hpp"
#include "Material.hpp"
#include "Renderer.hpp"
#include "Script.hpp"
#include "BBox.hpp"
#include "String.hpp"
#include "Speaker.hpp"
#include "Console.hpp"

bool Model::dontLoadMesh = false;
const int Model::maxAnimations = 8;
const char* Model::defaultMesh = "assets/block/block.FBX";

Model::Model(Entity& _entity, Component* _parent) :
	Component(_entity, _parent) {

	name = typeStr[COMPONENT_MODEL];
	meshStr = defaultMesh;
	loadAnimations();

	// add a bbox for editor usage
	if( mainEngine->isEditorRunning() && !dontLoadMesh ) {
		BBox* bbox = addComponent<BBox>();
		bbox->setShape(BBox::SHAPE_MESH);
		bbox->setEditorOnly(true);
		bbox->update();
	}

	// exposed attributes
	attributes.push(new AttributeFile("Mesh", "vox,FBX,dae,gltf,glb,blend,3ds,ase,obj,ifc,xgl,zgl,ply,dxf,lwo,lws,lxo,stl,x,ac,ms3d,cob,scn,xml,ogex,3d,mdl,md2,md3,pk3,mdc,md5,smd,vta,b3d,q3d,q3s,nff,noff,raw,ter,hmp,ndo", meshStr));
	attributes.push(new AttributeFile("Material", "json", materialStr));
	attributes.push(new AttributeFile("Depth Fail Material", "json", depthfailStr));
	attributes.push(new AttributeFile("Animation", "json", animationStr));
	attributes.push(new AttributeBool("Custom Color Enabled", shaderVars.customColorEnabled));
	attributes.push(new AttributeColor("Custom Red", shaderVars.customColorR));
	attributes.push(new AttributeColor("Custom Green", shaderVars.customColorG));
	attributes.push(new AttributeColor("Custom Blue", shaderVars.customColorB));
	attributes.push(new AttributeColor("Custom Glow", shaderVars.customColorA));
}

Model::~Model() {
}

float Model::getAnimTicks() const {
	const AnimationState* animation = animations.find(currentAnimation.get());
	if (animation) {
		return animation->getTicks();
	} else {
		return 0.f;
	}
}

bool Model::isAnimDone() const {
	const AnimationState* animation = animations.find(currentAnimation.get());
	if (animation && animation->isFinished()) {
		return true;
	} else {
		return false;
	}
}

void Model::process() {
	Component::process();

	// find speaker
	Speaker* speaker = findComponentByName<Speaker>("animSpeaker");

	// update animations
	for( auto& pair : animations ) {
		skinUpdateNeeded = pair.b.update(speaker) ? true : skinUpdateNeeded;
	}
}

Uint32 Model::findBoneIndex(const char* name) const {
	Mesh* mesh = mainEngine->getMeshResource().dataForString(meshStr.get());
	if( mesh ) {
		return (Uint32)mesh->boneIndexForName(name);
	} else {
		return UINT32_MAX;
	}
}

glm::mat4 Model::findBone(Uint32 bone) const {
	Uint32 c = 0;
	for( ; c < skincache.getSize(); ++c ) {
		if( bone >= skincache[c].anims.getSize() ) {
			bone -= (unsigned int)skincache[c].anims.getSize();
		} else {
			break;
		}
	}
	if( c >= skincache.getSize() ) {
		return glm::mat4(1.f);
	} else {
		return skincache[c].offsets[bone];
	}
}

AnimationState* Model::findAnimation(const char* name) {
	return animations.find(name);
}

void Model::setWeightOnChildren(const aiNode* root, AnimationState& animation, float rate, float weight) {
	animation.setWeight(root->mName.data, weight);
	animation.setWeightRate(root->mName.data, rate);
	for (unsigned int c = 0; c < root->mNumChildren; ++c) {
		setWeightOnChildren(root->mChildren[c], animation, rate, weight);
	}
}

bool Model::animate(const char* name, bool blend) {
	if (dontLoadMesh) {
		return false;
	}
	Mesh* mesh = mainEngine->getMeshResource().dataForString(meshStr.get());
	if (!mesh || !animations.exists(name)) {
		if (strcmp(name,"__tpose")) {
			animate("__tpose", false);
		}
		return false;
	}
	for (auto& pair : animations) {
		if (pair.a == name) {
			if (currentAnimation.empty() || !blend) {
				for (auto& subMesh : mesh->getSubMeshes()) {
					setWeightOnChildren(subMesh->getRootNode(), pair.b, 0.f, 1.f);
				}
			} else {
				for (auto& subMesh : mesh->getSubMeshes()) {
					setWeightOnChildren(subMesh->getRootNode(), pair.b, 0.1f, 0.f);
				}
			}
			pair.b.setTicks(0.f);
			pair.b.setTicksRate(1.f);
		} else if (pair.a == currentAnimation && !currentAnimation.empty() && blend) {
			for (auto& subMesh : mesh->getSubMeshes()) {
				setWeightOnChildren(subMesh->getRootNode(), pair.b, -0.1f, 1.f);
			}
			pair.b.setTicksRate(0.f);
		} else {
			pair.b.clearWeights();
		}
	}
	previousAnimation = currentAnimation;
	currentAnimation = name;
	updateSkin();
	return true;
}

void Model::loadAnimations() {
	animations.clear();
	Animation* animation = mainEngine->getAnimationResource().dataForString(animationStr.get());
	if (animation) {
		for (auto& entry : animation->getEntries()) {
			animations.insert(entry.name.get(), AnimationState(entry, animation->getSounds()));
		}
	}

	// insert default animation
	Animation::entry_t tPose;
	tPose.begin = 0;
	tPose.end = 1;
	tPose.loop = false;
	tPose.name = "__tpose";
	animations.insert("__tpose", AnimationState(tPose, ArrayList<Animation::sound_t>()));

	animate("idle", false);
}

void Model::updateSkin() {
	if (skinUpdateNeeded) {
		skinUpdateNeeded = false;
		Mesh* mesh = mainEngine->getMeshResource().dataForString(meshStr.get());
		if( mesh ) {
			mesh->skin(animations, skincache);
		}
	}
}

void Model::draw(Camera& camera, const ArrayList<Light*>& lights) {
	Component::draw(camera, lights);

	// don't draw the model if its assets are missing
	if (broken) {
		return;
	}

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
	if( camera.getEntity()->isShouldSave() && !entity->isShouldSave() ) {
		return;
	}
	if( camera.getDrawMode() == Camera::DRAW_SHADOW && (!entity->isShouldSave() && entity->getScriptStr() == "") ) {
		return;
	}

	// static lights only render static objects
	if( camera.getDrawMode()==Camera::DRAW_SHADOW && lights[0]->getEntity()->isFlag(Entity::FLAG_STATIC) && !entity->isFlag(Entity::FLAG_STATIC) )
		return;

	// skip certain passes if necessary
	if( camera.getDrawMode()==Camera::DRAW_SHADOW && !(entity->isFlag(Entity::flag_t::FLAG_SHADOW)) )
		return;
	if( camera.getDrawMode()==Camera::DRAW_STENCIL && !(entity->isFlag(Entity::flag_t::FLAG_SHADOW)) )
		return;
	if( camera.getDrawMode()==Camera::DRAW_GLOW && !(entity->isFlag(Entity::flag_t::FLAG_GLOWING)) )
		return;
	if( camera.getDrawMode()==Camera::DRAW_DEPTHFAIL && !(entity->isFlag(Entity::flag_t::FLAG_DEPTHFAIL)) )
		return;

	// don't render models marked genius
	if( ( entity->isFlag(Entity::flag_t::FLAG_GENIUS) || genius ) ) {
		if( camera.getEntity() == entity && camera.getDrawMode() != Camera::DRAW_STENCIL ) {
			return;
		}
	}

	// load assets
	Mesh* mesh = mainEngine->getMeshResource().dataForString(meshStr.get());
	Material* mat = mainEngine->getMaterialResource().dataForString(materialStr.get());
	Material* depthfailmat = mainEngine->getMaterialResource().dataForString(depthfailStr.get());

	// mark model as "broken"
	if( (!mesh && !meshStr.empty()) ||
		(!mat && !materialStr.empty()) ||
		(!depthfailmat && !depthfailStr.empty()) ) {
		broken = true;
	}

	// transparent objects not rendered to depth buffer
	if (camera.getDrawMode() == Camera::DRAW_DEPTH && mat && mat->isTransparent()) {
		return;
	}

	// non-shadow-casting materials don't cast shadows
	if (camera.getDrawMode() == Camera::DRAW_SHADOW && mat && !mat->isShadowing()) {
		return;
	}

	if( mesh ) {
		// skip models that aren't glowing in the "glow" pass...
		if( camera.getDrawMode() == Camera::DRAW_GLOW ) {
			if( !mat ) {
				return;
			} else if( !mat->isGlowing() ) {
				return;
			}
		}

		// skip models that don't have depth fail materials in the depth fail pass...
		if( camera.getDrawMode() == Camera::DRAW_DEPTHFAIL ) {
			if( !depthfailmat ) {
				return;
			}
		}

		// load shader
		ShaderProgram* shader = nullptr;
		if( camera.getDrawMode() == Camera::DRAW_DEPTHFAIL ) {
			shader = mesh->loadShader(*this, camera, lights, depthfailmat, shaderVars, gMat);
		} else {
			shader = mesh->loadShader(*this, camera, lights, mat, shaderVars, gMat);
		}

		// update skin
		if( skinUpdateNeeded ) {
			skinUpdateNeeded = false;
			mesh->skin(animations, skincache);
		}

		// draw mesh
		if( shader ) {
			mesh->draw(camera, this, skincache, shader);
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
			if( shader ) {
				mesh->draw(camera, this, skincache, shader);
			}
			glDepthMask(GL_TRUE);
			glDisable(GL_STENCIL_TEST);
			glStencilFunc(GL_ALWAYS, 0x00, 0xFF);
		}
	}
}

bool Model::hasAnimations() const {
	Mesh* mesh = mainEngine->getMeshResource().dataForString(meshStr.get());
	if( mesh ) {
		if( mesh->hasAnimations() ) {
			return true;
		}
	}
	return false;
}

void Model::load(FILE* fp) {
	// DEPRECATED, DO NOT USE
	Component::load(fp);

	Uint32 len = 0;

	len = 0;
	char* mesh = nullptr;
	Engine::freadl(&len, sizeof(Uint32), 1, fp, nullptr, "Model::load()");
	if( len > 0 && len < 128 ) {
		mesh = (char *) calloc( len+1, sizeof(char));
		Engine::freadl(mesh, sizeof(char), len, fp, nullptr, "Model::load()");
	} else if( len >= 128 ) {
		assert(0);
	}
	meshStr = mesh;
	free(mesh);

	len = 0;
	char* material = nullptr;
	Engine::freadl(&len, sizeof(Uint32), 1, fp, nullptr, "Model::load()");
	if( len > 0 && len < 128 ) {
		material = (char *) calloc( len+1, sizeof(char));
		Engine::freadl(material, sizeof(char), len, fp, nullptr, "Model::load()");
	} else if( len >= 128 ) {
		assert(0);
	}
	materialStr = material;
	free(material);

	len = 0;
	char* depthfail = nullptr;
	Engine::freadl(&len, sizeof(Uint32), 1, fp, nullptr, "Model::load()");
	if( len > 0 && len < 128 ) {
		depthfail = (char *) calloc( len+1, sizeof(char));
		Engine::freadl(depthfail, sizeof(char), len, fp, nullptr, "Model::load()");
	} else if( len >= 128 ) {
		assert(0);
	}
	depthfailStr = depthfail;
	free(depthfail);

	len = 0;
	char* animation = nullptr;
	Engine::freadl(&len, sizeof(Uint32), 1, fp, nullptr, "Model::load()");
	if( len > 0 && len < 128 ) {
		animation = (char *) calloc( len+1, sizeof(char));
		Engine::freadl(animation, sizeof(char), len, fp, nullptr, "Model::load()");
	} else if( len >= 128 ) {
		assert(0);
	}
	animationStr = animation;
	free(animation);

	Engine::freadl(&shaderVars.customColorEnabled, sizeof(GLboolean), 1, fp, nullptr, "Model::load()");
	Engine::freadl(&shaderVars.customColorR[0], sizeof(GLfloat), 4, fp, nullptr, "Model::load()");
	Engine::freadl(&shaderVars.customColorG[0], sizeof(GLfloat), 4, fp, nullptr, "Model::load()");
	Engine::freadl(&shaderVars.customColorB[0], sizeof(GLfloat), 4, fp, nullptr, "Model::load()");
	Engine::freadl(&shaderVars.customColorA[0], sizeof(GLfloat), 4, fp, nullptr, "Model::load()");

	Uint32 reserved = 0;
	Engine::freadl(&reserved, sizeof(Uint32), 1, fp, nullptr, "Model::load()");

	loadAnimations();

	loadSubComponents(fp);
}

void Model::serialize(FileInterface * file) {
	Component::serialize(file);

	Uint32 version = 1;
	file->property("Model::version", version);
	file->property("meshStr", meshStr);
	file->property("materialStr", materialStr);
	file->property("depthfailStr", depthfailStr);
	file->property("animationStr", animationStr);

	file->property("shaderVars", shaderVars);

	if (file->isReading()) {
		loadAnimations();
	}

	if (version >= 1) {
		file->property("genius", genius);
	}
}
