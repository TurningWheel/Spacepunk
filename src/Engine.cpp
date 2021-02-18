// Engine.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Renderer.hpp"
#include "Directory.hpp"
#include "World.hpp"
#include "Console.hpp"
#include "Editor.hpp"
#include "Mixer.hpp"

#ifdef PLATFORM_WINDOWS
#include <DbgHelp.h>
#endif

//! native file dialog
#include <nfd/nfd.h>

std::atomic_bool Engine::paused(false);

// log message code strings
const char* Engine::msgTypeStr[Engine::MSG_TYPE_LENGTH] = {
	"DEBUG",
	"INFO",
	"WARN",
	"ERROR",
	"CRITICAL",
	"FATAL",
	"NOTE",
	"CHAT"
};

Engine::Engine(int argc, char **argv) :
	game("base")
{
	for (int c = 0; c < 256; ++c) {
		keystatus[c] = false;
	}
	for (int c = 0; c < 8; ++c) {
		mousestatus[c] = false;
	}
	for (int c = 0; c < 32; ++c) {
		frameval[c] = 0;
	}

	// open log file
	logLock = SDL_CreateMutex();
	if (!logFile)
		logFile = freopen("log.txt", "wb" /*or "wt"*/, stderr);
	fmsg(Engine::MSG_INFO, "hello.");

	// read command line
	commandLine((const int)argc, (const char **)argv);
}

Engine::~Engine() {
	term();
}

static int console_clear(int argc, const char** argv) {
	mainEngine->getTextResource().dumpCache();
	mainEngine->clearLog();
	return 0;
}

static int console_windowed(int argc, const char** argv) {
	mainEngine->setFullscreen(false);

	// update renderer settings, if running
	Client* client = mainEngine->getLocalClient();
	if (mainEngine->isInitialized() && client) {
		Renderer* renderer = client->getRenderer();
		if (renderer) {
			renderer->setFullscreen(false);
			renderer->changeVideoMode();
			return 0;
		}
	}
	return 1;
}

static int console_fullscreen(int argc, const char** argv) {
	mainEngine->setFullscreen(true);

	// update renderer settings, if running
	Client* client = mainEngine->getLocalClient();
	if (mainEngine->isInitialized() && client) {
		Renderer* renderer = client->getRenderer();
		if (renderer) {
			renderer->setFullscreen(true);
			renderer->changeVideoMode();
			return 0;
		}
	}
	return 1;
}

static int console_vsync(int argc, const char** argv) {
	Client* client = mainEngine->getLocalClient();
	if (!client || !client->getRenderer()) {
		return 1;
	}
	if (argc != 1) {
		return 2;
	}

	int vsync = strtol(argv[1], nullptr, 10);
	client->getRenderer()->setVsync(vsync);
	SDL_GL_SetSwapInterval(vsync);

	return 0;
}

static int console_size(int argc, const char** argv) {
	if (argc < 3) {
		mainEngine->fmsg(Engine::MSG_ERROR, "not enough args. ex: %s 1280 720", argv[0]);
		return 1;
	}
	int xres = max(320, atoi(argv[1]));
	int yres = max(200, atoi(argv[2]));

	// update renderer settings, if running
	Client* client = mainEngine->getLocalClient();
	if (mainEngine->isInitialized() && client) {
		Renderer* renderer = client->getRenderer();
		if (renderer) {
			mainEngine->setXres(xres);
			mainEngine->setYres(yres);
			renderer->changeVideoMode();
			return 0;
		}
	}
	return 1;
}

static int console_shutdown(int argc, const char** argv) {
	mainEngine->shutdown();
	return 0;
}

static int console_version(int argc, const char** argv) {
	mainEngine->fmsg(Engine::MSG_INFO, mainEngine->version());
	return 0;
}

static int console_dump(int argc, const char** argv) {
	if (argc > 1) {
		mainEngine->dumpResources(argv[1]);
	} else {
		mainEngine->dumpResources(nullptr);
	}
	mainEngine->loadAllResources();
	return 0;
}

static int console_help(int argc, const char** argv) {
	const char* search = "";
	if (argc > 1) {
		search = argv[1];
	}
	Uint32 len = static_cast<Uint32>(strlen(search));

	// print out cvars
	mainEngine->fmsg(Engine::MSG_INFO, "Cvars:");
	ArrayList<Cvar*> cvars;
	for (auto& pair : Cvar::getMap()) {
		Cvar* cvar = pair.b;
		if (strncmp(search, cvar->getName(), len) == 0) {
			cvars.push(cvar);
		}
	}
	class CvarSort : public ArrayList<Cvar*>::SortFunction {
	public:
		virtual const bool operator()(Cvar* a, Cvar* b) const override {
			return strcmp(a->getName(), b->getName()) < 0;
		};
	} cvarSort;
	cvars.sort(cvarSort);
	for (auto cvar : cvars) {
		mainEngine->fmsg(Engine::MSG_NOTE, "%s: %s (cur: %s, default: %s)", cvar->getName(), cvar->getDesc(), cvar->toStr(), cvar->getDefault());
	}

	// print out ccmds
	mainEngine->fmsg(Engine::MSG_INFO, "Ccmds:");
	ArrayList<Ccmd*> ccmds;
	for (auto& pair : Ccmd::getMap()) {
		Ccmd* ccmd = pair.b;
		if (strncmp(search, ccmd->name.get(), len) == 0) {
			ccmds.push(ccmd);
		}
	}
	class CcmdSort : public ArrayList<Ccmd*>::SortFunction {
	public:
		virtual const bool operator()(Ccmd* a, Ccmd* b) const override {
			return strcmp(a->name.get(), b->name.get()) < 0;
		};
	} ccmdSort;
	ccmds.sort(ccmdSort);
	for (auto ccmd : ccmds) {
		mainEngine->fmsg(Engine::MSG_NOTE, "%s: %s", ccmd->name.get(), ccmd->desc.get());
	}

	return 0;
}

static int console_mount(int argc, const char** argv) {
	if (argc < 2) {
		mainEngine->fmsg(Engine::MSG_ERROR, "A mod folder is needed. ex: %s testmod", argv[0]);
		return 1;
	}
	if (mainEngine->addMod(argv[1])) {
		mainEngine->dumpResources(nullptr);
		mainEngine->loadAllResources();
		mainEngine->loadAllDefs();
		auto client = mainEngine->getLocalClient();
		if (client) {
			Editor* editor = client->getEditor();
			if (editor) {
				editor->updateContentNavigatorFilters();
			}
		}
		return 0;
	} else {
		return 1;
	}
}

static int console_unmount(int argc, const char** argv) {
	if (argc < 2) {
		mainEngine->fmsg(Engine::MSG_ERROR, "A mod folder is needed. ex: %s testmod", argv[0]);
		return 1;
	}
	if (mainEngine->removeMod(argv[1])) {
		mainEngine->dumpResources(nullptr);
		mainEngine->loadAllResources();
		mainEngine->loadAllDefs();
		auto client = mainEngine->getLocalClient();
		if (client) {
			Editor* editor = client->getEditor();
			if (editor) {
				editor->updateContentNavigatorFilters();
			}
		}
		return 0;
	} else {
		return 1;
	}
}

static int console_play(int argc, const char** argv) {
	if (argc < 1) {
		mainEngine->fmsg(Engine::MSG_ERROR, "A path is needed. ex: playsound test.wav");
		return 1;
	}
	mainEngine->playSound(argv[0]);
	return 0;
}

static int console_loadConfig(int argc, const char** argv) {
	if (argc < 2) {
		mainEngine->fmsg(Engine::MSG_ERROR, "A path is needed. ex: %s autoexec.cfg", argv[0]);
		return 1;
	}
	mainEngine->loadConfig(argv[1]);
	return 0;
}

static int console_sleep(int argc, const char** argv) {
	if (argc < 2) {
		mainEngine->fmsg(Engine::MSG_ERROR, "Please enter time to sleep in seconds. ex: %s 5", argv[0]);
		return 1;
	}
	float seconds;
	Engine::readFloat(argv[1], &seconds, 1);
	mainEngine->setConsoleSleep(mainEngine->getTicks() + seconds * mainEngine->getTicksPerSecond());
	return 0;
}

