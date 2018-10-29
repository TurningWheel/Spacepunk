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

const int Model::maxAnimations = 8;
const char* Model::defaultMesh = "assets/block/block.FBX";

Model::Model(Entity& _entity, Component* _parent) :
	Component(_entity, _parent) {

	name = typeStr[COMPONENT_MODEL];
	meshStr = defaultMesh;

	// add a bbox for editor usage
	if( mainEngine->isEditorRunning() ) {
		BBox* bbox = addComponent<BBox>();
		bbox->setShape(BBox::SHAPE_MESH);
		bbox->setEditorOnly(true);
		bbox->update();
	}
}

Model::~Model() {
}

const char* Model::getAnimName() const {
	if( animations.getSize() > 0 ) {
		return animations.peek().name;
	} else {
		return "";
	}
}

float Model::getAnimTicks() const {
	if( animations.getSize() > 0 ) {
		return animations.peek().ticks;
	} else {
		return 0.f;
	}
}

bool Model::isAnimDone() const {
	if( animations.getSize() > 0 ) {
		return animations.peek().ticks >= animations.peek().end - animations.peek().begin;
	} else {
		return true;
	}
}

static Cvar cvar_pauseAnimation("anim.rate", "controls rate of model anims", "1.0");

void Model::process() {
	Component::process();

	// find speaker
	Speaker* speaker = findComponentByName<Speaker>("animSpeaker");

	// cycle animations
	for( Uint32 animIndex = max(0, (Sint32)animations.getSize() - 2); animIndex < animations.getSize(); ++animIndex ) {
		Mesh::animframes_t& anim = animations[animIndex];

		if( !skinUpdateNeeded ) {
			if( anim.blend < 1.f || anim.ticks == 0.f || anim.begin + anim.ticks <= anim.end || (anim.loop && anim.end - anim.begin > 0.f) ) {
				skinUpdateNeeded = true;
			}
		}

		// animation blending
		anim.blend += anim.blendRate;
		if( anim.blend > 1.f ) {
			anim.blend = 1.f;
		}
		if( anim.blend <= 0.f ) {
			animations.remove(animIndex);
			--animIndex;
			continue;
		}

		// step animation
		float step = ((float)Engine::defaultTickRate / (float)mainEngine->getTicksPerSecond()) * animationSpeed;
		float oldAnimTicks = anim.ticks;
		anim.ticks += step * cvar_pauseAnimation.toFloat();

		// calculate animation length
		float animLength = anim.end - anim.begin;
		while( anim.ticks < 0.f ) {
			anim.ticks += animLength;
		}

		// play animation sound trigger
		if( speaker ) {
			if( animLength > 1.f ) {
				unsigned int oFrame = (unsigned int) fmod(oldAnimTicks, animLength);
				unsigned int nFrame = (unsigned int) fmod(anim.ticks, animLength);
				unsigned int sFrame = step > 0.f ? oFrame : nFrame;
				unsigned int eFrame = step > 0.f ? nFrame : oFrame;
				for( unsigned int c = 0; c < anim.sounds.getSize(); ++c ) {
					const Animation::sound_t& sound = anim.sounds[c];
					if( sound.files.getSize() ) {
						if( (sound.frame >= sFrame && sound.frame <= eFrame) &&
							(sound.frame < anim.beginLastSoundFrame || sound.frame > anim.endLastSoundFrame) ) {
							Random& rand = mainEngine->getRandom();
							Uint32 fileIndex = rand.getUint32() % sound.files.getSize();
							const char* file = sound.files[fileIndex].get();
							speaker->playSound(file, false, speaker->getDefaultRange());
							break;
						}
					}
				}
				anim.beginLastSoundFrame = sFrame;
				anim.endLastSoundFrame = eFrame;
			}
		}
	}
}

Model::bone_t Model::findBone(const char* name) const {
	Mesh* mesh = mainEngine->getMeshResource().dataForString(meshStr.get());
	if( mesh ) {
		unsigned int bone = mesh->boneIndexForName(name);
		if( bone != UINT32_MAX ) {
			size_t c = 0;
			for( ; c < skincache.getSize(); ++c ) {
				if( bone >= skincache[c].anims.getSize() ) {
					bone -= (unsigned int)skincache[c].anims.getSize();
				} else {
					break;
				}
			}
			if( c >= skincache.getSize() ) {
				return bone_t();
			} else {
				glm::mat4 mat = skincache[c].offsets[bone];

				bone_t bone;
				glm::extractEulerAngleXYZ(mat, bone.ang.roll, bone.ang.pitch, bone.ang.yaw);
				bone.valid = true;
				bone.name = name;
				bone.mat = mat;
				bone.pos = Vector( mat[3][0], mat[3][2], -mat[3][1] );
				bone.scale = Vector( glm::length( mat[0] ), glm::length( mat[2] ), glm::length( mat[1] ) );

				return bone;
			}
		} else {
			return bone_t();
		}
	}
	return bone_t();
}

