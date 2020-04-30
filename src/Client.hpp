//! @file Client.hpp

#pragma once

#include "Game.hpp"
#include "Tile.hpp"
#include "Vector.hpp"
#include "Console.hpp"

class Renderer;
class Mixer;
class Script;
class Frame;
class Editor;

//! A Client implements the Game interface and lives in the Engine.
//! In order to play a singleplayer or listening game, the engine instantiates a Client and a Server, meaning there are two Game states running at once.
//! A Server will dictate game updates to a client, but in other respects they are mostly identical.
//! The Client does own a Renderer however.
class Client : public Game {
public:
	Client();
	virtual ~Client();

	typedef LinkedList<Engine::logmsg_t> LogList;

	virtual bool	isServer() const override { return false; }
	virtual bool	isClient() const override { return true; }

	//! starts up the client
	virtual void init() override;

	//! read messages from the net interface
	void handleNetMessages();

	//! perform pre-processing on the current frame
	virtual void preProcess() override;

	//! process the current frame
	virtual void process() override;

	//! perform post-processing on the current frame
	virtual void postProcess() override;

	//! called when we connect to a server
	virtual void onEstablishConnection(Uint32 remoteID) override;

	//! called when we disconnect from a server
	//! @param remoteID always zero, since the server is our only remote host
	virtual void onDisconnect(Uint32 remoteID) override;

	//! spawns a player on the server
	//! @param localID the local number of player to spawn
	void spawn(Uint32 localID);

	//! starts up the level editor
	//! @param path optional path to a level to startup
	void startEditor(const char* path = "");

	//! closes the editor without terminating the client
	void closeEditor();

	Renderer*						getRenderer() { return renderer; }
	Mixer*							getMixer() { return mixer; }
	Frame*							getGUI() { return gui; }
	Editor*							getEditor() { return editor; }
	const bool						isConsoleAllowed() const { return consoleAllowed; }
	const bool						isConsoleActive() const { return consoleActive; }
	const bool						isEditorActive() const { return editor != nullptr; }
	const LogList&					getConsole() const { return console; }

	void	setCuCommand(Node<String>* const node) { cuCommand = node; }
	void	setLogStart(Node<Engine::logmsg_t>* const node) { logStart = node; }

private:
	Script*     script = nullptr; //! script engine
	Renderer*   renderer = nullptr; //! renderer
	Mixer*		mixer = nullptr; //! audio mixer
	Frame*		gui = nullptr; //! active gui
	Editor*		editor = nullptr; //! level editor

	// console variables
	static const int consoleLen = 80;
	bool consoleAllowed = true;
	bool consoleActive = false;
	Sint32 consoleHeight = 0;
	char consoleInput[consoleLen];
	char oldConsoleInput[consoleLen];
	LogList console;
	Node<String>* cuCommand = nullptr;
	Node<Engine::logmsg_t>* logStart = nullptr;

	//! process console input
	void runConsole();
};

extern Cvar cvar_showFPS;
extern Cvar cvar_showSpeed;
extern Cvar cvar_showMatrix;
extern Cvar cvar_showStats;