static int console_cacheSize(int argc, const char** argv) {
	mainEngine->printCacheSize();
	return 0;
}

static int console_printDir(int argc, const char** argv) {
	mainEngine->fmsg(Engine::MSG_INFO, mainEngine->getRunningDir());
	return 0;
}

static int console_map(int argc, const char** argv) {
	if (argc <= 1) {
		mainEngine->fmsg(Engine::MSG_ERROR, "usage: %s YourMapHere", argv[0]);
		return 1;
	}
	StringBuf<128> serverMap;
	serverMap.format("server.map %s", argv[1]);
	mainEngine->doCommand("server.reset");
	mainEngine->doCommand("sleep 0.1");
	mainEngine->doCommand("host");
	mainEngine->doCommand("sleep 0.1");
	mainEngine->doCommand(serverMap.get());
	mainEngine->doCommand("sleep 0.1");
	mainEngine->doCommand("client.reset");
	mainEngine->doCommand("sleep 0.1");
	mainEngine->doCommand("join localhost");
	mainEngine->doCommand("sleep 0.1");
	mainEngine->doCommand("client.spawn 0");
	return 0;
}

static Ccmd ccmd_help("help", "lists all console commands and variables", &console_help);
static Ccmd ccmd_shutdown("exit", "kill engine immediately", &console_shutdown);
static Ccmd ccmd_windowed("windowed", "set the engine to windowed mode", &console_windowed);
static Ccmd ccmd_fullscreen("fullscreen", "set the engine to fullscreen mode", &console_fullscreen);
static Ccmd ccmd_vsync("vsync", "set vsync on (1), off (0), or adaptive (-1)", &console_vsync);
static Ccmd ccmd_size("size", "change screen resolution", &console_size);
static Ccmd ccmd_clear("clear", "clear engine log", &console_clear);
static Ccmd ccmd_version("version", "display engine version", &console_version);
static Ccmd ccmd_dump("dump", "dumps entire engine cache", &console_dump);
static Ccmd ccmd_mount("mount", "mounts a new mod and dumps the engine cache", &console_mount);
static Ccmd ccmd_unmount("unmount", "unmounts a mod and dumps the engine cache", &console_unmount);
static Ccmd ccmd_play("play", "play the sound file with the given path", &console_play);
static Ccmd ccmd_loadconfig("loadconfig", "loads the given config file", &console_loadConfig);
static Ccmd ccmd_sleep("sleep", "waits X seconds before running the next command, useful for configs", &console_sleep);
static Ccmd ccmd_cachesize("cachesize", "prints the size of all resource caches in bytes", &console_cacheSize);
static Ccmd ccmd_printDir("printdir", "shows the directory that the engine is running from", &console_printDir);
static Ccmd ccmd_map("map", "start a new game map", &console_map);
static Cvar cvar_tickrate("tickrate", "number of frames processed in a second", "60");

// gameplay specific cvars:
static Cvar cvar_streamerMode("gameplay.streamer.enabled", "privacy mode for streamers, obscures addresses etc.", "0");
static Cvar cvar_disableScreenshake("gameplay.screenshake.disabled", "disable screenshake to alleviate motion sickness", "0");
static Cvar cvar_colorblindMode("gameplay.colorblind.enabled", "enable colorblind mode to help improve contrast for colorblind players", "0");

void Engine::printCacheSize() const {
	Uint32 total = 0;
	for (auto& pair : resources) {
		auto resource = pair.b;
		total += resource->getSizeInBytes();
		mainEngine->fmsg(Engine::MSG_INFO, "%s: %u bytes", Asset::typeStr[resource->getType()], resource->getSizeInBytes());
	}
	Client* client = mainEngine->getLocalClient();
	if (client) {
		Renderer* renderer = client->getRenderer(); assert(renderer);
		mainEngine->fmsg(Engine::MSG_INFO, "framebuffers: %u bytes", renderer->getFramebufferResource().getSizeInBytes());
		total += renderer->getFramebufferResource().getSizeInBytes();
	}
	mainEngine->fmsg(Engine::MSG_INFO, "total: %u bytes", total);
}

void Engine::commandLine(const int argc, const char **argv) {
	int c;

	for (c = 0; c < argc; ++c) {
		const char* arg = nullptr;

		// all commands at command line must be proceeded with -
		// ingame, these same commands do not require -
		if (initialized == false && argv[c][0] == '-') {
			arg = (const char *)(argv[c] + 1);
		} else if (initialized == true) {
			arg = (const char *)(argv[c]);
		} else {
			if (arg != nullptr) {
				fmsg(MSG_WARN, "unknown command: '%s'", arg);
			}
			continue;
		}

		// output a blank line
		if (!strcmp(arg, "")) {
			fmsg(MSG_INFO, " ");
			continue;
		}

		if (!initialized) {
			// these commands only work on game start

			// start a dedicated server (headless mode)
			if (!strcmp(arg, "dedicated")) {
				runningServer = true;
				runningClient = false;
				continue;
			}

			// set base game folder
			if (!strncmp(arg, "game=", 5)) {
				game = mod_t((const char *)(arg + 5));
				continue;
			}

			// add a mod folder
			if (!strncmp(arg, "mod=", 4)) {
				String modFolder = (const char *)(arg + 4);
				addMod(modFolder.get());
				continue;
			}

			// set config
			if (!strncmp(arg, "config=", 7)) {
				configName = (const char *)(arg + 7);
				continue;
			}
		} else { // if( !initialized )
			// cvars and ccmds only work past game start

			char cmd[128];
			cmd[127] = '\0';
			strncpy(cmd, arg, 127);
			char* token = strtok(cmd, " ");
			if (token == nullptr) {
				fmsg(MSG_WARN, "unknown command: '%s'", arg);
			} else {
				// search ccmds for a match
				Ccmd** ccmd = Ccmd::getMap().find(token);
				if (ccmd) {
					int argc = 0;
					const char** argv = new const char*[10];
					while (token && argc < 10) {
						argv[argc] = token;
						token = strtok(nullptr, " ");
						++argc;
					}
					int result = (*ccmd)->func(argc, argv);
					fmsg(MSG_DEBUG, "%s returned %d", cmd, result);
					delete[] argv;
					continue;
				}

				// search cvars for a match
				Cvar** cvar = Cvar::getMap().find(token);
				if (cvar) {
					if (strlen(arg) > strlen(token)) {
						(*cvar)->set((const char*)(arg + strlen(token) + 1));
					}
					fmsg(MSG_NOTE, "%s = %s", (*cvar)->getName(), (*cvar)->toStr());
					continue;
				}
			}
		}

		// unknown command
		fmsg(MSG_WARN, "unknown command: '%s'", arg);
	}
}

void Engine::doCommand(const char* arg) {
	if (consoleSleep > ticks) {
		// queue command
		String ccmd = arg;
		ccmdsToRun.addNodeLast(ccmd);
	} else {
		// run command immediately
		const char* command[1];
		command[0] = arg;
		mainEngine->commandLine(1, command);
	}
}

const char* Engine::getCvar(const char* name) {
	auto cvar = Cvar::getMap().find(name);
	if (cvar) {
		return (*cvar)->toStr();
	} else {
		return "";
	}
}

int Engine::loadConfig(const char* filename) {
	if (!filename) {
		return 1;
	}
	char str[1024];
	FILE *fp;

	String _filename = buildPath(filename);
	if (_filename.find(".cfg") == UINT32_MAX) {
		_filename.append(".cfg");
	}

	fmsg(Engine::MSG_INFO, "loading config '%s'...", _filename.get());

	// open the config file
	if ((fp = fopen(_filename.get(), "r")) == NULL) {
		fmsg(Engine::MSG_ERROR, "config file '%s' does not exist", _filename.get());
		return 1;
	}

	// read commands from it
	while (fgets(str, 1024, fp) != NULL) {
		if (str[0] != '#' && str[0] != '\n' && str[0] != '\r') {
			for (int c = 0; str[c] != '\0'; ++c) {
				if (str[c] == '\n' || str[c] == '\r') {
					str[c] = '\0';
					break;
				}
			}
			String ccmd = str;
			ccmdsToRun.addNodeLast(str);
		}
	}

	// close file
	fclose(fp);
	return 0;
}

