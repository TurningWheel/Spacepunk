//! @file Engine.hpp

#pragma once

#include "Asset.hpp"
#include "Resource.hpp"
#include "Entity.hpp"
#include "Random.hpp"
#include "Mesh.hpp"
#include "Shader.hpp"
#include "ShaderProgram.hpp"
#include "Image.hpp"
#include "Material.hpp"
#include "Text.hpp"
#include "Sound.hpp"
#include "Input.hpp"
#include "Animation.hpp"
#include "Dictionary.hpp"
#include "Cubemap.hpp"
#include "Font.hpp"

class Server;
class Client;
class FileInterface;

//! The Engine object is the root of the entire engine. Here lives top level data about the current engine state:
//! the local Client and Server (if they are running), functions to step everything forward, update devices, etc.
class Engine {
public:
	Engine(int argc, char **argv);
	~Engine();

	static const int stackTraceSize = 100;
	static const int stackTraceStrSize = 256;

	//! log message codes
	enum msg_t {
		MSG_DEBUG = 0,		//! only printed when NDEBUG is undefined

		MSG_INFO,			//! for when you have an unimportant message to report
		MSG_WARN,			//! for when the game has encountered a small problem
		MSG_ERROR,			//! for when the game has encountered a definite problem
		MSG_CRITICAL,		//! for when the game likely will not continue past this point
		MSG_FATAL,			//! for when you have to exit() immediately

		MSG_NOTE,			//! like info, but more important (not for errors!)
		MSG_CHAT,			//! for when a chat message was received over the net

		MSG_TYPE_LENGTH
	};

	//! a console message
	struct logmsg_t {
		unsigned int uid = 0;
		String text;
		glm::vec3 color = glm::vec3(1.f);
		msg_t kind = MSG_INFO;
	};

	//! a mod
	struct mod_t {
		mod_t(const char* _path);

		String path;
		String name;
		String author;
		bool loaded = false;

		void serialize(FileInterface * file);
	};

	//! default ticks per second
	static const unsigned int defaultTickRate = 60;

	//! amount of ticks within which two clicks register as a "double-click"
	static const unsigned int doubleClickTime = 30;

	bool								isInitialized() const { return initialized; }
	bool								isRunning() const { return running; }
	bool								isPaused() const { return paused; }
	bool								isFullscreen() const { return fullscreen; }
	bool								isRunningClient() const { return runningClient; }
	bool								isRunningServer() const { return runningServer; }
	const char*							getGameTitle() const { return game.name.get(); }
	Client*								getLocalClient() { return localClient; }
	Server*								getLocalServer() { return localServer; }
	int									getXres() const { return xres; }
	int									getYres() const { return yres; }
	bool								getKeyStatus(const int index) const { return keystatus[index]; }
	bool								getAnyKeyStatus() const { return anykeystatus; }
	const char*							getLastInputOfAnyKind() const { return lastInputOfAnyKind.get(); }
	const char*							getLastKeyPressed() const { return lastkeypressed; }
	bool								getMouseStatus(const int index) const { return mousestatus[index]; }
	bool								getDBCMouseStatus(const int index) const { return dbc_mousestatus[index]; }
	Sint32								getMouseX() const { return mousex; }
	Sint32								getMouseY() const { return mousey; }
	Sint32								getOldMouseX() const { return omousex; }
	Sint32								getOldMouseY() const { return omousey; }
	Sint32								getMouseWheelX() const { return mousewheelx; }
	Sint32								getMouseWheelY() const { return mousewheely; }
	Sint32								getMouseMoveX() const { return mousexrel; }
	Sint32								getMouseMoveY() const { return mouseyrel; }
	double								getFPS() const { return fps; }
	double								getTimeSync() const { return timesync; }
	Uint32								getTicks() const { return ticks; }
	int									getTicksPerSecond() const { return (int)ticksPerSecond; }
	auto&								getFontResource() { return *static_cast<Resource<Font, false>*>(*resources.find("font")); }
	auto&								getMeshResource() { return *static_cast<Resource<Mesh, true>*>(*resources.find("mesh")); }
	auto&								getImageResource() { return *static_cast<Resource<Image, true>*>(*resources.find("image")); }
	auto&								getMaterialResource() { return *static_cast<Resource<Material, false>*>(*resources.find("material")); }
	auto&								getTextResource() { return *static_cast<Resource<Text, false>*>(*resources.find("text")); }
	auto&								getSoundResource() { return *static_cast<Resource<Sound, false>*>(*resources.find("sound")); }
	auto&								getAnimationResource() { return *static_cast<Resource<Animation, false>*>(*resources.find("animation")); }
	auto&								getCubemapResource() { return *static_cast<Resource<Cubemap, false>*>(*resources.find("cubemap")); }
	const LinkedList<Entity::def_t*>&	getEntityDefs() { return entityDefs; }
	LinkedList<String>&					getCommandHistory() { return commandHistory; }
	const char*							getInputStr() { return inputstr; }
	bool								isCursorVisible() const { return (ticks - cursorflash) % ticksPerSecond < ticksPerSecond / 2; }
	bool								isMouseRelative() const { return mouseRelative; }
	bool								isKillSignal() const { return killSignal; }
	Random&								getRandom() { return rand; }
	const char*							getLastInput() const { return lastInput; }
	LinkedList<SDL_GameController*>&	getControllers() { return controllers; }
	LinkedList<SDL_Joystick*>&			getJoysticks() { return joysticks; }
	Input&								getInput(int index) { return inputs[index]; }
	bool								isPlayTest() const { return playTest; }

