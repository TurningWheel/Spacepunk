// Server.cpp

#include "Main.hpp"
#include "LinkedList.hpp"
#include "Node.hpp"
#include "Renderer.hpp"
#include "Entity.hpp"
#include "World.hpp"
#include "Script.hpp"
#include "Server.hpp"
#include "Engine.hpp"
#include "NetSDL.hpp"
#include "Console.hpp"
#include "TileWorld.hpp"
#include "BBox.hpp"

Server::Server() {
	net = new NetSDL(*this);
	script = new Script(*this);
}

Server::~Server() {
	mainEngine->fmsg(Engine::MSG_INFO,"shutting down server");

	// free script engine
	if( script ) {
		script->dispatch("term");
		delete script;
	}
}

void Server::init() {
	net->init();
	net->host(Net::defaultPort);

	script->load("scripts/server/main.lua");
	script->dispatch("init");

	// start a playtest
	if( mainEngine->isPlayTest() ) {
		StringBuf<32> path(".playtest.wlb");
		loadWorld(path.get(),true);
	}
}

void Server::handleNetMessages() {
	if( !net ) {
		return;
	}

	for( int remoteIndex=0; remoteIndex < (int)net->numRemoteHosts(); ++remoteIndex ) {
		while( !net->lockThread() );

		// receive packets
		Packet* recvPacket = nullptr;
		while( (recvPacket=net->recvPacket(remoteIndex)) != nullptr ) {
			Packet packet(*recvPacket);
			delete recvPacket;

			Uint32 id, timestamp;
			if( packet.read32(id) && packet.read32(timestamp) ) {
				char packetType[4] = {0};
				if( packet.read(packetType, 4) ) {
					if( int result = net->handleNetworkPacket(packet, (const char*)packetType, id) ) {
						if( result == 2 ) {
							// this means they've disconnected, so stop expecting more packets -- you won't get any
							--remoteIndex;
							break;
						} else {
							continue;
						}
					}
					
					// chat message
					if( strncmp( (const char*)packetType, "CMSG", 4) == 0 ) {
						Uint32 msgLen;
						if( packet.read32(msgLen) ) {
							char* msg = new char[msgLen+1];
							if( msg ) {
								msg[msgLen] = 0;
								packet.read(msg,msgLen);

								Packet msgPacket;
								msgPacket.write(msg);
								msgPacket.write32(msgLen);
								msgPacket.write("CMSG");
								net->signPacket(msgPacket);

								for( Uint32 c = 0; c < net->getRemoteHosts().getSize(); ++c ) {
									const Net::remote_t* remote = net->getRemoteHosts()[c];
									net->sendPacketSafe(remote->id,msgPacket);
								}
								delete[] msg;
							}
						}

						continue;
					}

					// player spawn
					else if( strncmp( (const char*)packetType, "SPWN", 4) == 0 ) {
						if( numWorlds() <= 0 ) {
							mainEngine->fmsg(Engine::MSG_ERROR,"Client wants to spawn, but there are no worlds!");
							continue;
						}

						bool playerSpawned = false;
						Uint32 clientID = id;
						Uint32 localID = Player::invalidID;
						Uint32 serverID = Player::invalidID;
						if( packet.read32(localID) ) {
							Player* player = findPlayer(clientID, localID);

							// this player has not been created yet, so go ahead and add them to the game
							if( player == nullptr ) {
								Player::colors_t colors;

								// read name
								Uint8 nameLen;
								StringBuf<64> nameStr = "";
								if( packet.read8(nameLen) ) {
									char* name = new char[nameLen+1];
									if( name ) {
										packet.read(name, nameLen);
										name[nameLen] = '\0';
										nameStr = name;
										delete[] name;
									}
								}

								// read head colors
								Uint8 headR[3], headG[3], headB[3];
								packet.read8(headR[0]); packet.read8(headR[1]); packet.read8(headR[2]);
								packet.read8(headG[0]); packet.read8(headG[1]); packet.read8(headG[2]);
								packet.read8(headB[0]); packet.read8(headB[1]); packet.read8(headB[2]);
								colors.headRChannel = { headR[0] / 255.f, headR[1] / 255.f, headR[2] / 255.f, 1.f };
								colors.headGChannel = { headG[0] / 255.f, headG[1] / 255.f, headG[2] / 255.f, 1.f };
								colors.headBChannel = { headB[0] / 255.f, headB[1] / 255.f, headB[2] / 255.f, 1.f };

								// read torso colors
								Uint8 torsoR[3], torsoG[3], torsoB[3];
								packet.read8(torsoR[0]); packet.read8(torsoR[1]); packet.read8(torsoR[2]);
								packet.read8(torsoG[0]); packet.read8(torsoG[1]); packet.read8(torsoG[2]);
								packet.read8(torsoB[0]); packet.read8(torsoB[1]); packet.read8(torsoB[2]);
								colors.torsoRChannel = { torsoR[0] / 255.f, torsoR[1] / 255.f, torsoR[2] / 255.f, 1.f };
								colors.torsoGChannel = { torsoG[0] / 255.f, torsoG[1] / 255.f, torsoG[2] / 255.f, 1.f };
								colors.torsoBChannel = { torsoB[0] / 255.f, torsoB[1] / 255.f, torsoB[2] / 255.f, 1.f };

								// read arms colors
								Uint8 armsR[3], armsG[3], armsB[3];
								packet.read8(armsR[0]); packet.read8(armsR[1]); packet.read8(armsR[2]);
								packet.read8(armsG[0]); packet.read8(armsG[1]); packet.read8(armsG[2]);
								packet.read8(armsB[0]); packet.read8(armsB[1]); packet.read8(armsB[2]);
								colors.armsRChannel = { armsR[0] / 255.f, armsR[1] / 255.f, armsR[2] / 255.f, 1.f };
								colors.armsGChannel = { armsG[0] / 255.f, armsG[1] / 255.f, armsG[2] / 255.f, 1.f };
								colors.armsBChannel = { armsB[0] / 255.f, armsB[1] / 255.f, armsB[2] / 255.f, 1.f };

								// read feet colors
								Uint8 feetR[3], feetG[3], feetB[3];
								packet.read8(feetR[0]); packet.read8(feetR[1]); packet.read8(feetR[2]);
								packet.read8(feetG[0]); packet.read8(feetG[1]); packet.read8(feetG[2]);
								packet.read8(feetB[0]); packet.read8(feetB[1]); packet.read8(feetB[2]);
								colors.feetRChannel = { feetR[0] / 255.f, feetR[1] / 255.f, feetR[2] / 255.f, 1.f };
								colors.feetGChannel = { feetG[0] / 255.f, feetG[1] / 255.f, feetG[2] / 255.f, 1.f };
								colors.feetBChannel = { feetB[0] / 255.f, feetB[1] / 255.f, feetB[2] / 255.f, 1.f };

								// create new player
								Player newPlayer(nameStr.get(), colors);

								serverID = (Uint32)players.getSize();
								newPlayer.setServerID(serverID);
								newPlayer.setClientID(clientID);
								newPlayer.setLocalID(localID);

								player = &players.addNodeLast(newPlayer)->getData();
							}

							// pick a random world to spawn in
							int worldID = mainEngine->getRandom().getUint32() % numWorlds();
							World* world = getWorld(worldID);

							// count spawn locations in world
							LinkedList<Entity*> spawnLocations;
							for (auto pair : world->getEntities()) {
								Entity* entity = pair.b;

								if( strcmp(entity->getScriptStr(),"PlayerStart")==0 ) {
									if( !entity->checkCollision(entity->getPos()) ) {
										spawnLocations.addNodeLast(entity);
									}
								}
							}
							
							// pick spawn location at random
							if( spawnLocations.getSize() > 0 ) {
								Uint32 spawnIndex = mainEngine->getRandom().getUint32() % spawnLocations.getSize();
								Node<Entity*>* node = spawnLocations.nodeForIndex(spawnIndex);
								Entity* entity = node->getData();
								playerSpawned = player->spawn(*world, entity->getPos(), entity->getAng());

								if( playerSpawned ) {
									// tell client where to spawn their player
									Packet packet;

									packet.write32(entity->getAng().degreesRoll());
									packet.write32(entity->getAng().degreesPitch());
									packet.write32(entity->getAng().degreesYaw());
									packet.write32(entity->getPos().z);
									packet.write32(entity->getPos().y);
									packet.write32(entity->getPos().x);

									packet.write(world->getShortname().get());
									packet.write32((Uint32)world->getShortname().length());

									packet.write32(serverID);
									packet.write32(localID);
									packet.write32(clientID);
									packet.write("SPWN");

									net->signPacket(packet);
									net->sendPacketSafe(clientID, packet);

									// update other clients about this player
									for( Uint32 c = 0; c < net->getRemoteHosts().getSize(); ++c ) {
										Net::remote_t* remote = net->getRemoteHosts()[c];
										if( remote->id == clientID ) {
											continue;
										}
										updateClientAboutPlayers(remote->id);
									}
								}
							} else {
								mainEngine->fmsg(Engine::MSG_ERROR,"Client wants to spawn, but there are no spawn locations!");
							}
						}
						
						if( !playerSpawned ) {
							mainEngine->fmsg(Engine::MSG_ERROR,"Failed to spawn player (%d) of client (%d)!", localID, clientID);
						}

						continue;
					}

					// entity remote function call
					else if (strncmp((const char*)packetType, "ENTF", 4) == 0) {
						// read world
						Uint32 worldID;
						packet.read32(worldID);
						Node<World*>* node = worlds[worldID];
						if (node) {
							World& world = *node->getData();

							// get uid
							Uint32 uid;
							packet.read32(uid);
							Entity* entity = world.uidToEntity(uid);
							if (!entity) {
								continue;
							}

							// read func name
							Uint32 funcNameLen;
							packet.read32(funcNameLen);
							char funcName[128];
							funcName[127] = '\0';
							packet.read(funcName, funcNameLen);
							funcName[funcNameLen] = '\0';

							// read args
							Uint32 argsLen;
							packet.read32(argsLen);
							Script::Args args;
							for (Uint32 c = 0; c < argsLen; ++c) {
								char argType;
								packet.read8((Uint8&)argType);
								switch (argType) {
								case 'b': {
									char value;
									packet.read8((Uint8&)value);
									if (value == 't') {
										args.addBool(true);
									} else if (value == 'f') {
										args.addBool(false);
									}
									break;
								}
								case 'i': {
									Uint32 value;
									packet.read32(value);
									args.addInt((int)value);
									break;
								}
								case 'f': {
									float value;
									packet.read32((Uint32&)value);
									args.addFloat(value);
									break;
								}
								case 's': {
									Uint32 len;
									packet.read32(len);
									String value;
									value.alloc(len + 1);
									value[len] = '\0';
									packet.read(&value[0], len);
									args.addString(value);
									break;
								}
								case 'p': {
									mainEngine->fmsg(Engine::MSG_ERROR, "Client got RFC with a pointer, will be nullptr");
									args.addPointer(nullptr);
									break;
								}
								case 'n': {
									args.addNil();
									break;
								}
								default: {
									mainEngine->fmsg(Engine::MSG_ERROR, "Unknown arg type for remote function call!");
									args.addNil();
									break;
								}
								}
							}

							// run function
							entity->dispatch(funcName, args);
						}

						continue;
					}

					// player update
					else if( strncmp( (const char*)packetType, "PLAY", 4) == 0 ) {
						Uint32 localID;
						if( packet.read32(localID) ) {
							Player* player = findPlayer(id, localID);
							if( player && player->getEntity() ) {
								Entity* entity = player->getEntity();
								entity->setLastUpdate(entity->getTicks());

								// get their current world
								Uint32 worldID;
								packet.read32(worldID);
								Node<World*>* node = worlds.nodeForIndex(worldID);
								World* world = node->getData();

								// only update the player's position if they are on the world that they say they are.
								if( world != entity->getWorld() ) {
									Packet packet;
									packet.write32(entity->getOffset().z);
									packet.write32(entity->getOffset().y);
									packet.write32(entity->getOffset().x);

									const Entity* anchor = entity->getAnchor();
									packet.write32(anchor ? anchor->getUID() : World::nuid);

									World* world = entity->getWorld();
									assert(world);
									packet.write(world->getShortname().get());
									packet.write32((Uint32)world->getShortname().length());

									packet.write32(player->getServerID());
									packet.write32(player->getLocalID());
									packet.write32(player->getClientID());

									packet.write("PLVL");

									net->signPacket(packet);
									net->sendPacket(id,packet);
								} else {
									// read pos
									Uint32 posInt[3];
									packet.read32(posInt[0]);
									packet.read32(posInt[1]);
									packet.read32(posInt[2]);
									Vector pos( ((Sint32)posInt[0]) / 32.f, ((Sint32)posInt[1]) / 32.f, ((Sint32)posInt[2]) / 32.f );

									// read vel
									Uint32 velInt[3];
									packet.read32(velInt[0]);
									packet.read32(velInt[1]);
									packet.read32(velInt[2]);
									Vector vel( ((Sint32)velInt[0]) / 128.f, ((Sint32)velInt[1]) / 128.f, ((Sint32)velInt[2]) / 128.f );

									// read ang
									Uint32 angInt[3];
									packet.read32(angInt[0]);
									packet.read32(angInt[1]);
									packet.read32(angInt[2]);
									Angle ang( (((Sint32)angInt[0]) * PI / 180.f) / 32.f, (((Sint32)angInt[1]) * PI / 180.f) / 32.f, (((Sint32)angInt[2]) * PI / 180.f) / 32.f );

									entity->setNewPos(pos);
									entity->setNewAng(ang);
									entity->setVel(vel);
									entity->setLastUpdate(entity->getTicks());

									// read falling status
									Uint8 falling;
									packet.read8(falling);
									entity->setFalling((falling == 1) ? true : false);

									// read crouch status
									Uint8 crouch;
									packet.read8(crouch);
									player->putInCrouch((crouch == 1) ? true : false);

									// read moving status
									Uint8 moving;
									packet.read8(moving);
									player->setMoving((moving == 1) ? true : false);

									// read jumped status
									Uint8 jumped;
									packet.read8(jumped);
									player->setJumped((jumped == 1) ? true : false);

									// read look direction
									Uint32 lookDirInt[3];
									packet.read32(lookDirInt[0]);
									packet.read32(lookDirInt[1]);
									packet.read32(lookDirInt[2]);
									Angle lookDir( (((Sint32)lookDirInt[0]) * PI / 180.f) / 32.f, (((Sint32)lookDirInt[1]) * PI / 180.f) / 32.f, (((Sint32)lookDirInt[2]) * PI / 180.f) / 32.f );
									player->setLookDir(lookDir);
								}
							}
						}
					}

					// entity selected
					else if( strncmp( (const char*)packetType, "ESEL", 4) == 0 ) {
						Uint32 localID;
						Entity *playerEntity = nullptr;
						Entity *selectedEntity = nullptr;
						BBox *selectedBBox = nullptr;
						Uint32 worldID;
						Player* player = nullptr;
						Uint32 selectedEntityUID;
						Uint32 selectedBBoxUID;

						if( packet.read32(localID) ) {
							player = findPlayer(id, localID);
							if( player && player->getEntity() ) {
								playerEntity = player->getEntity();

								// get player's current world
								packet.read32(worldID);
								Node<World*>* node = worlds.nodeForIndex(worldID);
								World* world = node->getData();
								if ( world && packet.read32(selectedEntityUID) )
								{
									selectedEntity = world->uidToEntity(selectedEntityUID);
									if ( packet.read32(selectedBBoxUID) )
									{
										selectedBBox = selectedEntity->findComponentByUID<BBox>(selectedBBoxUID);
									}
								}
							}
						}

						if ( selectedEntity && selectedBBox && playerEntity )
						{
							mainEngine->fmsg(Engine::MSG_DEBUG, "Client %d selected entity '%s': UID %d", localID, selectedEntity->getName().get(), selectedEntity->getUID());
							selectedEntity->interact(*playerEntity, *selectedBBox);

							Packet packet;
							packet.write32(selectedEntityUID);
							packet.write32(worldID);
							packet.write32(player->getServerID());
							packet.write("PINT");
							getNet()->signPacket(packet);
							getNet()->broadcastSafe(packet);
						}
					}
				}
			}
		}

		// unlock packet receiving thread
		net->unlockThread();
	}
}

