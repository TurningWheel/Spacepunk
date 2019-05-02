// Script.hpp

#pragma once

class Engine;
class Client;
class Server;
class World;
class Entity;
class Light;
class Frame;
class Editor;

#include "String.hpp"
#include <luajit-2.0/lua.hpp>

class Script {
public:
	Script(Client& _client);
	Script(Server& _server);
	Script(World& _world);
	Script(Entity& _entity);
	Script(Frame& _frame);
	~Script();

	// script variable types
	enum var_t {
		TYPE_BOOLEAN,
		TYPE_INTEGER,
		TYPE_FLOAT,
		TYPE_STRING,
		TYPE_POINTER,
		TYPE_NIL,
		TYPE_MAX
	};

	// script function parameter
	struct param_t {
		param_t() {}
		virtual ~param_t() {}

		virtual var_t getType() const = 0;
		virtual void push(lua_State* lua) const = 0;
		virtual param_t* copy() const = 0;
		virtual const char* str() = 0;
		Uint32 strSize() const { return string.getSize(); }

	protected:
		String string;
	};

	// boolean parameter
	struct param_bool_t : param_t {
		param_bool_t() {
			string.alloc(2);
		}
		param_bool_t(const bool _value) : value(_value) {
			string.alloc(2);
		}
		virtual ~param_bool_t() {}
		virtual var_t getType() const override { return TYPE_BOOLEAN; }
		virtual void push(lua_State* lua) const override {
			lua_pushboolean(lua, value ? 1 : 0);
		}
		virtual param_t* copy() const override {
			return new param_bool_t(value);
		}
		virtual const char* str() override {
			return value ? "tb" : "fb";
		}
		bool value = false;
	};

	// integer parameter
	struct param_int_t : param_t {
		param_int_t() {
			string.alloc(5);
		}
		param_int_t(const int _value) : value(_value) {
			string.alloc(5);
		}
		virtual ~param_int_t() {}
		virtual var_t getType() const override { return TYPE_INTEGER; }
		virtual void push(lua_State* lua) const override {
			lua_pushinteger(lua, static_cast<lua_Integer>(value));
		}
		virtual param_t* copy() const override {
			return new param_int_t(value);
		}
		virtual const char* str() override {
			Uint32* p = (Uint32*)(&string[0]);
			*p = value;
			string[4] = 'i';
			return string.get();
		}
		int value = 0;
	};

	// float parameter
	struct param_float_t : param_t {
		param_float_t() {
			string.alloc(5);
		}
		param_float_t(const float _value) : value(_value) {
			string.alloc(5);
		}
		virtual ~param_float_t() {}
		virtual var_t getType() const override { return TYPE_FLOAT; }
		virtual void push(lua_State* lua) const override {
			lua_pushnumber(lua, static_cast<lua_Number>(value));
		}
		virtual param_t* copy() const override {
			return new param_float_t(value);
		}
		virtual const char* str() override {
			float* p = (float*)(&string[0]);
			*p = value;
			string[4] = 'f';
			return string.get();
		}
		float value = 0.f;
	};

	// string parameter
	struct param_string_t : param_t {
		param_string_t() {}
		param_string_t(const String& _value) : value(_value) {}
		virtual ~param_string_t() {}
		virtual var_t getType() const override { return TYPE_STRING; }
		virtual void push(lua_State* lua) const override {
			lua_pushlstring(lua, value.get(), value.getSize());
		}
		virtual param_t* copy() const override {
			return new param_string_t(value);
		}
		virtual const char* str() override {
			string.alloc(value.getSize() + 4);
			string.format("%s\0\0\0\0", value.get());
			Uint32* p = (Uint32*)(&string[value.length()]);
			*p = value.length();
			string[string.getSize() - 1] = 's';
			return string.get();
		}
		String value;
	};