	void								setPaused(const bool _paused) { paused = _paused; }
	void								setInputStr(char* const _inputstr) { inputstr = _inputstr; inputnum = false; }
	void								setInputLen(const int _inputlen) { inputlen = _inputlen; }
	void								setInputNumbersOnly(const bool _inputnum) { inputnum = _inputnum; }
	void								setMouseRelative(const bool _mouseRelative) { mouseRelative = _mouseRelative; }
	void								setKillSignal(const bool _killSignal) { killSignal = _killSignal; }
	void								setFullscreen(const bool _b) { fullscreen = _b; }
	void								setXres(const Sint32 i) { xres = i; }
	void								setYres(const Sint32 i) { yres = i; }
	void								setPlayTest(const bool b) { playTest = b; }
	void								setConsoleSleep(Uint32 i) { consoleSleep = i; }

	//! get the directory the game is running in
	const char* getRunningDir();

	//! initialize the engine
	void init();

	//! loads all game resources
	void loadAllResources();

	//! loads resources from a particular mod / game folder
	void loadResources(const char* folder);

	//! load entity defs
	void loadAllDefs();

	//! load entity defs from a particular mod / game folder
	void loadDefs(const char* folder);

	//! clears all resource caches, effectively starting the engine "fresh"
	//! this does NOT unmount mods! It simply causes the engine to recache any loaded resources
	//! @param type if not nullptr, tries to clear a specific resource cache by name
	void dumpResources(const char* type);

	//! shuts down any active games and starts the editor
	//! @param path optional path to a level to startup
	void startEditor(const char* path = "");

	//! start a new process to playtest the current level
	void editorPlaytest();

	//! shuts down the active server and starts a new one
	void startServer();

	//! joins the given server
	//! @param address the ip address (and port) of the server
	void joinServer(const char* address);

	//! sends a chat message
	//! @param msg the message to relay
	void chat(const char* msg);

	//! @return true if the editor is currently running
	bool isEditorRunning();

	//! @return the current vsync mode as a string
	const char* getVsyncMode() const;

	//! plays a sound file
	//! @param name the filename of the sound to play
	void playSound(const char* name);

	//! game version string
	const char* version();