void Server::onEstablishConnection(Uint32 remoteID) {
	Packet packet;
	for( Node<World*>* node = worlds.getLast(); node != nullptr; node = node->getPrev() ) {
		World* world = node->getData();

		if( world->isGenerated() ) {
			packet.write(world->getZone().get());
			packet.write32((Uint32)world->getZone().length());
			packet.write8('g');
		} else {
			packet.write(world->getShortname().get());
			packet.write32((Uint32)world->getShortname().length());
			packet.write8('f');
		}
	}
	packet.write32((Uint32)worlds.getSize());
	packet.write("MAPS");
	net->signPacket(packet);
	net->sendPacketSafe(remoteID, packet);

	// tell client about connected players
	updateClientAboutPlayers(remoteID);
}

void Server::updateAllClientsAboutPlayers() {
	for( Uint32 c = 0; c < net->getRemoteHosts().getSize(); ++c ) {
		Net::remote_t* remote = net->getRemoteHosts()[c];
		updateClientAboutPlayers(remote->id);
	}
}

void Server::updateClientAboutPlayers(Uint32 remoteID) {
	for( Node<Player>* node = players.getFirst(); node != nullptr; node = node->getNext() ) {
		Player& player = node->getData();

		Packet packet;

		packet.write8((Uint8)floor(player.getColors().feetBChannel[2] * 255.f));
		packet.write8((Uint8)floor(player.getColors().feetBChannel[1] * 255.f));
		packet.write8((Uint8)floor(player.getColors().feetBChannel[0] * 255.f));
		packet.write8((Uint8)floor(player.getColors().feetGChannel[2] * 255.f));
		packet.write8((Uint8)floor(player.getColors().feetGChannel[1] * 255.f));
		packet.write8((Uint8)floor(player.getColors().feetGChannel[0] * 255.f));
		packet.write8((Uint8)floor(player.getColors().feetRChannel[2] * 255.f));
		packet.write8((Uint8)floor(player.getColors().feetRChannel[1] * 255.f));
		packet.write8((Uint8)floor(player.getColors().feetRChannel[0] * 255.f));

		packet.write8((Uint8)floor(player.getColors().armsBChannel[2] * 255.f));
		packet.write8((Uint8)floor(player.getColors().armsBChannel[1] * 255.f));
		packet.write8((Uint8)floor(player.getColors().armsBChannel[0] * 255.f));
		packet.write8((Uint8)floor(player.getColors().armsGChannel[2] * 255.f));
		packet.write8((Uint8)floor(player.getColors().armsGChannel[1] * 255.f));
		packet.write8((Uint8)floor(player.getColors().armsGChannel[0] * 255.f));
		packet.write8((Uint8)floor(player.getColors().armsRChannel[2] * 255.f));
		packet.write8((Uint8)floor(player.getColors().armsRChannel[1] * 255.f));
		packet.write8((Uint8)floor(player.getColors().armsRChannel[0] * 255.f));

		packet.write8((Uint8)floor(player.getColors().torsoBChannel[2] * 255.f));
		packet.write8((Uint8)floor(player.getColors().torsoBChannel[1] * 255.f));
		packet.write8((Uint8)floor(player.getColors().torsoBChannel[0] * 255.f));
		packet.write8((Uint8)floor(player.getColors().torsoGChannel[2] * 255.f));
		packet.write8((Uint8)floor(player.getColors().torsoGChannel[1] * 255.f));
		packet.write8((Uint8)floor(player.getColors().torsoGChannel[0] * 255.f));
		packet.write8((Uint8)floor(player.getColors().torsoRChannel[2] * 255.f));
		packet.write8((Uint8)floor(player.getColors().torsoRChannel[1] * 255.f));
		packet.write8((Uint8)floor(player.getColors().torsoRChannel[0] * 255.f));

		packet.write8((Uint8)floor(player.getColors().headBChannel[2] * 255.f));
		packet.write8((Uint8)floor(player.getColors().headBChannel[1] * 255.f));
		packet.write8((Uint8)floor(player.getColors().headBChannel[0] * 255.f));
		packet.write8((Uint8)floor(player.getColors().headGChannel[2] * 255.f));
		packet.write8((Uint8)floor(player.getColors().headGChannel[1] * 255.f));
		packet.write8((Uint8)floor(player.getColors().headGChannel[0] * 255.f));
		packet.write8((Uint8)floor(player.getColors().headRChannel[2] * 255.f));
		packet.write8((Uint8)floor(player.getColors().headRChannel[1] * 255.f));
		packet.write8((Uint8)floor(player.getColors().headRChannel[0] * 255.f));

		packet.write(player.getName());
		packet.write8((Uint8)strlen(player.getName()));

		packet.write32(player.getServerID());
		packet.write32(player.getLocalID());
		packet.write32(player.getClientID());

		packet.write("CENS");
		net->signPacket(packet);
		net->sendPacketSafe(remoteID, packet);
	}
}