	// pointer parameter
	struct param_pointer_t : param_t {
		param_pointer_t() {
			string.alloc(1);
		}
		param_pointer_t(void* _value) : value(_value) {
			string.alloc(1);
		}
		virtual ~param_pointer_t() {}
		virtual var_t getType() const override { return TYPE_POINTER; }
		virtual void push(lua_State* lua) const override {
			lua_pushlightuserdata(lua, value);
		}
		virtual param_t* copy() const override {
			return new param_pointer_t(value);
		}
		virtual const char* str() override {
			return "p";
		}
		void* value = nullptr;
	};

	// nil parameter
	struct param_nil_t : param_t {
		param_nil_t() {
			string.alloc(1);
		}
		virtual ~param_nil_t() {}
		virtual var_t getType() const override { return TYPE_NIL; }
		virtual void push(lua_State* lua) const override {
			lua_pushnil(lua);
		}
		virtual param_t* copy() const override {
			return new param_nil_t();
		}
		virtual const char* str() override {
			return "n";
		}
	};

	// script function arguments
	class Args {
	public:
		Args() {}
		Args(const Args& src) {
			for (Uint32 c = 0; c < src.list.getSize(); ++c) {
				list.push(src.list[c]->copy());
			}
		}
		~Args() {
			while (list.getSize() > 0) {
				delete list.pop();
			}
		}

		// getters & setters
		const ArrayList<param_t*>&		getList() const		{ return list; }
		const Uint32					getSize() const		{ return list.getSize(); }

		// push all args onto the lua stack
		// @param lua the lua stack to push args into
		void push(lua_State* lua) {
			for (Uint32 c = 0; c < list.getSize(); ++c) {
				param_t* param = list[c];
				param->push(lua);
			}
			while (list.getSize() > 0) {
				delete list.pop();
			}
		}
		
		// add a bool to the args list
		// @param value the value to init with
		void addBool(const bool value) {
			list.push(new param_bool_t(value));
		}

		// add an int to the args list
		// @param value the value to init with
		void addInt(const int value) {
			list.push(new param_int_t(value));
		}

		// add a float to the args list
		// @param value the value to init with
		void addFloat(const float value) {
			list.push(new param_float_t(value));
		}

		// add a string to the args list
		// @param value the value to init with
		void addString(const String& value) {
			list.push(new param_string_t(value));
		}

		// add a pointer to the args list
		// @param value the value to init with
		void addPointer(void* value) {
			list.push(new param_pointer_t(value));
		}

		// add a nil to the args list
		void addNil() {
			list.push(new param_nil_t());
		}

	private:
		ArrayList<param_t*> list;
	};

	// native callback function for processing script args
	class Function {
	public:
		virtual ~Function() {}

		// handle the script args
		// @param args the args to consume
		// @return error code
		virtual int operator()(Args& args) const = 0;
	};

	// load and evaluate the given script
	// @param filename filename of the script to run
	// @return 0 on success, nonzero on failure
	int load(const char* filename);

	// evaluate a function. args are discarded after use
	// @param function name of the function to execute
	// @param args a list of args to pass to the function
	// @return 0 on success, nonzero on failure
	int dispatch(const char* function, Args* args = nullptr);

private:
	// class pointers:
	// if these are set, this script engine reliably owns that object's functionality
	Engine* engine = nullptr;
	Client* client = nullptr;
	Editor* editor = nullptr;
	Server* server = nullptr;
	World*  world  = nullptr;
	Entity* entity = nullptr;
	Frame*  frame  = nullptr;

	// script filename
	String filename;

	// if an error occurs, this flag will raise, then no more dispatches will work
	bool broken = false;

	// exposition functions
	void exposeEngine();
	void exposeFrame();
	void exposeAngle();
	void exposeVector();
	void exposeString();
	void exposeGame();
	void exposeClient();
	void exposeServer();
	void exposeWorld();
	void exposeEntity();
	void exposeComponent();
	void exposeBBox();
	void exposeModel();
	void exposeLight();
	void exposeCamera();
	void exposeSpeaker();
	void exposeCharacter();
	void exposeEmitter();
	void exposeMultimesh();
	void exposeEditor(Editor& _editor);

	lua_State* lua = nullptr;
};
