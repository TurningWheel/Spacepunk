// Component.hpp

#pragma once

#include "Main.hpp"
#include "Vector.hpp"
#include "Angle.hpp"
#include "ArrayList.hpp"
#include "String.hpp"
#include "LinkedList.hpp"
#include "Rect.hpp"
#include "WideVector.hpp"
#include "Script.hpp"
#include "Frame.hpp"

class Chunk;
class Entity;
class Camera;
class Light;
class World;
class Field;

class Component {
public:
	// component type
	enum type_t {
		COMPONENT_BASIC,
		COMPONENT_BBOX,
		COMPONENT_MODEL,
		COMPONENT_LIGHT,
		COMPONENT_CAMERA,
		COMPONENT_SPEAKER,
		COMPONENT_EMITTER,
		COMPONENT_CHARACTER,
		COMPONENT_MULTIMESH,
		COMPONENT_MAX
	};
	static const char* typeStr[COMPONENT_MAX];
	static const char* typeIcon[COMPONENT_MAX];

	// An exposed attribute (modifiable in editor)
	class Attribute {
	public:
		Attribute(const char* _label);

		enum class Type {
			TYPE_BOOLEAN,
			TYPE_INTEGER,
			TYPE_FLOAT,
			TYPE_STRING,
			TYPE_VECTOR,
			TYPE_COLOR,
			TYPE_ENUM,
			TYPE_FILE,
			TYPE_MAX
		};

		virtual const Type getType() const = 0;
		const char* getLabel() const { return label.get(); }
		virtual void createAttributeUI(Frame& properties, int x, int& y, int width) const = 0;

	protected:
		String label;
	};

	// bool attribute
	class AttributeBool : public Attribute {
	public:
		AttributeBool(const char* _label, bool& _value);
		virtual ~AttributeBool() {}
		virtual const Type getType() const override { return Type::TYPE_BOOLEAN; }
		virtual void createAttributeUI(Frame& properties, int x, int& y, int width) const override;
		class Callback : public Script::Function {
		public:
			Callback(bool& _value) :
				value(_value)
			{}
			virtual ~Callback() {}
			virtual int operator()(Script::Args& args) const override {
				value = !value;
				return 0;
			}
		private:
			bool& value;
		};

	private:
		bool& value;
	};

	// integer attribute
	class AttributeInt : public Attribute {
	public:
		AttributeInt(const char* _label, int& _value);
		virtual ~AttributeInt() {}
		virtual const Type getType() const override { return Type::TYPE_INTEGER; }
		virtual void createAttributeUI(Frame& properties, int x, int& y, int width) const override;
		class Callback : public Script::Function {
		public:
			Callback(int& _value) :
				value(_value)
			{}
			virtual ~Callback() {}
			virtual int operator()(Script::Args& args) const override {
				if (args.getSize() < 1 || args.getList()[0]->getType() != Script::var_t::TYPE_STRING) {
					return 1;
				}
				value = strtol(static_cast<Script::param_string_t*>(args.getList()[0])->value.get(), nullptr, 10);
				return 0;
			}
		private:
			int& value;
		};

	private:
		int& value;
	};

	// float attribute
	class AttributeFloat : public Attribute {
	public:
		AttributeFloat(const char* _label, float& _value);
		virtual ~AttributeFloat() {}
		virtual const Type getType() const override { return Type::TYPE_FLOAT; }
		virtual void createAttributeUI(Frame& properties, int x, int& y, int width) const override;
		class Callback : public Script::Function {
		public:
			Callback(float& _value) :
				value(_value)
			{}
			virtual ~Callback() {}
			virtual int operator()(Script::Args& args) const override {
				if (args.getSize() < 1 || args.getList()[0]->getType() != Script::var_t::TYPE_STRING) {
					return 1;
				}
				value = strtof(static_cast<Script::param_string_t*>(args.getList()[0])->value.get(), nullptr);
				return 0;
			}
		private:
			float& value;
		};

	private:
		float& value;
	};

