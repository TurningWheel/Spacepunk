// Client.cpp

using namespace std;

#define GLM_FORCE_RADIANS
#include <glm/vec3.hpp>

#include "Main.hpp"
#include "LinkedList.hpp"
#include "Node.hpp"
#include "Engine.hpp"
#include "Client.hpp"
#include "Renderer.hpp"
#include "Mixer.hpp"
#include "Script.hpp"
#include "Frame.hpp"
#include "Camera.hpp"
#include "Editor.hpp"
#include "NetSDL.hpp"
#include "Console.hpp"
#include "Player.hpp"
#include "World.hpp"

Client::Client() {
	net = new NetSDL(*this);
	renderer = new Renderer();
	mixer = new Mixer();
	script = new Script(*this);
}

Client::~Client() {
	if( script ) {
		script->dispatchFunction("term");
		delete script;
	}
	if( gui ) {
		delete gui;
		gui = nullptr;
	}
	if( editor ) {
		delete editor;
		editor = nullptr;
	}
	if( renderer ) {
		delete renderer;
		renderer = nullptr;
	}
	if( mixer ) {
		delete mixer;
		mixer = nullptr;
	}
}

void Client::init() {
	net->init();

	renderer->init();
	if( !renderer->isInitialized() ) {
		mainEngine->smsg(Engine::MSG_ERROR,"failed to initialize renderer");
		return;
	}

	mixer->init();
	if( !mixer->isInitialized() ) {
		mainEngine->smsg(Engine::MSG_ERROR,"failed to initialize mixer");
		return;
	}

	script->load("scripts/client/main.lua");
	script->dispatchFunction("init");
}