int Engine::saveConfig(const char* filename, const ArrayList<String>& cvars, const ArrayList<String>& ccmds) {
	if (!filename) {
		return 1;
	}
	FILE *fp;

	StringBuf<64> _filename = filename;
	if (_filename.find(".cfg") == UINT32_MAX) {
		_filename.append(".cfg");
	}

	fmsg(Engine::MSG_INFO, "Saving config '%s'...\n", _filename.get());

	// open the config file
	if ((fp = fopen(_filename.get(), "w")) == NULL) {
		fmsg(Engine::MSG_ERROR, "failed to open '%s' for writing.", _filename.get());
		return 1;
	}

	// write header comment to file
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	fprintf(fp, "# this file was auto-generated on %d-%02d-%02d at %02d:%02d:%02d\n\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

	// write cvars
	if (cvars.getSize() == 1 && cvars[0] == "*") {
		for (auto& cvar : Cvar::getMap()) {
			fprintf(fp, "%s %s\n", cvar.b->getName(), cvar.b->toStr());
		}
	} else {
		for (auto& name : cvars) {
			auto cvar = Cvar::getMap().find(name);
			if (cvar) {
				fprintf(fp, "%s %s\n", (*cvar)->getName(), (*cvar)->toStr());
			}
		}
	}

	// write ccmds
	for (auto& ccmd : ccmds) {
		fprintf(fp, "%s\n", ccmd.get());
	}

	// close file
	fclose(fp);
	return 0;
}

String Engine::fileOpenDialog(const char* filterList, const char* defaultPath) {
	setPaused(true);
	nfdchar_t* outPath = nullptr;
	nfdresult_t result = NFD_OpenDialog(filterList, defaultPath, &outPath);
	setPaused(false);
	if (result == NFD_OKAY) {
		return String(outPath);
	} else if (result == NFD_CANCEL) {
		return String();
	} else {
		mainEngine->fmsg(Engine::MSG_ERROR, "failed to open load dialog: %s", NFD_GetError());
		return String();
	}
}

String Engine::fileSaveDialog(const char* filterList, const char* defaultPath) {
	setPaused(true);
	nfdchar_t* outPath = nullptr;
	nfdresult_t result = NFD_SaveDialog(filterList, defaultPath, &outPath);
	setPaused(false);
	if (result == NFD_OKAY) {
		return String(outPath);
	} else if (result == NFD_CANCEL) {
		return String();
	} else {
		mainEngine->fmsg(Engine::MSG_ERROR, "failed to open save dialog: %s", NFD_GetError());
		return String();
	}
}

void Engine::startEditor(const char* path) {
	if (!runningClient) {
		Engine::fmsg(Engine::MSG_ERROR, "cannot start editor in headless mode");
		return;
	}

	if (localServer) {
		delete localServer;
		localServer = nullptr;
	}
	if (localClient) {
		localClient->startEditor(path);
	}
}

void Engine::editorPlaytest() {
	// TODO: reloading defs here insidiously breaks a lot of game physics. why?
	//mainEngine->loadAllDefs();
	if (localClient && localClient->isEditorActive()) {
		World* world = localClient->getWorld(0);
		StringBuf<64> path("%s/maps/.playtest.wlb", 1, game.path.get());
		if (world->saveFile(path.get())) {
			playTest = true;
			startServer();
		} else {
			fmsg(MSG_ERROR, "Failed to save temp playtest file.");
		}
	}
}

void Engine::startServer() {
	if (initialized) {
		if (localServer) {
			localServer->reset();
		} else {
			localServer = new Server();
			localServer->init();
			runningServer = true;
		}
	} else {
		runningServer = true;
	}
}

void Engine::joinServer(const char* address) {
	if (localClient) {
		Net* net = localClient->getNet();
		if (net) {
			net->connect(address, Net::defaultPort);
		}
	}
}

void Engine::chat(const char* msg) {
	if (localClient) {
		Net* net = localClient->getNet();
		if (net) {
			Packet packet;
			packet.write(msg);
			packet.write32((Uint32)strlen(msg));
			packet.write("CMSG");
			net->signPacket(packet);
			net->sendPacketSafe(0, packet);
		}
	}
}

void Engine::playSound(const char* name) {
	localClient->getMixer()->playSound(name, false);
}

const char* Engine::version() {
	if (combinedVersion.empty()) {
		combinedVersion.alloc(64);
		combinedVersion.format("%s %s %s", versionStr, __DATE__, __TIME__);
	}
	return combinedVersion.get();
}

void Engine::init() {
	if (isInitialized())
		return;

#ifdef PLATFORM_WINDOWS
	// get symbols
	HANDLE process = GetCurrentProcess();
	int result = SymInitialize(process, NULL, 1);
	assert(result);
#endif

	// setup signal handlers
	signal(SIGSEGV, handleSIGSEGV);

	// print version data
	fmsg(Engine::MSG_INFO, "game version:");
	fmsg(Engine::MSG_INFO, "%s", version());

	// init sdl
	fmsg(Engine::MSG_INFO, "initializing SDL...");
	Uint32 initFlags = 0;
	initFlags |= SDL_INIT_TIMER;
	initFlags |= SDL_INIT_AUDIO;
	initFlags |= SDL_INIT_VIDEO;
	initFlags |= SDL_INIT_JOYSTICK;
	initFlags |= SDL_INIT_HAPTIC;
	initFlags |= SDL_INIT_GAMECONTROLLER;
	initFlags |= SDL_INIT_EVENTS;
	if (SDL_Init(initFlags) == -1) {
		fmsg(Engine::MSG_CRITICAL, "failed to initialize SDL: %s", SDL_GetError());
		initialized = false;
		return;
	}
	SDL_StopTextInput();

	// init sdl_mixer
	fmsg(Engine::MSG_INFO, "initializing SDL_mixer...");
	if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers)) {
		fmsg(Engine::MSG_CRITICAL, "failed to initialize SDL_mixer: %s", Mix_GetError());
		initialized = false;
		return;
	}

	// init sdl_image
	fmsg(Engine::MSG_INFO, "initializing SDL_image...");
	if (IMG_Init(IMG_INIT_PNG) != (IMG_INIT_PNG)) {
		fmsg(Engine::MSG_CRITICAL, "failed to initialize SDL_image: %s", IMG_GetError());
		initialized = false;
		return;
	}

	// init sdl_ttf
	fmsg(Engine::MSG_INFO, "initializing SDL_ttf...");
	if (TTF_Init() == -1) {
		fmsg(Engine::MSG_CRITICAL, "failed to initialize SDL_ttf: %s", TTF_GetError());
		initialized = false;
		return;
	}

	// init sdl_net
	fmsg(Engine::MSG_INFO, "initializing SDL_net...");
	if (SDLNet_Init() == -1) {
		fmsg(Engine::MSG_CRITICAL, "failed to initialize SDL_net: %s", SDLNet_GetError());
		initialized = false;
		return;
	}

	// open game controllers
	fmsg(Engine::MSG_INFO, "opening game controllers...");
	for (int c = 0; c < SDL_NumJoysticks(); ++c) {
		if (SDL_IsGameController(c) == SDL_TRUE) {
			SDL_GameController* pad = SDL_GameControllerOpen(c);
			if (pad) {
				controllers.addNodeLast(pad);
				fmsg(MSG_INFO, "Added gamepad: %s", SDL_GameControllerName(pad));
			}
		} else {
			SDL_Joystick* joystick = SDL_JoystickOpen(c);
			if (joystick) {
				joysticks.addNodeLast(joystick);
				fmsg(MSG_INFO, "Added joystick: %s", SDL_JoystickName(joystick));
				fmsg(MSG_INFO, " NumAxes: %d", SDL_JoystickNumAxes(joystick));
				fmsg(MSG_INFO, " NumButtons: %d", SDL_JoystickNumButtons(joystick));
				fmsg(MSG_INFO, " NumHats: %d", SDL_JoystickNumHats(joystick));
			}
		}
	}

	// instantiate a timer
	lastTick = std::chrono::steady_clock::now();
	SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);

	// initialize renderer
	if (renderer) {
		delete renderer;
		renderer = nullptr;
	}
	renderer = new Renderer();

	// instantiate local server
	if (runningServer) {
		localServer = new Server();
		localServer->init();
	}

	// instantiate local client
	if (runningClient) {
		localClient = new Client();
		localClient->init();
	}

	// init resource managers
	resources.insertUnique("font", new Resource<Font>());
	resources.insertUnique("mesh", new Resource<Mesh, true>());
	resources.insertUnique("staticmesh", new Resource<Mesh, false>());
	resources.insertUnique("image", new Resource<Image, true>());
	resources.insertUnique("material", new Resource<Material>());
	resources.insertUnique("text", new Resource<Text>());
	resources.insertUnique("sound", new Resource<Sound>());
	resources.insertUnique("animation", new Resource<Animation>());
	resources.insertUnique("cubemap", new Resource<Cubemap>());

	// cache resources
	fmsg(Engine::MSG_INFO, "game folder is '%s'", game.path.get());
	loadAllResources();
	loadAllDefs();

	fmsg(Engine::MSG_INFO, "done");
	initialized = true;
}

