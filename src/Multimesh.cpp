// Multimesh.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "Multimesh.hpp"

Multimesh::Multimesh(Entity& _entity, Component* _parent) :
	Component(_entity, _parent) {
	name = typeStr[COMPONENT_MULTIMESH];
	// todo
}

Multimesh::~Multimesh() {
	// todo
}

void Multimesh::draw(Camera& camera, const ArrayList<Light*>& lights) {
	// todo
}

void Multimesh::process() {
	Component::process();
	// todo
}

void Multimesh::serialize(FileInterface * file) {
	Component::serialize(file);
	Uint32 version = 0;
	file->property("Multimesh::version", version);
	// todo
}