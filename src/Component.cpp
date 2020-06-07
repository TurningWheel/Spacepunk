// Component.cpp

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Main.hpp"
#include "Engine.hpp"
#include "Component.hpp"
#include "Entity.hpp"
#include "Frame.hpp"
#include "Button.hpp"
#include "Field.hpp"

//Component headers.
#include "BBox.hpp"
#include "Model.hpp"
#include "Light.hpp"
#include "Camera.hpp"
#include "Speaker.hpp"
#include "Character.hpp"
#include "Multimesh.hpp"

Component::Attribute::Attribute(const char* _label) {
	label = _label;
}

Component::AttributeBool::AttributeBool(const char * _label, bool & _value) :
	Attribute(_label),
	value(_value)
{}

void Component::AttributeBool::createAttributeUI(Frame& properties, int x, int& y, int width) const {
	static const int border = 3;

	Button* button = properties.addButton("");
	button->setBorder(1);
	button->setIcon("images/gui/checkmark.png");
	button->setStyle(Button::STYLE_CHECKBOX);
	button->setPressed(value);
	button->setCallback(new Callback(value));

	Rect<int> size;
	size.x = border * 2 + x; size.w = 30;
	size.y = y; size.h = 30;
	button->setSize(size);

	// label
	{
		Field* label = properties.addField(this->label.get(), this->label.length() + 1);

		Rect<int> size;
		size.x = border * 2 + 30 + border + x;
		size.w = width - border * 4 - 30 - border - x;
		size.y = y + 5;
		size.h = 30;
		label->setSize(size);
		label->setText(this->label.get());
	}

	y += size.h + border;
}

Component::AttributeInt::AttributeInt(const char * _label, int & _value) :
	Attribute(_label),
	value(_value)
{}

void Component::AttributeInt::createAttributeUI(Frame& properties, int x, int& y, int width) const {
	Frame* frame = properties.addFrame("");

	static const int border = 3;

	Rect<int> size;
	size.x = 0;
	size.w = width / 3 - border * 2 - x;
	size.y = 0;
	size.h = 30;
	frame->setActualSize(size);
	size.x = x + border * 2;
	size.w = width / 3 - border * 2 - x;
	size.y = y;
	size.h = 30;
	frame->setSize(size);
	frame->setColor(WideVector(.25, .25, .25, 1.0));
	frame->setHigh(false);

	Field* field = frame->addField("field", 9);
	size.x = border; size.w = frame->getSize().w - border * 2;
	size.y = border; size.h = frame->getSize().h - border * 2;
	field->setSize(size);
	field->setEditable(true);
	field->setNumbersOnly(true);
	field->setJustify(Field::RIGHT);
	field->setColor(WideVector(1.f, 1.f, 1.f, 1.f));
	field->setCallback(new Callback(value));

	char i[16];
	snprintf(i, 16, "%d", value);
	field->setText(i);

	// label
	{
		Field* label = properties.addField(this->label.get(), this->label.length() + 1);

		Rect<int> size;
		size.x = x + border + width / 3;
		size.w = width - border * 4 - 30 - border - x;
		size.y = y + 5;
		size.h = 30;
		label->setSize(size);
		label->setText(this->label.get());
	}

	y += size.h + border * 3;
}

Component::AttributeFloat::AttributeFloat(const char * _label, float & _value) :
	Attribute(_label),
	value(_value)
{}

void Component::AttributeFloat::createAttributeUI(Frame& properties, int x, int& y, int width) const {
	Frame* frame = properties.addFrame("");

	static const int border = 3;

	Rect<int> size;
	size.x = 0;
	size.w = width / 3 - border * 2 - x;
	size.y = 0;
	size.h = 30;
	frame->setActualSize(size);
	size.x = x + border * 2;
	size.w = width / 3 - border * 2 - x;
	size.y = y;
	size.h = 30;
	frame->setSize(size);
	frame->setColor(WideVector(.25, .25, .25, 1.0));
	frame->setHigh(false);

	Field* field = frame->addField("field", 9);
	size.x = border; size.w = frame->getSize().w - border * 2;
	size.y = border; size.h = frame->getSize().h - border * 2;
	field->setSize(size);
	field->setEditable(true);
	field->setNumbersOnly(true);
	field->setJustify(Field::RIGHT);
	field->setColor(WideVector(1.f, 1.f, 1.f, 1.f));
	field->setCallback(new Callback(value));

	char f[16];
	snprintf(f, 16, "%.2f", value);
	field->setText(f);

	// label
	{
		Field* label = properties.addField(this->label.get(), this->label.length() + 1);

		Rect<int> size;
		size.x = x + border + width / 3;
		size.w = width - border * 4 - 30 - border - x;
		size.y = y + 5;
		size.h = 30;
		label->setSize(size);
		label->setText(this->label.get());
	}

	y += size.h + border * 3;
}

