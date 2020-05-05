// Script.cpp

#include <luajit-2.0/lua.hpp>
#include <LuaBridge/LuaBridge.h>
#include <functional>

#include "Main.hpp"
#include "Engine.hpp"
#include "Client.hpp"
#include "Server.hpp"
#include "World.hpp"
#include "Entity.hpp"
#include "Script.hpp"
#include "Tile.hpp"
#include "Chunk.hpp"
#include "Frame.hpp"
#include "Button.hpp"
#include "Field.hpp"
#include "Rotation.hpp"
#include "Editor.hpp"
#include "Path.hpp"
#include "AnimationState.hpp"
#include "Vector.hpp"
#include "WideVector.hpp"
#include "Slider.hpp"

//Component headers
#include "Component.hpp"
#include "BBox.hpp"
#include "Model.hpp"
#include "Light.hpp"
#include "Camera.hpp"
#include "Speaker.hpp"
#include "Character.hpp"
#include "Multimesh.hpp"

int Script::load(const char* _filename) {
	filename = mainEngine->buildPath(_filename);

	int result = luaL_dofile(lua, filename.get());
	if (result) {
		mainEngine->fmsg(Engine::MSG_ERROR, "failed to load script '%s':", filename.get());
		mainEngine->fmsg(Engine::MSG_ERROR, " %s", lua_tostring(lua, -1));
		broken = true;
		return 1;
	} else {
		broken = false;
		return 0;
	}
}

int Script::dispatch(const char* function, Args* args) {
	if (broken) {
		return -1;
	}
	lua_getglobal(lua, function);

	Uint32 numArgs = 0;
	if (args)
	{
		numArgs = args->getSize();
		args->push(lua);
	}

	int status = lua_pcall(lua, (int)numArgs, 0, 0);
	if (status) {
		mainEngine->fmsg(Engine::MSG_ERROR, "script error in '%s' (dispatch '%s'):", filename.get(), function);
		mainEngine->fmsg(Engine::MSG_ERROR, " %s", lua_tostring(lua, -1));
		broken = true;
		return -2;
	}

	return 0;
}

Script::Script(Client& _client) {
	client = &_client;
	engine = mainEngine;

	lua = luaL_newstate();
	luaL_openlibs(lua);

	// expose functions
	exposeEngine();
	exposeGame();
	exposeClient();
}

Script::Script(Server& _server) {
	server = &_server;
	engine = mainEngine;

	lua = luaL_newstate();
	luaL_openlibs(lua);

	// expose functions
	exposeEngine();
	exposeGame();
	exposeServer();
}

Script::Script(World& _world) {
	world = &_world;
	engine = mainEngine;

	lua = luaL_newstate();
	luaL_openlibs(lua);

	// expose functions
	exposeEngine();
	exposeAngle();
	exposeVector();
	exposeWorld();
	exposeExtra();
}

Script::Script(Entity& _entity) {
	entity = &_entity;
	engine = mainEngine;

	lua = luaL_newstate();
	luaL_openlibs(lua);

	// expose functions
	exposeEngine();
	exposeGame();
	exposeClient();
	exposeServer();
	exposeAngle();
	exposeVector();
	exposeEntity();
	exposeFrame();
	exposeWorld();
	exposeExtra();
}

Script::Script(Frame& _frame) {
	frame = &_frame;
	engine = mainEngine;
	client = engine->getLocalClient();

	lua = luaL_newstate();
	luaL_openlibs(lua);

	// expose functions
	exposeEngine();
	exposeGame();
	exposeClient();
	if (client->getEditor()) {
		exposeEditor(*client->getEditor());
	}
	exposeEntity();
	exposeFrame();
	exposeExtra();
	exposeVector();
	exposeWorld();
}

Script::~Script() {
	if (lua) {
		lua_close(lua);
		lua = nullptr;
	}
}

void Script::exposeEngine() {
	luabridge::getGlobalNamespace(lua)
		.beginClass<Engine>("Engine")
		.addFunction("isInitialized", &Engine::isInitialized)
		.addFunction("isRunning", &Engine::isRunning)
		.addFunction("isPaused", &Engine::isPaused)
		.addFunction("isFullscreen", &Engine::isFullscreen)
		.addFunction("getVsyncMode", &Engine::getVsyncMode)
		.addFunction("isRunningClient", &Engine::isRunningClient)
		.addFunction("isRunningServer", &Engine::isRunningServer)
		.addFunction("getGameTitle", &Engine::getGameTitle)
		.addFunction("getLocalClient", &Engine::getLocalClient)
		.addFunction("getLocalServer", &Engine::getLocalServer)
		.addFunction("getXres", &Engine::getXres)
		.addFunction("getYres", &Engine::getYres)
		.addFunction("getKeyStatus", &Engine::getKeyStatus)
		.addFunction("getMouseStatus", &Engine::getMouseStatus)
		.addFunction("getMouseX", &Engine::getMouseX)
		.addFunction("getMouseY", &Engine::getMouseY)
		.addFunction("getMouseMoveX", &Engine::getMouseMoveX)
		.addFunction("getMouseMoveY", &Engine::getMouseMoveY)
		.addFunction("getFPS", &Engine::getFPS)
		.addFunction("getTimeSync", &Engine::getTimeSync)
		.addFunction("getTicksPerSecond", &Engine::getTicksPerSecond)
		.addFunction("random", &Engine::random)
		.addFunction("playSound", &Engine::playSound)
		.addFunction("commandLine", &Engine::commandLine)
		.addFunction("doCommand", &Engine::doCommand)
		.addFunction("getCvar", &Engine::getCvar)
		.addFunction("shutdown", &Engine::shutdown)
		.addFunction("editorPlaytest", &Engine::editorPlaytest)
		.addFunction("smsg", &Engine::smsg)
		.addFunction("msg", &Engine::msg)
		.addFunction("getInput", &Engine::getInput)
		.addStaticFunction("triangleCoords", &Engine::triangleCoords)
		.addStaticFunction("pointInTriangle", &Engine::pointInTriangle)
		.addStaticFunction("strcmp", &Engine::strCompare)
		.addFunction("loadConfig", &Engine::loadConfig)
		.addFunction("saveConfig", &Engine::saveConfig)
		.addFunction("getDisplayModes", &Engine::getDisplayModes)
		.endClass()
		;

	luabridge::getGlobalNamespace(lua)
		.beginClass<Input>("Input")
		.addConstructor<void(*)()>()
		.addFunction("analog", &Input::analog)
		.addFunction("binary", &Input::binary)
		.addFunction("binaryToggle", &Input::binaryToggle)
		.addFunction("consumeBinaryToggle", &Input::consumeBinaryToggle)
		.addFunction("binding", &Input::binding)
		.addFunction("rebind", &Input::rebind)
		.addFunction("refresh", &Input::refresh)
		.addFunction("update", &Input::update)
		.addFunction("isInverted", &Input::isInverted)
		.addFunction("setInverted", &Input::setInverted)
		.endClass()
		;

	luabridge::getGlobalNamespace(lua)
		.beginClass<Script::Args>("ScriptArgs")
		.addConstructor<void(*)()>()
		.addFunction("getSize", &Script::Args::getSize)
		.addFunction("copy", &Script::Args::copy)
		.addFunction("addBool", &Script::Args::addBool)
		.addFunction("addInt", &Script::Args::addInt)
		.addFunction("addFloat", &Script::Args::addFloat)
		.addFunction("addString", &Script::Args::addString)
		.addFunction("addPointer", &Script::Args::addPointer)
		.addFunction("addNil", &Script::Args::addNil)
		.endClass()
		;

	luabridge::getGlobalNamespace(lua)
		.beginClass<glm::mat4>("matrix4x4")
		.endClass()
		;

	if (engine) {
		luabridge::push(lua, engine);
		lua_setglobal(lua, "engine");
	}

	exposeString();

	Rect<Sint32>::exposeToScript(lua, "RectSint32");
	Rect<Uint32>::exposeToScript(lua, "RectUint32");
	ArrayList<int>::exposeToScript(lua, "ArrayListInt");
	ArrayList<float>::exposeToScript(lua, "ArrayListFloat");
}