	// string attribute
	class AttributeString : public Attribute {
	public:
		AttributeString(const char* _label, String& _value);
		virtual ~AttributeString() {}
		virtual const Type getType() const override { return Type::TYPE_STRING; }
		virtual void createAttributeUI(Frame& properties, int x, int& y, int width) const override;
		class Callback : public Script::Function {
		public:
			Callback(String& _value) :
				value(_value)
			{}
			virtual ~Callback() {}
			virtual int operator()(Script::Args& args) const override {
				if (args.getSize() < 1 || args.getList()[0]->getType() != Script::var_t::TYPE_STRING) {
					return 1;
				}
				value = static_cast<Script::param_string_t*>(args.getList()[0])->value.get();
				return 0;
			}
		private:
			String& value;
		};

	private:
		String& value;
	};

	// vector attribute
	class AttributeVector : public Attribute {
	public:
		AttributeVector(const char* _label, Vector& _value);
		virtual ~AttributeVector() {}
		virtual const Type getType() const override { return Type::TYPE_VECTOR; }
		virtual void createAttributeUI(Frame& properties, int x, int& y, int width) const override;
		class Callback : public Script::Function {
		public:
			Callback(Vector& _value) :
				value(_value)
			{}
			virtual ~Callback() {}
			virtual int operator()(Script::Args& args) const override {
				if (args.getSize() < 2 ||
					args.getList()[0]->getType() != Script::var_t::TYPE_INTEGER ||
					args.getList()[1]->getType() != Script::var_t::TYPE_STRING) {
					return 1;
				}
				int dim = static_cast<Script::param_int_t*>(args.getList()[0])->value;
				switch (dim) {
				case 0:
					value.x = strtof(static_cast<Script::param_string_t*>(args.getList()[1])->value.get(), nullptr);
					break;
				case 1:
					value.y = strtof(static_cast<Script::param_string_t*>(args.getList()[1])->value.get(), nullptr);
					break;
				case 2:
					value.z = strtof(static_cast<Script::param_string_t*>(args.getList()[1])->value.get(), nullptr);
					break;
				default:
					break;
				}
				return 0;
			}
		private:
			Vector& value;
		};

	private:
		Vector& value;
	};

	// color attribute
	class AttributeColor : public Attribute {
	public:
		AttributeColor(const char* _label, ArrayList<GLfloat>& _value);
		virtual ~AttributeColor() {}
		virtual const Type getType() const override { return Type::TYPE_COLOR; }
		virtual void createAttributeUI(Frame& properties, int x, int& y, int width) const override;
		class Callback : public Script::Function {
		public:
			Callback(ArrayList<GLfloat>& _value) :
				value(_value)
			{}
			virtual ~Callback() {}
			virtual int operator()(Script::Args& args) const override {
				if (args.getSize() < 2 ||
					args.getList()[0]->getType() != Script::var_t::TYPE_INTEGER ||
					args.getList()[1]->getType() != Script::var_t::TYPE_STRING) {
					return 1;
				}
				int color = static_cast<Script::param_int_t*>(args.getList()[0])->value;
				value[color] = strtof(static_cast<Script::param_string_t*>(args.getList()[1])->value.get(), nullptr);
				return 0;
			}
		private:
			ArrayList<GLfloat>& value;
		};

	private:
		ArrayList<GLfloat>& value;
	};

	// enum attribute
	template<typename E>
	class AttributeEnum : public Attribute {
	public:
		AttributeEnum(const char* _label, const char** _values, Uint32 _maxValue, E& _value) :
			Attribute(_label),
			value(_value),
			maxValue(_maxValue),
			values(_values)
		{}
		virtual ~AttributeEnum() {}
		virtual const Type getType() const override { return Type::TYPE_ENUM; }
		virtual void createAttributeUI(Frame& properties, int x, int& y, int width) const override {
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

			// box of entries
			{
				Frame* frame = properties.addFrame("");

				Rect<int> size;
				size.x = 0; size.w = width - border * 4 - x;
				size.y = 0; size.h = 150;
				frame->setActualSize(size);
				size.x = x + border * 2; size.w = width - border * 4 - x;
				size.y = y; size.h = 150;
				frame->setSize(size);
				frame->setColor(glm::vec4(.25, .25, .25, 1.0));
				frame->setHigh(false);
				frame->setBorder(0);

				// entry list
				for (Uint32 c = 0; c < maxValue; ++c) {
					Frame::entry_t* entry = frame->addEntry("entry", true);
					entry->text = values[c];
					entry->params.addInt(c);
					entry->click = new Callback(value);
					entry->ctrlClick = new Callback(value);
					entry->color = glm::vec4(1.f);
				}

				y += size.h + border;
			}

			y += border;
		}
		class Callback : public Script::Function {
		public:
			Callback(E& _value) :
				value(_value)
			{}
			virtual ~Callback() {}
			virtual int operator()(Script::Args& args) const override {
				if (args.getSize() < 1 || args.getList()[0]->getType() != Script::var_t::TYPE_INTEGER) {
					return 1;
				}
				int i = static_cast<Script::param_int_t*>(args.getList()[0])->value;
				value = static_cast<E>(i);
				return 0;
			}
		private:
			E& value;
		};