Component::AttributeString::AttributeString(const char * _label, String & _value) :
	Attribute(_label),
	value(_value)
{}

void Component::AttributeString::createAttributeUI(Frame& properties, int x, int& y, int width) const {
	static const int border = 3;

	// label
	{
		Field* label = properties.addField(this->label.get(), this->label.length() + 1);

		Rect<int> size;
		size.x = border * 2 + x;
		size.w = width - border * 4 - x;
		size.y = y;
		size.h = 20;
		label->setSize(size);
		label->setText(this->label.get());

		y += size.h + border;
	}

	// string field
	{
		Frame* frame = properties.addFrame("");

		Rect<int> size;
		size.x = 0; size.w = width - border * 4 - x;
		size.y = 0; size.h = 30;
		frame->setActualSize(size);
		size.x = x + border * 2; size.w = width - border * 4 - x;
		size.y = y; size.h = 30;
		frame->setSize(size);
		frame->setColor(WideVector(.25, .25, .25, 1.0));
		frame->setHigh(false);

		Field* field = frame->addField("field", 128);
		size.x = border; size.w = frame->getSize().w - border * 2;
		size.y = border; size.h = frame->getSize().h - border * 2;
		field->setSize(size);
		field->setEditable(true);

		field->setText(value.get());
		field->setCallback(new Callback(value));

		y += size.h + border;
	}

	y += border;
}

Component::AttributeVector::AttributeVector(const char * _label, Vector & _value) :
	Attribute(_label),
	value(_value)
{}

void Component::AttributeVector::createAttributeUI(Frame& properties, int x, int& y, int width) const {
	static const int border = 3;

	// label
	{
		Field* label = properties.addField(this->label.get(), this->label.length() + 1);

		Rect<int> size;
		size.x = x + border * 2;
		size.w = width - border * 4 - x;
		size.y = y;
		size.h = 20;
		label->setSize(size);
		label->setText(this->label.get());

		y += size.h + border;
	}

	// dimensions
	for (int dim = 0; dim < 3; ++dim) {
		Frame* frame = properties.addFrame("");

		Rect<int> size;
		size.x = 0;
		size.w = (width - x) / 3 - border * 2;
		size.y = 0;
		size.h = 30;
		frame->setActualSize(size);
		size.x = x + border * 2 + ((width - x) / 3 - border) * dim;
		size.w = (width - x) / 3 - border * 2;
		size.y = y;
		size.h = 30;
		frame->setSize(size);
		frame->setColor(WideVector(.25, .25, .25, 1.0));
		frame->setHigh(false);

		Field* field = frame->addField("field", 9);
		size.x = border; size.w = frame->getSize().w - border * 2;
		size.y = border; size.h = frame->getSize().h - border * 2;
		field->setSize(size);
		field->setEditable(true);
		field->setNumbersOnly(true);
		field->setJustify(Field::RIGHT);

		char f[16];
		switch (dim) {
		case 0:
			field->setColor(WideVector(1.f, .2f, .2f, 1.f));
			snprintf(f, 16, "%.2f", value.x);
			break;
		case 1:
			field->setColor(WideVector(.2f, 1.f, .2f, 1.f));
			snprintf(f, 16, "%.2f", value.y);
			break;
		default:
			field->setColor(WideVector(.2f, .2f, 1.f, 1.f));
			snprintf(f, 16, "%.2f", value.z);
			break;
		}
		field->setCallback(new Callback(value));
		field->getParams().addInt(dim);
		field->setText(f);
	}

	y += 30 + border;
}

Component::AttributeColor::AttributeColor(const char* _label, ArrayList<GLfloat>& _value) :
	Attribute(_label),
	value(_value)
{}