void Script::exposeFrame() {
	luabridge::getGlobalNamespace(lua)
		.beginClass<Frame>("Frame")
		.addFunction("getFont", &Frame::getFont)
		.addFunction("setFont", &Frame::setFont)
		.addFunction("getBorder", &Frame::getBorder)
		.addFunction("getSize", &Frame::getSize)
		.addFunction("getActualSize", &Frame::getActualSize)
		.addFunction("getBorderStyle", &Frame::getBorderStyle)
		.addFunction("getFrames", &Frame::getFrames)
		.addFunction("getButtons", &Frame::getButtons)
		.addFunction("getSliders", &Frame::getSliders)
		.addFunction("setBorder", &Frame::setBorder)
		.addFunction("setSize", &Frame::setSize)
		.addFunction("setActualSize", &Frame::setActualSize)
		.addFunction("setBorderStyle", &Frame::setBorderStyle)
		.addFunction("setBorderColor", &Frame::setBorderColor)
		.addFunction("setHigh", &Frame::setHigh)
		.addFunction("setColor", &Frame::setColor)
		.addFunction("addFrame", &Frame::addFrame)
		.addFunction("addButton", &Frame::addButton)
		.addFunction("addSlider", &Frame::addSlider)
		.addFunction("addField", &Frame::addField)
		.addFunction("addImage", &Frame::addImage)
		.addFunction("addEntry", &Frame::addEntry)
		.addFunction("clear", &Frame::clear)
		.addFunction("removeSelf", &Frame::removeSelf)
		.addFunction("remove", &Frame::remove)
		.addFunction("removeEntry", &Frame::removeEntry)
		.addFunction("findFrame", &Frame::findFrame)
		.addFunction("findButton", &Frame::findButton)
		.addFunction("findField", &Frame::findField)
		.addFunction("findImage", &Frame::findImage)
		.addFunction("findEntry", &Frame::findEntry)
		.addFunction("findSlider", &Frame::findSlider)
		.addFunction("isDropDown", &Frame::isDropDown)
		.addFunction("setDropDown", &Frame::setDropDown)
		.endClass()
		;

	luabridge::getGlobalNamespace(lua)
		.beginClass<Field>("Field")
		.addFunction("getName", &Field::getName)
		.addFunction("getFont", &Field::getFont)
		.addFunction("getText", &Field::getText)
		.addFunction("getTextLen", &Field::getTextLen)
		.addFunction("getColor", &Field::getColor)
		.addFunction("getSize", &Field::getSize)
		.addFunction("getHJustify", &Field::getHJustify)
		.addFunction("getVJustify", &Field::getVJustify)
		.addFunction("isSelected", &Field::isSelected)
		.addFunction("isEditable", &Field::isEditable)
		.addFunction("isNumbersOnly", &Field::isNumbersOnly)
		.addFunction("getTabDestField", &Field::getTabDestField)
		.addFunction("getTabDestFrame", &Field::getTabDestFrame)
		.addFunction("getParams", &Field::getParams)
		.addFunction("getCallback", &Field::getCallback)
		.addFunction("setName", &Field::setName)
		.addFunction("setFont", &Field::setFont)
		.addFunction("setText", &Field::setText)
		.addFunction("setPos", &Field::setPos)
		.addFunction("setSize", &Field::setSize)
		.addFunction("setColor", &Field::setColor)
		.addFunction("setEditable", &Field::setEditable)
		.addFunction("setNumbersOnly", &Field::setNumbersOnly)
		.addFunction("setJustify", &Field::setJustify)
		.addFunction("setHJustify", &Field::setHJustify)
		.addFunction("setVJustify", &Field::setVJustify)
		.addFunction("setScroll", &Field::setScroll)
		.addFunction("setTabDestField", &Field::setTabDestField)
		.addFunction("setTabDestFrame", &Field::setTabDestFrame)
		.endClass()
		;

	luabridge::getGlobalNamespace(lua)
		.beginClass<Button>("Button")
		.addFunction("getName", &Button::getName)
		.addFunction("getText", &Button::getText)
		.addFunction("getFont", &Button::getFont)
		.addFunction("getBorder", &Button::getBorder)
		.addFunction("getSize", &Button::getSize)
		.addFunction("isPressed", &Button::isPressed)
		.addFunction("isHighlighted", &Button::isHighlighted)
		.addFunction("isDisabled", &Button::isDisabled)
		.addFunction("getStyle", &Button::getStyle)
		.addFunction("setBorder", &Button::setBorder)
		.addFunction("setPos", &Button::setPos)
		.addFunction("setSize", &Button::setSize)
		.addFunction("setColor", &Button::setColor)
		.addFunction("setTextColor", &Button::setTextColor)
		.addFunction("setName", &Button::setName)
		.addFunction("setText", &Button::setText)
		.addFunction("setFont", &Button::setFont)
		.addFunction("setIcon", &Button::setIcon)
		.addFunction("setTooltip", &Button::setTooltip)
		.addFunction("setDisabled", &Button::setDisabled)
		.addFunction("setStyle", &Button::setStyle)
		.addFunction("setPressed", &Button::setPressed)
		.endClass()
		;

	luabridge::getGlobalNamespace(lua)
		.beginClass<Frame::entry_t>("Entry")
		.addData("name", &Frame::entry_t::name)
		.addData("text", &Frame::entry_t::text)
		.addData("tooltip", &Frame::entry_t::tooltip)
		.addData("color", &Frame::entry_t::color)
		.addData("image", &Frame::entry_t::image)
		.addFunction("setParams", &Frame::entry_t::setParams)
		.endClass()
		;

	luabridge::getGlobalNamespace(lua)
		.beginClass<Slider>("Slider")
		.addFunction("getName", &Slider::getName)
		.addFunction("getValue", &Slider::getValue)
		.addFunction("getMaxValue", &Slider::getMaxValue)
		.addFunction("getMinValue", &Slider::getMinValue)
		.addFunction("getBorder", &Slider::getBorder)
		.addFunction("getHandleSize", &Slider::getHandleSize)
		.addFunction("getRailSize", &Slider::getRailSize)
		.addFunction("getTooltip", &Slider::getTooltip)
		.addFunction("isPressed", &Slider::isPressed)
		.addFunction("isHighlighted", &Slider::isHighlighted)
		.addFunction("isDisabled", &Slider::isDisabled)
		.addFunction("getColor", &Slider::getColor)
		.addFunction("setName", &Slider::setName)
		.addFunction("setValue", &Slider::setValue)
		.addFunction("setMaxValue", &Slider::setMaxValue)
		.addFunction("setMinValue", &Slider::setMinValue)
		.addFunction("setBorder", &Slider::setBorder)
		.addFunction("setHandleSize", &Slider::setHandleSize)
		.addFunction("setRailSize", &Slider::setRailSize)
		.addFunction("setTooltip", &Slider::setTooltip)
		.addFunction("setDisabled", &Slider::setDisabled)
		.addFunction("setColor", &Slider::setColor)
		.addFunction("setCallback", &Slider::setCallback)
		.endClass()
		;

	if (frame) {
		luabridge::push(lua, frame);
		lua_setglobal(lua, "frame");
	}
}

