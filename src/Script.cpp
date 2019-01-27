// Script.cpp

//#include <luajit-2.0/lua.hpp>
//#include <LuaBridge/LuaBridge.h>

#include <functional>
#include <assert.h>

#include "Main.hpp"
#include "Engine.hpp"
#include "Client.hpp"
#include "Server.hpp"
#include "World.hpp"
#include "Entity.hpp"
#include "Tile.hpp"
#include "Chunk.hpp"
#include "Frame.hpp"
#include "Button.hpp"
#include "Field.hpp"
#include "Angle.hpp"
#include "Editor.hpp"
#include "Path.hpp"
#include "AnimationState.hpp"
#include "Vector.hpp"
#include "WideVector.hpp"

#include "Script.hpp"

//Includes bindings for these modules.
#include "Node.hpp"
#include "LinkedList.hpp"
#include "ArrayList.hpp"
#include "Rect.hpp"

//Component headers
#include "Component.hpp"
#include "BBox.hpp"
#include "Model.hpp"
#include "Light.hpp"
#include "Camera.hpp"
#include "Speaker.hpp"
#include "Character.hpp"

Script::Script()
{
	lua.open_libraries(sol::lib::base, sol::lib::io, sol::lib::package, sol::lib::math);
}

Script::Script(Client& _client)
					:	Script()
{
	client = &_client;
	engine = mainEngine;

	// expose functions
	exposeEngine();
	exposeClient();
}

Script::Script(Server& _server)
					:	Script()
{
	server = &_server;
	engine = mainEngine;

	// expose functions
	exposeEngine();
	exposeServer();
}

Script::Script(World& _world)
					:	Script()
{
	world = &_world;
	engine = mainEngine;

	// expose functions
	exposeEngine();
	exposeAngle();
	exposeVector();
	exposeWorld();
}

Script::Script(Entity& _entity)
					:	Script()
{
	entity = &_entity;
	engine = mainEngine;

	// expose functions
	exposeEngine();
	exposeAngle();
	exposeVector();
	exposeEntity();
}

Script::Script(Frame& _frame)
					:	Script()
{
	frame = &_frame;
	engine = mainEngine;
	client = engine->getLocalClient();

	// expose functions
	exposeEngine();
	exposeClient();
	if( client->getEditor() ) {
		exposeEditor(*client->getEditor());
		exposeEntity();
	}
	exposeFrame();
	exposeWorld();
}

Script::~Script() {
}

bool Script::load(const char* _filename)
{
	filename = mainEngine->buildPath(_filename);

	try
	{
		sol::protected_function_result script = lua.safe_script_file(filename.get());
		if (!script.valid())
		{
			sol::error err = script;
			mainEngine->fmsg(Engine::MSG_ERROR,"failed to load script '%s':", filename.get());
			mainEngine->fmsg(Engine::MSG_ERROR," %s", err.what());
			broken = true;
			return false;
		}
	}
	catch(const sol::error& e)
	{
		mainEngine->fmsg(Engine::MSG_ERROR,"failed to load script '%s':", filename.get());
		mainEngine->fmsg(Engine::MSG_ERROR," %s", e.what());
		broken = true;
		return false;
	}
	// catch(const std::exception& e)
	// {
		// mainEngine->fmsg(Engine::MSG_ERROR,"failed to load script '%s':", filename.get());
		// mainEngine->fmsg(Engine::MSG_ERROR," %s", e.what());
		// broken = true;
		// return false;
	//	return false;
	// }
	// catch(...)
	// {
		// mainEngine->fmsg(Engine::MSG_ERROR,"failed to load script '%s':", filename.get());
		// mainEngine->fmsg(Engine::MSG_ERROR," %s", e.what());
		// broken = true;
		// return false;
	//	return false;
	// }

	broken = false;
	return true;
}

void Script::dispatchFunctionErrorMessage(const String& filename, const char* functionName, const sol::error& err)
{
	mainEngine->fmsg(Engine::MSG_ERROR, "script error in '%s' (dispatch '%s'):", filename.get(), functionName);
	mainEngine->fmsg(Engine::MSG_ERROR," %s", err.what());
}



