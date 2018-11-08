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
#include "Angle.hpp"
#include "Editor.hpp"
#include "Path.hpp"
#include "AnimationState.hpp"

//Component headers
#include "Component.hpp"
#include "BBox.hpp"
#include "Model.hpp"
#include "Light.hpp"
#include "Camera.hpp"
#include "Speaker.hpp"
#include "Character.hpp"

int Script::load(const char* _filename) {
	filename = mainEngine->buildPath(_filename);

	int result = luaL_dofile(lua, filename.get());
	if( result ) {
		mainEngine->fmsg(Engine::MSG_ERROR,"failed to load script '%s':", filename.get());
		mainEngine->fmsg(Engine::MSG_ERROR," %s", lua_tostring(lua, -1));
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

	size_t numArgs = 0;
	if (args)
	{
		numArgs = args->getSize();
		args->push(lua);
	}

	int status = lua_pcall(lua, (int)numArgs, 0, 0);
	if (status) {
		mainEngine->fmsg(Engine::MSG_ERROR,"script error in '%s' (dispatch '%s'):", filename.get(), function);
		mainEngine->fmsg(Engine::MSG_ERROR," %s", lua_tostring(lua, -1));
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
	exposeClient();
}

Script::Script(Server& _server) {
	server = &_server;
	engine = mainEngine;

	lua = luaL_newstate();
	luaL_openlibs(lua);

	// expose functions
	exposeEngine();
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
}

Script::Script(Entity& _entity) {
	entity = &_entity;
	engine = mainEngine;

	lua = luaL_newstate();
	luaL_openlibs(lua);

	// expose functions
	exposeEngine();
	exposeAngle();
	exposeVector();
	exposeEntity();
}

Script::Script(Frame& _frame) {
	frame = &_frame;
	engine = mainEngine;
	client = engine->getLocalClient();

	lua = luaL_newstate();
	luaL_openlibs(lua);

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
	if ( lua ) {
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
		.addFunction("shutdown", &Engine::shutdown)
		.addFunction("editorPlaytest", &Engine::editorPlaytest)
		.addFunction("smsg", &Engine::smsg)
		.addFunction("msg", &Engine::msg)
		.endClass()
	;

	luabridge::getGlobalNamespace(lua)
		.beginClass<glm::mat4>("matrix4x4")
		.endClass()
	;

	if( engine ) {
		luabridge::push(lua, engine);
		lua_setglobal(lua, "engine");
	}

	exposeString();
}

void Script::exposeFrame() {
	luabridge::getGlobalNamespace(lua)
		.beginClass<Frame>("Frame")
		.addFunction("getBorder", &Frame::getBorder)
		.addFunction("getSize", &Frame::getSize)
		.addFunction("getActualSize", &Frame::getActualSize)
		.addFunction("isHigh", &Frame::isHigh)
		.addFunction("getFrames", &Frame::getFrames)
		.addFunction("getButtons", &Frame::getButtons)
		.addFunction("setBorder", &Frame::setBorder)
		.addFunction("setSize", &Frame::setSize)
		.addFunction("setActualSize", &Frame::setActualSize)
		.addFunction("setHigh", &Frame::setHigh)
		.addFunction("setColor", &Frame::setColor)
		.addFunction("addFrame", &Frame::addFrame)
		.addFunction("addButton", &Frame::addButton)
		.addFunction("addField", &Frame::addField)
		.addFunction("addImage", &Frame::addImage)
		.addFunction("addEntry", &Frame::addEntry)
		.addFunction("clear", &Frame::clear)
		.addFunction("removeSelf", &Frame::removeSelf)
		.addFunction("remove", &Frame::remove)
		.addFunction("removeEntry", &Frame::removeEntry)
		.addFunction("findEntry", &Frame::findEntry)
		.addFunction("findFrame", &Frame::findFrame)
		.endClass()
	;

	if( frame ) {
		luabridge::push(lua, frame);
		lua_setglobal(lua, "frame");
	}
}

void Script::exposeString() {
	typedef size_t (String::*FindFn)(const char*, size_t) const;
	FindFn find = static_cast<FindFn>(&String::find);

	typedef size_t (String::*FindCharFn)(const char, size_t) const;
	FindCharFn findChar = static_cast<FindCharFn>(&String::find);

	luabridge::getGlobalNamespace(lua)
		.beginClass<String>("String")
		.addConstructor<void (*) (const char*)>()
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
		.beginClass<Angle>("Angle")
		.addConstructor<void (*) (float, float, float)>()
		.addData("yaw", &Angle::yaw, true)
		.addData("pitch", &Angle::pitch, true)
		.addData("roll", &Angle::roll, true)
		.addFunction("radiansYaw", &Angle::radiansYaw)
		.addFunction("radiansPitch", &Angle::radiansPitch)
		.addFunction("radiansRoll", &Angle::radiansRoll)
		.addFunction("degreesYaw", &Angle::degreesYaw)
		.addFunction("degreesPitch", &Angle::degreesPitch)
		.addFunction("degreesRoll", &Angle::degreesRoll)
		.addFunction("wrapAngles", &Angle::wrapAngles)
		.addFunction("toVector", &Angle::toVector)
		.endClass()
	;

	LinkedList<Angle>::exposeToScript(lua, "LinkedListAngle", "NodeAngle");
	LinkedList<Angle*>::exposeToScript(lua, "LinkedListAnglePtr", "NodeAnglePtr");
	ArrayList<Angle>::exposeToScript(lua, "ArrayListAngle");
	ArrayList<Angle*>::exposeToScript(lua, "ArrayListAnglePtr");
}

void Script::exposeVector() {
	luabridge::getGlobalNamespace(lua)
		.beginClass<Vector>("Vector")
		.addConstructor<void (*) (float, float, float)>()
		.addData("x", &Vector::x, true)
		.addData("y", &Vector::y, true)
		.addData("z", &Vector::z, true)
		.addFunction("hasVolume", &Vector::hasVolume)
		.addFunction("dot", &Vector::dot)
		.addFunction("cross", &Vector::cross)
		.addFunction("length", &Vector::length)
		.addFunction("lengthSquared", &Vector::lengthSquared)
		.addFunction("normal", &Vector::normal)
		.addFunction("normalize", &Vector::normalize)
		.endClass()
	;

	LinkedList<Vector>::exposeToScript(lua, "LinkedListVector", "NodeVector");
	LinkedList<Vector*>::exposeToScript(lua, "LinkedListVectorPtr", "NodeVectorPtr");
	ArrayList<Vector>::exposeToScript(lua, "ArrayListVector");
	ArrayList<Vector*>::exposeToScript(lua, "ArrayListVectorPtr");
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
	exposeGame();

	luabridge::getGlobalNamespace(lua)
		.deriveClass<Client, Game>("Client")
		.addFunction("isConsoleAllowed", &Client::isConsoleAllowed)
		.addFunction("isConsoleActive", &Client::isConsoleActive)
		.addFunction("isEditorActive", &Client::isEditorActive)
		.addFunction("getGUI", &Client::getGUI)
		.addFunction("startEditor", &Client::startEditor)
		.endClass()
	;

	if( client ) {
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
		.addFunction("entityBBoxShape", &Editor::entityBBoxShape)
		.addFunction("entityBBoxEnabled", &Editor::entityBBoxEnabled)
		.addFunction("entityModelLoadMesh", &Editor::entityModelLoadMesh)
		.addFunction("entityModelLoadMaterial", &Editor::entityModelLoadMaterial)
		.addFunction("entityModelLoadDepthFailMat", &Editor::entityModelLoadDepthFailMat)
		.addFunction("entityModelLoadAnimation", &Editor::entityModelLoadAnimation)
		.addFunction("entityModelCustomColor", &Editor::entityModelCustomColor)
		.addFunction("entityModelCustomColorChannel", &Editor::entityModelCustomColorChannel)
		.addFunction("entityLightColorR", &Editor::entityLightColorR)
		.addFunction("entityLightColorG", &Editor::entityLightColorG)
		.addFunction("entityLightColorB", &Editor::entityLightColorB)
		.addFunction("entityLightIntensity", &Editor::entityLightIntensity)
		.addFunction("entityLightRadius", &Editor::entityLightRadius)
		.addFunction("entityLightShape", &Editor::entityLightShape)
		.addFunction("entityCameraClipNear", &Editor::entityCameraClipNear)
		.addFunction("entityCameraClipFar", &Editor::entityCameraClipFar)
		.addFunction("entityCameraWinX", &Editor::entityCameraWinX)
		.addFunction("entityCameraWinY", &Editor::entityCameraWinY)
		.addFunction("entityCameraWinW", &Editor::entityCameraWinW)
		.addFunction("entityCameraWinH", &Editor::entityCameraWinH)
		.addFunction("entityCameraFOV", &Editor::entityCameraFOV)
		.addFunction("entityCameraOrtho", &Editor::entityCameraOrtho)
		.addFunction("entitySpeakerDefaultSound", &Editor::entitySpeakerDefaultSound)
		.addFunction("entitySpeakerDefaultRange", &Editor::entitySpeakerDefaultRange)
		.addFunction("entitySpeakerDefaultLoop", &Editor::entitySpeakerDefaultLoop)
		.addFunction("entityCharacterHp", &Editor::entityCharacterHp)
		.addFunction("entityCharacterMp", &Editor::entityCharacterMp)
		.addFunction("entityCharacterSex", &Editor::entityCharacterSex)
		.addFunction("entityCharacterLevel", &Editor::entityCharacterLevel)
		.addFunction("entityCharacterXp", &Editor::entityCharacterXp)
		.addFunction("entityCharacterHunger", &Editor::entityCharacterHunger)
		.addFunction("entityCharacterNanoMatter", &Editor::entityCharacterNanoMatter)
		.addFunction("entityCharacterBioMatter", &Editor::entityCharacterBioMatter)
		.addFunction("entityCharacterNeuroThread", &Editor::entityCharacterNeuroThread)
		.addFunction("entityCharacterGold", &Editor::entityCharacterGold)
		.addFunction("entityCharacterStrength", &Editor::entityCharacterStrength)
		.addFunction("entityCharacterDexterity", &Editor::entityCharacterDexterity)
		.addFunction("entityCharacterIntelligence", &Editor::entityCharacterIntelligence)
		.addFunction("entityCharacterConstitution", &Editor::entityCharacterConstitution)
		.addFunction("entityCharacterPerception", &Editor::entityCharacterPerception)
		.addFunction("entityCharacterCharisma", &Editor::entityCharacterCharisma)
		.addFunction("entityCharacterLuck", &Editor::entityCharacterLuck)
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
	exposeGame();

	luabridge::getGlobalNamespace(lua)
		.deriveClass<Server, Game>("Server")
		.endClass()
	;

	if( server ) {
		luabridge::push(lua, server);
		lua_setglobal(lua, "server");
	}
}

void Script::exposeWorld() {
	luabridge::getGlobalNamespace(lua)
		.beginClass<World>("World")
		.addFunction("getTicks", &World::getTicks)
		.addFunction("getFilename", &World::getFilename)
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
		.endClass()
	;

	if( world ) {
		luabridge::push(lua, world);
		lua_setglobal(lua, "world");
	}

	LinkedList<World*>::exposeToScript(lua, "LinkedListWorldPtr", "NodeWorldPtr");
	ArrayList<World*>::exposeToScript(lua, "ArrayListWorldPtr");
}

void Script::exposeEntity() {
	luabridge::getGlobalNamespace(lua)
		.beginClass<Entity>("Entity")
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
		.addFunction("findComponentByName", &Entity::findComponentByName<Component>)
		.addFunction("findBBoxByName", &Entity::findComponentByName<BBox>)
		.addFunction("findModelByName", &Entity::findComponentByName<Model>)
		.addFunction("findLightByName", &Entity::findComponentByName<Light>)
		.addFunction("findCameraByName", &Entity::findComponentByName<Camera>)
		.addFunction("findSpeakerByName", &Entity::findComponentByName<Speaker>)
		.addFunction("findCharacterByName", &Entity::findComponentByName<Character>)
		.addFunction("lineTrace", &Entity::lineTrace)
		.addFunction("findAPath", &Entity::findAPath)
		.addFunction("pathFinished", &Entity::pathFinished)
		.addFunction("getCurrentTileX", &Entity::getCurrentTileX)
		.addFunction("getCurrentTileY", &Entity::getCurrentTileY)
		.addFunction("getCurrentTileZ", &Entity::getCurrentTileZ)
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

	if( entity ) {
		luabridge::push(lua, entity);
		lua_setglobal(lua, "entity");
	}

	LinkedList<Entity*>::exposeToScript(lua, "LinkedListEntityPtr", "NodeEntityPtr");
	ArrayList<Entity*>::exposeToScript(lua, "ArrayListEntityPtr");
}

void Script::exposeComponent() {
	luabridge::getGlobalNamespace(lua)
		.beginClass<Component>("Component")
		.addFunction("getType", &Component::getType)
		.addFunction("getParent", &Component::getParent)
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
		.endClass()
	;

	LinkedList<Component*>::exposeToScript(lua, "LinkedListComponentPtr", "NodeComponentPtr");
	ArrayList<Component*>::exposeToScript(lua, "ArrayListComponentPtr");
}

void Script::exposeBBox() {
	luabridge::getGlobalNamespace(lua)
		.deriveClass<BBox, Component>("BBox")
		.addFunction("containsPoint", &BBox::containsPoint)
		.addFunction("nearestCeiling", &BBox::nearestCeiling)
		.addFunction("nearestFloor", &BBox::nearestFloor)
		.addFunction("distToCeiling", &BBox::distToCeiling)
		.addFunction("distToFloor", &BBox::distToFloor)
		.addFunction("getShape", &BBox::getShape)
		.addFunction("isEnabled", &BBox::isEnabled)
		.addFunction("setEnabled", &BBox::setEnabled)
		.addFunction("setShape", &BBox::setShape)
		.addFunction("findAllOverlappingEntities", &BBox::findAllOverlappingEntities)
		.endClass()
	;

	LinkedList<BBox*>::exposeToScript(lua, "LinkedListBBoxPtr", "NodeBBoxPtr");
	ArrayList<BBox*>::exposeToScript(lua, "ArrayListBBoxPtr");
}

void Script::exposeModel() {
	luabridge::getGlobalNamespace(lua)
		.deriveClass<Model, Component>("Model")
		.addFunction("findBone", &Model::findBone)
		.addFunction("findAnimation", &Model::findAnimation)
		.addFunction("animate", &Model::animate)
		.addFunction("hasAnimations", &Model::hasAnimations)
		.addFunction("getMesh", &Model::getMesh)
		.addFunction("getMaterial", &Model::getMaterial)
		.addFunction("getDepthFailMat", &Model::getDepthFailMat)
		.addFunction("getAnimation", &Model::getAnimation)
		.addFunction("getShaderVars", &Model::getShaderVars)
		.addFunction("getAnimName", &Model::getAnimName)
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
		.endClass()
	;

	luabridge::getGlobalNamespace(lua)
		.beginClass<Model::bone_t>("bone_t")
		.addData("valid", &Model::bone_t::valid, true)
		.addData("name", &Model::bone_t::name, true)
		.addData("mat", &Model::bone_t::mat, true)
		.addData("pos", &Model::bone_t::pos, true)
		.addData("ang", &Model::bone_t::ang, true)
		.addData("scale", &Model::bone_t::scale, true)
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
		.addFunction("setColor", &Light::setColor)
		.addFunction("setIntensity", &Light::setIntensity)
		.addFunction("setRadius", &Light::setRadius)
		.addFunction("setShape", &Light::setShape)
		.endClass()
	;

	LinkedList<Light*>::exposeToScript(lua, "LinkedListLightPtr", "NodeLightPtr");
	ArrayList<Light*>::exposeToScript(lua, "ArrayListLightPtr");
}

void Script::exposeCamera() {
	luabridge::getGlobalNamespace(lua)
		.deriveClass<Camera, Component>("Camera")
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
