//! @file Light.hpp

#pragma once

#define GLM_FORCE_RADIANS
#include <glm/vec4.hpp>

#include "LinkedList.hpp"
#include "Node.hpp"
#include "Entity.hpp"
#include "ArrayList.hpp"
#include "Model.hpp"
#include "Shadow.hpp"

class World;
class Camera;
class Chunk;

extern Cvar cvar_shadowsEnabled;
extern Cvar cvar_shadowsStaticOnly;

//! An Entity Component that adds a dynamic, shadow-casting light inside of a World.
class Light : public Component {
public:
	//! light volume shape
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

	//! light model
	static const char* meshStr;
	static const char* materialStr;

	virtual type_t		getType() const override { return COMPONENT_LIGHT; }
	const Vector& 		getColor() const { return color; }
	const float&		getIntensity() const { return intensity; }
	const float&		getRadius() const { return radius; }
	ArrayList<Chunk*>&	getChunksLit() { return chunksLit; }
	const shape_t&		getShape() const { return shape; }
	const bool			isChosen() const { return chosen; }
	const float			getArc() const { return arc; }
	const bool			isShadow() const { return shadow; }
	Uint32				getShadowTicks() const { return shadowTicks; }
	Shadow&				getShadowMap() { return shadowMap; }

	void	setColor(const Vector& _color) { color = _color; }
	void	setIntensity(const float _intensity) { intensity = _intensity; }
	void	setRadius(const float _radius) { radius = _radius; updateNeeded = true; }
	void	setChosen(const bool _chosen) { chosen = _chosen; }
	void	setShape(const shape_t& _shape) { shape = _shape; }
	void	setArc(const float _arc) { arc = _arc; }
	void	setShadow(const bool _shadow) { shadow = _shadow; }

	//! updates matrices
	virtual void update() override;

	//! draws the light as a bounded cube (generally for editing purposes)
	//! @param camera the camera to draw the light from
	//! @param light the light to light the light with (whew) (unused)
	virtual void draw(Camera& camera, const ArrayList<Light*>& lights) override;

	//! creates a shadow map from the light source
	void createShadowMap();

	//! deletes the shadow map
	void deleteShadowMap();

	//! load the component from a file
	//! @param fp the file to read from
	virtual void load(FILE* fp) override;

	//! save/load this object to a file
	//! @param file interface to serialize with
	virtual void serialize(FileInterface * file) override;

	Light& operator=(const Light& src) {
		color = src.color;
		intensity = src.intensity;
		radius = src.radius;
		shape = src.shape;
		arc = src.arc;
		shadow = src.shadow;
		updateNeeded = true;
		return *this;
	}

protected:
	ArrayList<Chunk*> chunksLit;
	Shadow shadowMap;
	bool shadowMapDrawn = false;
	Uint32 shadowTicks = 0;
	Uint32 lastUpdate = 0;
	bool chosen = false;

	Vector color = Vector(1.f);
	float intensity = 1.f;
	float radius = 512.f;
	float arc = 70.f;
	bool shadow = true;
	shape_t shape = SHAPE_SPHERE;
};