void Script::exposeFrame() {
	{
		//First identify the constructors.
		sol::constructors<Frame(const char*, const char*), Frame(Frame&, const char*, const char*)> constructors;

		//Then do the thing.
		sol::usertype<Frame> usertype(constructors,
			"getBorder", &Frame::getBorder,
			"getSize", &Frame::getSize,
			"getActualSize", &Frame::getActualSize,
			"isHigh", &Frame::isHigh,
			"getFrames", &Frame::getFrames,
			"getButtons", &Frame::getButtons,
			"setBorder", &Frame::setBorder,
			"setSize", &Frame::setSize,
			"setActualSize", &Frame::setActualSize,
			"setHigh", &Frame::setHigh,
			"setColor", &Frame::setColor,
			"addFrame", &Frame::addFrame,
			"addButton", &Frame::addButton,
			"addField", &Frame::addField,
			"addImage", &Frame::addImage,
			"addEntry", &Frame::addEntry,
			"clear", &Frame::clear,
			"removeSelf", &Frame::removeSelf,
			"remove", &Frame::remove,
			"removeEntry", &Frame::removeEntry,
			"findEntry", &Frame::findEntry,
			"findFrame", &Frame::findFrame
		);

		//Finally register the thing.
		lua.set_usertype("Frame", usertype);
	}

	if( frame ) {
		lua["frame"] = frame;
	}
}

void Script::exposeString() {
	{
		//First identify the constructors.
		sol::constructors<String(), String(const String), String(const char*)> constructors;

		//Then do the thing.
		sol::usertype<String> usertype(constructors,
			"get", &String::get,
			"getSize", &String::getSize,
			"alloc", &String::alloc,
			"empty", &String::empty,
			"length", &String::length,
			"assign", &String::assign,
			"append", &String::append,
			"substr", &String::substr,
			"find", sol::resolve<size_t(const char*, size_t) const>(&String::find),
			"findChar", sol::resolve<size_t(const char, size_t) const>(&String::find),
			"toInt", &String::toInt,
			"toFloat", &String::toFloat
		);

		//Finally register the thing.
		lua.set_usertype("String", usertype);
	}

	LinkedList<String>::exposeToScript(lua, "LinkedListString", "NodeString");
	LinkedList<String*>::exposeToScript(lua, "LinkedListStringPtr", "NodeStringPtr");
	ArrayList<String>::exposeToScript(lua, "ArrayListString");
	ArrayList<String*>::exposeToScript(lua, "ArrayListStringPtr");
}

void Script::exposeAngle() {
	{
		//First identify the constructors.
		sol::constructors<Angle(), Angle(const Angle&), Angle(float, float, float)> constructors;

		//Then do the thing.
		sol::usertype<Angle> usertype(constructors,
			"yaw", &Angle::yaw,
			"pitch", &Angle::pitch,
			"roll", &Angle::roll,
			"radiansYaw", &Angle::radiansYaw,
			"radiansPitch", &Angle::radiansPitch,
			"radiansRoll", &Angle::radiansRoll,
			"degreesYaw", &Angle::degreesYaw,
			"degreesPitch", &Angle::degreesPitch,
			"degreesRoll", &Angle::degreesRoll,
			"wrapAngles", &Angle::wrapAngles,
			"toVector", &Angle::toVector
		);

		//Finally register the thing.
		lua.set_usertype("Angle", usertype);
	}

	LinkedList<Angle>::exposeToScript(lua, "LinkedListAngle", "NodeAngle");
	LinkedList<Angle*>::exposeToScript(lua, "LinkedListAnglePtr", "NodeAnglePtr");
	ArrayList<Angle>::exposeToScript(lua, "ArrayListAngle");
	ArrayList<Angle*>::exposeToScript(lua, "ArrayListAnglePtr");
}