void Engine::loadResources(const char* folder) {
	// deprecated
}

void Engine::loadDefs(const char* folder) {
	StringBuf<64> entitiesDirPath;
	entitiesDirPath.format("%s/entities", folder);
	Directory entitiesDir(entitiesDirPath.get());
	if (entitiesDir.isLoaded()) {
		for (Node<String>* node = entitiesDir.getList().getFirst(); node != nullptr; node = node->getNext()) {
			String& str = node->getData();
			StringBuf<256> entityPath("entities/");
			entityPath.append(str.get());
			entityPath = buildPath(entityPath.get()).get();
			Entity::def_t* def = Entity::loadDef(entityPath.get());
			entityDefs.addNodeLast(def);
		}
	}
}

Uint32 Engine::findEntityDefIndexByName(const char* name) {
	if (!name || name[0] == '\0') {
		return UINT32_MAX;
	}
	for (auto def : entityDefs) {
		if (def->entity.getName() == name) {
			return def->index;
		}
	}
	return UINT32_MAX;
}

void Engine::term() {
	if (localClient) {
		delete localClient;
		localClient = nullptr;
		runningClient = false;
	}
	if (localServer) {
		delete localServer;
		localServer = nullptr;
		runningServer = false;
	}
	if (renderer) {
		delete renderer;
		renderer = nullptr;
	}

	// stop engine timer
	fmsg(MSG_INFO, "closing engine...");

	// free sounds
	Mix_HaltMusic();
	Mix_HaltChannel(-1);

	// close game controllers
	for (Node<SDL_GameController*>* node = controllers.getFirst(); node != nullptr; node = node->getNext()) {
		SDL_GameController* pad = node->getData();
		SDL_GameControllerClose(pad);
	}
	controllers.removeAll();
	for (auto node = joysticks.getFirst(); node != nullptr; node = node->getNext()) {
		SDL_Joystick* joystick = node->getData();
		SDL_JoystickClose(joystick);
	}
	joysticks.removeAll();

	// dump engine resources
	dumpResources(nullptr);
	for (auto& pair : resources) {
		delete pair.b;
	}
	resources.clear();

	// shutdown SDL subsystems
	fmsg(MSG_INFO, "shutting down SDL and its subsystems...");
	SDLNet_Quit();
	TTF_Quit();
	IMG_Quit();
	Mix_CloseAudio();
	SDL_Quit();

	fmsg(MSG_INFO, "successfully shut down game engine.");
	fmsg(MSG_INFO, "goodbye.");

	if (logFile)
		fclose(logFile);

#ifdef PLATFORM_WINDOWS
	// unload symbols
	HANDLE process = GetCurrentProcess();
	SymCleanup(process);
#endif
}

bool Engine::isEditorRunning() {
	if (playTest) {
		return false;
	}
	if (localClient) {
		if (localClient->isEditorActive()) {
			return true;
		}
	}
	return false;
}

const char* Engine::getVsyncMode() const {
	if (localClient && localClient->getRenderer()) {
		switch (localClient->getRenderer()->getVsync()) {
		case 1:
			return "On";
		case -1:
			return "Adaptive";
		default:
		case 0:
			return "Off";
		}
	} else {
		return "Off";
	}
}

void Engine::loadAllResources() {
	return; // DEPRECATED

	fmsg(MSG_INFO, "loading engine resources...");

	// reload the important assets
	loadResources(game.path.get());
	for (mod_t& mod : mods) {
		loadResources(mod.path.get());
	}
}

void Engine::loadAllDefs() {
	while (entityDefs.getFirst()) {
		delete entityDefs.getFirst()->getData();
		entityDefs.removeNode(entityDefs.getFirst());
	}
	loadDefs(game.path.get());
	for (mod_t& mod : mods) {
		loadDefs(mod.path.get());
	}
}

void Engine::dumpResources(const char* type) {
	auto resource = type ? resources.find(type) : nullptr;
	if (resource) {
		fmsg(MSG_INFO, "dumping all %s assets...", Asset::typeStr[(*resource)->getType()]);
		(*resource)->dumpCache();
	} else {
		fmsg(MSG_INFO, "dumping all resources...");
		for (auto& pair : resources) {
			auto resource = pair.b;
			resource->dumpCache();
		}
	}
}

void Engine::fmsg(const Uint32 msgType, const char* fmt, ...) {
	if (fmt == nullptr) {
		return;
	}

#ifdef NDEBUG
	if (msgType == Engine::MSG_DEBUG) {
		return;
	}
#endif

	// wait for current log to finish
	if (!logLock) {
		while (1) {
			SDL_LockMutex(logLock);
			if (logging) {
				SDL_UnlockMutex(logLock);
			} else {
				logging = true;
				SDL_UnlockMutex(logLock);
				break;
			}
		}
	}

	char str[1024];
	str[1024 - 1] = '\0';

	// format the string
	va_list argptr;
	va_start(argptr, fmt);
	vsnprintf(str, 1024, fmt, argptr);
	va_end(argptr);
	str[1024 - 1] = '\0';

	// bust multi-line strings into individual strings...
	for (Uint32 c = 0; c < (Uint32)strlen(str); ++c) {
		if (str[c] == '\n') {
			str[c] = 0;

			// disable lock temporarily
			if (logLock) {
				SDL_LockMutex(logLock);
				logging = false;
				SDL_UnlockMutex(logLock);
			}

			// write second line
			fmsg(msgType, (const char *)(str + c + 1));

			// reactivate log lock
			if (logLock) {
				while (1) {
					SDL_LockMutex(logLock);
					if (logging) {
						SDL_UnlockMutex(logLock);
					} else {
						logging = true;
						SDL_UnlockMutex(logLock);
						break;
					}
				}
			}
		}
	}

	// create timestamp string
	time_t timer;
	char buffer[32];
	struct tm* tm_info;
	time(&timer);
	tm_info = localtime(&timer);
	strftime(buffer, 32, "%H-%M-%S", tm_info);

	// print message to stderr and stdout
	fprintf(stderr, "[%s] %s: %s\r\n", buffer, (const char *)msgTypeStr[msgType], str);
	fprintf(stdout, "[%s] %s: %s\r\n", buffer, (const char *)msgTypeStr[msgType], str);
	fflush(stderr);
	fflush(stdout);

	// add message to log list
	logmsg_t logMsg;
	logMsg.text = str;
	logMsg.uid = logUids++;
	logMsg.kind = static_cast<msg_t>(msgType);
	switch (msgType) {
	case MSG_DEBUG:
		logMsg.color = glm::vec3(0.f, .7f, 0.f);
		break;

	case MSG_WARN:
		logMsg.color = glm::vec3(1.f, 1.f, 0.f);
		break;
	case MSG_ERROR:
		logMsg.color = glm::vec3(1.f, .5f, 0.f);
		break;
	case MSG_CRITICAL:
		logMsg.color = glm::vec3(1.f, 0.f, 1.f);
		break;
	case MSG_FATAL:
		logMsg.color = glm::vec3(1.f, 0.f, 0.f);
		break;

	case MSG_INFO:
		logMsg.color = glm::vec3(1.f, 1.f, 1.f);
		break;
	case MSG_NOTE:
		logMsg.color = glm::vec3(0.f, 1.f, 1.f);
		break;
	case MSG_CHAT:
		logMsg.color = glm::vec3(0.f, 1.f, 0.f);
		break;

	default:
		logMsg.color = glm::vec3(1.f, 1.f, 1.f);
		break;
	}

	logList.addNodeLast(logMsg);

	// unlock log stuff
	if (logLock) {
		SDL_LockMutex(logLock);
		logging = false;
		SDL_UnlockMutex(logLock);
	}
}

