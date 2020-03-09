// Main.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "Rotation.hpp"
#include "LinkedList.hpp"
#include "Console.hpp"

Engine* mainEngine = nullptr;

const float Rotation::radiansToDegrees = 180.f / PI;
const float PI = 3.14159265358979323f;
const float SQRT2 = 1.41421356237309504f;
const char* versionStr = "0.0.0.1";

int main(int argc, char **argv) {
	mainEngine = new Engine(argc, argv);

	// initialize mainEngine
	mainEngine->init();
	if (!mainEngine->isInitialized()) {
		mainEngine->fmsg(Engine::MSG_CRITICAL, "failed to start engine.");
		delete mainEngine;
		return 1;
	} else {
		// load default config
		const char* configName = "autoexec.cfg";
		mainEngine->loadConfig(configName);
	}

	// main loop
	while (mainEngine->isRunning()) {
		mainEngine->preProcess();
		mainEngine->process();
		mainEngine->postProcess();
	}

	delete mainEngine;

	return 0;
}