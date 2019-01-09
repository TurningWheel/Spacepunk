// Script.hpp

#pragma once

#include <vector>

class Client;
class Server;
class World;
class Entity;
class Light;
class Frame;
class Editor;

//#include "Engine.hpp"
#include "String.hpp"
#include "sol.hpp"

class Script
{
public:
	Script(Client& client);
	Script(Server& server);
	Script(World& world);
	Script(Entity& entity);
	Script(Frame& frame);
	~Script();

	//NOTE: Every Script::Args is inextricably tied to its respective Script engine instance! You shall never reuse a Script::Args with another Script engine instance.
	struct Args //TODO: Suspect this is the issue.
	{
		std::vector<sol::object> params;
	};

	// load and evaluate the given script
	// @param filename: filename of the script to run
	// @return 0 on success, nonzero on failure
	bool load(const char* filename);

	static void dispatchFunctionErrorMessage(const String& filename, const char* functionName, const sol::error& err);

	template<typename...Args>
	bool dispatchFunction(const char* functionName, Args&&...args)
	{
		if (broken)
		{
			return false;
		}

		sol::protected_function myFunc = lua[functionName];

		sol::protected_function_result result;
		result = myFunc(args...);
		if (!result.valid())
		{
			sol::error err = result;
			dispatchFunctionErrorMessage(filename, functionName, err);
			broken = true;
			return false;
		}

		return true;
	}

	bool dispatchFunction(const char* functionName, Script::Args& params)
	{
		if (0 == params.params.size())
		{
			return dispatchFunction(functionName);
		}

		if (broken)
		{
			return false;
		}

		sol::protected_function myFunc = lua[functionName];

		sol::protected_function_result result;
		result = myFunc(sol::as_args(params.params));

		if (!result.valid())
		{
			sol::error err = result;
			dispatchFunctionErrorMessage(filename, functionName, err);
			broken = true;
			return false;
		}

		return true;
	}

	template<typename...Args>
	bool dispatchFunction(const char* functionName, Script::Args& params, Args&&...args)
	{
		if (0 == params.params.size())
		{
			return dispatchFunction(functionName, args...);
		}

		if (broken)
		{
			return false;
		}

		sol::protected_function myFunc = lua[functionName];

		sol::protected_function_result result;
		result = myFunc(sol::as_args(params.params), args...);

		if (!result.valid())
		{
			sol::error err = result;
			dispatchFunctionErrorMessage(filename, functionName, err);
			broken = true;
			return false;
		}

		return true;
	}

	template <typename T>
	void addParam(T&& obj, Script::Args& params)
	{
		params.params.push_back(makeObject(obj));
	}

	template <typename T>
	sol::object makeObject(T&& obj)
	{
		return sol::make_object(lua, obj);
	}

private:
	sol::state lua;

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

	// common constructor that initializes all universal base data.
	Script();

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
	void exposeEditor(Editor& _editor);

};