void Script::exposeString() {
	typedef Uint32(String::*FindFn)(const char*, Uint32) const;
	FindFn find = static_cast<FindFn>(&String::find);

	typedef Uint32(String::*FindCharFn)(const char, Uint32) const;
	FindCharFn findChar = static_cast<FindCharFn>(&String::find);

	luabridge::getGlobalNamespace(lua)
		.beginClass<String>("String")
		.addConstructor<void(*) (const char*)>()
		.addFunction("get", &String::get)
		.addFunction("getSize", &String::getSize)
		.addFunction("alloc", &String::alloc)
		.addFunction("empty", &String::empty)
		.addFunction("length", &String::length)
		.addFunction("assign", &String::assign)
		.addFunction("append", &String::append)
		.addFunction("substr", &String::substr)
		.addFunction("find", find)
		.addFunction("findChar", findChar)
		.addFunction("toInt", &String::toInt)
		.addFunction("toFloat", &String::toFloat)
		.endClass()
		;

	LinkedList<String>::exposeToScript(lua, "LinkedListString", "NodeString");
	LinkedList<String*>::exposeToScript(lua, "LinkedListStringPtr", "NodeStringPtr");
	ArrayList<String>::exposeToScript(lua, "ArrayListString");
	ArrayList<String*>::exposeToScript(lua, "ArrayListStringPtr");
}

void Script::exposeAngle() {
	luabridge::getGlobalNamespace(lua)
		.beginClass<Rotation>("Rotation")
		.addConstructor<void(*) (float, float, float)>()
		.addData("yaw", &Rotation::yaw, true)
		.addData("pitch", &Rotation::pitch, true)
		.addData("roll", &Rotation::roll, true)
		.addFunction("radiansYaw", &Rotation::radiansYaw)
		.addFunction("radiansPitch", &Rotation::radiansPitch)
		.addFunction("radiansRoll", &Rotation::radiansRoll)
		.addFunction("degreesYaw", &Rotation::degreesYaw)
		.addFunction("degreesPitch", &Rotation::degreesPitch)
		.addFunction("degreesRoll", &Rotation::degreesRoll)
		.addFunction("wrapAngles", &Rotation::wrapAngles)
		.addFunction("toVector", &Rotation::toVector)
		.endClass()
		;

	LinkedList<Rotation>::exposeToScript(lua, "LinkedListRotation", "NodeRotation");
	LinkedList<Rotation*>::exposeToScript(lua, "LinkedListRotationPtr", "NodeRotationPtr");
	ArrayList<Rotation>::exposeToScript(lua, "ArrayListRotation");
	ArrayList<Rotation*>::exposeToScript(lua, "ArrayListRotationPtr");

	luabridge::getGlobalNamespace(lua)
		.beginClass<Quaternion>("Quaternion")
		.addConstructor<void(*) (float, float, float, float)>()
		.addData("x", &Quaternion::x, true)
		.addData("y", &Quaternion::y, true)
		.addData("z", &Quaternion::z, true)
		.addData("w", &Quaternion::w, true)
		.addFunction("toVector", &Quaternion::toVector)
		.addFunction("toRotation", &Quaternion::toRotation)
		.addFunction("rotate", &Quaternion::rotate)
		.addFunction("lerp", &Quaternion::lerp)
		.addFunction("slerp", &Quaternion::slerp)
		.addFunction("mul", &Quaternion::mul)
		.endClass()
		;

	LinkedList<Quaternion>::exposeToScript(lua, "LinkedListQuaternion", "NodeQuaternion");
	LinkedList<Quaternion*>::exposeToScript(lua, "LinkedListQuaternionPtr", "NodeQuaternionPtr");
	ArrayList<Quaternion>::exposeToScript(lua, "ArrayListQuaternion");
	ArrayList<Quaternion*>::exposeToScript(lua, "ArrayListQuaternionPtr");
}