	//! parse command line arguments
	//! @param argc the number of arguments to execute
	//! @param argv array of char arrays (args)
	void commandLine(const int argc, const char **argv);

	//! parse a single console command
	//! @param arg the command to process
	void doCommand(const char* arg);

	//! get the value of the given cvar
	//! @param name the name of the cvar
	//! @return the value of the cvar as a string
	const char* getCvar(const char* name);

	//! load and execute a config file
	//! @param filename filename of the config file to load (NOT including mod folder)
	//! @return 0 on success, non-zero on error
	int loadConfig(const char* filename);

	//! write a config file
	//! @param filename filename of the config file to write (including mod folder!)
	//! @param cvars the specific cvars to write to the config file, or a list with one element ("*") to represent ALL cvars
	//! @param ccmds an array of specific console commands to write to the config file
	//! @return 0 on success, non-zero on error
	int saveConfig(const char* filename, const ArrayList<String>& cvars, const ArrayList<String>& ccmds);

	//! create a system dialog for opening files (blocking)
	//! @param filterList list of file extensions (eg "txt,exe")
	//! @param defaultPath default path to open in (or nullptr for system default)
	//! @return file path on success, empty string on cancel/error
	String fileOpenDialog(const char* filterList, const char* defaultPath);

	//! create a system dialog for saving files (blocking)
	//! @param filterList list of file extensions (eg "txt,exe")
	//! @param defaultPath default path to open in (or nullptr for system default)
	//! @return file path on success, empty string on cancel/error
	String fileSaveDialog(const char* filterList, const char* defaultPath);

	//! copy the contents of the engine log to another one
	//! @param dest the destination log to copy to
	//! @return true if the dest was cleared in the process
	bool copyLog(LinkedList<Engine::logmsg_t>& dest);

	//! clears the log
	void clearLog();

	//! shutdown the engine safely (perhaps from another class)
	void shutdown();

	//! logs a formatted char string to the console
	//! @param msgType the type of message to send to the console
	//! @param fmt a formatted string to print to the console
	void fmsg(const Uint32 msgType, const char* fmt, ...);

	//! logs a String to the console
	//! @param msgType the type of message to send to the console
	//! @param str a String to print to the console
	void smsg(const Uint32 msgType, const String& str);

	//! logs a char string to the console
	//! @param msgType the type of message to send to the console
	//! @param str a char string to print to the console
	void msg(const Uint32 msgType, const char* str);

	//! find the index of the entity def with the given name
	//! @param name the name of the entity def to look for
	//! @return the index of the entity with the given name, or UINT32_MAX if not found
	Uint32 findEntityDefIndexByName(const char* name);

	//! reads data from a file stream and outputs errors to the log
	//! @param ptr a pointer to the variable in which to store the data read
	//! @param size the size of each data element to read
	//! @param count the number of data elements to read
	//! @param stream the file stream to read the data from
	//! @param filename the filename of the file that is being read from
	//! @param funcName the name of the function wherein the file is being read
	static void freadl(void* ptr, Uint32 size, Uint32 count, FILE* stream, const char* filename, const char* funcName);

	//! reads multiple ints from a character string
	//! @param str the string to read the space-separated character-encoded ints from
	//! @param arr an array to store each of the ints in
	//! @param numToRead the number of ints to read from str
	//! @return the number of ints that were successfully read from str
	static int readInt(const char* str, int* arr, int numToRead);

	//! reads multiple floats from a character string
	//! @param str the string to read the space-separated character-encoded floats from
	//! @param arr an array to store each of the floats in
	//! @param numToRead the number of floats to read from str
	//! @return the number of floats that were successfully read from str
	static int readFloat(const char* str, float* arr, int numToRead);

	//! determines if some text is anything but a number
	//! @param arr the array of characters to check for letters
	//! @param len the length of the array
	//! @return true if the character array has non-numeral characters, false otherwise
	static bool charsHaveLetters(const char* arr, Uint32 len);