void Engine::smsg(const Uint32 msgType, const String& str) {
	fmsg(msgType, str.get());
}

void Engine::msg(const Uint32 msgType, const char* str) {
	fmsg(msgType, str);
}

void Engine::freadl(void* ptr, Uint32 size, Uint32 count, FILE* stream, const char* filename, const char* funcName) {
	if (fread(ptr, size, count, stream) != count) {
		if (filename != nullptr) {
			if (funcName != nullptr) {
				mainEngine->fmsg(Engine::MSG_WARN, "%s: file read error in '%s': %s", funcName, filename, strerror(errno));
			} else {
				mainEngine->fmsg(Engine::MSG_WARN, "file read error in '%s': %s", filename, strerror(errno));
			}
		}
	}
}

int Engine::readInt(const char* str, int* arr, int numToRead) {
	int numRead = 0;
	char* curr = nullptr;
	while (numRead < numToRead) {
		if (str[0] == '\0') {
			if (numRead != numToRead) {
				mainEngine->fmsg(Engine::MSG_DEBUG, "readInt(): could only read %d numbers of %d", numRead, numToRead);
			}
			break;
		}
		if (numRead != 0) {
			++str;
			if (str[0] == '\0') {
				if (numRead != numToRead) {
					mainEngine->fmsg(Engine::MSG_DEBUG, "readInt(): could only read %d numbers of %d", numRead, numToRead);
				}
				break;
			}
		}
		if (curr) {
			arr[numRead] = strtol((const char*)curr, &curr, 10);
		} else {
			arr[numRead] = strtol(str, &curr, 10);
		}
		++numRead;
	}
	return numRead;
}

int Engine::readFloat(const char* str, float* arr, int numToRead) {
	int numRead = 0;
	char* curr = nullptr;
	while (numRead < numToRead) {
		if (str[0] == '\0') {
			if (numRead != numToRead) {
				mainEngine->fmsg(Engine::MSG_DEBUG, "readFloat(): could only read %d numbers of %d", numRead, numToRead);
			}
			break;
		}
		if (numRead != 0) {
			++str;
			if (str[0] == '\0') {
				if (numRead != numToRead) {
					mainEngine->fmsg(Engine::MSG_DEBUG, "readFloat(): could only read %d numbers of %d", numRead, numToRead);
				}
				break;
			}
		}
		if (curr) {
			arr[numRead] = strtof((const char*)(curr), &curr);
		} else {
			arr[numRead] = strtof(str, &curr);
		}
		++numRead;
	}
	return numRead;
}

bool Engine::lineIntersectPlane(const Vector& lineStart, const Vector& lineEnd, const Vector& planeOrigin, const Vector& planeNormal, Vector& outIntersection) {
	Vector u = lineEnd - lineStart;
	Vector w = lineStart - planeOrigin;

	float d = u.dot(planeNormal);
	float n = -w.dot(planeNormal);

	// check that the line isn't parallel to the plane
	if (fabs(d) <= 0.f) {
		if (n == 0.f) {
			// the line is in the plane
			outIntersection = lineStart;
			return true;
		} else {
			// no intersection
			return false;
		}
	}

	// compute point of intersection
	float intersect = n / d;
	if (intersect < 0.f || intersect > 1.f)
		return false; // out of range, no intersection

	// compute intersection
	outIntersection = lineStart + Vector(intersect) * u;
	return true;
}

Vector Engine::triangleCoords(const Vector& a, const Vector& b, const Vector& c, const Vector& p) {
	Vector v0 = c - a;
	Vector v1 = b - a;
	Vector v2 = p - a;

	float dot00 = v0.dot(v0);
	float dot01 = v0.dot(v1);
	float dot02 = v0.dot(v2);
	float dot11 = v1.dot(v1);
	float dot12 = v1.dot(v2);

	// barycentric coords
	float invDenom = 1.f / (dot00 * dot11 - dot01 * dot01);
	float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
	float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

	return Vector(u, v, 0.f);
}

bool Engine::pointInTriangle(const Vector& a, const Vector& b, const Vector& c, const Vector& p) {
	Vector result = triangleCoords(a, b, c, p);
	float u = result.x;
	float v = result.y;
	return (u >= 0) && (v >= 0) && (u + v < 1.f);
}

bool Engine::triangleOverlapsTriangle(const Vector& a0, const Vector& b0, const Vector& c0, const Vector& a1, const Vector& b1, const Vector& c1) {
	if (pointInTriangle(a0, b0, c0, a1)) {
		return true;
	}
	if (pointInTriangle(a0, b0, c0, b1)) {
		return true;
	}
	if (pointInTriangle(a0, b0, c0, c1)) {
		return true;
	}
	return false;
}

float Engine::measurePointToBounds(const Vector& point, const Vector& boundsMin, const Vector& boundsMax) {
	Vector d;
	d.x = std::max(boundsMin.x - point.x, std::max(0.f, point.x - boundsMax.x));
	d.y = std::max(boundsMin.y - point.y, std::max(0.f, point.y - boundsMax.y));
	d.z = std::max(boundsMin.z - point.z, std::max(0.f, point.z - boundsMax.z));
	return d.lengthSquared();
}

void Engine::shutdown() {
	running = false;
}

bool Engine::charsHaveLetters(const char *arr, Uint32 len) {
	for (Uint32 c = 0; c < len; ++c) {
		if (arr[c] == 0) {
			return false;
		}

		else if ((arr[c] < '0' || arr[c] > '9') && arr[c] != '.' && arr[c] != '-') {
			return true;
		}
	}

	return false;
}

int Engine::strCompare(const char* a, const char* b) {
	return strcmp(a, b);
}

const char* Engine::getRunningDir() {
	if (runDir.empty()) {
#ifdef PLATFORM_WINDOWS
		wchar_t pBuf[256];
		int len = (int)sizeof(pBuf);
		int bytes = GetModuleFileName(NULL, pBuf, len);
		if (bytes > 0) {
			char str[256];
			wcstombs(str, pBuf, sizeof(str));
			runDir = str;
		}
#else
		char szTmp[64];
		snprintf(szTmp, 64, "/proc/%d/exe", getpid());
		char pBuf[256];
		ssize_t len = sizeof(pBuf);
		int bytes = std::min(readlink(szTmp, pBuf, len), len - 1u);
		if (bytes >= 0) {
			pBuf[bytes] = '\0';
			runDir = pBuf;
		}
#endif
	}
	return runDir.get();
}