	private:
		const char** values;
		Uint32 maxValue;
		E& value;
	};

	// file attribute
	class AttributeFile : public Attribute {
	public:
		AttributeFile(const char* _label, const char* _extensions, String& _value);
		virtual ~AttributeFile() {}
		virtual const Type getType() const override { return Type::TYPE_FILE; }
		virtual void createAttributeUI(Frame& properties, int x, int& y, int width) const override;
		class FieldCallback : public Script::Function {
		public:
			FieldCallback(String& _value) :
				value(_value)
			{}
			virtual ~FieldCallback() {}
			virtual int operator()(Script::Args& args) const override {
				if (args.getSize() < 1 || args.getList()[0]->getType() != Script::var_t::TYPE_STRING) {
					return 1;
				}
				value = static_cast<Script::param_string_t*>(args.getList()[0])->value.get();
				return 0;
			}
		private:
			String& value;
		};
		class ButtonCallback : public Script::Function {
		public:
			ButtonCallback(String& _value, const char* _extensions, Field* _field) :
				value(_value),
				extensions(_extensions),
				field(_field)
			{}
			virtual ~ButtonCallback() {}
			virtual int operator()(Script::Args& args) const override;
		private:
			String& value;
			const char* extensions = nullptr;
			Field* field = nullptr;
		};

	private:
		String& value;
		const char* extensions = nullptr;
	};

	// bbox rect
	struct bboxrect_t {
		Rect<Sint32> size;
		Entity* entity;
	};

	static const unsigned int nuid = UINT32_MAX;

	Component(Entity& _entity, Component* _parent);
	virtual ~Component();

	// draws the component
	// @param camera the camera through which to draw the component
	// @param light the light by which the component should be illuminated (or nullptr for no illumination)
	virtual void draw(Camera& camera, const ArrayList<Light*>& lights);

	// update the component
	virtual void process();

	// called just before the parent is inserted into a new world
	// @param world the world we will be placed into, if any
	virtual void beforeWorldInsertion(const World* world);

	// called just after the parent is inserted into a new world
	// @param world the world we have been placed into, if any
	virtual void afterWorldInsertion(const World* world);

	// check whether the component collides with anything at the current location
	// @return true if we collide, false if we do not
	virtual bool checkCollision() const;

	// marks tiles, chunks, and sectors that are visible to the component
	// @param range maximum range to test occlusion with
	// @param accuracy resolution for the occlusion test
	void occlusionTest(float range, int accuracy);

	// determine if the component has culled the given entity from LOS
	// @param entity the entity to test visibility of
	// @param accuracy bitfield:
	//		* 1: also cast from immediate neighbors
	//		* 2: neighbor tiles always visible by extension
	//		* 4: respect entities with OCCLUDE flag
	//		* 8: with 2: diagonal neighbors also counted
	// @return true if the entity might pass a line-of-sight test
	bool seesEntity(const Entity& entity, float range, int accuracy);

	// delete occlusion data
	void deleteVisMaps();

	// delete occlusion data for self and children
	void deleteAllVisMaps();

	// updates matrices
	virtual void update();

	// checks the component for any components with the given type
	// @param type the type to look for
	// @return true if the component was found, false otherwise
	bool hasComponent(type_t type) const;

