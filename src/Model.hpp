// Model.hpp

// A component represented by meshes that supports animation.

#pragma once

#define GLM_FORCE_RADIANS
#include <glm/vec4.hpp>

#include <btBulletDynamicsCommon.h>

#include "ArrayList.hpp"
#include "LinkedList.hpp"
#include "Node.hpp"
#include "Component.hpp"
#include "Light.hpp"
#include "String.hpp"
#include "Mesh.hpp"
#include "Material.hpp"
#include "Map.hpp"
#include "AnimationState.hpp"

class Engine;
class Script;
class World;
class ShaderProgram;
class Material;
class BBox;
class Camera;

class Model : public Component {
public:
	// bone data
	struct bone_t {
		bool valid = false;
		String name;
		glm::mat4 mat;
		Vector pos;
		Angle ang;
		Vector scale;
	};

	typedef ArrayList<Mesh::skincache_t> SkinCache;

	Model(Entity& _entity, Component* _parent);
	virtual ~Model();

	// max animations that a model can play
	static const int maxAnimations;

	// default mesh
	static const char* defaultMesh;

	// draws the component
	// @param camera the camera through which to draw the component
	// @param light the light by which the component should be illuminated (or nullptr for no illumination)
	virtual void draw(Camera& camera, const ArrayList<Light*>& lights) override;

	// update the component
	virtual void process() override;

	// finds a bone with the given name
	// @param name the name of the bone to search for
	// @return a struct containing bone position, orientation, etc.
	bone_t findBone(const char* name) const;

	// @return true if the model has any animation keyfile assigned
	bool hasAnimations() const;

	// load the component from a file
	// @param fp the file to read from
	virtual void load(FILE* fp) override;

	// save/load this object to a file
	// @param file interface to serialize with
	virtual void serialize(FileInterface * file) override;

	// updates skin if necessary
	void updateSkin();

	// find and return the animation state with the given name
	// @param name The name of the animation state
	// @return The animation state with the given name, if any
	AnimationState* findAnimation(const char* name);

	// cancel all other animations on the mesh and use only the given one
	// @param name The name of the animation to play
	// @param blend Blend from the previous animation to the new one
	// @return true if the animation was found, otherwise false (and do nothing)
	bool animate(const char* name, bool blend = true);

	// get the duration of the currently playing animation
	// @return the amount of time the current animation has been playing
	float getAnimTicks() const;

	// determines if the current animation is done
	// @return true if the animation has wrapped, otherwise false
	bool isAnimDone() const;

	// getters & setters
	virtual type_t						getType() const override			{ return COMPONENT_MODEL; }
	const char*							getMesh() const						{ return meshStr.get(); }
	const char*							getMaterial() const					{ return materialStr.get(); }
	const char*							getDepthFailMat() const				{ return depthfailStr.get(); }
	const char*							getAnimation() const				{ return animationStr.get(); }
	const Map<AnimationState>&			getAnimations() const				{ return animations; }
	Map<AnimationState>&				getAnimations()						{ return animations; }
	const Mesh::shadervars_t&			getShaderVars() const				{ return shaderVars; }
	const SkinCache&					getSkinCache() const				{ return skincache; }
	float								getAnimationSpeed() const			{ return animationSpeed; }
	bool								isSkinUpdateNeeded() const			{ return skinUpdateNeeded; }
	bool								isGenius() const					{ return genius; }
	const char*							getCurrentAnimation() const			{ return currentAnimation.get(); }
	const char*							getPreviousAnimation() const		{ return previousAnimation.get(); }

	void	setMesh(const char* _mesh)										{ meshStr = _mesh; loadAnimations(); updateNeeded = true; broken = false; }
	void	setMaterial(const char* _material)								{ materialStr = _material; broken = false; }
	void	setDepthFailMat(const char* _depthfailmat)						{ depthfailStr = _depthfailmat; broken = false; }
	void	setAnimation(const char* _animation)							{ animationStr = _animation; loadAnimations(); }
	void	setShaderVars(const Mesh::shadervars_t& _shaderVars)			{ shaderVars = _shaderVars; }
	void	setAnimationSpeed(const float _animationSpeed)					{ animationSpeed = _animationSpeed; }
	void	setGenius(const bool _genius)									{ genius = _genius; }

	Model& operator=(const Model& src) {
		genius = src.genius;
		meshStr = src.meshStr;
		materialStr = src.materialStr;
		depthfailStr = src.depthfailStr;
		animationStr = src.animationStr;
		shaderVars = src.shaderVars;
		animationSpeed = src.animationSpeed;
		animations.copy(src.animations);
		skinUpdateNeeded = true;
		updateNeeded = true;
		return *this;
	}

private:
	bool genius = false;				// if true, and camera belongs to same entity as model, model is not rendered
	String meshStr;						// mesh filename
	String materialStr;					// standard material
	String depthfailStr;				// depth fail material
	String animationStr;				// animation keyframe file
	Mesh::shadervars_t shaderVars;		// colors
	bool broken = false;				// if true, assets were not found and the model won't be drawn

	bool skinUpdateNeeded = false;		// if true, skin will get tossed on next draw call
	SkinCache skincache;				// bone transforms
	Map<AnimationState> animations;		// animation states
	float animationSpeed = 1.f;			// anim speed factor
	String currentAnimation;			// currently playing animation (deprecated)
	String previousAnimation;			// previously playing animation (deprecated)

	// loads all animations from the current animation manifest
	void loadAnimations();
	void setWeightOnChildren(const aiNode* root, AnimationState& animation, float rate, float weight);
};