void Engine::preProcess() {
	// lock mouse to window?
	SDL_SetRelativeMouseMode((SDL_bool)mainEngine->isMouseRelative());

	// update resources
	for (auto& resource : resources) {
		resource.b->update();
	}

	// do timer
	ticksPerSecond = cvar_tickrate.toInt();
	std::chrono::duration<double> msInterval(1.0 / ticksPerSecond);
	auto now = std::chrono::high_resolution_clock::now();
	int framesToDo = (now - lastTick) / msInterval;
	if (framesToDo) {
		lastTick = now;
		if (!paused) {
			for (int c = 0; c < framesToDo; ++c) {
				SDL_Event event;
				SDL_UserEvent userevent;

				userevent.type = SDL_USEREVENT;
				userevent.code = 0;
				userevent.data1 = nullptr;
				userevent.data2 = nullptr;

				event.type = SDL_USEREVENT;
				event.user = userevent;

				SDL_PushEvent(&event);
			}
		}
	}

	SDL_GameController* pad = nullptr;
	SDL_Joystick* joystick = nullptr;

	if (framesToDo) {
		lastInputOfAnyKind = "";
	}
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT: // if SDL receives the shutdown signal
		{
			if (localClient) {
				if (localClient->isEditorActive()) {
					killSignal = true;
					break;
				}
			}
			shutdown();
			break;
		}
		case SDL_KEYDOWN: // if a key is pressed...
		{
			if (SDL_IsTextInputActive()) {
				if (event.key.keysym.sym == SDLK_BACKSPACE && strlen(inputstr) > 0) {
					inputstr[strlen(inputstr) - 1] = 0;
					strcpy(lastInput, "");
					cursorflash = ticks;
				} else if (event.key.keysym.sym == SDLK_c && SDL_GetModState()&KMOD_CTRL) {
					if (inputstr)
						SDL_SetClipboardText(inputstr);
					cursorflash = ticks;
				} else if (event.key.keysym.sym == SDLK_v && SDL_GetModState()&KMOD_CTRL) {
					if (inputstr)
						strcpy(inputstr, SDL_GetClipboardText());
					cursorflash = ticks;
				}
			}
			lastkeypressed = SDL_GetKeyName(SDL_GetKeyFromScancode(event.key.keysym.scancode));
			lastInputOfAnyKind = lastkeypressed;
			keystatus[event.key.keysym.scancode] = true;
			anykeystatus = true;
			break;
		}
		case SDL_KEYUP: // if a key is unpressed...
		{
			keystatus[event.key.keysym.scancode] = false;
			anykeystatus = false;
			break;
		}
		case SDL_TEXTINPUT:
		{
			if (!inputnum || !charsHaveLetters(event.text.text, SDL_TEXTINPUTEVENT_TEXT_SIZE)) {
				if ((event.text.text[0] != 'c' && event.text.text[0] != 'C') || !(SDL_GetModState()&KMOD_CTRL)) {
					if ((event.text.text[0] != 'v' && event.text.text[0] != 'V') || !(SDL_GetModState()&KMOD_CTRL)) {
						if (inputstr) {
							int i = inputlen - (int)strlen(inputstr) - 1;
							strncat(inputstr, event.text.text, max(0, i));
							strcpy(lastInput, event.text.text);
						}
						cursorflash = ticks;
					}
				}
			}
			break;
		}
		case SDL_MOUSEBUTTONDOWN: // if a mouse button is pressed...
		{
			if (!mousestatus[event.button.button]) {
				lastInputOfAnyKind.format("Mouse%d", event.button.button);
				mousestatus[event.button.button] = true;
				if (ticks - mouseClickTime <= doubleClickTime) {
					dbc_mousestatus[event.button.button] = true;
				}
				mouseClickTime = ticks;
			}
			break;
		}
		case SDL_MOUSEBUTTONUP: // if a mouse button is released...
		{
			mousestatus[event.button.button] = false;
			dbc_mousestatus[event.button.button] = false;
			break;
		}
		case SDL_MOUSEWHEEL:
		{
			mousewheelx = event.wheel.x;
			mousewheely = event.wheel.y;
			break;
		}
		case SDL_MOUSEMOTION: // if the mouse is moved...
		{
			if (ticks == 0) {
				// fixes a bug with unpredictable starting mouse movement
				break;
			}
			mousex = event.motion.x;
			mousey = event.motion.y;
			mousexrel += event.motion.xrel;
			mouseyrel += event.motion.yrel;

			if (std::abs(mousexrel) > 2 || std::abs(mouseyrel) > 2) {
				mouseClickTime = 0;
			}
			break;
		}
		case SDL_JOYDEVICEADDED:
		{
			joystick = SDL_JoystickOpen(event.jdevice.which);
			if (!joystick) {
				fmsg(MSG_WARN, "A joystick was plugged in, but no handle is available!");
			} else {
				joysticks.addNode(event.jdevice.which, joystick);
				fmsg(MSG_INFO, "Added joystick '%s' with device index (%d)", SDL_JoystickName(joystick), event.jdevice.which);
				fmsg(MSG_INFO, " NumAxes: %d", SDL_JoystickNumAxes(joystick));
				fmsg(MSG_INFO, " NumButtons: %d", SDL_JoystickNumButtons(joystick));
				fmsg(MSG_INFO, " NumHats: %d", SDL_JoystickNumHats(joystick));
				for (int c = 0; c < 4; ++c) {
					inputs[c].refresh();
				}
			}
			break;
		}
		case SDL_JOYBUTTONDOWN:
		{
			lastInputOfAnyKind.format("Joy%dButton%d", event.jbutton.which, event.jbutton.button);
			break;
		}
		case SDL_JOYAXISMOTION:
		{
			if (event.jaxis.value < 0) {
				lastInputOfAnyKind.format("Joy%dAxis-%d", event.jaxis.which, event.jaxis.axis);
			} else {
				lastInputOfAnyKind.format("Joy%dAxis+%d", event.jaxis.which, event.jaxis.axis);
			}
			break;
		}
		case SDL_JOYHATMOTION:
		{
			switch (event.jhat.value) {
			case SDL_HAT_LEFTUP: lastInputOfAnyKind.format("Joy%dHat%dLeftUp", event.jhat.which, event.jhat.hat); break;
			case SDL_HAT_UP: lastInputOfAnyKind.format("Joy%dHat%dUp", event.jhat.which, event.jhat.hat); break;
			case SDL_HAT_RIGHTUP: lastInputOfAnyKind.format("Joy%dHat%dRightUp", event.jhat.which, event.jhat.hat); break;
			case SDL_HAT_RIGHT: lastInputOfAnyKind.format("Joy%dHat%dRight", event.jhat.which, event.jhat.hat); break;
			case SDL_HAT_RIGHTDOWN: lastInputOfAnyKind.format("Joy%dHat%dRightDown", event.jhat.which, event.jhat.hat); break;
			case SDL_HAT_DOWN: lastInputOfAnyKind.format("Joy%dHat%dDown", event.jhat.which, event.jhat.hat); break;
			case SDL_HAT_LEFTDOWN: lastInputOfAnyKind.format("Joy%dHat%dLeftDown", event.jhat.which, event.jhat.hat); break;
			case SDL_HAT_LEFT: lastInputOfAnyKind.format("Joy%dHat%dLeft", event.jhat.which, event.jhat.hat); break;
			case SDL_HAT_CENTERED: lastInputOfAnyKind.format("Joy%dHat%dCentered", event.jhat.which, event.jhat.hat); break;
			}
			break;
		}
		case SDL_JOYDEVICEREMOVED:
		{
			joystick = SDL_JoystickFromInstanceID(event.jdevice.which);
			if (joystick == nullptr) {
				fmsg(MSG_WARN, "A joystick was removed, but I don't know which one!");
			} else {
				Uint32 index = 0;
				Node<SDL_Joystick*>* node = joysticks.getFirst();
				for (; node != nullptr; node = node->getNext(), ++index) {
					SDL_Joystick* curr = node->getData();
					if (joystick == curr) {
						SDL_JoystickClose(curr);
						joysticks.removeNode(node);
						fmsg(MSG_INFO, "Removed joystick with device index (%d), instance id (%d)", index, event.jdevice.which);
						break;
					}
				}
			}
			break;
		}
		case SDL_CONTROLLERDEVICEADDED:
		{
			pad = SDL_GameControllerOpen(event.cdevice.which);
			if (pad == nullptr) {
				fmsg(MSG_WARN, "A gamepad was plugged in, but no handle is available!");
			} else {
				controllers.addNode(event.cdevice.which, pad);
				fmsg(MSG_INFO, "Added gamepad '%s' with device index (%d)", SDL_GameControllerName(pad), event.cdevice.which);
				for (int c = 0; c < 4; ++c) {
					inputs[c].refresh();
				}
				break;
			}
			break;
		}
		case SDL_CONTROLLERBUTTONDOWN:
		{
			switch (event.cbutton.button) {
			case SDL_CONTROLLER_BUTTON_A: lastInputOfAnyKind.format("Pad%dButtonA", event.cbutton.which); break;
			case SDL_CONTROLLER_BUTTON_B: lastInputOfAnyKind.format("Pad%dButtonB", event.cbutton.which); break;
			case SDL_CONTROLLER_BUTTON_X: lastInputOfAnyKind.format("Pad%dButtonX", event.cbutton.which); break;
			case SDL_CONTROLLER_BUTTON_Y: lastInputOfAnyKind.format("Pad%dButtonY", event.cbutton.which); break;
			case SDL_CONTROLLER_BUTTON_BACK: lastInputOfAnyKind.format("Pad%dButtonBack", event.cbutton.which); break;
			case SDL_CONTROLLER_BUTTON_START: lastInputOfAnyKind.format("Pad%dButtonStart", event.cbutton.which); break;
			case SDL_CONTROLLER_BUTTON_LEFTSTICK: lastInputOfAnyKind.format("Pad%dButtonLeftStick", event.cbutton.which); break;
			case SDL_CONTROLLER_BUTTON_RIGHTSTICK: lastInputOfAnyKind.format("Pad%dButtonRightStick", event.cbutton.which); break;
			case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: lastInputOfAnyKind.format("Pad%dButtonLeftBumper", event.cbutton.which); break;
			case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: lastInputOfAnyKind.format("Pad%dButtonRightBumper", event.cbutton.which); break;
			case SDL_CONTROLLER_BUTTON_DPAD_LEFT: lastInputOfAnyKind.format("Pad%dDpadX-", event.cbutton.which); break;
			case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: lastInputOfAnyKind.format("Pad%dDpadX+", event.cbutton.which); break;
			case SDL_CONTROLLER_BUTTON_DPAD_UP: lastInputOfAnyKind.format("Pad%dDpadY-", event.cbutton.which); break;
			case SDL_CONTROLLER_BUTTON_DPAD_DOWN: lastInputOfAnyKind.format("Pad%dDpadY+", event.cbutton.which); break;
			}
			break;
		}
		case SDL_CONTROLLERAXISMOTION:
		{
			switch (event.caxis.axis) {
			case SDL_CONTROLLER_AXIS_LEFTX: event.caxis.value < 0 ?
				lastInputOfAnyKind.format("Pad%dStickLeftX-", event.caxis.which):
				lastInputOfAnyKind.format("Pad%dStickLeftX+", event.caxis.which);
				break;
			case SDL_CONTROLLER_AXIS_LEFTY: event.caxis.value < 0 ?
				lastInputOfAnyKind.format("Pad%dStickLeftY-", event.caxis.which):
				lastInputOfAnyKind.format("Pad%dStickLeftY+", event.caxis.which);
				break;
			case SDL_CONTROLLER_AXIS_RIGHTX: event.caxis.value < 0 ?
				lastInputOfAnyKind.format("Pad%dStickRightX-", event.caxis.which):
				lastInputOfAnyKind.format("Pad%dStickRightX+", event.caxis.which);
				break;
			case SDL_CONTROLLER_AXIS_RIGHTY: event.caxis.value < 0 ?
				lastInputOfAnyKind.format("Pad%dStickRightY-", event.caxis.which):
				lastInputOfAnyKind.format("Pad%dStickRightY+", event.caxis.which);
				break;
			case SDL_CONTROLLER_AXIS_TRIGGERLEFT: lastInputOfAnyKind.format("Pad%dLeftTrigger"); break;
			case SDL_CONTROLLER_AXIS_TRIGGERRIGHT: lastInputOfAnyKind.format("Pad%dRightTrigger"); break;
			}
			break;
		}
		case SDL_CONTROLLERDEVICEREMOVED:
		{
			pad = SDL_GameControllerFromInstanceID(event.cdevice.which);
			if (pad == nullptr) {
				fmsg(MSG_WARN, "A gamepad was removed, but I don't know which one!");
			} else {
				Uint32 index = 0;
				Node<SDL_GameController*>* node = controllers.getFirst();
				for (; node != nullptr; node = node->getNext(), ++index) {
					SDL_GameController* curr = node->getData();
					if (pad == curr) {
						SDL_GameControllerClose(curr);
						controllers.removeNode(node);
						fmsg(MSG_INFO, "Removed gamepad with device index (%d), instance id (%d)", index, event.cdevice.which);
						break;
					}
				}
			}
			break;
		}
		case SDL_DROPFILE:
		{
			String droppedFile = shortenPath(event.drop.file);
			if (!inputnum && inputstr) {
				int i = inputlen - (int)strlen(inputstr) - 1;
				strncpy(inputstr, droppedFile.get(), std::min(inputlen - 1, (int)droppedFile.length()));
			}
			SDL_free(event.drop.file);
		}
		case SDL_USEREVENT: // if the game timer has elapsed
		{
			executedFrames = true;

			++ticks;
			if (localServer) {
				localServer->incrementFrame();
			}
			if (localClient) {
				localClient->incrementFrame();
			}
			break;
		}
		}
	}
	if (!mousestatus[SDL_BUTTON_LEFT]) {
		omousex = mousex;
		omousey = mousey;
	}

	// update input maps
	for (int c = 0; c < 4; ++c) {
		inputs[c].update();
	}

	if (executedFrames) {
		// calculate engine rate
		t = SDL_GetTicks();
		timesync = t - ot;
		ot = t;

		// calculate fps
		if (timesync != 0) {
			frameval[cycles&(fpsAverage - 1)] = 1.0 / timesync;
		} else {
			frameval[cycles&(fpsAverage - 1)] = 1.0;
		}
		double d = frameval[0];
		for (Uint32 i = 1; i < fpsAverage; ++i) {
			d += frameval[i];
		}
		if (SDL_GetTicks() - lastfpscount > 500) {
			lastfpscount = SDL_GetTicks();
			fps = (d / fpsAverage) * 1000;
		}

		// run console
		if (consoleSleep <= ticks) {
			Node<String>* ccmdToRun;
			while ((ccmdToRun = ccmdsToRun.getFirst()) != nullptr) {
				doCommand(ccmdToRun->getData().get());
				ccmdsToRun.removeNode(ccmdToRun);
				if (consoleSleep) {
					break;
				}
			}
		}
	}

	// run local server
	if (localServer) {
		localServer->preProcess();
	}

	// run local client
	if (localClient) {
		localClient->preProcess();
	}
}

