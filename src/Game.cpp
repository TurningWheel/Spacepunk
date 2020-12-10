// Game.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "World.hpp"
#include "BasicWorld.hpp"
#include "Camera.hpp"
#include "Game.hpp"
#include "Generator.hpp"

Game::~Game() {
	mainEngine->fmsg(Engine::MSG_INFO, "cleaning up gamestate");

	if (net) {
		net->setParent(nullptr);
		net->term();
		delete net;
		net = nullptr;
	}

	// free world data
	while (worlds.getFirst()) {
		delete worlds.getFirst()->getData();
		worlds.removeNode(worlds.getFirst());
	}
}

void Game::init() {
	mainEngine->fmsg(Engine::MSG_INFO, "initializing gamestate");
}

void Game::incrementFrame() {
	if (framesToRun < 4) {
		++framesToRun;
	}
}

World* Game::loadWorld(const char* filename, bool buildPath) {
	World* world = nullptr;
	if (buildPath) {
		String path = mainEngine->buildPath(filename).get();

		auto bworld = new BasicWorld(this, false, (Uint32)worlds.getSize(), "Untitled World");
		bworld->changeFilename(path.get());
		FileHelper::readObject(path.get(), *bworld);
		bworld->initialize(!bworld->isLoaded());
		world = bworld;
	} else {
		auto bworld = new BasicWorld(this, false, (Uint32)worlds.getSize(), "Untitled World");
		bworld->changeFilename(filename);
		FileHelper::readObject(filename, *bworld);
		bworld->initialize(!bworld->isLoaded());
		world = bworld;
	}

	// unload our existing copy of the world if we've already loaded it
	bool alreadyExists = false;
	for (auto otherWorldNode = worlds.getFirst(); otherWorldNode != nullptr; otherWorldNode = otherWorldNode->getNext()) {
		auto otherWorld = otherWorldNode->getData();
		if (otherWorld->getFilename() == world->getFilename()) {
			delete otherWorld;
			otherWorldNode->setData(world);
			alreadyExists = true;
			break;
		}
	}
	if (!alreadyExists) {
		worlds.addNodeLast(world);
	}
	return world;
}

BasicWorld* Game::newBasicWorld(const char* name) {
	BasicWorld* world = new BasicWorld(this, false, (Uint32)worlds.getSize(), name);
	world->initialize(!world->isLoaded());
	worlds.addNodeLast(world);
	return world;
}

World* Game::worldForName(const char* name) {
	for (auto world : worlds) {
		if (world->getFilename() == name) {
			return world;
		}
	}
	return nullptr;
}

Uint32 Game::indexForWorld(World* world) {
	Uint32 index;
	Node<World*>* node;
	for (index = 0, node = worlds.getFirst(); node != nullptr; node = node->getNext(), ++index) {
		if (node->getData() == world) {
			return index;
		}
	}
	return invalidID;
}

void Game::closeAllWorlds() {
	while (worlds.getFirst()) {
		delete worlds.getFirst()->getData();
		worlds.removeNode(worlds.getFirst());
	}
}

void Game::preProcess() {
	for (Uint32 frame = 0; frame < framesToRun; ++frame) {
		// update net interface
		if (net) {
			net->update();
		}

		for (Node<World*>* node = worlds.getFirst(); node != nullptr; node = node->getNext()) {
			World* world = node->getData();
			world->preProcess();
		}
	}
}

void Game::process() {
	for (Uint32 frame = 0; frame < framesToRun; ++frame) {
		// process worlds
		for (Node<World*>* node = worlds.getFirst(); node != nullptr; node = node->getNext()) {
			World* world = node->getData();
			world->process();
		}
	}
}

void Game::postProcess() {
	for (Node<World*>* node = worlds.getFirst(); node != nullptr; node = node->getNext()) {
		World* world = node->getData();
		world->postProcess();
	}

	// increment ticks
	++ticks;
}

Player* Game::findPlayer(Uint32 clientID, Uint32 localID) {
	for (Node<Player>* node = players.getFirst(); node != nullptr; node = node->getNext()) {
		Player& player = node->getData();
		if (player.getClientID() == clientID && player.getLocalID() == localID) {
			return &player;
		}
	}
	return nullptr;
}

Player* Game::findPlayer(Uint32 serverID) {
	for (Node<Player>* node = players.getFirst(); node != nullptr; node = node->getNext()) {
		Player& player = node->getData();
		if (player.getServerID() == serverID) {
			return &player;
		}
	}
	return nullptr;
}

int Game::numLocalPlayers() const {
	int result = 0;
	for (const Node<Player>* node = players.getFirst(); node != nullptr; node = node->getNext()) {
		const Player& player = node->getData();

		if (player.getClientID() == invalidID) {
			++result;
		}
	}
	return result;
}