	//! does string comparison (helper function for lua)
	//! @param a the first string
	//! @param b the second string
	//! @return the result of the string comparison
	static int strCompare(const char* a, const char* b);

	//! finds the intersection between a point and a plane in 3D
	//! @param lineStart the start of the line segment
	//! @param lineEnd the end of the line segment
	//! @param planeOrigin where the plane is "centered" in 3D
	//! @param planeNormal the surface normal of the plane
	//! @param outIntersection the point of intersection between the line and the plane
	//! @return true if the line intersects with the plane, false otherwise
	static bool lineIntersectPlane(const Vector& lineStart, const Vector& lineEnd, const Vector& planeOrigin, const Vector& planeNormal, Vector& outIntersection);

	//! determines where in the triangle defined by a, b, c, that the point p lies (does not use Z coord)
	//! @param a point A on the triangle
	//! @param b point B on the triangle
	//! @param c point C on the triangle
	//! @param p the point to test
	//! @return a pair of coordinates that define where the point is in the triangle
	static Vector triangleCoords(const Vector& a, const Vector& b, const Vector& c, const Vector& p);

	//! determines if the given point p lies in the triangle defined by a, b, c (does not use Z coord)
	//! @param a point A on the triangle
	//! @param b point B on the triangle
	//! @param c point C on the triangle
	//! @param p the point to test
	//! @return true if the point is in the triangle, otherwise false
	static bool pointInTriangle(const Vector& a, const Vector& b, const Vector& c, const Vector& p);

	//! determines if one triangle lies inside of another (two-dimensional)
	//! @param a0 point A on first triangle (container)
	//! @param b0 point B on first triangle (container)
	//! @param c0 point C on first triangle (container)
	//! @param a1 point A on second triangle
	//! @param b1 point B on second triangle
	//! @param c1 point C on second triangle
	//! @return true if the second triangle is in the first one, otherwise false
	static bool triangleOverlapsTriangle(const Vector& a0, const Vector& b0, const Vector& c0, const Vector& a1, const Vector& b1, const Vector& c1);

	//! measure the distance squared between a point and a 3D bounding box
	//! @param point the point to measure from
	//! @param boundsMin min coords of the bounding box
	//! @param boundsMax max coords of the bounding box
	//! @return the distance squared
	static float measurePointToBounds(const Vector& point, const Vector& boundsMin, const Vector& boundsMax);

	//! perform pre-processing on the current frame
	void preProcess();

	//! process the current frame
	void process();

	//! perform post-processing on the current frame
	void postProcess();

	//! gather a list of available display modes
	//! @return a list of display modes
	ArrayList<String> getDisplayModes() const;

	//! generate a stack trace
	//! @return a list of strings representing stack frames
	ArrayList<StringBuf<Engine::stackTraceStrSize>> stackTrace() const;

	//! return the value of a key and reset it if it's been pressed
	//! !IMPORTANT! should generally NOT be used, as it "locks" the key for the rest of the frame after being pressed
	//! use a local variable to record the press for your object instead!
	//! @param index the key scancode
	//! @return true if the key has been pressed, false otherwise
	const bool pressKey(const int index) { if (keystatus[index]) { keystatus[index] = false; return true; } else { return false; } }

	//! return the value of a mouse button and reset it if it's been clicked
	//! !IMPORTANT! should generally NOT be used, as it "locks" the button for the rest of the frame after being pressed
	//! use a local variable to record the press for your object instead!
	//! @param index the button number
	//! @return true if the button has been clicked, false otherwise
	const bool pressMouse(const int index) { if (mousestatus[index]) { mousestatus[index] = false; return true; } else { return false; } }

	//! takes a full system path to a local asset and attempts to cut it down to a virtual path
	//! @param path the full filename
	String shortenPath(const char* path) const;

