// Game.hpp
// Oversees game state, controls worlds, etc.
// Abstract class, implemented by Server and Client

#pragma once

#include "LinkedList.hpp"
#include "Player.hpp"
#include "Net.hpp"

class Engine;
class Entity;
class World;
class TileWorld;
class SectorWorld;

class Game {
public:
	Game();
	virtual ~Game();

	// invalid world id
	static const Uint32 invalidID = UINT32_MAX;

	// getters & setters
	World*				getWorld(const int index)		{ return worlds[index]->getData(); }
	const Uint32		getTicks() const				{ return ticks; }
	Net*				getNet() const					{ return net; }
	bool				isSuicide() const				{ return suicide; }
	const LinkedList<Player>	getPlayers() const				{ return players; }
	virtual bool		isServer() const = 0;
	virtual bool		isClient() const = 0;

	// marks the sim to be reinitialized
	void reset() { suicide = true; }

	// causes the game to execute game logic this cycle
	void incrementFrame();

	// @return the number of worlds in this sim
	size_t numWorlds() const { return worlds.getSize(); }

	// finds the world node with the given name (case sensitive) and returns it
	// @param name: the name of the world (not the filename, but the title)
	// @return the world node with the given name or nullptr if the world node could not be found
	Node<World*>* worldForName(const char* name);

	// finds the index of the given world
	// @param world: the world whose index we are looking for
	// @return the index associated by that world or invalidID if it could not be found
	Uint32 indexForWorld(World* world);

	// loads the world with the given filename
	// if the given world is already loaded, it is unloaded and then reloaded
	// @param filename: the filename of the world to load
	// @return a pointer to the World, or nullptr on error
	World* loadWorld(const char* filename, bool buildPath);

	// creates a new sector world
	// @param name: the world's name
	// @return the newly created world, or nullptr on error
	SectorWorld* newSectorWorld(const char* name);

	// creates a new tile world with the given attributes
	// @param name: the world's name
	// @param width: the width of the world, in tiles
	// @param height: the height of the world, in tiles
	// @return the newly created world, or nullptr on error
	TileWorld* newTileWorld(const char* name, int width, int height);

	// generates a new tile world
	// @param path: the path to the generator .json
	// @return the newly generated world, or nullptr on error
	TileWorld* genTileWorld(const char* path);

	// locate the player with the given id nums
	// @param clientID: id of the player's client
	// @param localID: id of the player on the client
	// @return the associated Player, or nullptr if the Player could not be found
	Player* findPlayer(Uint32 clientID, Uint32 localID);

	// locate the player with the given server id
	// @param serverID: canonical player id
	// @return the associated Player, or nullptr if the Player could not be found
	Player* findPlayer(Uint32 serverID);

	// when a net connection is setup, this function gets called
	virtual void onEstablishConnection(Uint32 remoteID) = 0;

	// when a net connection is ended, this function gets called
	// @param remoteID: index associated with the disconnected host
	virtual void onDisconnect(Uint32 remoteID) = 0;

	// shuts down all world instances
	void closeAllWorlds();

	// calculate the total number of local players
	// @return the number of local players
	int numLocalPlayers() const;

protected:
	// sets up the game
	virtual void init();

	// perform pre-processing on the current frame
	virtual void preProcess();

	// process the current frame
	virtual void process();

	// perform post-processing on the current frame
	virtual void postProcess();

	LinkedList<Player> players;
	LinkedList<World*> worlds;
	Uint32 ticks=0;
	Uint32 framesToRun=0;
	bool suicide=false; // if a game wants to end itself, setting this flag is the way to do it.

	// the net interface
	Net* net = nullptr;
};