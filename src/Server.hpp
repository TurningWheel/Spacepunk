//! @file Server.hpp

#pragma once

#include "Game.hpp"

class Script;

//! A Server implements the Game interface and lives in the Engine.
//! In order to play a singleplayer or listening game, the engine instantiates a Client and a Server, meaning there are two Game states running at once.
//! A Server will dictate game updates to a client, but in other respects they are mostly identical.
class Server : public Game {
public:
	Server();
	virtual ~Server();

	//! getters & setters
	virtual bool	isServer() const override { return true; }
	virtual bool	isClient() const override { return false; }

	//! starts up the server
	virtual void init() override;

	//! read messages from the net interface
	void handleNetMessages();

	//! perform pre-processing on the current frame
	virtual void preProcess() override;

	//! process the current frame
	virtual void process() override;

	//! perform post-processing on the current frame
	virtual void postProcess() override;

	//! called when a client connects to our server
	virtual void onEstablishConnection(Uint32 remoteID) override;

	//! called when we disconnect from a client
	//! @param remoteID canonical id associated with this client
	virtual void onDisconnect(Uint32 remoteID) override;

	//! update the specified client about players on this server
	//! @param remoteID id of the client to update
	void updateClientAboutPlayers(Uint32 remoteID);

	//! update all clients about the players that are connected to me
	void updateAllClientsAboutPlayers();

private:
	Script* script = nullptr;
};