void Script::exposeVector() {
	{
		//First identify the constructors.
		sol::constructors<Vector(), Vector(const Vector&), Vector(const float), Vector(float, float, float), Vector(float*)> constructors;

		//Then do the thing.
		sol::usertype<Vector> usertype(constructors,
			"x", &Vector::x,
			"y", &Vector::y,
			"z", &Vector::z,
			"r", &Vector::x,
			"g", &Vector::y,
			"b", &Vector::z,
			"hasVolume", &Vector::hasVolume,
			"dot", &Vector::dot,
			"cross", &Vector::cross,
			"length", &Vector::length,
			"lengthSquared", &Vector::lengthSquared,
			"normal", &Vector::normal,
			"normalize", &Vector::normalize
		);

		//Finally register the thing.
		lua.set_usertype("Vector", usertype);
	}

	LinkedList<Vector>::exposeToScript(lua, "LinkedListVector", "NodeVector");
	LinkedList<Vector*>::exposeToScript(lua, "LinkedListVectorPtr", "NodeVectorPtr");
	ArrayList<Vector>::exposeToScript(lua, "ArrayListVector");
	ArrayList<Vector*>::exposeToScript(lua, "ArrayListVectorPtr");

	{
		//First identify the constructors.
		sol::constructors<WideVector(), WideVector(const WideVector&), WideVector(const float), WideVector(float, float, float, float), WideVector(float*)> constructors;

		//Then do the thing.
		sol::usertype<WideVector> usertype(constructors,
			sol::base_classes, sol::bases<Vector>(),
			"x", &WideVector::x,
			"y", &WideVector::y,
			"z", &WideVector::z,
			"w", &WideVector::w,
			"r", &WideVector::x,
			"g", &WideVector::y,
			"b", &WideVector::z,
			"a", &WideVector::w,
			"hasVolume", &WideVector::hasVolume,
			"dot", &WideVector::dot,
			"cross", &WideVector::cross,
			"length", &WideVector::length,
			"lengthSquared", &WideVector::lengthSquared,
			"normal", &WideVector::normal,
			"normalize", &WideVector::normalize
		);

		//Finally register the thing.
		lua.set_usertype("WideVector", usertype);
	}

	LinkedList<WideVector>::exposeToScript(lua, "LinkedListWideVector", "NodeWideVector");
	LinkedList<WideVector*>::exposeToScript(lua, "LinkedListWideVectorPtr", "NodeWideVectorPtr");
	ArrayList<WideVector>::exposeToScript(lua, "ArrayListWideVector");
	ArrayList<WideVector*>::exposeToScript(lua, "ArrayListWideVectorPtr");
}

void Script::exposeGame() {
	lua.new_usertype<Game>("Game",
		"getWorld", &Game::getWorld,
		"worldForName", &Game::worldForName,
		"loadWorld", &Game::loadWorld,
		"closeAllWorlds", &Game::closeAllWorlds
	);
}

void Script::exposeClient() {
	exposeGame();
	{
		//First identify the constructors.
		sol::constructors<Client()> constructors;

		//Then do the thing.
		sol::usertype<Client> usertype(constructors,
			sol::base_classes, sol::bases<Game>(),
			"isConsoleAllowed", &Client::isConsoleAllowed,
			"isConsoleActive", &Client::isConsoleActive,
			"isEditorActive", &Client::isEditorActive,
			"getGUI", &Client::getGUI,
			"startEditor", &Client::startEditor
		);

		//Finally register the thing.
		lua.set_usertype("Client", usertype);
	}

	if( client ) {
		lua["client"] = client;
	}
}

void Script::exposeServer() {
	exposeGame();

	{
		//First identify the constructors.
		sol::constructors<Server()> constructors;

		//Then do the thing.
		sol::usertype<Server> usertype(constructors,
			sol::base_classes, sol::bases<Game>()
		);

		//Finally register the thing.
		lua.set_usertype("Server", usertype);
	}

	if( server ) {
		lua["server"] = server;
	}
}

void Script::exposeWorld() {
	lua.new_usertype<World>("World",
			"getTicks", &World::getTicks,
			"getFilename", &World::getFilename,
			"getNameStr", &World::getNameStr,
			"isClientObj", &World::isClientObj,
			"isServerObj", &World::isServerObj,
			"isShowTools", &World::isShowTools,
			"uidToEntity", &World::uidToEntity,
			"lineTrace", &World::lineTrace,
			"lineTraceList", &World::lineTraceList,
			"lineTraceNoEntities", &World::lineTraceNoEntities,
			"saveFile", &World::saveFile,
			"setShowTools", &World::setShowTools,
			"selectEntity", &World::selectEntity,
			"selectEntities", &World::selectEntities,
			"deselectGeometry", &World::deselectGeometry
	);

	if( world ) {
		lua["world"] = world;
	}

	LinkedList<World*>::exposeToScript(lua, "LinkedListWorldPtr", "NodeWorldPtr");
	ArrayList<World*>::exposeToScript(lua, "ArrayListWorldPtr"); //TODO: For a quick test, NOW try exposing that getArray() method that was erroring (since the World class is now bound).
}

void Script::exposeEmitter() {
 	// todo
}