	//! gets the virtual file path to an asset and returns its full path as a string
	//! @param path the path to the asset without mod or base folders
	//! @return the complete path string
	String buildPath(const char* path) const;

	//! add a mod to the game
	//! @param name the name of the mod folder to add
	//! @return true if the mod was added, false otherwise
	bool addMod(const char* name);

	//! remove a mod from the game
	//! @param name the name of the mod folder to remove
	//! @return true if the mod was removed, false otherwise
	bool removeMod(const char* name);

	//! generate a random number
	//! @return a random 32-bit number
	Uint32 random();

	//! print cache sizes to show memory consumption
	void printCacheSize() const;

private:
	static const char* msgTypeStr[MSG_TYPE_LENGTH];

	//! shuts down the engine
	void term();

	//! handles segfault
	static void handleSIGSEGV(int input);

	//! general data
	bool playTest = false;
	String combinedVersion;
	bool initialized = false;
	bool running = true;
	SDL_Event event;
	static std::atomic_bool paused;
	unsigned int ticksPerSecond = defaultTickRate;
	String runDir;

	//! mod data
	mod_t game;
	LinkedList<mod_t> mods;

	//! log data
	FILE *logFile = nullptr;
	LinkedList<logmsg_t> logList;
	LinkedList<String> commandHistory;
	unsigned int logUids = 0;
	SDL_mutex* logLock = nullptr;
	bool logging = false;

	//! local client and server data
	bool runningClient = true;
	bool runningServer = false;
	Client* localClient = nullptr;
	Server* localServer = nullptr;

	//! resource caches
	Map<StringBuf<32>, ResourceBase*> resources;

	//! entity definitions
	LinkedList<Entity::def_t*> entityDefs;

	//! random number generator
	Random rand;

	//! video data (startup settings)
	bool fullscreen = false;
	Sint32 xres = 1280;
	Sint32 yres = 720;

	//! timing data
	double fps = 0, timesync = 0, t = 0, ot = 0;
	static const int fpsAverage = 32;
	double frameval[fpsAverage];
	Uint32 ticks = 0, cycles = 0, lastfpscount = 0;
	bool executedFrames = false;
	std::chrono::time_point<std::chrono::steady_clock> lastTick;

	//! console data
	Uint32 consoleSleep = 0;
	LinkedList<String> ccmdsToRun;

	//! input data
	StringBuf<64> lastInputOfAnyKind;
	bool inputAllowed = true;
	const char* lastkeypressed = nullptr;
	char lastInput[SDL_TEXTINPUTEVENT_TEXT_SIZE] = { 0 };
	bool keystatus[SDL_NUM_SCANCODES];
	bool anykeystatus = false;
	char keypressed = 0;
	bool mousestatus[8];
	bool dbc_mousestatus[8];
	Uint32 mouseClickTime = 0;
	bool mouseRelative = false;
	Sint32 mousex = 0, mousey = 0;
	Sint32 omousex = 0, omousey = 0;
	Sint32 mousexrel = 0, mouseyrel = 0;
	Sint32 mousewheelx = 0, mousewheely = 0;
	char *inputstr = nullptr; //!< text input buffer
	int inputlen = 0; //!< length of the text input buffer
	bool inputnum = false; //!< if true, the text input is for numbers only
	Uint32 cursorflash = 0;
	bool killSignal = false; //!< if true, engine received a quit signal from the OS
	LinkedList<SDL_GameController*> controllers;
	LinkedList<SDL_Joystick*> joysticks;
	Input inputs[4];

	//! sound data
	int audio_rate = 44100;
	Uint16 audio_format = AUDIO_S16;
	int audio_channels = 1;
	int audio_buffers = 512;

	//! load a map on the server
	//! @param path the pathname of the world to load
	void loadMapServer(const char* path);

	//! load a map on the client
	//! @param path the pathname of the world to load
	void loadMapClient(const char* path);
};