void Component::AttributeColor::createAttributeUI(Frame& properties, int x, int& y, int width) const {
	static const int border = 3;

	// label
	{
		Field* label = properties.addField(this->label.get(), this->label.length() + 1);

		Rect<int> size;
		size.x = x + border * 2;
		size.w = width - border * 4 - x;
		size.y = y;
		size.h = 20;
		label->setSize(size);
		label->setText(this->label.get());

		y += size.h + border;
	}

	// color channels
	for (int color = 0; color < 3; ++color) {
		Frame* frame = properties.addFrame("");

		Rect<int> size;
		size.x = 0;
		size.w = (width - x) / 3 - border * 2;
		size.y = 0;
		size.h = 30;
		frame->setActualSize(size);
		size.x = x + border * 2 + ((width - x) / 3 - border) * color;
		size.w = (width - x) / 3 - border * 2;
		size.y = y;
		size.h = 30;
		frame->setSize(size);
		frame->setColor(WideVector(.25, .25, .25, 1.0));
		frame->setHigh(false);

		Field* field = frame->addField("field", 9);
		size.x = border; size.w = frame->getSize().w - border * 2;
		size.y = border; size.h = frame->getSize().h - border * 2;
		field->setSize(size);
		field->setEditable(true);
		field->setNumbersOnly(true);
		field->setJustify(Field::RIGHT);

		switch (color) {
		case 0:
			field->setColor(WideVector(1.f, .2f, .2f, 1.f));
			break;
		case 1:
			field->setColor(WideVector(.2f, 1.f, .2f, 1.f));
			break;
		default:
			field->setColor(WideVector(.2f, .2f, 1.f, 1.f));
			break;
		}
		field->setCallback(new Callback(value));
		field->getParams().addInt(color);

		char f[16];
		snprintf(f, 16, "%.2f", value[color]);
		field->setText(f);
	}

	y += 30 + border;
}

Component::AttributeFile::AttributeFile(const char* _label, const char* _extensions, String& _value) :
	Attribute(_label),
	extensions(_extensions),
	value(_value)
{}

void Component::AttributeFile::createAttributeUI(Frame& properties, int x, int& y, int width) const {
	static const int border = 3;

	// label
	{
		Field* label = properties.addField(this->label.get(), this->label.length() + 1);

		Rect<int> size;
		size.x = border * 2 + x;
		size.w = width - border * 4 - x;
		size.y = y;
		size.h = 20;
		label->setSize(size);
		label->setText(this->label.get());

		y += size.h + border;
	}

	// string field
	Field* field = nullptr;
	{
		Frame* frame = properties.addFrame("");

		Rect<int> size;
		size.x = 0; size.w = width - border * 4 - x - 30 - border;
		size.y = 0; size.h = 30;
		frame->setActualSize(size);
		size.x = x + border * 2; size.w = width - border * 4 - x - 30 - border;
		size.y = y; size.h = 30;
		frame->setSize(size);
		frame->setColor(WideVector(.25, .25, .25, 1.0));
		frame->setHigh(false);

		field = frame->addField("field", 128);
		size.x = border; size.w = frame->getSize().w - border * 2;
		size.y = border; size.h = frame->getSize().h - border * 2;
		field->setSize(size);
		field->setEditable(true);

		field->setText(value.get());
		field->setCallback(new FieldCallback(value));
	}

	// button
	{
		Button* button = properties.addButton("");
		button->setBorder(1);
		button->setIcon("images/gui/open.png");
		button->setStyle(Button::STYLE_NORMAL);
		button->setCallback(new ButtonCallback(value, extensions, field));

		Rect<int> size;
		size.x = border * 2 + x + (width - border * 4 - x - 30); size.w = 30;
		size.y = y; size.h = 30;
		button->setSize(size);

		y += size.h + border;
	}

	y += border;
}

int Component::AttributeFile::ButtonCallback::operator()(Script::Args& args) const {
	String result = mainEngine->fileOpenDialog(extensions, nullptr);
	if (!result.empty()) {
		value = mainEngine->shortenPath(result.get());
		field->setText(value.get());
		return 0;
	} else {
		return 1;
	}
}

const char* Component::typeStr[COMPONENT_MAX] = {
	"basic",
	"bbox",
	"model",
	"light",
	"camera",
	"speaker",
	"emitter",
	"character",
	"multimesh"
};