void Script::exposeVector() {
	luabridge::getGlobalNamespace(lua)
		.beginClass<Vector>("Vector")
		.addConstructor<void(*) (float, float, float)>()
		.addData("x", &Vector::x, true)
		.addData("y", &Vector::y, true)
		.addData("z", &Vector::z, true)
		.addData("r", &Vector::x, true)
		.addData("g", &Vector::y, true)
		.addData("b", &Vector::z, true)
		.addFunction("hasVolume", &Vector::hasVolume)
		.addFunction("dot", &Vector::dot)
		.addFunction("cross", &Vector::cross)
		.addFunction("length", &Vector::length)
		.addFunction("lengthSquared", &Vector::lengthSquared)
		.addFunction("normal", &Vector::normal)
		.addFunction("normalize", &Vector::normalize)
		.addFunction("reflect", &Vector::reflect)
		.endClass()
		;

	LinkedList<Vector>::exposeToScript(lua, "LinkedListVector", "NodeVector");
	LinkedList<Vector*>::exposeToScript(lua, "LinkedListVectorPtr", "NodeVectorPtr");
	ArrayList<Vector>::exposeToScript(lua, "ArrayListVector");
	ArrayList<Vector*>::exposeToScript(lua, "ArrayListVectorPtr");

	luabridge::getGlobalNamespace(lua)
		.beginClass<WideVector>("WideVector")
		.addConstructor<void(*) (float, float, float, float)>()
		.addData("x", (float WideVector::*)&WideVector::x, true)
		.addData("y", (float WideVector::*)&WideVector::y, true)
		.addData("z", (float WideVector::*)&WideVector::z, true)
		.addData("w", &WideVector::w, true)
		.addData("r", (float WideVector::*)&WideVector::x, true)
		.addData("g", (float WideVector::*)&WideVector::y, true)
		.addData("b", (float WideVector::*)&WideVector::z, true)
		.addData("a", &WideVector::w, true)
		.addFunction("hasVolume", &WideVector::hasVolume)
		.addFunction("dot", &WideVector::dot)
		.addFunction("cross", &WideVector::cross)
		.addFunction("length", &WideVector::length)
		.addFunction("lengthSquared", &WideVector::lengthSquared)
		.addFunction("normal", &WideVector::normal)
		.addFunction("normalize", &WideVector::normalize)
		.endClass()
		;

	LinkedList<WideVector>::exposeToScript(lua, "LinkedListWideVector", "NodeWideVector");
	LinkedList<WideVector*>::exposeToScript(lua, "LinkedListWideVectorPtr", "NodeWideVectorPtr");
	ArrayList<WideVector>::exposeToScript(lua, "ArrayListWideVector");
	ArrayList<WideVector*>::exposeToScript(lua, "ArrayListWideVectorPtr");
}

void Script::exposeGame() {
	luabridge::getGlobalNamespace(lua)
		.beginClass<Game>("Game")
		.addFunction("getWorld", &Game::getWorld)
		.addFunction("worldForName", &Game::worldForName)
		.addFunction("loadWorld", &Game::loadWorld)
		.addFunction("closeAllWorlds", &Game::closeAllWorlds)
		.endClass()
		;
}

void Script::exposeClient() {
	luabridge::getGlobalNamespace(lua)
		.deriveClass<Client, Game>("Client")
		.addFunction("isConsoleAllowed", &Client::isConsoleAllowed)
		.addFunction("isConsoleActive", &Client::isConsoleActive)
		.addFunction("isEditorActive", &Client::isEditorActive)
		.addFunction("getGUI", &Client::getGUI)
		.addFunction("startEditor", &Client::startEditor)
		.endClass()
		;

	if (client) {
		luabridge::push(lua, client);
		lua_setglobal(lua, "client");
	}
}

void Script::exposeEditor(Editor& _editor) {
	luabridge::getGlobalNamespace(lua)
		.beginClass<Editor>("Editor")
		.addFunction("setEditingMode", &Editor::setEditingMode)
		.addFunction("setCeilingMode", &Editor::setCeilingMode)
		.addFunction("setHighlightedObj", &Editor::setHighlightedObj)
		.addFunction("setWidgetMode", &Editor::setWidgetMode)
		.addFunction("selectEntity", &Editor::selectEntity)
		.addFunction("toggleSelectEntity", &Editor::toggleSelectEntity)
		.addFunction("selectAllEntities", &Editor::selectAllEntities)
		.addFunction("selectEntityForSpawn", &Editor::selectEntityForSpawn)
		.addFunction("entitiesName", &Editor::entitiesName)
		.addFunction("entitiesScript", &Editor::entitiesScript)
		.addFunction("entitiesFlag", &Editor::entitiesFlag)
		.addFunction("entitiesSave", &Editor::entitiesSave)
		.addFunction("entityKeyValueEnter", &Editor::entityKeyValueEnter)
		.addFunction("entityKeyValueRemove", &Editor::entityKeyValueRemove)
		.addFunction("entityKeyValueSelect", &Editor::entityKeyValueSelect)
		.addFunction("entityAddComponent", &Editor::entityAddComponent)
		.addFunction("entityRemoveComponent", &Editor::entityRemoveComponent)
		.addFunction("entityCopyComponent", &Editor::entityCopyComponent)
		.addFunction("entityComponentExpand", &Editor::entityComponentExpand)
		.addFunction("entityComponentCollapse", &Editor::entityComponentCollapse)
		.addFunction("entityComponentName", &Editor::entityComponentName)
		.addFunction("entityComponentTranslate", &Editor::entityComponentTranslate)
		.addFunction("entityComponentRotate", &Editor::entityComponentRotate)
		.addFunction("entityComponentScale", &Editor::entityComponentScale)
		.addFunction("widgetTranslateX", &Editor::widgetTranslateX)
		.addFunction("widgetTranslateY", &Editor::widgetTranslateY)
		.addFunction("widgetTranslateZ", &Editor::widgetTranslateZ)
		.addFunction("widgetRotateYaw", &Editor::widgetRotateYaw)
		.addFunction("widgetRotatePitch", &Editor::widgetRotatePitch)
		.addFunction("widgetRotateRoll", &Editor::widgetRotateRoll)
		.addFunction("widgetScaleX", &Editor::widgetScaleX)
		.addFunction("widgetScaleY", &Editor::widgetScaleY)
		.addFunction("widgetScaleZ", &Editor::widgetScaleZ)
		.addFunction("getEditingMode", &Editor::getEditingMode)
		.addFunction("getHighlightedObj", &Editor::getHighlightedObj)
		.addFunction("getWidgetMode", &Editor::getWidgetMode)
		.addFunction("buttonEntityProperties", &Editor::buttonEntityProperties)
		.addFunction("buttonEntityAddComponent", &Editor::buttonEntityAddComponent)
		.addFunction("buttonNewConfirm", &Editor::buttonNewConfirm)
		.addFunction("buttonNew", &Editor::buttonNew)
		.addFunction("buttonSave", &Editor::buttonSave)
		.addFunction("buttonLoad", &Editor::buttonLoad)
		.addFunction("buttonEditorSettings", &Editor::buttonEditorSettings)
		.addFunction("buttonEditorSettingsApply", &Editor::buttonEditorSettingsApply)
		.addFunction("buttonMapSettings", &Editor::buttonMapSettings)
		.addFunction("buttonMapSettingsApply", &Editor::buttonMapSettingsApply)
		.addFunction("buttonHelp", &Editor::buttonHelp)
		.addFunction("buttonWorldRotate", &Editor::buttonWorldRotate)
		.addFunction("playSound", &Editor::playSound)
		.addFunction("optimizeChunks", &Editor::optimizeChunks)
		.endClass()
		;

	editor = &_editor;
	luabridge::push(lua, editor);
	lua_setglobal(lua, "editor");
}