void Server::onDisconnect(Uint32 remoteID) {
	Node<Player>* node;
	Node<Player>* nextNode;
	for( node = players.getFirst(); node != nullptr; node = nextNode ) {
		nextNode = node->getNext();
		Player& player = node->getData();
		if( player.getClientID() == remoteID ) {
			player.despawn();
			players.removeNode(node);
		}
	}
}

void Server::preProcess() {
	if( framesToRun ) {
		script->dispatch("preprocess");
	}

	handleNetMessages();

	Game::preProcess();
}

void Server::process() {
	Game::process();

	for( Uint32 frame=0; frame<framesToRun; ++frame ) {
		script->dispatch("process");
	}
}

void Server::postProcess() {
	Game::postProcess();

	if( framesToRun ) {
		script->dispatch("postprocess");

		// send entity updates to client
		if( net->isConnected() ) {
			if( ticks % (mainEngine->getTicksPerSecond()/10) == 0 ) {
				for( auto world : worlds ) {
					for (auto pair : world->getEntities()) {
						auto entity = pair.b;

						if( !entity->isFlag(Entity::flag_t::FLAG_UPDATE) || entity->isFlag(Entity::flag_t::FLAG_LOCAL) ) {
							// don't update local-only entities
							continue;
						}

						for( Uint32 c = 0; c < net->getRemoteHosts().getSize(); ++c ) {
							const Net::remote_t* remote = net->getRemoteHosts()[c];

							Player* player = entity->getPlayer();
							if( player && player->getClientID() == remote->id ) {
								// do not (normally) tell a client where their players are!
								continue;
							}

							// update entity for client
							Packet packet;
							entity->updatePacket(packet);
							net->signPacket(packet);
							net->sendPacket(remote->id, packet);
						}
					}
				}
			}
		}

		framesToRun=0;
	}
}