const char* Component::typeIcon[COMPONENT_MAX] = {
	"images/gui/component.png",
	"images/gui/bbox.png",
	"images/gui/mesh.png",
	"images/gui/light.png",
	"images/gui/camera.png",
	"images/gui/speaker.png",
	"images/gui/emitter.png",
	"images/gui/character.png",
	"images/gui/mesh.png"
};

Component::Component(Entity& _entity, Component* _parent) {
	entity = &_entity;
	parent = _parent;

	uid = entity->getNewComponentID();

	name = typeStr[COMPONENT_BASIC];
	lPos = Vector(0.f, 0.f, 0.f);
	lAng = Quaternion(0.f, 0.f, 0.f, 1.f);
	lScale = Vector(1.f, 1.f, 1.f);
	lMat = glm::mat4(1.f);

	update();
}

Component::~Component() {
	for (Uint32 c = 0; c < components.getSize(); ++c) {
		if (components[c]) {
			delete components[c];
			components[c] = nullptr;
		}
	}
	components.clear();

	// delete attributes
	for (auto attribute : attributes) {
		if (attribute) {
			delete attribute;
			attribute = nullptr;
		}
	}
}

void Component::process() {
	if (boundToBone) {
		boneModel->updateSkin();
		glm::mat4 mat = boneModel->findBone(boneIndex);
		setLocalMat(mat);
		rotate(boneRotate);
		translate(boneTranslate);
		scale(boneScale);
	}
	if (updateNeeded) {
		update();
	}

	for (Uint32 c = 0; c < components.getSize(); ++c) {
		components[c]->process();
	}
}

void Component::beforeWorldInsertion(const World* world) {
	for (Uint32 c = 0; c < components.getSize(); ++c) {
		components[c]->beforeWorldInsertion(world);
	}
}

void Component::afterWorldInsertion(const World* world) {
	for (Uint32 c = 0; c < components.getSize(); ++c) {
		components[c]->afterWorldInsertion(world);
	}
}

bool Component::checkCollision() const {
	for (Uint32 c = 0; c < components.getSize(); ++c) {
		if (components[c]->checkCollision()) {
			return true;
		}
	}
	return false;
}

void Component::draw(Camera& camera, const ArrayList<Light*>& lights) {
	for (Uint32 c = 0; c < components.getSize(); ++c) {
		components[c]->draw(camera, lights);
	}
}

bool Component::hasComponent(type_t type) const {
	for (Uint32 c = 0; c < components.getSize(); ++c) {
		if (components[c]->isEditorOnly()) {
			continue;
		}
		if (components[c]->getType() == type) {
			return true;
		}
		if (components[c]->hasComponent(type)) {
			return true;
		}
	}
	return false;
}

void Component::rotate(const Rotation& ang) {
	if (updateNeeded) {
		update();
	}
	lAng = lAng * Quaternion(ang);
	updateNeeded = true;
}

void Component::translate(const Vector& vec) {
	if (updateNeeded) {
		update();
	}
	lMat = glm::translate(lMat, glm::vec3(vec.x, -vec.z, vec.y));
	lPos = Vector(lMat[3][0], lMat[3][2], -lMat[3][1]);
	lScale = Vector(glm::length(lMat[0]), glm::length(lMat[2]), glm::length(lMat[1]));
	lAng = Quaternion(lMat);
	updateNeeded = true;
}

void Component::scale(const Vector& vec) {
	if (updateNeeded) {
		update();
	}
	lMat = glm::scale(lMat, glm::vec3(vec.x, vec.z, vec.y));
	lPos = Vector(lMat[3][0], lMat[3][2], -lMat[3][1]);
	lScale = Vector(glm::length(lMat[0]), glm::length(lMat[2]), glm::length(lMat[1]));
	lAng = Quaternion(lMat);
	updateNeeded = true;
}

void Component::revertRotation() {
	lAng = Quaternion(0.f, 0.f, 0.f, 1.f);
	updateNeeded = true;
}

void Component::revertTranslation() {
	lPos = Vector(0.f, 0.f, 0.f);
	updateNeeded = true;
}

void Component::revertScale() {
	lScale = Vector(1.f, 1.f, 1.f);
	updateNeeded = true;
}

