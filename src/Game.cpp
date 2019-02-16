// Game.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "World.hpp"
#include "TileWorld.hpp"
#include "SectorWorld.hpp"
#include "Camera.hpp"
#include "Game.hpp"
#include "Tile.hpp"
#include "Generator.hpp"

Game::Game() {
}

Game::~Game() {
	mainEngine->fmsg(Engine::MSG_INFO,"cleaning up gamestate");

	if( net ) {
		net->setParent(nullptr);
		net->term();
		delete net;
		net = nullptr;
	}

	// free world data
	while( worlds.getFirst() ) {
		delete worlds.getFirst()->getData();
		worlds.removeNode(worlds.getFirst());
	}
}

void Game::init() {
	mainEngine->fmsg(Engine::MSG_INFO,"initializing gamestate");
}

void Game::incrementFrame() {
	if( framesToRun < 4 ) {
		++framesToRun;
	}
}

World* Game::loadWorld(const char* filename, bool buildPath) {
	World* world = nullptr;
	if (buildPath) {
		StringBuf<64> fullPath("maps/%s", 1, filename);
		String path = mainEngine->buildPath(fullPath.get()).get();

		size_t len = strlen(filename);
		if( len >= 4 && strcmpi((const char*)(filename + len - 4), ".swd") == 0 ) {
			world = new SectorWorld(this, false, (Uint32)worlds.getSize(), "Untitled World");
		} else {
			world = new TileWorld(this, false, (Uint32)worlds.getSize(), Tile::SIDE_EAST, path.get());
		}
		world->initialize(!world->isLoaded());
	} else {
		size_t len = strlen(filename);
		if( len >= 4 && strcmpi((const char*)(filename + len - 4), ".swd") == 0 ) {
			world = new SectorWorld(this, false, (Uint32)worlds.getSize(), "Untitled World");
		} else {
			world = new TileWorld(this, false, (Uint32)worlds.getSize(), Tile::SIDE_EAST, filename);
		}
		world->initialize(!world->isLoaded());
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

SectorWorld* Game::newSectorWorld(const char* name) {
	SectorWorld* world = new SectorWorld(this, false, (Uint32)worlds.getSize(), name);
	world->initialize(!world->isLoaded());
	worlds.addNodeLast(world);
	return world;
}

TileWorld* Game::newTileWorld(const char* name, int width, int height) {
	TileWorld* world = new TileWorld(this, false, (Uint32)worlds.getSize(), Tile::SIDE_EAST, "", width, height, name);
	world->initialize(!world->isLoaded());
	worlds.addNodeLast(world);
	return world;
}

TileWorld* Game::genTileWorld(const char* path) {
	String fullPath = mainEngine->buildPath(path);

	Generator gen(isClient());
	FileHelper::readObject(fullPath, gen);
	gen.createDungeon();

	TileWorld* world = new TileWorld(this, (Uint32)worlds.getSize(), path, gen);
	world->initialize(!world->isLoaded());
	worlds.addNodeLast(world);

	return world;
}

World* Game::worldForName(const char* name) {
	for (auto world : worlds) {
		if (world->getShortname() == name) {
			return world;
		}
	}
	return nullptr;
}

Uint32 Game::indexForWorld(World* world) {
	Uint32 index;
	Node<World*>* node;
	for( index = 0, node = worlds.getFirst(); node != nullptr; node = node->getNext(), ++index ) {
		if( node->getData() == world ) {
			return index;
		}
	}
	return invalidID;
}

void Game::closeAllWorlds() {
	while( worlds.getFirst() ) {
		delete worlds.getFirst()->getData();
		worlds.removeNode(worlds.getFirst());
	}
}

void Game::preProcess() {
	// stub
}

void Game::process() {
	for( Uint32 frame=0; frame<framesToRun; ++frame ) {
		// update net interface
		if( net ) {
			net->update();
		}

		// process worlds
		for( Node<World*>* node=worlds.getFirst(); node!=nullptr; node=node->getNext() ) {
			World* world = node->getData();
			world->process();
		}

		// increment ticks
		++ticks;
	}
}

void Game::postProcess() {
	// stub
}

Player* Game::findPlayer(Uint32 clientID, Uint32 localID) {
	for( Node<Player>* node = players.getFirst(); node != nullptr; node = node->getNext() ) {
		Player& player = node->getData();
		if( player.getClientID() == clientID && player.getLocalID() == localID ) {
			return &player;
		}
	}
	return nullptr;
}

Player* Game::findPlayer(Uint32 serverID) {
	for( Node<Player>* node = players.getFirst(); node != nullptr; node = node->getNext() ) {
		Player& player = node->getData();
		if( player.getServerID() == serverID ) {
			return &player;
		}
	}
	return nullptr;
}

int Game::numLocalPlayers() const {
	int result = 0;
	for( const Node<Player>* node = players.getFirst(); node != nullptr; node = node->getNext() ) {
		const Player& player = node->getData();

		if( player.getClientID() == invalidID ) {
			++result;
		}
	}
	return result;
}