static int console_serverDisconnect(int argc, const char** argv) {
	Server* server = mainEngine->getLocalServer();
	if( server ) {
		server->getNet()->disconnectAll();
		return 0;
	} else {
		return 1;
	}
}

static int console_serverReset(int argc, const char** argv) {
	Server* server = mainEngine->getLocalServer();
	if( server ) {
		mainEngine->setPlayTest(false);
		server->reset();
		return 0;
	} else {
		return 1;
	}
}

static int console_serverMap(int argc, const char** argv) {
	if( argc < 1 ) {
		mainEngine->fmsg(Engine::MSG_ERROR,"A path is needed. ex: server.map TestWorld");
		return 1;
	}
	Server* server = mainEngine->getLocalServer();
	if( server ) {
		server->loadWorld(argv[0],true);
		return 0;
	} else {
		return 1;
	}
}

static int console_host(int argc, const char** argv) {
	mainEngine->startServer();
	return 0;
}

static int console_serverGen(int argc, const char** argv) {
	Server* server = mainEngine->getLocalServer();
	if( server == nullptr ) {
		mainEngine->fmsg(Engine::MSG_ERROR,"No server currently running.");
		return 1;
	}
	
	String path;
	if (argc >= 1) {
		path = argv[0];
	} else {
		path = "maps/tilesets/template.json";
	}
	server->genTileWorld( path.get() );

	return 0;
}