void Component::revertToIdentity() {
	revertTranslation();
	revertRotation();
	revertScale();
}

void Component::update() {
	glm::mat4 translationM = glm::translate(glm::mat4(1.f), glm::vec3(lPos.x, -lPos.z, lPos.y));
	glm::mat4 rotationM(glm::quat(lAng.w, lAng.x, lAng.y, lAng.z));
	glm::mat4 scaleM = glm::scale(glm::mat4(1.f), glm::vec3(lScale.x, lScale.z, lScale.y));
	lMat = translationM * rotationM * scaleM;

	if (parent) {
		gMat = parent->getGlobalMat() * lMat;
		gAng = parent->getGlobalAng() * lAng;
	} else {
		gMat = entity->getMat() * lMat;
		gAng = entity->getAng() * lAng;
	}
	gPos = Vector(gMat[3][0], gMat[3][2], -gMat[3][1]);
	gScale = Vector(glm::length(gMat[0]), glm::length(gMat[2]), glm::length(gMat[1]));

	for (Uint32 c = 0; c < components.getSize(); ++c) {
		if (components[c]->isToBeDeleted()) {
			delete components[c];
			components.remove(c);
			--c;
		} else {
			components[c]->update();
		}
	}

	if (updateNeeded && entity->getWorld()) {
		entity->updateBounds();
		updateNeeded = false;
	}
}

void Component::updateBounds() {
	if (!entity->getWorld()) {
		return;
	}
	boundsMax = gPos - entity->getPos();
	boundsMin = gPos - entity->getPos();
	for (Uint32 c = 0; c < components.getSize(); ++c) {
		if (!components[c]->editorOnly && !components[c]->toBeDeleted) {
			components[c]->updateBounds();
			boundsMax.x = std::max(boundsMax.x, components[c]->getBoundsMax().x);
			boundsMax.y = std::max(boundsMax.y, components[c]->getBoundsMax().y);
			boundsMax.z = std::max(boundsMax.z, components[c]->getBoundsMax().z);
			boundsMin.x = std::min(boundsMin.x, components[c]->getBoundsMin().x);
			boundsMin.y = std::min(boundsMin.y, components[c]->getBoundsMin().y);
			boundsMin.z = std::min(boundsMin.z, components[c]->getBoundsMin().z);
		}
	}
}

void Component::copy(Component* dest) {
	if (dest == nullptr)
	{
		return;
	}
	Component* component = nullptr;
	switch (getType()) {
	case Component::COMPONENT_BASIC:
	{
		component = dest->addComponent<Component>();
		break;
	}
	case Component::COMPONENT_BBOX:
	{
		component = dest->addComponent<BBox>();
		BBox* bbox0 = static_cast<BBox*>(this);
		BBox* bbox1 = static_cast<BBox*>(component);
		*bbox1 = *bbox0;
		break;
	}
	case Component::COMPONENT_MODEL:
	{
		component = dest->addComponent<Model>();
		Model* model0 = static_cast<Model*>(this);
		Model* model1 = static_cast<Model*>(component);
		*model1 = *model0;
		break;
	}
	case Component::COMPONENT_LIGHT:
	{
		component = dest->addComponent<Light>();
		Light* light0 = static_cast<Light*>(this);
		Light* light1 = static_cast<Light*>(component);
		*light1 = *light0;
		break;
	}
	case Component::COMPONENT_CAMERA:
	{
		component = dest->addComponent<Camera>();
		Camera* camera0 = static_cast<Camera*>(this);
		Camera* camera1 = static_cast<Camera*>(component);
		*camera1 = *camera0;
		break;
	}
	case Component::COMPONENT_SPEAKER:
	{
		component = dest->addComponent<Speaker>();
		Speaker* speaker0 = static_cast<Speaker*>(this);
		Speaker* speaker1 = static_cast<Speaker*>(component);
		*speaker1 = *speaker0;
		break;
	}
	case Component::COMPONENT_CHARACTER:
	{
		component = dest->addComponent<Character>();
		Character* character0 = static_cast<Character*>(this);
		Character* character1 = static_cast<Character*>(component);
		*character1 = *character0;
		break;
	}
	case Component::COMPONENT_MULTIMESH:
	{
		component = dest->addComponent<Multimesh>();
		Multimesh* mm0 = static_cast<Multimesh*>(this);
		Multimesh* mm1 = static_cast<Multimesh*>(component);
		*mm1 = *mm0;
		break;
	}
	default:
	{
		break;
	}
	}
	if (!component) {
		mainEngine->fmsg(Engine::MSG_ERROR, "failed to copy component");
	} else {
		*component = *this;
		copyComponents(*component);
		mainEngine->fmsg(Engine::MSG_DEBUG, "copied %s component", Component::typeStr[(int)getType()]);
	}
}

