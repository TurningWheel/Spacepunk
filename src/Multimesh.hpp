//! @file Multimesh.hpp

#pragma once

#include "Main.hpp"
#include "Component.hpp"
#include "String.hpp"

//! A Multimesh is a unique kind of Entity Component that allows you to combine multiple meshes into a single draw call.
//! This allows you to build a room for instance out of multiple small meshes that get optimized for the engine.
class Multimesh : public Component {
public:
	Multimesh() = delete;
	Multimesh(Entity& _entity, Component* _parent);
	Multimesh(const Multimesh&) = delete;
	Multimesh(Multimesh&&) = delete;
	virtual ~Multimesh();

	//! draws the component
	//! @param camera the camera through which to draw the component
	//! @param light the light by which the component should be illuminated (or nullptr for no illumination)
	virtual void draw(Camera& camera, const ArrayList<Light*>& lights) override;

	//! called just after the parent is inserted into a new world
	//! @param world the world we have been placed into, if any
	virtual void afterWorldInsertion(const World* world) override;

	//! updates matrices
	virtual void update() override;

	//! save/load this object to a file
	//! @param file interface to serialize with
	virtual void serialize(FileInterface * file) override;

	virtual type_t					getType() const override { return COMPONENT_MULTIMESH; }
	const char*						getMaterial() const { return materialStr.get(); }
	const char*						getDepthFailMat() const { return depthfailStr.get(); }
	const Mesh::shadervars_t&		getShaderVars() const { return shaderVars; }

	void	setMaterial(const char* _material) { materialStr = _material; }
	void	setDepthFailMat(const char* _depthfailmat) { depthfailStr = _depthfailmat; }
	void	setShaderVars(const Mesh::shadervars_t& _shaderVars) { shaderVars = _shaderVars; }

	Multimesh& operator=(const Multimesh& src) {
		materialStr = src.materialStr;
		depthfailStr = src.depthfailStr;
		shaderVars = src.shaderVars;
		updateNeeded = true;
		return *this;
	}

	Multimesh& operator=(Multimesh&&) = delete;

private:
	String materialStr;					//! standard material
	String depthfailStr;				//! depth fail material
	Mesh::shadervars_t shaderVars;		//! colors

	String meshStr;						//! composite mesh name in cache

	//! generate a name for the mesh in the mesh cache
	//! @return a string name for the mesh
	String generateGUID();
};