static int console_serverSaveMap(int argc, const char** argv) {
	if( argc < 2 ) {
		mainEngine->fmsg(Engine::MSG_ERROR,"missing args. ex: server.savemap 0 TestOutput");
		return 1;
	}

	Server* server = mainEngine->getLocalServer();
	if( server == nullptr ) {
		mainEngine->fmsg(Engine::MSG_ERROR,"No server currently running.");
		return 1;
	}

	int worldID = strtol(argv[0], nullptr, 10);
	World* world = server->getWorld(worldID);
	StringBuf<256> path("maps/%s", 1, argv[1]);
	path = mainEngine->buildPath(path.get()).get();
	world->saveFile(path.get(), true);

	return 0;
}

static int console_serverCount(int argc, const char** argv) {
	Server* server = mainEngine->getLocalServer();
	if( server == nullptr ) {
		mainEngine->fmsg(Engine::MSG_ERROR,"No server currently running.");
		return 1;
	}

	unsigned int count = (Uint32)server->numWorlds();
	mainEngine->fmsg(Engine::MSG_INFO, "There are %d worlds on the server.", count);

	return 0;
}

static int console_serverCountEntities(int argc, const char** argv) {
	Server* server = mainEngine->getLocalServer();
	if (!server) {
		return 1;
	}
	Uint32 count = 0;
	for (Uint32 i = 0; i < server->getNumWorlds(); ++i) {
		World* world = server->getWorld(i);
		count += world->getEntities().getSize();
	}
	mainEngine->fmsg(Engine::MSG_INFO, "Server has %u entities", count);
	return 0;
}

static Ccmd ccmd_host("host","inits a new local server",&console_host);
static Ccmd ccmd_serverReset("server.reset","restarts the local server",&console_serverReset);
static Ccmd ccmd_serverDisconnect("server.disconnect","disconnects the server from all remote hosts",&console_serverDisconnect);
static Ccmd ccmd_serverMap("server.map","loads a world file on the local server",&console_serverMap);
static Ccmd ccmd_serverGen("server.gen","generates a level using the given properties",&console_serverGen);
static Ccmd ccmd_serverSaveMap("server.savemap","saves the given level to disk",&console_serverSaveMap);
static Ccmd ccmd_serverCount("server.count","counts the number of levels running on the server",&console_serverCount);
static Ccmd ccmd_serverCountEntities("server.countentities", "count the number of entities in all worlds on the server", &console_serverCountEntities);