void Engine::process() {
	// run local server
	if (localServer) {
		localServer->process();
	}

	// run local client
	if (localClient) {
		localClient->process();
	}
}

void Engine::postProcess() {
	// run local server
	if (localServer) {
		localServer->postProcess();
	}

	// run local client
	if (localClient) {
		localClient->postProcess();
	}

	// reset mouse motion
	if (executedFrames) {
		executedFrames = false;

		mousexrel = 0;
		mouseyrel = 0;
		mousewheelx = 0;
		mousewheely = 0;
	}

	// reset client and server, if necessary
	if (localServer) {
		if (localServer->isSuicide()) {
			delete localServer;
			localServer = nullptr;

			if (runningServer) {
				localServer = new Server();
				localServer->init();
			}
		}
	}
	if (localClient) {
		if (localClient->isSuicide()) {
			delete localClient;
			localClient = nullptr;

			if (runningClient) {
				localClient = new Client();
				localClient->init();
			}

			// some resources will be corrupted when the client is dropped, anyway
			dumpResources(nullptr);
			loadAllResources();
		}
	}
	++cycles;

	// sleep thread for a bit
	std::chrono::duration<double> msInterval(1.0 / ticksPerSecond);
	auto sleepTime = (msInterval - (std::chrono::high_resolution_clock::now() - lastTick)) / 2;
	if (sleepTime > std::chrono::milliseconds(1)) {
		std::this_thread::sleep_for(sleepTime);
	} else {
		std::this_thread::yield();
	}
}