void Client::handleNetMessages() {
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
					else if( strncmp( (const char*)packetType, "CMSG", 4) == 0 ) {
						Uint32 msgLen;
						if( packet.read32(msgLen) ) {
							char* msg = new char[msgLen+1];
							if( msg ) {
								msg[msgLen] = 0;
								packet.read(msg,msgLen);
								mainEngine->fmsg(Engine::MSG_CHAT, msg);
								delete[] msg;
							}
						}

						continue;
					}

					// server map list
					else if( strncmp( (const char*)packetType, "MAPS", 4) == 0 ) {
						closeAllWorlds();

						Uint32 numWorlds;
						packet.read32(numWorlds);
						for( Uint32 c=0; c<numWorlds; ++c ) {
							Uint8 worldType;
							packet.read8(worldType);
							if( worldType == 'g' ) {
								Uint32 pathLen;
								packet.read32(pathLen);
								char* path = new char[pathLen+1];
								if( path ) {
									packet.read(path,pathLen);
									path[pathLen] = '\0';
									genTileWorld(path);
									delete[] path;
								}
							} else if( worldType == 'f' ) {
								Uint32 filenameLen;
								packet.read32(filenameLen);
								char* filename = new char[filenameLen+1];
								if( filename ) {
									packet.read(filename,filenameLen);
									filename[filenameLen] = '\0';
									loadWorld(filename,true);
									delete[] filename;
								}
							}
						}

						// for playtesting
						if( mainEngine->isPlayTest() && net->getLocalID() != Net::invalidID ) {
							spawn(0);
						}

						continue;
					}

					// server telling us about a player
					else if( strncmp( (const char*)packetType, "CENS", 4) == 0 ) {
						Player::colors_t colors;

						Uint32 localID, clientID, serverID;
						packet.read32(clientID);
						packet.read32(localID);
						packet.read32(serverID);

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

						Player* player = findPlayer(clientID, localID);
						if( player == nullptr ) {
							player = &players.addNodeLast(Player(nameStr.get(), colors))->getData();
							player->setLocalID(localID);
							player->setClientID(clientID);
						} else {
							player->setName(nameStr.get());
							if( player->getEntity() ) {
								player->updateColors(colors);
							}
						}
						player->setServerID(serverID);

						continue;
					}

					// server has spawned a player
					else if( strncmp( (const char*)packetType, "SPWN", 4) == 0 ) {
						Uint32 localID, clientID, serverID;
						packet.read32(clientID);
						packet.read32(localID);
						packet.read32(serverID);

						// read world
						Uint32 worldID;
						packet.read32(worldID);
						Node<World*>* node = worlds[worldID];
						if( node ) {
							World& world = *node->getData();

							// read pos
							Uint32 posInt[3];
							packet.read32(posInt[0]);
							packet.read32(posInt[1]);
							packet.read32(posInt[2]);
							Vector pos( (Sint32)posInt[0], (Sint32)posInt[1], (Sint32)posInt[2] );

							// read ang
							Uint32 angInt[3];
							packet.read32(angInt[0]);
							packet.read32(angInt[1]);
							packet.read32(angInt[2]);
							Angle ang( (Sint32)angInt[0] * PI / 180.f, (Sint32)angInt[1] * PI / 180.f, (Sint32)angInt[2] * PI / 180.f );

							// spawn player
							if( clientID == net->getLocalID() ) {
								clientID = Player::invalidID;
							}
							Player* player = findPlayer(clientID, localID);
							assert(player);
							player->setServerID(serverID);

							// only actually spawn the player if they belong to us
							if( clientID == Player::invalidID ) {
								player->spawn(world, pos, ang);
							}
						}

						continue;
					}

					// entity update
					else if( strncmp( (const char*)packetType, "ENTU", 4) == 0 ) {
						// read world
						Uint32 worldID;
						packet.read32(worldID);
						Node<World*>* node = worlds[worldID];
						if( node ) {
							World& world = *node->getData();

							// get uid
							Uint32 uid;
							packet.read32(uid);
							Entity* entity = world.uidToEntity(uid);

							// get entity type
							Uint32 type;
							packet.read32(type);

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

							// read isFalling
							Uint8 isFalling;
							packet.read8(isFalling);

							// update the entity
							if( !entity ) {
								// we need to spawn the entity
								if( type != UINT32_MAX ) {
									const Entity::def_t* def = Entity::findDef(type);
									entity = Entity::spawnFromDef(&world, *def, pos, ang, uid);
									if( entity ) {
										entity->setVel(vel);
										entity->setLastUpdate(ticks);
									}
								} else {
									// we have no idea what the entity is!
									// this could be real bad!
									// we don't want to spam the log with messages though, so...
									// if you're in a debugger and you see this, I'm sorry :(
									continue;
								}
							} else {
								// the entity already exists. update it
								entity->setNewPos(pos);
								entity->setNewAng(ang);
								entity->setVel(vel);
								entity->setLastUpdate(entity->getTicks());
							}

							if( entity ) {
								entity->setFalling((isFalling == 1) ? true : false);

								// update the player (if this is one)
								Uint8 isPlayer;
								packet.read8(isPlayer);
								if( isPlayer ) {
									Uint32 serverID;
									packet.read32(serverID);
									Player* player = findPlayer(serverID);
									if( player ) {
										if( entity->getPlayer() == nullptr ) {
											entity->setPlayer(player);
											player->setEntity(entity);
											player->updateColors(player->getColors());
										}

										// read crouch status
										Uint8 isCrouching;
										packet.read8(isCrouching);
										if( isCrouching ) {
											player->putInCrouch(true);
										} else {
											player->putInCrouch(false);
										}

										// read moving status
										Uint8 isMoving;
										packet.read8(isMoving);
										if( isMoving ) {
											player->setMoving(true);
										} else {
											player->setMoving(false);
										}

										// read jumped status
										Uint8 hasJumped;
										packet.read8(hasJumped);
										if( hasJumped ) {
											player->setJumped(true);
										} else {
											player->setJumped(false);
										}

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

						continue;
					}

					// entity deleted
					else if( strncmp( (const char*)packetType, "ENTD", 4) == 0 ) {
						// read world
						Uint32 worldID;
						packet.read32(worldID);
						Node<World*>* node = worlds[worldID];
						if( node ) {
							World& world = *node->getData();

							// get uid
							Uint32 uid;
							packet.read32(uid);

							// find entity
							Entity* entity = world.uidToEntity(uid);
							if( entity ) {
								entity->remove();
							}
						}

						continue;
					}

					// player interaction broadcast message
					else if( strncmp( (const char*)packetType, "PINT", 4) == 0 ) {
						// read world
						Uint32 playerID;
						packet.read32(playerID);
						Uint32 worldID;
						packet.read32(worldID);
						Uint32 selectedEntityID;
						packet.read32(selectedEntityID);
						Node<World*>* node = worlds.nodeForIndex(worldID);
						if ( node ) {
							World* world = node->getData();

							Player* player = findPlayer(playerID);

							mainEngine->fmsg(Engine::MSG_DEBUG, "CLIENT received broadcast that Player %d interacted with entity %d", playerID, selectedEntityID);
						}

						//TODO: Play interact animation.

						continue;
					}
				}
			}
		}

		// unlock packet receiving thread
		net->unlockThread();
	}
}