	// shoots a laser forward from the component origin until an obstacle is hit
	// @param mat The start location of the laser
	// @param color The laser's color
	// @param size The laser's size
	// @param life The laser's lifespan (in ticks, 60 ticks = 1 sec)
	void shootLaser(const glm::mat4& mat, WideVector& color, float size, float life);

	// find all components of a given type
	// @param type the type of component to search for
	// @param list list to populate
	template <typename T>
	void findAllComponents(Component::type_t type, LinkedList<T*>& list) const {
		for( Uint32 c = 0; c < components.getSize(); ++c ) {
			if( components[c]->getType() == type ) {
				list.addNodeLast(dynamic_cast<T*>(components[c]));
			}
			components[c]->findAllComponents(type,list);
		}
	}

	// find the component with the given name
	// @param name the name of the component
	// @return the component, or nullptr if it could not be found
	template <typename T>
	T* findComponentByName(const char* name) {
		if( name == nullptr || strcmp(name,"")==0 ) {
			return nullptr;
		}
		for( Uint32 c = 0; c < components.getSize(); ++c ) {
			if( strcmp( components[c]->getName(), name ) == 0 ) {
				return static_cast<T*>(components[c]);
			} else {
				T* result = components[c]->findComponentByName<T>(name);
				if( result ) {
					return static_cast<T*>(result);
				}
			}
		}
		return nullptr;
	}

	// find the component with the given uid
	// @param uid the uid of the component
	// @return the component, or nullptr if it could not be found
	template <typename T>
	T* findComponentByUID(const Uint32 uid) {
		if( uid == nuid ) {
			return nullptr;
		}
		for( Uint32 c = 0; c < components.getSize(); ++c ) {
			if( components[c]->getUID() == uid ) {
				return static_cast<T*>(components[c]);
			} else {
				T* result = components[c]->findComponentByUID<T>(uid);
				if( result ) {
					return static_cast<T*>(result);
				}
			}
		}
		return nullptr;
	}

	// find the component with the given name and remove it
	// @param uid the name of the component
	// @return true if the component was removed, otherwise false
	bool removeComponentByName(const char* name);

	// find the component with the given uid and remove it
	// @param uid the uid of the component
	// @return true if the component was removed, otherwise false
	bool removeComponentByUID(const Uint32 uid);

	// mark the component to be deleted on the next update
	void remove();

	Component* addComponent(Component::type_t type);

	// adds a new component to our list of components
	// @return a pointer to our new component
	template <typename T>
	T* addComponent() {
		T* component = new T(*entity, this);
		components.push(component);
		return component;
	}

	// copy this component and all sub-components into another component
	// @param dest the component which will contain our copies
	void copy(Component& dest);

	// copy this component and all sub-components into another entity
	// @param dest the entity which will contain our copies
	void copy(Entity& dest);

	// copy sub-components into another component
	// @param dest the component which will contain our copies
	void copyComponents(Component& dest);

	// clears the node pointing to us in the chunk we are occupying
	void clearChunkNode() { if( chunkNode ) { chunkNode->getList()->removeNode(chunkNode); chunkNode = nullptr; } }

	// clears the chunk nodes of all components
	void clearAllChunkNodes();

	// load the component from a file
	// @param fp the file to read from
	virtual void load(FILE* fp);

	// save/load this object to a file
	// @param file interface to serialize with
	virtual void serialize(FileInterface* file);

	// rotate the component by a given amount
	// @param ang the amount to rotate
	void rotate(const Angle& ang);

	// translate the component by a given amount
	// @param vec the amount to translate
	void translate(const Vector& vec);

	// scale the component by a given amount
	// @param vec the amount to scale
	void scale(const Vector& vec);

	// reverts rotation to 0, 0, 0
	void revertRotation();

	// reverts translation to 0, 0, 0
	void revertTranslation();

	// reverts the scale to 1:1:1
	void revertScale();

	// resets rotation, translation, and scale at once
	void revertToIdentity();

