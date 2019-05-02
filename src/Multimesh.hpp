// Multimesh.hpp

#pragma once

#include "Main.hpp"
#include "Component.hpp"
#include "String.hpp"

class Multimesh : public Component {
public:
	Multimesh(Entity& _entity, Component* _parent);
	virtual ~Multimesh();

	// draws the component
	// @param camera the camera through which to draw the component
	// @param light the light by which the component should be illuminated (or nullptr for no illumination)
	virtual void draw(Camera& camera, const ArrayList<Light*>& lights) override;

	// update the component
	virtual void process() override;

	// save/load this object to a file
	// @param file interface to serialize with
	virtual void serialize(FileInterface * file) override;

	// getters & setters
	virtual type_t		getType() const override { return COMPONENT_MULTIMESH; }

	Multimesh& operator=(const Multimesh& src) {
		updateNeeded = true;
		return *this;
	}

private:
};