void Client::onEstablishConnection(Uint32 remoteID) {
	if( mainEngine->isPlayTest() && numWorlds() > 0 ) {
		spawn(0);
	}
}

void Client::onDisconnect(Uint32 remoteID) {
	Node<Player>* node;
	Node<Player>* nextNode;
	for( node = players.getFirst(); node != nullptr; node = nextNode ) {
		nextNode = node->getNext();
		Player& player = node->getData();
		if( player.getClientID() == Player::invalidID ) {
			player.despawn();
		} else {
			players.removeNode(node);
		}
	}
}

void Client::spawn(Uint32 localID) {
	Player* player = findPlayer(Player::invalidID, localID);
	if( player == nullptr ) {
		player = &players.addNodeLast(Player())->getData();
		player->setLocalID(localID);
	}

	Packet packet;

	packet.write8((Uint8)floor(player->getColors().feetBChannel[2] * 255.f));
	packet.write8((Uint8)floor(player->getColors().feetBChannel[1] * 255.f));
	packet.write8((Uint8)floor(player->getColors().feetBChannel[0] * 255.f));
	packet.write8((Uint8)floor(player->getColors().feetGChannel[2] * 255.f));
	packet.write8((Uint8)floor(player->getColors().feetGChannel[1] * 255.f));
	packet.write8((Uint8)floor(player->getColors().feetGChannel[0] * 255.f));
	packet.write8((Uint8)floor(player->getColors().feetRChannel[2] * 255.f));
	packet.write8((Uint8)floor(player->getColors().feetRChannel[1] * 255.f));
	packet.write8((Uint8)floor(player->getColors().feetRChannel[0] * 255.f));

	packet.write8((Uint8)floor(player->getColors().armsBChannel[2] * 255.f));
	packet.write8((Uint8)floor(player->getColors().armsBChannel[1] * 255.f));
	packet.write8((Uint8)floor(player->getColors().armsBChannel[0] * 255.f));
	packet.write8((Uint8)floor(player->getColors().armsGChannel[2] * 255.f));
	packet.write8((Uint8)floor(player->getColors().armsGChannel[1] * 255.f));
	packet.write8((Uint8)floor(player->getColors().armsGChannel[0] * 255.f));
	packet.write8((Uint8)floor(player->getColors().armsRChannel[2] * 255.f));
	packet.write8((Uint8)floor(player->getColors().armsRChannel[1] * 255.f));
	packet.write8((Uint8)floor(player->getColors().armsRChannel[0] * 255.f));

	packet.write8((Uint8)floor(player->getColors().torsoBChannel[2] * 255.f));
	packet.write8((Uint8)floor(player->getColors().torsoBChannel[1] * 255.f));
	packet.write8((Uint8)floor(player->getColors().torsoBChannel[0] * 255.f));
	packet.write8((Uint8)floor(player->getColors().torsoGChannel[2] * 255.f));
	packet.write8((Uint8)floor(player->getColors().torsoGChannel[1] * 255.f));
	packet.write8((Uint8)floor(player->getColors().torsoGChannel[0] * 255.f));
	packet.write8((Uint8)floor(player->getColors().torsoRChannel[2] * 255.f));
	packet.write8((Uint8)floor(player->getColors().torsoRChannel[1] * 255.f));
	packet.write8((Uint8)floor(player->getColors().torsoRChannel[0] * 255.f));

	packet.write8((Uint8)floor(player->getColors().headBChannel[2] * 255.f));
	packet.write8((Uint8)floor(player->getColors().headBChannel[1] * 255.f));
	packet.write8((Uint8)floor(player->getColors().headBChannel[0] * 255.f));
	packet.write8((Uint8)floor(player->getColors().headGChannel[2] * 255.f));
	packet.write8((Uint8)floor(player->getColors().headGChannel[1] * 255.f));
	packet.write8((Uint8)floor(player->getColors().headGChannel[0] * 255.f));
	packet.write8((Uint8)floor(player->getColors().headRChannel[2] * 255.f));
	packet.write8((Uint8)floor(player->getColors().headRChannel[1] * 255.f));
	packet.write8((Uint8)floor(player->getColors().headRChannel[0] * 255.f));

	packet.write(player->getName());
	packet.write8((Uint8)strlen(player->getName()));
	packet.write32(localID);
	packet.write("SPWN");

	net->signPacket(packet);
	net->sendPacketSafe(0, packet);
}

