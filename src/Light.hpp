// Light.hpp
// Class for a dynamic, shadow-casting point light inside of a World

#pragma once

#define GLM_FORCE_RADIANS
#include <glm/vec4.hpp>

#include "LinkedList.hpp"
#include "Node.hpp"
#include "Entity.hpp"
#include "ArrayList.hpp"
#include "Model.hpp"

class World;
class Camera;
class Chunk;

class Light : public Component {
public:
	// light volume shape
	enum shape_t {
		SHAPE_SPHERE = 0,
		SHAPE_BOX = 1,
		SHAPE_CAPSULE = 2,
		SHAPE_CYLINDER = 3,
		SHAPE_CONE = 4,
		SHAPE_PYRAMID = 5,
		SHAPE_NUM
	};
	static const char* shapeStr[static_cast<int>(shape_t::SHAPE_NUM)];

	Light(Entity& _entity, Component* _parent);
	virtual ~Light();

	// light model
	static const char* meshStr;
	static const char* materialStr;

	// getters & setters
	virtual type_t		getType() const override	{ return COMPONENT_LIGHT; }
	const Vector& 		getColor() const			{ return color; }
	const float&		getIntensity() const		{ return intensity; }
	const float&		getRadius() const			{ return radius; }
	ArrayList<Chunk*>&	getChunksLit()				{ return chunksLit; }
	ArrayList<Chunk*>&	getChunksShadow()			{ return chunksShadow; }
	const shape_t&		getShape() const			{ return shape; }
	const bool			isChosen() const			{ return chosen; }

	void	setColor(const Vector& _color)			{ color = _color; }
	void	setIntensity(const float _intensity)	{ intensity = _intensity; }
	void	setRadius(const float _radius)			{ radius = _radius; updateNeeded = true; }
	void	setChosen(const bool _chosen)			{ chosen = _chosen; }
	void	setShape(const shape_t& _shape)			{ shape = _shape; }

	// updates matrices
	virtual void update() override;

	// draws the light as a bounded cube (generally for editing purposes)
	// @param camera: the camera to draw the light from
	// @param light: the light to light the light with (whew) (unused)
	virtual void draw(Camera& camera, Light* light) override;

	// load the component from a file
	// @param fp: the file to read from
	virtual void load(FILE* fp) override;

	// save/load this object to a file
	// @param file interface to serialize with
	virtual void serialize(FileInterface * file) override;

protected:
	ArrayList<Chunk*> chunksLit;
	ArrayList<Chunk*> chunksShadow;

	Uint32 lastUpdate = 0;
	bool chosen = false;

	Vector color = Vector(1.f);
	float intensity = 1.f;
	float radius = 512.f;
	shape_t shape = SHAPE_SPHERE;
};