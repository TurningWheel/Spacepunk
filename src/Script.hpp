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
#include "sol_forward.hpp"

class SolScript;
struct SolScriptArgs; //Opaque type to hide internal sol types from rest of engine. NOTE: Every SolScriptArgs is inextricably tied to its respective Script engine instance! You can never reuse a SolScriptArgs with another Script engine instance.

class Script
{
public:
	Script(Client& client);
	Script(Server& server);
	Script(World& world);
	Script(Entity& entity);
	Script(Frame& frame);
	~Script();

	// load and evaluate the given script
	// @param filename: filename of the script to run
	// @return 0 on success, nonzero on failure
	bool load(const char* filename);

	//template<typename A, typename B, typename...C>
	//bool dispatchFunction(const char* functionName, A arg1, B arg2, C...arg3); //TODO: SO post about making this sort of function virtual.
	template<typename...Args>
	bool dispatchFunction(const char* functionName, Args&&...args);

	bool dispatchFunction(const char* functionName, SolScriptArgs* params);

	template<typename...Args>
	bool dispatchFunction(const char* functionName, SolScriptArgs* params, Args&&...args);

	template <typename T>
	void addParam(T&& obj, SolScriptArgs& params);

private:
	SolScript* pImpl = nullptr;
};