void Script::exposeServer() {
	luabridge::getGlobalNamespace(lua)
		.deriveClass<Server, Game>("Server")
		.endClass()
		;

	if (server) {
		luabridge::push(lua, server);
		lua_setglobal(lua, "server");
	}
}

void Script::exposeWorld() {
	luabridge::getGlobalNamespace(lua)
		.beginClass<World>("World")
		.addFunction("getTicks", &World::getTicks)
		.addFunction("getFilename", &World::getFilename)
		.addFunction("getShortname", &World::getShortname)
		.addFunction("getNameStr", &World::getNameStr)
		.addFunction("isClientObj", &World::isClientObj)
		.addFunction("isServerObj", &World::isServerObj)
		.addFunction("isShowTools", &World::isShowTools)
		.addFunction("uidToEntity", &World::uidToEntity)
		.addFunction("lineTrace", &World::lineTrace)
		.addFunction("lineTraceList", &World::lineTraceList)
		.addFunction("lineTraceNoEntities", &World::lineTraceNoEntities)
		.addFunction("saveFile", &World::saveFile)
		.addFunction("setShowTools", &World::setShowTools)
		.addFunction("selectEntity", &World::selectEntity)
		.addFunction("selectEntities", &World::selectEntities)
		.addFunction("deselectGeometry", &World::deselectGeometry)
		.addFunction("getEntitiesByName", &World::getEntitiesByName)
		.addFunction("generateDungeon", &World::generateDungeon)
		.addFunction("spawnEntity", &World::spawnEntity)
		.endClass()
		;

	if (world) {
		luabridge::push(lua, world);
		lua_setglobal(lua, "world");
	}

	LinkedList<World*>::exposeToScript(lua, "LinkedListWorldPtr", "NodeWorldPtr");
	ArrayList<World*>::exposeToScript(lua, "ArrayListWorldPtr");
}

