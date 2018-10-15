// Model.hpp

// A component represented by meshes that supports animation.

#pragma once

#define GLM_FORCE_RADIANS
#include <glm/vec4.hpp>

#ifdef PLATFORM_LINUX
#include <btBulletDynamicsCommon.h>
#else
#include <bullet3/btBulletDynamicsCommon.h>
#endif

#include "ArrayList.hpp"
#include "LinkedList.hpp"
#include "Node.hpp"
#include "Component.hpp"
#include "Light.hpp"
#include "String.hpp"
#include "Mesh.hpp"
#include "Material.hpp"

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

	typedef ArrayList<Mesh::animframes_t,0,1> AnimationList;
	typedef ArrayList<Mesh::skincache_t,0> SkinCache;

	Model(Entity& _entity, Component* _parent);
	virtual ~Model();

	// max animations that a model can play
	static const int maxAnimations;

	// default mesh
	static const char* defaultMesh;

	// draws the component
	// @param camera: the camera through which to draw the component
	// @param light: the light by which the component should be illuminated (or nullptr for no illumination)
	virtual void draw(Camera& camera, Light* light) override;

	// update the component
	virtual void process() override;

	// plays an animation
	// @param name: the name of the animation to play
	// @param blend: if true, blends from the old animation to the new
	// @param loop: if true, the animation will loop; otherwise it will stop at the last frame
	void animate(const char* name, bool blend, bool loop);

	// finds a bone with the given name
	// @param name: the name of the bone to search for
	// @return a struct containing bone position, orientation, etc.
	bone_t findBone(const char* name) const;

	// @return true if the model has any animation keyfile assigned
	bool hasAnimations() const;

	// load the component from a file
	// @param fp: the file to read from
	virtual void load(FILE* fp) override;

	// save/load this object to a file
	// @param file interface to serialize with
	virtual void serialize(FileInterface * file) override;

	// get the name of the currently playing animation
	const char* getAnimName() const;

	// get the duration of the currently playing animation
	// @return the amount of time the current animation has been playing
	float getAnimTicks() const;

	// determines if the current animation is done
	// @return true if the animation has wrapped, otherwise false
	bool isAnimDone() const;

	// updates skin if necessary
	void updateSkin();

	// getters & setters
	virtual type_t						getType() const override			{ return COMPONENT_MODEL; }
	const char*							getMesh() const						{ return meshStr.get(); }
	const char*							getMaterial() const					{ return materialStr.get(); }
	const char*							getDepthFailMat() const				{ return depthfailStr.get(); }
	const char*							getAnimation() const				{ return animationStr.get(); }
	const Mesh::shadervars_t&			getShaderVars() const				{ return shaderVars; }
	const AnimationList&				getAnimations() const				{ return animations; }
	const SkinCache&					getSkinCache() const				{ return skincache; }
	unsigned int						getNumActiveAnimations() const		{ return (unsigned int)animations.getSize(); }
	float								getAnimationSpeed() const			{ return animationSpeed; }
	bool								isSkinUpdateNeeded() const			{ return skinUpdateNeeded; }

	void	setMesh(const char* _mesh)										{ meshStr = _mesh; updateNeeded = true; }
	void	setMaterial(const char* _material)								{ materialStr = _material; }
	void	setDepthFailMat(const char* _depthfailmat)						{ depthfailStr = _depthfailmat; }
	void	setAnimation(const char* _animation)							{ animationStr = _animation; }
	void	setShaderVars(const Mesh::shadervars_t& _shaderVars)			{ shaderVars = _shaderVars; }
	void	setAnimationSpeed(const float _animationSpeed)					{ animationSpeed = _animationSpeed; }

private:
	bool genius = false;				// if true, and camera belongs to same entity as model, model is not rendered
	String meshStr;						// mesh filename
	String materialStr;					// standard material
	String depthfailStr;				// depth fail material
	String animationStr;				// animation keyframe file
	Mesh::shadervars_t shaderVars;		// colors

	bool skinUpdateNeeded = false;		// if true, skin will get tossed on next draw call
	SkinCache skincache;				// bone transforms
	AnimationList animations;			// animation list
	float animationSpeed = 1.f;			// anim speed factor
};