void Component::copy(Entity* dest) {
	if (dest == nullptr)
	{
		return;
	}
	Component* component = nullptr;
	switch (getType()) {
	case Component::COMPONENT_BASIC:
	{
		component = dest->addComponent<Component>();
		break;
	}
	case Component::COMPONENT_BBOX:
	{
		component = dest->addComponent<BBox>();
		BBox* bbox0 = static_cast<BBox*>(this);
		BBox* bbox1 = static_cast<BBox*>(component);
		*bbox1 = *bbox0;
		break;
	}
	case Component::COMPONENT_MODEL:
	{
		component = dest->addComponent<Model>();
		Model* model0 = static_cast<Model*>(this);
		Model* model1 = static_cast<Model*>(component);
		*model1 = *model0;
		break;
	}
	case Component::COMPONENT_LIGHT:
	{
		component = dest->addComponent<Light>();
		Light* light0 = static_cast<Light*>(this);
		Light* light1 = static_cast<Light*>(component);
		*light1 = *light0;
		break;
	}
	case Component::COMPONENT_CAMERA:
	{
		component = dest->addComponent<Camera>();
		Camera* camera0 = static_cast<Camera*>(this);
		Camera* camera1 = static_cast<Camera*>(component);
		*camera1 = *camera0;
		break;
	}
	case Component::COMPONENT_SPEAKER:
	{
		component = dest->addComponent<Speaker>();
		Speaker* speaker0 = static_cast<Speaker*>(this);
		Speaker* speaker1 = static_cast<Speaker*>(component);
		*speaker1 = *speaker0;
		break;
	}
	case Component::COMPONENT_CHARACTER:
	{
		component = dest->addComponent<Character>();
		Character* character0 = static_cast<Character*>(this);
		Character* character1 = static_cast<Character*>(component);
		*character1 = *character0;
		break;
	}
	case Component::COMPONENT_MULTIMESH:
	{
		component = dest->addComponent<Multimesh>();
		Multimesh* mm0 = static_cast<Multimesh*>(this);
		Multimesh* mm1 = static_cast<Multimesh*>(component);
		*mm1 = *mm0;
		break;
	}
	default:
	{
		break;
	}
	}
	if (!component) {
		mainEngine->fmsg(Engine::MSG_ERROR, "failed to copy component");
	} else {
		*component = *this;
		copyComponents(*component);
		mainEngine->fmsg(Engine::MSG_DEBUG, "copied %s component", Component::typeStr[(int)getType()]);
	}
}

void Component::copyComponents(Component& dest) {
	for (Uint32 c = 0; c < components.getSize(); ++c) {
		components[c]->copy(&dest);
	}
}

bool Component::removeComponentByName(const char* name) {
	Component* component = findComponentByName<Component>(name);
	if (component) {
		component->remove();
		return true;
	} else {
		return false;
	}
}

bool Component::removeComponentByUID(const Uint32 uid) {
	Component* component = findComponentByUID<Component>(uid);
	if (component) {
		component->remove();
		return true;
	} else {
		return false;
	}
}

void Component::remove() {
	entity->updateBounds();
	toBeDeleted = true;
}

void Component::load(FILE* fp) {
	// DEPRECATED
	return;
}

void Component::loadSubComponents(FILE* fp) {
	Uint32 numComponents = 0;
	Engine::freadl(&numComponents, sizeof(Uint32), 1, fp, nullptr, "Component::loadSubComponents()");
	for (Uint32 c = 0; c < numComponents; ++c) {
		Component::type_t type = Component::type_t::COMPONENT_BASIC;
		Engine::freadl(&type, sizeof(Component::type_t), 1, fp, nullptr, "Component::loadSubComponents()");

		Component* component = addComponent(type);
		if (!component) {
			mainEngine->fmsg(Engine::MSG_ERROR, "failed to load component for entity '%s'", entity->getName().get());
		} else {
			component->load(fp);
		}
	}
}