void Script::exposeEntity() {
	typedef World* (Entity::*GetWorldFn)();
	GetWorldFn getWorld = static_cast<GetWorldFn>(&Entity::getWorld);

	luabridge::getGlobalNamespace(lua)
		.beginClass<Entity>("Entity")
		.addFunction("getGame", &Entity::getGame)
		.addFunction("getWorld", getWorld)
		.addFunction("getName", &Entity::getName)
		.addFunction("getUID", &Entity::getUID)
		.addFunction("getTicks", &Entity::getTicks)
		.addFunction("getPos", &Entity::getPos)
		.addFunction("getVel", &Entity::getVel)
		.addFunction("getAng", &Entity::getAng)
		.addFunction("getRot", &Entity::getRot)
		.addFunction("getScale", &Entity::getScale)
		.addFunction("getScriptStr", &Entity::getScriptStr)
		.addFunction("isToBeDeleted", &Entity::isToBeDeleted)
		.addFunction("getFlags", &Entity::getFlags)
		.addFunction("setKeyValue", &Entity::setKeyValue)
		.addFunction("deleteKeyValue", &Entity::deleteKeyValue)
		.addFunction("getKeyValueAsString", &Entity::getKeyValueAsString)
		.addFunction("getKeyValueAsFloat", &Entity::getKeyValueAsFloat)
		.addFunction("getKeyValueAsInt", &Entity::getKeyValueAsInt)
		.addFunction("getKeyValueAsBool", &Entity::getKeyValueAsBool)
		.addFunction("getKeyValue", &Entity::getKeyValue)
		.addFunction("isFlag", &Entity::isFlag)
		.addFunction("isFalling", &Entity::isFalling)
		.addFunction("getLastUpdate", &Entity::getLastUpdate)
		.addFunction("getDefName", &Entity::getDefName)
		.addFunction("getSort", &Entity::getSort)
		.addFunction("isSelected", &Entity::isSelected)
		.addFunction("isHighlighted", &Entity::isHighlighted)
		.addFunction("setName", &Entity::setName)
		.addFunction("setFlags", &Entity::setFlags)
		.addFunction("setFlag", &Entity::setFlag)
		.addFunction("resetFlag", &Entity::resetFlag)
		.addFunction("toggleFlag", &Entity::toggleFlag)
		.addFunction("setPos", &Entity::setPos)
		.addFunction("setVel", &Entity::setVel)
		.addFunction("setAng", &Entity::setAng)
		.addFunction("setRot", &Entity::setRot)
		.addFunction("setScale", &Entity::setScale)
		.addFunction("setScriptStr", &Entity::setScriptStr)
		.addFunction("setSelected", &Entity::setSelected)
		.addFunction("setHighlighted", &Entity::setHighlighted)
		.addFunction("setFalling", &Entity::setFalling)
		.addFunction("remove", &Entity::remove)
		.addFunction("animate", &Entity::animate)
		.addFunction("isNearCharacter", &Entity::isNearCharacter)
		.addFunction("isCrouching", &Entity::isCrouching)
		.addFunction("isMoving", &Entity::isMoving)
		.addFunction("hasJumped", &Entity::hasJumped)
		.addFunction("getLookDir", &Entity::getLookDir)
		.addFunction("checkCollision", &Entity::checkCollision)
		.addFunction("copy", &Entity::copy)
		.addFunction("update", &Entity::update)
		.addFunction("hasComponent", &Entity::hasComponent)
		.addFunction("removeComponentByName", &Entity::removeComponentByName)
		.addFunction("removeComponentByUID", &Entity::removeComponentByUID)
		.addFunction("addComponent", &Entity::addComponent<Component>)
		.addFunction("addBBox", &Entity::addComponent<BBox>)
		.addFunction("addModel", &Entity::addComponent<Model>)
		.addFunction("addCamera", &Entity::addComponent<Camera>)
		.addFunction("addLight", &Entity::addComponent<Light>)
		.addFunction("addSpeaker", &Entity::addComponent<Speaker>)
		.addFunction("addCharacter", &Entity::addComponent<Character>)
		.addFunction("addMultimesh", &Entity::addComponent<Multimesh>)
		.addFunction("findComponentByName", &Entity::findComponentByName<Component>)
		.addFunction("findBBoxByName", &Entity::findComponentByName<BBox>)
		.addFunction("findModelByName", &Entity::findComponentByName<Model>)
		.addFunction("findLightByName", &Entity::findComponentByName<Light>)
		.addFunction("findCameraByName", &Entity::findComponentByName<Camera>)
		.addFunction("findSpeakerByName", &Entity::findComponentByName<Speaker>)
		.addFunction("findCharacterByName", &Entity::findComponentByName<Character>)
		.addFunction("findMultimeshByName", &Entity::findComponentByName<Multimesh>)
		.addFunction("lineTrace", &Entity::lineTrace)
		.addFunction("findAPath", &Entity::findAPath)
		.addFunction("findRandomPath", &Entity::findRandomPath)
		.addFunction("pathFinished", &Entity::pathFinished)
		.addFunction("hasPath", &Entity::hasPath)
		.addFunction("getPathNodePosition", &Entity::getPathNodePosition)
		.addFunction("getPathNodeDir", &Entity::getPathNodeDir)
		.addFunction("getCurrentTileX", &Entity::getCurrentTileX)
		.addFunction("getCurrentTileY", &Entity::getCurrentTileY)
		.addFunction("getCurrentTileZ", &Entity::getCurrentTileZ)
		.addFunction("isLocalPlayer", &Entity::isLocalPlayer)
		.addFunction("insertIntoWorld", &Entity::insertIntoWorld)
		.addFunction("warp", &Entity::warp)
		.addFunction("remoteExecute", &Entity::remoteExecute)
		.addFunction("dispatch", &Entity::dispatch)
		.addFunction("isClientObj", &Entity::isClientObj)
		.addFunction("isServerObj", &Entity::isServerObj)
		.addFunction("nearestCeiling", &Entity::nearestCeiling)
		.addFunction("nearestFloor", &Entity::nearestFloor)
		.endClass()
		;

	// expose components
	exposeComponent();
	exposeBBox();
	exposeModel();
	exposeLight();
	exposeCamera();
	exposeSpeaker();
	exposeCharacter();
	exposeMultimesh();

	if (entity) {
		luabridge::push(lua, entity);
		lua_setglobal(lua, "entity");
	}

	LinkedList<Entity*>::exposeToScript(lua, "LinkedListEntityPtr", "NodeEntityPtr");
	ArrayList<Entity*>::exposeToScript(lua, "ArrayListEntityPtr");
}

void Script::exposeComponent() {
	typedef Component* (Component::*GetParentFn)();
	GetParentFn getParent = static_cast<GetParentFn>(&Component::getParent);
	typedef void (Component::*CopyComponentFn)(Component*);
	CopyComponentFn copyToComponent = static_cast<CopyComponentFn>(&Component::copy);
	typedef void (Component::*CopyEntityFn)(Entity*);
	CopyEntityFn copyToEntity = static_cast<CopyEntityFn>(&Component::copy);

	luabridge::getGlobalNamespace(lua)
		.beginClass<Component>("Component")
		.addFunction("getType", &Component::getType)
		.addFunction("getParent", getParent)
		.addFunction("isEditorOnly", &Component::isEditorOnly)
		.addFunction("isUpdateNeeded", &Component::isUpdateNeeded)
		.addFunction("getUID", &Component::getUID)
		.addFunction("getName", &Component::getName)
		.addFunction("getLocalPos", &Component::getLocalPos)
		.addFunction("getLocalAng", &Component::getLocalAng)
		.addFunction("getLocalScale", &Component::getLocalScale)
		.addFunction("getGlobalPos", &Component::getGlobalPos)
		.addFunction("getGlobalAng", &Component::getGlobalAng)
		.addFunction("getGlobalScale", &Component::getGlobalScale)
		.addFunction("setEditorOnly", &Component::setEditorOnly)
		.addFunction("setName", &Component::setName)
		.addFunction("setLocalPos", &Component::setLocalPos)
		.addFunction("setLocalAng", &Component::setLocalAng)
		.addFunction("setLocalScale", &Component::setLocalScale)
		.addFunction("setLocalMat", &Component::setLocalMat)
		.addFunction("update", &Component::update)
		.addFunction("remove", &Component::remove)
		.addFunction("checkCollision", &Component::checkCollision)
		.addFunction("hasComponent", &Component::hasComponent)
		.addFunction("removeComponentByName", &Component::removeComponentByName)
		.addFunction("removeComponentByUID", &Component::removeComponentByUID)
		.addFunction("copyToEntity", copyToEntity)
		.addFunction("copyToComponent", copyToComponent)
		.addFunction("copyComponents", &Component::copyComponents)
		.addFunction("addComponent", &Component::addComponent<Component>)
		.addFunction("addBBox", &Component::addComponent<BBox>)
		.addFunction("addModel", &Component::addComponent<Model>)
		.addFunction("addCamera", &Component::addComponent<Camera>)
		.addFunction("addLight", &Component::addComponent<Light>)
		.addFunction("addSpeaker", &Component::addComponent<Speaker>)
		.addFunction("addCharacter", &Component::addComponent<Character>)
		.addFunction("findComponentByName", &Component::findComponentByName<Component>)
		.addFunction("findBBoxByName", &Component::findComponentByName<BBox>)
		.addFunction("findModelByName", &Component::findComponentByName<Model>)
		.addFunction("findLightByName", &Component::findComponentByName<Light>)
		.addFunction("findCameraByName", &Component::findComponentByName<Camera>)
		.addFunction("findSpeakerByName", &Component::findComponentByName<Speaker>)
		.addFunction("findCharacterByName", &Component::findComponentByName<Character>)
		.addFunction("rotate", &Component::rotate)
		.addFunction("translate", &Component::translate)
		.addFunction("scale", &Component::scale)
		.addFunction("revertRotation", &Component::revertRotation)
		.addFunction("revertTranslation", &Component::revertTranslation)
		.addFunction("revertScale", &Component::revertScale)
		.addFunction("revertToIdentity", &Component::revertToIdentity)
		.addFunction("shootLaser", &Component::shootLaser)
		.addFunction("bindToBone", &Component::bindToBone)
		.addFunction("unbindFromBone", &Component::unbindFromBone)
		.endClass()
		;

	LinkedList<Component*>::exposeToScript(lua, "LinkedListComponentPtr", "NodeComponentPtr");
	ArrayList<Component*>::exposeToScript(lua, "ArrayListComponentPtr");
}