	// getters & setters
	virtual type_t					getType() const						{ return COMPONENT_BASIC; }
	const Entity*					getEntity() const					{ return entity; }
	Entity*							getEntity()							{ return entity; }
	const Component*				getParent() const					{ return parent; }
	Component*						getParent()							{ return parent; }
	bool							isToBeDeleted() const				{ return toBeDeleted; }
	bool							isEditorOnly() const				{ return editorOnly; }
	bool							isUpdateNeeded() const				{ return updateNeeded; }
	ArrayList<Component*>&			getComponents()						{ return components; }
	Uint32							getUID() const						{ return uid; }
	const char*						getName() const						{ return name.get(); }
	const Vector&					getLocalPos() const					{ return lPos; }
	const Angle&					getLocalAng() const					{ return lAng; }
	const Vector&					getLocalScale() const				{ return lScale; }
	const glm::mat4&				getLocalMat() const					{ return lMat; }
	const Vector&					getGlobalPos() const				{ return gPos; }
	const Angle&					getGlobalAng() const				{ return gAng; }
	const Vector&					getGlobalScale() const				{ return gScale; }
	const glm::mat4&				getGlobalMat() const				{ return gMat; }
	bool							isCollapsed() const					{ return collapsed; }
	const bool*						getTilesVisible() const				{ return tilesVisible; }
	const bool*						getChunksVisible() const			{ return chunksVisible; }
	const ArrayList<Chunk*>&		getVisibleChunks() const			{ return visibleChunks; }
	bool							isLocalMatSet() const				{ return lMatSet; }
	const ArrayList<Attribute*>&	getAttributes() const				{ return attributes; }

	void				setEditorOnly(bool _editorOnly)			{ editorOnly = _editorOnly; }
	void				setName(const char* _name)				{ name = _name; }
	void				setLocalPos(const Vector& _pos)			{ lPos = _pos; updateNeeded = true; lMatSet = false; }
	void				setLocalAng(const Angle& _ang)			{ lAng = _ang; updateNeeded = true; lMatSet = false; }
	void				setLocalScale(const Vector& _scale)		{ lScale = _scale; updateNeeded = true; lMatSet = false; }
	void				setLocalMat(const glm::mat4& _mat)		{ lMat = _mat; updateNeeded = true; lMatSet = true; }
	void				setCollapsed(bool _collapsed)			{ collapsed = _collapsed; }

	Component& operator=(const Component& src) {
		toBeDeleted = src.toBeDeleted;
		editorOnly = src.editorOnly;
		name = src.name;
		lPos = src.lPos;
		lAng = src.lAng;
		lScale = src.lScale;
		lMat = src.lMat;
		updateNeeded = true;
		return *this;
	}

protected:
	Entity* entity = nullptr;
	Component* parent = nullptr;
	Node<Component*>* chunkNode = nullptr; // pointer to our node in the chunk we are occupying (if any)

	bool toBeDeleted = false;
	bool editorOnly = false;
	bool updateNeeded = true;
	bool collapsed = true;

	// load sub-components from a file
	// @param fp the file to read from
	void loadSubComponents(FILE* fp);

	// save/load this object to a file
	// @param file interface to serialize with
	void serializeComponents(FileInterface* file);

	ArrayList<Component*> components;	// sub-component list

	Uint32 uid = nuid;		// component uid
	String name;			// component name

	// local space
	Vector		lPos;		// position
	Angle		lAng;		// angle
	Vector		lScale;		// scale
	glm::mat4	lMat;		// matrix (position * angle * scale)
	bool		lMatSet = false;

	// global space
	Vector		gPos;		// position
	Angle		gAng;		// angle
	Vector		gScale;		// scale
	glm::mat4	gMat;		// matrix (position * angle * scale)

	Sint32 currentCX = INT32_MAX;	// X coord of the chunk we are currently occupying
	Sint32 currentCY = INT32_MAX;	// Y coord of the chunk we are currently occupying

	// occlusion test tile world
	Uint32 tilesWidth = 0;
	Uint32 tilesHeight = 0;
	bool* tilesVisible = nullptr;
	bool* chunksVisible = nullptr;
	ArrayList<Chunk*> visibleChunks;
	void occlusionTestTiles(float range, int accuracy);
	void occlusionTestTilesStep(float range, Sint32 tX, Sint32 tY, int accuracy);
	bool occlusionTestTilesLine(Sint32 sX, Sint32 sY, Sint32 eX, Sint32 eY, bool entities);

	// attributes available for reflection
	ArrayList<Attribute*> attributes;
};