void Client::closeEditor() {
	net->disconnectAll();

	if( editor ) {
		delete editor;
		editor = nullptr;
	}

	if( gui ) {
		delete gui;
		gui = nullptr;
	}

	closeAllWorlds();
}

void Client::startEditor(const char* path) {
	closeEditor();
	editor = new Editor();
	gui = new Frame("editor_gui");

	// destroy all players
	players.removeAll();

	editor->init(*this, path);
}

void Client::runConsole() {
	if( mainEngine->copyLog(console) ) {
		logStart = nullptr;
	}

	#ifdef PLATFORM_LINUX
	if( mainEngine->pressKey(SDL_SCANCODE_F1) ) {
	#else
	if( mainEngine->pressKey(SDL_SCANCODE_GRAVE) ) { // doesn't work on linux for some reason?
	#endif
		consoleActive = (consoleActive==false);
		if( consoleActive ) {
			for( int i=0; i<consoleLen; ++i ) {
				consoleInput[i] = 0;
			}
			mainEngine->setInputStr(consoleInput);
			mainEngine->setInputLen(consoleLen);
			SDL_StartTextInput();
		} else {
			mainEngine->setInputStr(nullptr);
			mainEngine->setInputLen(0);
			SDL_StopTextInput();
		}
	}
	if( consoleActive ) {
		consoleHeight = min(consoleHeight+renderer->getYres()/10,renderer->getYres()/2);

		// submit console command
		if( mainEngine->pressKey(SDL_SCANCODE_RETURN) || mainEngine->getKeyStatus(SDL_SCANCODE_KP_ENTER) ) {
			const char* command[1];
			command[0] = consoleInput;
			mainEngine->commandLine(1,command);
			String oldCommand(consoleInput);
			mainEngine->getCommandHistory().addNodeLast(oldCommand);
			cuCommand = nullptr;
			for( int i=0; i<consoleLen; ++i ) {
				consoleInput[i] = 0;
			}
		}

		// help on current console buffer
		if( mainEngine->pressKey(SDL_SCANCODE_TAB) ) {
			StringBuf<64> cmd("help %s", consoleInput);
			mainEngine->doCommand(cmd.get());
		}

		// navigate command history
		if( mainEngine->pressKey(SDL_SCANCODE_UP) ) {
			if( cuCommand==nullptr ) {
				strncpy(oldConsoleInput, consoleInput, consoleLen);
				cuCommand = mainEngine->getCommandHistory().getLast();
				if( cuCommand ) {
					strncpy(consoleInput,cuCommand->getData().get(),consoleLen);
				}
			} else {
				if( cuCommand->getPrev()!=nullptr ) {
					cuCommand = cuCommand->getPrev();
					strncpy(consoleInput,cuCommand->getData().get(),consoleLen);
				}
			}
		}
		if( mainEngine->pressKey(SDL_SCANCODE_DOWN) ) {
			if( cuCommand!=nullptr ) {
				if( cuCommand->getNext()!=nullptr ) {
					cuCommand = cuCommand->getNext();
					strncpy(consoleInput,cuCommand->getData().get(),consoleLen);
				} else {
					cuCommand = nullptr;
					strncpy(consoleInput,oldConsoleInput,consoleLen);
				}
			}
		}

		// navigate console log
		if( mainEngine->pressKey(SDL_SCANCODE_PAGEUP) ) {
			if( logStart==nullptr ) {
				logStart = console.getLast();
			}
			for( int c=0; logStart!=console.getFirst() && c<10; logStart=logStart->getPrev(), ++c ) {
				size_t i=-1;
				String* str = &logStart->getData().text;
				while( (i=str->find('\n',i+1)) != UINT32_MAX ) {
					++c;
					if( c>=9 )
						break;
				}
			}
		}
		if( mainEngine->pressKey(SDL_SCANCODE_PAGEDOWN) ) {
			if( logStart!=nullptr ) {
				for( int c=0; logStart!=console.getLast() && c<10; logStart=logStart->getNext(), ++c ) {
					size_t i=-1;
					String* str = &logStart->getData().text;
					while( (i=str->find('\n',i+1)) != UINT32_MAX ) {
						++c;
						if( c>=9 )
							break;
					}
				}
				if( logStart == console.getLast() ) {
					logStart = nullptr;
				}
			}
		}
		if( mainEngine->pressKey(SDL_SCANCODE_HOME) ) {
			logStart = console.getFirst();
		}
		if( mainEngine->pressKey(SDL_SCANCODE_END) ) {
			logStart = nullptr;
		}
	} else {
		consoleHeight = max(consoleHeight-renderer->getYres()/10,0);
	}
}

void Client::preProcess() {
	if( mainEngine->isPlayTest() && editor ) {
		closeEditor();
		mainEngine->joinServer("localhost");
	}
	if( framesToRun ) {
		renderer->clearBuffers();
		script->dispatchFunction("preprocess");

		if( editor ) {
			editor->preProcess();
		}

		// player controls
		for( Node<Player>* node = players.getFirst(); node != nullptr; node = node->getNext() ) {
			Player& player = node->getData();
			
			if( player.getClientID() == Player::invalidID ) {
				// in the context of a client, this means *we* own the player.
				// in any other circumstance, it would contain a legitimate client ID.
				player.control();
			}
		}
	}
	handleNetMessages();
	Game::preProcess();
}

void Client::process() {
	Game::process();

	for( Uint32 frame=0; frame<framesToRun; ++frame ) {
		script->dispatchFunction("process");

		// drop down console
		if( consoleAllowed ) {
			runConsole();
		}

		bool editorFullscreen = false;
		bool textureSelectorActive = false;
		if( editor ) {
			editorFullscreen = editor->isFullscreen();
			textureSelectorActive = editor->isTextureSelectorActive();
		}

		if( !consoleActive ) {
			// process gui
			bool usableInterface = true;
			if( gui && !editorFullscreen && !textureSelectorActive ) {
				Frame::result_t result = gui->process();
				usableInterface = result.usable;
			}

			// process editor
			if( editor && editor->isInitialized() ) {
				editor->process(usableInterface);
			}
		}
	}
}

void Client::postProcess() {
	Game::postProcess();

	if( framesToRun ) {
		bool editorFullscreen = false;
		if( editor ) {
			editorFullscreen = editor->isFullscreen();
		}

		// update player cameras
		if( framesToRun ) {
			for( Node<Player>* node = players.getFirst(); node != nullptr; node = node->getNext() ) {
				Player& player = node->getData();

				if( player.getClientID() == Player::invalidID ) {
					// in the context of a client, this means *we* own the player.
					// in any other circumstance, it would contain a legitimate client ID.
					player.updateCamera();
				}
			}
		}

		// rendering
		bool textureSelectorActive = false;
		if( editor && editor->isInitialized() ) {
			textureSelectorActive = editor->isTextureSelectorActive();
		}

		if( !textureSelectorActive ) {
			for( Node<World*>* node = worlds.getFirst(); node != nullptr; node = node->getNext() ) {
				World* world = node->getData();
				world->draw();
			}

			// editor interface
			if( editor && editor->isInitialized() ) {
				editor->draw(*renderer);
			}

			// draw frames
			if( gui && !editorFullscreen ) {
				gui->draw(*renderer);
			}

			// draw console
			if( consoleHeight>0 ) {
				renderer->drawConsole(consoleHeight,consoleInput,console,logStart);
			}

			// debug counters
			if( (!editor || !editor->isInitialized()) ) {
				// fps counter
				if( cvar_showFPS.toInt() ) {
					char fps[16];
					snprintf(fps,16,"%.1f",mainEngine->getFPS());

					int width;
					TTF_SizeUTF8(renderer->getMonoFont(),fps,&width,NULL);

					Rect<int> pos;
					Rect<int> rect;
					rect.x = 0; rect.w = renderer->getXres();
					rect.y = 0; rect.h = renderer->getYres();
					pos.x = rect.x+rect.w-18-width; pos.w = 0;
					pos.y = rect.y+18; pos.h = 0;

					renderer->printText( pos, fps );
				}

				// player speedometer
				if( cvar_showSpeed.toInt() ) {
					Node<Player>* node = players.getFirst();
					if( node ) {
						Player& player = node->getData();

						if( player.getEntity() ) {
							float fSpeed = player.getEntity()->getVel().length() * mainEngine->getTicksPerSecond() / 64.f;

							char speed[16];
							snprintf(speed,16,"%.1f m/sec",fSpeed);

							int width;
							TTF_SizeUTF8(renderer->getMonoFont(),speed,&width,NULL);

							Rect<int> pos;
							Rect<int> rect;
							rect.x = 0; rect.w = renderer->getXres();
							rect.y = 0; rect.h = renderer->getYres();
							pos.x = rect.x+rect.w-18-width; pos.w = 0;
							pos.y = rect.y+18+36; pos.h = 0;

							renderer->printText( pos, speed );
						}
					}
				}

				// camera matrix
				if( cvar_showMatrix.toInt() ) {
					Node<Player>* node = players.getFirst();
					if( node ) {
						Player& player = node->getData();

						if( player.getCamera() ) {
							const glm::mat4& mat = player.getCamera()->getViewMatrix();

							char matrixChars[256];
							snprintf(matrixChars, 256,
								"%+07.1f %+07.1f %+07.1f %+07.1f\n"
								"%+07.1f %+07.1f %+07.1f %+07.1f\n"
								"%+07.1f %+07.1f %+07.1f %+07.1f\n"
								"%+07.1f %+07.1f %+07.1f %+07.1f\n",
								mat[ 0][ 0], mat[ 0][ 1], mat[ 0][ 2], mat[ 0][ 3],
								mat[ 1][ 0], mat[ 1][ 1], mat[ 1][ 2], mat[ 1][ 3],
								mat[ 2][ 0], mat[ 2][ 1], mat[ 2][ 2], mat[ 2][ 3],
								mat[ 3][ 0], mat[ 3][ 1], mat[ 3][ 2], mat[ 3][ 3]
							);

							Rect<int> pos;
							pos.x = 18; pos.w = 0;
							pos.y = 18; pos.h = 0;
							renderer->printText( pos, matrixChars );
						}
					}
				}
			}
		} else {
			// just the editor interface
			if( editor && editor->isInitialized() ) {
				editor->draw(*renderer);
			}
		}

		// screenshots
		if( mainEngine->pressKey(SDL_SCANCODE_F6) ) {
			renderer->takeScreenshot();
		}

		// swap screen buffer
		renderer->swapWindow();

		// run script
		script->dispatchFunction("postprocess");

		if( editor ) {
			editor->postProcess();
		}

		// update server on player positions 4 times per second
		if( net->isConnected() ) {
			if( ticks % (mainEngine->getTicksPerSecond()/10) == 0 ) {
				for( Node<Player>* node = players.getFirst(); node != nullptr; node = node->getNext() ) {
					Player& player = node->getData();
					if( player.getClientID() == Player::invalidID ) {
						if( Entity* entity = player.getEntity() ) {
							Packet packet;

							Uint32 worldIndex = indexForWorld(entity->getWorld());

							if( worldIndex != invalidID ) {
								packet.write32((Sint32)(entity->getLookDir().degreesRoll() * 32));
								packet.write32((Sint32)(entity->getLookDir().degreesPitch() * 32));
								packet.write32((Sint32)(entity->getLookDir().degreesYaw() * 32));
								packet.write8(entity->hasJumped() ? 1 : 0);
								packet.write8(entity->isMoving() ? 1 : 0);
								packet.write8(entity->isCrouching() ? 1 : 0);
								packet.write8(entity->isFalling() ? 1 : 0);
								packet.write32((Sint32)(entity->getAng().degreesRoll() * 32));
								packet.write32((Sint32)(entity->getAng().degreesPitch() * 32));
								packet.write32((Sint32)(entity->getAng().degreesYaw() * 32));
								packet.write32((Sint32)(entity->getVel().z * 128));
								packet.write32((Sint32)(entity->getVel().y * 128));
								packet.write32((Sint32)(entity->getVel().x * 128));
								packet.write32((Sint32)(entity->getPos().z * 32));
								packet.write32((Sint32)(entity->getPos().y * 32));
								packet.write32((Sint32)(entity->getPos().x * 32));
								packet.write32(worldIndex);
								packet.write32(player.getLocalID());
								packet.write("PLAY");

								// not a guaranteed packet.
								// this is done so often it doesn't matter if some packets get dropped.

								net->signPacket(packet);
								net->sendPacket(0, packet);
							}
						}
					}
				}
			}
		}

		framesToRun=0;
	}
}

static int console_clientDisconnect(int argc, const char** argv) {
	Client* client = mainEngine->getLocalClient();
	if( client ) {
		client->getNet()->disconnectAll();
		return 0;
	} else {
		return 1;
	}
}

static int console_clientReset(int argc, const char** argv) {
	Client* client = mainEngine->getLocalClient();
	if( client ) {
		mainEngine->setInputStr(nullptr);
		mainEngine->setPlayTest(false);
		mainEngine->dumpResources();
		client->reset();
		return 0;
	} else {
		return 1;
	}
}

static int console_clientMap(int argc, const char** argv) {
	if( argc < 1 ) {
		mainEngine->fmsg(Engine::MSG_ERROR,"A path is needed. ex: client.map TestWorld");
	}
	Client* client = mainEngine->getLocalClient();
	if( client ) {
		client->loadWorld(argv[0],true);
		return 0;
	} else {
		return 1;
	}
}

static int console_clientSpawn(int argc, const char** argv) {
	if( argc < 1 ) {
		mainEngine->fmsg(Engine::MSG_ERROR,"Please specify player number. ex: client.spawn 0");
		return 1;
	}

	int num = strtol(argv[0], nullptr, 10);
	Client* client = mainEngine->getLocalClient();
	if( client ) {
		int numPlayers = client->numLocalPlayers();
		if( numPlayers < 4 ) {
			client->spawn(num);
			return 0;
		} else {
			mainEngine->fmsg(Engine::MSG_ERROR,"Local player limit reached.");
		}
	} else {
		mainEngine->fmsg(Engine::MSG_INFO,"No client to spawn.");
	}
	return 1;
}

static Ccmd ccmd_clientReset("client.reset","restarts the local client",&console_clientReset);
static Ccmd ccmd_clientDisconnect("client.disconnect","disconnects the client from the remote host",&console_clientDisconnect);
static Ccmd ccmd_clientMap("client.map","loads a world file on the local client",&console_clientMap);
static Ccmd ccmd_clientSpawn("client.spawn","spawns a new player into the world we are connected to",&console_clientSpawn);
Cvar cvar_showFPS("showfps","displays an FPS counter","0");
Cvar cvar_showSpeed("showspeed","displays player speedometer","0");
Cvar cvar_showMatrix("showmatrix","displays the camera matrix of the first player","0");