void Script::exposeBBox() {
	luabridge::getGlobalNamespace(lua)
		.deriveClass<BBox, Component>("BBox")
		.addFunction("getShape", &BBox::getShape)
		.addFunction("getMass", &BBox::getMass)
		.addFunction("isEnabled", &BBox::isEnabled)
		.addFunction("setEnabled", &BBox::setEnabled)
		.addFunction("setShape", &BBox::setShape)
		.addFunction("setMass", &BBox::setMass)
		.addFunction("findAllOverlappingEntities", &BBox::findAllOverlappingEntities)
		.addFunction("createRigidBody", &BBox::createRigidBody)
		.endClass()
		;

	LinkedList<BBox*>::exposeToScript(lua, "LinkedListBBoxPtr", "NodeBBoxPtr");
	ArrayList<BBox*>::exposeToScript(lua, "ArrayListBBoxPtr");
}

void Script::exposeModel() {
	luabridge::getGlobalNamespace(lua)
		.deriveClass<Model, Component>("Model")
		.addFunction("findBone", &Model::findBone)
		.addFunction("findBoneIndex", &Model::findBoneIndex)
		.addFunction("findAnimation", &Model::findAnimation)
		.addFunction("animate", &Model::animate)
		.addFunction("hasAnimations", &Model::hasAnimations)
		.addFunction("getMesh", &Model::getMesh)
		.addFunction("getMaterial", &Model::getMaterial)
		.addFunction("getDepthFailMat", &Model::getDepthFailMat)
		.addFunction("getAnimation", &Model::getAnimation)
		.addFunction("getShaderVars", &Model::getShaderVars)
		.addFunction("getAnimTicks", &Model::getAnimTicks)
		.addFunction("isAnimDone", &Model::isAnimDone)
		.addFunction("getAnimationSpeed", &Model::getAnimationSpeed)
		.addFunction("setMesh", &Model::setMesh)
		.addFunction("setMaterial", &Model::setMaterial)
		.addFunction("setDepthFailMat", &Model::setDepthFailMat)
		.addFunction("setAnimation", &Model::setAnimation)
		.addFunction("setShaderVars", &Model::setShaderVars)
		.addFunction("setAnimationSpeed", &Model::setAnimationSpeed)
		.addFunction("updateSkin", &Model::updateSkin)
		.addFunction("getCurrentAnimation", &Model::getCurrentAnimation)
		.addFunction("getPreviousAnimation", &Model::getPreviousAnimation)
		.endClass()
		;

	luabridge::getGlobalNamespace(lua)
		.beginClass<AnimationState>("AnimationState")
		.addFunction("getName", &AnimationState::getName)
		.addFunction("getTicks", &AnimationState::getTicks)
		.addFunction("getTicksRate", &AnimationState::getTicksRate)
		.addFunction("getBegin", &AnimationState::getBegin)
		.addFunction("getEnd", &AnimationState::getEnd)
		.addFunction("getLength", &AnimationState::getLength)
		.addFunction("getWeight", &AnimationState::getWeight)
		.addFunction("getWeightRate", &AnimationState::getWeightRate)
		.addFunction("isLoop", &AnimationState::isLoop)
		.addFunction("isFinished", &AnimationState::isFinished)
		.addFunction("setTicks", &AnimationState::setTicks)
		.addFunction("setTicksRate", &AnimationState::setTicksRate)
		.addFunction("setWeight", &AnimationState::setWeight)
		.addFunction("setWeightRate", &AnimationState::setWeightRate)
		.addFunction("setWeights", &AnimationState::setWeights)
		.addFunction("setWeightRates", &AnimationState::setWeightRates)
		.addFunction("clearWeights", &AnimationState::clearWeights)
		.endClass()
		;

	LinkedList<Model*>::exposeToScript(lua, "LinkedListModelPtr", "NodeModelPtr");
	ArrayList<Model*>::exposeToScript(lua, "ArrayListModelPtr");
}

void Script::exposeLight() {
	luabridge::getGlobalNamespace(lua)
		.deriveClass<Light, Component>("Light")
		.addFunction("getColor", &Light::getColor)
		.addFunction("getIntensity", &Light::getIntensity)
		.addFunction("getRadius", &Light::getRadius)
		.addFunction("getArc", &Light::getArc)
		.addFunction("isShadow", &Light::isShadow)
		.addFunction("setColor", &Light::setColor)
		.addFunction("setIntensity", &Light::setIntensity)
		.addFunction("setRadius", &Light::setRadius)
		.addFunction("setShape", &Light::setShape)
		.addFunction("setArc", &Light::setArc)
		.addFunction("setShadow", &Light::setShadow)
		.endClass()
		;

	LinkedList<Light*>::exposeToScript(lua, "LinkedListLightPtr", "NodeLightPtr");
	ArrayList<Light*>::exposeToScript(lua, "ArrayListLightPtr");
}