void Model::animate(const char* name, bool blend, bool loop) {
	if( animations.getSize() >= maxAnimations ) {
		mainEngine->fmsg(Engine::MSG_WARN,"Model '%s' tried to play '%s', but we have reached the limit (%d)", this->name.get(), name, maxAnimations);
		return;
	}

	Animation* animation = mainEngine->getAnimationResource().dataForString(animationStr.get());
	if( !animation ) {
		mainEngine->fmsg(Engine::MSG_DEBUG,"Model '%s' tried to play '%s', but has no animation key file", this->name.get(), name);
		return;
	}

	const Animation::entry_t* entry = animation->findEntry(name);
	if( !entry ) {
		mainEngine->fmsg(Engine::MSG_WARN,"Model '%s' tried to play '%s', but the specified animation was not found", this->name.get(), name);
		return;
	}

	Mesh::animframes_t anim;
	anim.name = name;
	anim.begin = entry->begin;
	anim.ticks = 0.f;
	anim.end = entry->end;
	anim.loop = loop;
	
	// copy sound triggers
	const LinkedList<Animation::sound_t>& sounds = animation->getSounds();
	for( const Node<Animation::sound_t>* node = sounds.getFirst(); node != nullptr; node = node->getNext() ) {
		const Animation::sound_t& sound = node->getData();

		if( sound.frame >= entry->begin && sound.frame < entry->end ) {
			Animation::sound_t newSound;
			newSound.frame = sound.frame - anim.begin;
			newSound.files = sound.files;
			anim.sounds.push(newSound);
		}
	}

	if( animations.getSize() > 0 ) {
		if( blend ) {
			float blendRate = 3.75f / mainEngine->getTicksPerSecond();

			anim.blend = 0.f;
			anim.blendRate = blendRate;
			animations[animations.getSize()-1].blendRate = -blendRate;
		} else {
			anim.blend = 1.f;
			anim.blendRate = 0;
			animations.clear();
		}
	} else {
		anim.blend = 1.f;
		anim.blendRate = 0;
	}
	anim.beginLastSoundFrame = UINT32_MAX;
	anim.endLastSoundFrame = UINT32_MAX;
	animations.push(anim);
}

void Model::updateSkin() {
	if( skinUpdateNeeded ) {
		skinUpdateNeeded = false;
		Mesh* mesh = mainEngine->getMeshResource().dataForString(meshStr.get());
		if( mesh ) {
			mesh->skin(animations, skincache);
		}
	}
}

void Model::draw(Camera& camera, Light* light) {
	Component::draw(camera, light);

	// prevents editor widgets from drawing for non-editor cameras
	if( camera.getEntity()->isShouldSave() && !entity->isShouldSave() ) {
		return;
	}

	// skip certain passes if necessary
	if( camera.getDrawMode()==Camera::DRAW_STENCIL && !(entity->isFlag(Entity::flag_t::FLAG_SHADOW)) )
		return;
	if( camera.getDrawMode()==Camera::DRAW_GLOW && !(entity->isFlag(Entity::flag_t::FLAG_GLOWING)) )
		return;
	if( camera.getDrawMode()==Camera::DRAW_DEPTHFAIL && !(entity->isFlag(Entity::flag_t::FLAG_DEPTHFAIL)) )
		return;

	// don't render models marked genius
	if( ( entity->isFlag(Entity::flag_t::FLAG_GENIUS) || genius ) && camera.getEntity() == entity ) {
		if( camera.getDrawMode() != Camera::DRAW_STENCIL ) {
			return;
		}
	}

	// load assets
	Mesh* mesh = mainEngine->getMeshResource().dataForString(meshStr.get());
	Material* mat = mainEngine->getMaterialResource().dataForString(materialStr.get());
	Material* depthfailmat = mainEngine->getMaterialResource().dataForString(depthfailStr.get());
	Animation* animation = mainEngine->getAnimationResource().dataForString(animationStr.get());

	// clear invalid data
	if( !mesh ) {
		meshStr = "";
	}
	if( !mat ) {
		materialStr = "";
	}
	if( !depthfailmat ) {
		depthfailStr = "";
	}
	if( !animation ) {
		animationStr = "";
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
			shader = mesh->loadShader(*this, camera, light, depthfailmat, shaderVars, gMat);
		} else {
			shader = mesh->loadShader(*this, camera, light, mat, shaderVars, gMat);
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

	if( hasAnimations() ) {
		animate("idle", false, true);
	}

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

	if (file->isReading() && hasAnimations()) {
		animate("idle", false, true);
	}

	if (version >= 1) {
		file->property("genius", genius);
	}
}