Component* Component::addComponent(Component::type_t type) {
	Component* component = nullptr;
	switch (type) {
	case Component::COMPONENT_BASIC:
		return addComponent<Component>();
	case Component::COMPONENT_BBOX:
		return addComponent<BBox>();
	case Component::COMPONENT_MODEL:
		return addComponent<Model>();
	case Component::COMPONENT_LIGHT:
		return addComponent<Light>();
	case Component::COMPONENT_CAMERA:
		return addComponent<Camera>();
	case Component::COMPONENT_SPEAKER:
		return addComponent<Speaker>();
	case Component::COMPONENT_CHARACTER:
		return addComponent<Character>();
	case Component::COMPONENT_MULTIMESH:
		return addComponent<Multimesh>();
	default:
		mainEngine->fmsg(Engine::MSG_ERROR, "addComponent: Unknown component type %u", (Uint32)type);
		return nullptr;
	}
}

void Component::serialize(FileInterface * file) {
	Uint32 version = 1;
	file->property("Component::version", version);
	file->property("name", name);
	file->property("lPos", lPos);
	if (version == 0) {
		Rotation rotation;
		file->property("lAng", rotation);
		lAng = Quaternion(rotation);
	} else {
		file->property("lAng", lAng);
	}
	file->property("lScale", lScale);
	serializeComponents(file);
}

void Component::serializeComponents(FileInterface* file) {
	if (file->isReading()) {
		Uint32 componentCount = 0;
		file->propertyName("components");
		file->beginArray(componentCount);
		for (Uint32 index = 0; index < componentCount; ++index) {
			file->beginObject();

			Component::type_t type = Component::type_t::COMPONENT_BASIC;
			file->property("type", type);

			Component* newComponent = addComponent(type);
			file->property("data", *newComponent);

			file->endObject();
		}
		file->endArray();
	} else {
		Uint32 componentCount = 0;
		for (Uint32 index = 0; index < components.getSize(); ++index) {
			if (components[index]->isEditorOnly()) {
				continue;
			}
			++componentCount;
		}

		file->propertyName("components");
		file->beginArray(componentCount);
		for (Uint32 index = 0; index < components.getSize(); ++index) {
			if (components[index]->isEditorOnly()) {
				continue;
			}

			file->beginObject();

			Component::type_t type = components[index]->getType();
			file->property("type", type);

			file->property("data", *components[index]);

			file->endObject();
		}
		file->endArray();
	}
}

void Component::shootLaser(const glm::mat4& mat, WideVector& color, float size, float life) {
	Vector start = Vector(mat[3].x, mat[3].z, -mat[3].y);
	//glm::mat4 endMat = glm::translate(mat, glm::vec3(-1024.f, 0.f, 0.f));
	Quaternion q = entity->getAng();
	q = q.rotate(entity->getLookDir());
	Vector end = start + q.toVector() * 10000.f;
	World* world = entity->getWorld();
	World::hit_t hit = world->lineTrace(start, end);
	end = hit.pos;
	if (hit.manifest) {
		if (hit.manifest->entity) {
			Entity* hitEntity = hit.manifest->entity;
			if (hitEntity) {
				hitEntity->applyForce((hit.pos - entity->getPos()).normal() * 1000.f, hit.pos);
			}
		}
	}
	world->addLaser(start, end, color, size, life);
}

void Component::setLocalMat(const glm::mat4& mat) {
	lMat = mat;
	lPos = Vector(mat[3][0], mat[3][2], -mat[3][1]);
	lScale = Vector(glm::length(mat[0]), glm::length(mat[2]), glm::length(mat[1]));
	lAng = Quaternion(mat);
	updateNeeded = true;
}

void Component::bindToBone(Model* model, const char* bone, const Vector& translation, const Rotation& rotation, const Vector& scale) {
	if (!model || !bone) {
		unbindFromBone();
		return;
	}
	boundToBone = true;
	boneModel = model;
	boneName = bone;
	boneIndex = model->findBoneIndex(bone);
	boneTranslate = translation;
	boneRotate = rotation;
	boneScale = scale;
}

void Component::unbindFromBone() {
	boundToBone = false;
}