void Script::exposeCamera() {
	luabridge::getGlobalNamespace(lua)
		.deriveClass<Camera, Component>("Camera")
		.addFunction("setListener", &Camera::setListener)
		.addFunction("setupProjection", &Camera::setupProjection)
		.addFunction("worldPosToScreenPos", &Camera::worldPosToScreenPos)
		.addFunction("screenPosToWorldRay", &Camera::screenPosToWorldRay)
		.addFunction("getClipNear", &Camera::getClipNear)
		.addFunction("getClipFar", &Camera::getClipFar)
		.addFunction("getWin", &Camera::getWin)
		.addFunction("getFov", &Camera::getFov)
		.addFunction("isOrtho", &Camera::isOrtho)
		.addFunction("setClipNear", &Camera::setClipNear)
		.addFunction("setClipFar", &Camera::setClipFar)
		.addFunction("setWin", &Camera::setWin)
		.addFunction("setFov", &Camera::setFov)
		.addFunction("setOrtho", &Camera::setOrtho)
		.endClass()
		;

	LinkedList<Camera*>::exposeToScript(lua, "LinkedListCameraPtr", "NodeCameraPtr");
	ArrayList<Camera*>::exposeToScript(lua, "ArrayListCameraPtr");
}

void Script::exposeSpeaker() {
	luabridge::getGlobalNamespace(lua)
		.deriveClass<Speaker, Component>("Speaker")
		.addFunction("playSound", &Speaker::playSound)
		.addFunction("stopSound", &Speaker::stopSound)
		.addFunction("stopAllSounds", &Speaker::stopAllSounds)
		.addFunction("getDefaultSound", &Speaker::getDefaultSound)
		.addFunction("getDefaultRange", &Speaker::getDefaultRange)
		.addFunction("isDefaultLoop", &Speaker::isDefaultLoop)
		.addFunction("isPlaying", &Speaker::isPlaying)
		.addFunction("isPlayingAnything", &Speaker::isPlayingAnything)
		.endClass()
		;

	LinkedList<Speaker*>::exposeToScript(lua, "LinkedListSpeakerPtr", "NodeSpeakerPtr");
	ArrayList<Speaker*>::exposeToScript(lua, "ArrayListSpeakerPtr");
}

void Script::exposeCharacter() {
	luabridge::getGlobalNamespace(lua)
		.deriveClass<Character, Component>("Character")
		.addFunction("getHp", &Character::getHp)
		.addFunction("getMp", &Character::getMp)
		.addFunction("getSex", &Character::getSex)
		.addFunction("getLevel", &Character::getLevel)
		.addFunction("getXp", &Character::getXp)
		.addFunction("getHunger", &Character::getHunger)
		.addFunction("getNanoMatter", &Character::getNanoMatter)
		.addFunction("getBioMatter", &Character::getBioMatter)
		.addFunction("getNeuroThread", &Character::getNeuroThread)
		.addFunction("getGold", &Character::getGold)
		.addFunction("getStrength", &Character::getStrength)
		.addFunction("getDexterity", &Character::getDexterity)
		.addFunction("getIntelligence", &Character::getIntelligence)
		.addFunction("getConstitution", &Character::getConstitution)
		.addFunction("getPerception", &Character::getPerception)
		.addFunction("getCharisma", &Character::getCharisma)
		.addFunction("getLuck", &Character::getLuck)
		.addFunction("setHp", &Character::setHp)
		.addFunction("setMp", &Character::setMp)
		.addFunction("setSex", &Character::setSex)
		.addFunction("setLevel", &Character::setLevel)
		.addFunction("setXp", &Character::setXp)
		.addFunction("setHunger", &Character::setHunger)
		.addFunction("setNanoMatter", &Character::setNanoMatter)
		.addFunction("setBioMatter", &Character::setBioMatter)
		.addFunction("setNeuroThread", &Character::setNeuroThread)
		.addFunction("setGold", &Character::setGold)
		.addFunction("setStrength", &Character::setStrength)
		.addFunction("setDexterity", &Character::setDexterity)
		.addFunction("setIntelligence", &Character::setIntelligence)
		.addFunction("setConstitution", &Character::setConstitution)
		.addFunction("setPerception", &Character::setPerception)
		.addFunction("setCharisma", &Character::setCharisma)
		.addFunction("setLuck", &Character::setLuck)
		.endClass()
		;

	LinkedList<Character*>::exposeToScript(lua, "LinkedListCharacterPtr", "NodeCharacterPtr");
	ArrayList<Character*>::exposeToScript(lua, "ArrayListCharacterPtr");
}

void Script::exposeEmitter() {
	// todo
}

void Script::exposeMultimesh() {
	luabridge::getGlobalNamespace(lua)
		.deriveClass<Multimesh, Component>("Multimesh")
		.addFunction("getMaterial", &Model::getMaterial)
		.addFunction("getDepthFailMat", &Model::getDepthFailMat)
		.addFunction("getShaderVars", &Model::getShaderVars)
		.addFunction("setMaterial", &Model::setMaterial)
		.addFunction("setDepthFailMat", &Model::setDepthFailMat)
		.addFunction("setShaderVars", &Model::setShaderVars)
		.endClass()
		;

	LinkedList<Multimesh*>::exposeToScript(lua, "LinkedListMultimeshPtr", "NodeMultimeshPtr");
	ArrayList<Multimesh*>::exposeToScript(lua, "ArrayListMultimeshPtr");
}

void Script::exposeExtra() {
	luabridge::getGlobalNamespace(lua)
		.beginClass<World::hit_t>("Hit")
		.addConstructor<void(*)()>()
		.addData("pos", &World::hit_t::pos, true)
		.addData("normal", &World::hit_t::normal, true)
		.addData("manifest", &World::hit_t::manifest, true)
		.endClass()
		;

	luabridge::getGlobalNamespace(lua)
		.beginClass<World::physics_manifest_t>("PhysicsManifest")
		.addConstructor<void(*)()>()
		.addData("bbox", &World::physics_manifest_t::bbox, true)
		.addData("entity", &World::physics_manifest_t::entity, true)
		.addData("world", &World::physics_manifest_t::world, true)
		.endClass()
		;
}