ArrayList<String> Engine::getDisplayModes() const {
	ArrayList<String> result;

	// build a sorted list of all the modes supported by the primary display
	ArrayList<SDL_DisplayMode> modeList;
	int modes = SDL_GetNumDisplayModes(0);
	for (int c = 0; c < modes; ++c) {
		modeList.push(SDL_DisplayMode());
		int index = modeList.getSize() - 1;
		if (SDL_GetDisplayMode(0, c, &modeList[index]) == 0) {
			for (int i = 0; i < index; ++i) {
				if (modeList[i].w == modeList[index].w &&
					modeList[i].h == modeList[index].h) {
					modeList.pop();
					break;
				}
			}
		}
	}
	class SortFn : public ArrayList<SDL_DisplayMode>::SortFunction {
	public:
		virtual const bool operator()(const SDL_DisplayMode a, const SDL_DisplayMode b) const override {
			return a.w == b.w ? a.h > b.h : a.w > b.w;
		}
	};
	modeList.sort(SortFn());

	// produce a string version of the list
	for (auto& mode : modeList) {
		result.push(StringBuf<32>("%d %d", 2, mode.w, mode.h));
	}
	return result;
}

String Engine::shortenPath(const char* path) const {
	if (path == nullptr || path[0] == '\0') {
		return String();
	}

	Uint32 offset = 0;
	String pathStr = path;

	// windows has to convert backslashes to forward slashes
#ifdef PLATFORM_WINDOWS
	for (Uint32 c = 0; c < pathStr.getSize(); ++c) {
		if (pathStr[c] == '\0') {
			break;
		}
		if (pathStr[c] == '\\') {
			pathStr[c] = '/';
		}
	}
#endif

	StringBuf<32> withSlashes;
	withSlashes.format("/%s/", game.path.get());
	offset = pathStr.find(withSlashes.get(), 0);
	if (offset != String::npos) {
		pathStr = pathStr.substr(offset + withSlashes.length());
	} else {
		for (const mod_t& mod : mods) {
			withSlashes.format("/%s/", mod.path.get());
			offset = pathStr.find(withSlashes.get(), 0);
			if (offset != String::npos) {
				pathStr = pathStr.substr(offset + withSlashes.length());
				break;
			}
		}
	}

	return pathStr;
}

String Engine::buildPath(const char* path) const {
	if (path == nullptr || path[0] == '\0') {
		return String();
	}

	StringBuf<256> result(game.path.get());
	result.appendf("/%s", path);

	// if a mod has the same path, use the mod's path instead...
	for (const mod_t& mod : mods) {
		StringBuf<256> modResult(mod.path.get());
		modResult.appendf("/%s", path);

		FILE* fp = nullptr;
		if ((fp = fopen(modResult.get(), "rb")) != nullptr) {
			result = modResult;
			fclose(fp);
		}
	}

	return result;
}

void Engine::loadMapServer(const char* path) {
	if (!localServer)
		return;

	localServer->loadWorld(path, true);
}

void Engine::loadMapClient(const char* path) {
	if (!localClient)
		return;

	localClient->loadWorld(path, true);
}

bool Engine::addMod(const char* name) {
	if (name == nullptr || name[0] == '\0' || game.path.get() == name)
		return false;

	// check that we have not already added the mod
	bool foundMod = false;
	for (mod_t& mod : mods) {
		if (mod.path == name) {
			foundMod = true;
			break;
		}
	}

	if (!foundMod) {
		mod_t mod(name);
		if (mod.loaded == false) {
			Engine::fmsg(MSG_ERROR, "failed to install '%s' mod.", name);
			return false;
		}
		mods.addNodeLast(mod);

		Engine::fmsg(MSG_INFO, "installed '%s' mod", name);

		return true;
	} else {
		Engine::fmsg(MSG_ERROR, "'%s' mod is already installed.", name);

		return false;
	}
}

bool Engine::removeMod(const char* name) {
	if (name == nullptr || name[0] == '\0' || game.path == name)
		return false;

	// check that we have not already added the mod
	Uint32 index = 0;
	for (mod_t& mod : mods) {
		if (mod.path == name) {
			mods.removeNode(index);
			Engine::fmsg(MSG_INFO, "uninstalled '%s' mod", name);
			return true;
		}
		++index;
	}

	Engine::fmsg(MSG_ERROR, "'%s' mod is not installed.", name);
	return false;
}

bool Engine::copyLog(LinkedList<Engine::logmsg_t>& dest) {
	bool result = false;

	// wait for current logging action to finish
	while (1) {
		SDL_LockMutex(logLock);
		if (logging) {
			SDL_UnlockMutex(logLock);
		} else {
			logging = true;
			SDL_UnlockMutex(logLock);
			break;
		}
	}

	if (dest.getSize() == logList.getSize()) {
		SDL_LockMutex(logLock);
		logging = false;
		SDL_UnlockMutex(logLock);
		return false;
	} else if (dest.getSize() > logList.getSize()) {
		dest.removeAll();
		result = true;
	}

	for (Node<Engine::logmsg_t>* node = logList.nodeForIndex(dest.getSize()); node != nullptr; node = node->getNext()) {
		Engine::logmsg_t newMsg;
		newMsg = node->getData();
		dest.addNodeLast(newMsg);
	}

	SDL_LockMutex(logLock);
	logging = false;
	SDL_UnlockMutex(logLock);

	return result;
}

void Engine::clearLog() {

	// wait for current logging action to finish
	while (1) {
		SDL_LockMutex(logLock);
		if (logging) {
			SDL_UnlockMutex(logLock);
		} else {
			logging = true;
			SDL_UnlockMutex(logLock);
			break;
		}
	}

	logList.removeAll();

	SDL_LockMutex(logLock);
	logging = false;
	SDL_UnlockMutex(logLock);
}

Uint32 Engine::random() {
	return rand.getUint32();
}

Engine::mod_t::mod_t(const char* _path) :
	path(_path),
	name("Untitled"),
	author("Unknown")
{
	if (!path) {
		return;
	}
	StringBuf<128> fullPath("%s/game.json", 1, _path);
	if (!FileHelper::readObject(fullPath, *this)) {
		mainEngine->fmsg(Engine::MSG_ERROR, "Failed to read mod manifest: '%s'", fullPath.get());
		return;
	}
	loaded = true;
}

void Engine::mod_t::serialize(FileInterface* file) {
	file->property("name", name);
	file->property("author", author);
}

#ifdef PLATFORM_WINDOWS

ArrayList<String> Engine::stackTrace() const {
	HANDLE process = GetCurrentProcess();

	void* stack[stackTraceSize];
	unsigned short frames;
	frames = CaptureStackBackTrace(0, stackTraceSize, stack, NULL);

	SYMBOL_INFO* symbol = (SYMBOL_INFO *)malloc(
		sizeof(SYMBOL_INFO) + stackTraceStrSize * sizeof(char));
	assert(symbol);
	symbol->MaxNameLen = stackTraceStrSize - 1;
	symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

	ArrayList<String> result;
	for (int c = 0; c < frames; ++c) {
		SymFromAddr(process, (DWORD64)(stack[c]), 0, symbol);
		String str;
		str.alloc(stackTraceStrSize);
		str.format("#%d %s (%p)", c + 1, symbol->Name, symbol->Address);
		result.push(str);
	}

	free(symbol);
	return result;
}

#else // POSIX

#include <execinfo.h>

ArrayList<String> Engine::stackTrace() const {
	void* funcs[stackTraceSize];
	int numCalls = backtrace(funcs, stackTraceSize);
	char** strings = backtrace_symbols(funcs, numCalls);

	ArrayList<String> result;
	for (int c = 0; c < numCalls; ++c) {
		String str;
		str.alloc(stackTraceStrSize);
		str.format("#%d %s (%p)", c + 1, strings[c], funcs[c]);
		result.push(str);
	}

	free(strings);
	return result;
}

#endif

void Engine::handleSIGSEGV(int input) {
	auto stack = mainEngine->stackTrace();
	mainEngine->fmsg(Engine::MSG_FATAL, "Segmentation fault. Stack dump:");
	for (auto& str : stack) {
		mainEngine->fmsg(Engine::MSG_FATAL, " %s", str.get());
	}
	exit(input);
}