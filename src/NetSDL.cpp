// NetSDL.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "Net.hpp"
#include "NetSDL.hpp"
#include "Game.hpp"

NetSDL::NetSDL(Game& _parent) : Net(_parent) {
	if( (SDLsendPacket = SDLNet_AllocPacket(Packet::maxLen)) == nullptr ) {
		mainEngine->fmsg(Engine::MSG_CRITICAL, "failed to allocate SDL packet for NetSDL!");
	}
	if( (SDLrecvPacket = SDLNet_AllocPacket(Packet::maxLen)) == nullptr ) {
		mainEngine->fmsg(Engine::MSG_CRITICAL, "failed to allocate SDL packet for NetSDL!");
	}

	/*execLock = SDL_CreateMutex();
	killLock = SDL_CreateMutex();
	connectLock = SDL_CreateMutex();
	if( _parent.isClient() ) {
		threadName = "Client Networking";
	} else if( _parent.isServer() ) {
		threadName = "Server Networking";
	}
	thread = SDL_CreateThread( runThread, threadName.get(), (void *)this );
	if( thread == nullptr ) {
		mainEngine->fmsg(Engine::MSG_CRITICAL, "failed to create thread '%s'", threadName.get());
	}*/
}

NetSDL::~NetSDL() {
}

void NetSDL::init() {
}

void NetSDL::term() {
	// kill network thread
	/*SDL_LockMutex(killLock);
	kill = true;
	SDL_UnlockMutex(killLock);
	if( thread ) {
		SDL_WaitThread(thread, nullptr);
	}
	SDL_DestroyMutex(execLock); execLock = nullptr;
	SDL_DestroyMutex(killLock); killLock = nullptr;
	SDL_DestroyMutex(connectLock); connectLock = nullptr;*/

	// disconnect everyone
	disconnectAll();

	// delete packets
	if( SDLsendPacket ) {
		SDLNet_FreePacket(SDLsendPacket);
		SDLsendPacket = nullptr;
	}
	if( SDLrecvPacket ) {
		SDLNet_FreePacket(SDLrecvPacket);
		SDLrecvPacket = nullptr;
	}
}

bool NetSDL::host(Uint16 port) {
	if( connected ) {
		return false;
	}

	IPaddress host;
	if( SDLNet_ResolveHost(&host, NULL, port) == -1) {
		mainEngine->fmsg(Engine::MSG_ERROR,"resolving host at localhost:%d has failed", port);
		return false;
	}

	SDLsocket = SDLNet_UDP_Open(port);
	connected = true;
	hosting = true;

	localID = 0;

	mainEngine->fmsg(Engine::MSG_INFO,"opened server on localhost:%d", port);
	return true;
}

bool NetSDL::connect(const char* address, Uint16 port) {
	if( !address )
		return false;

	sdlremote_t* remote = new sdlremote_t();
	strncpy(remote->address, address, 256);
	remote->port = port;
	SDLremotes.push(remote);
	remotes.push(remote);
	remote->parent = this;

	if( SDLNet_ResolveHost(&remote->host, address, port) == -1) {
		mainEngine->fmsg(Engine::MSG_ERROR,"resolving host at %s:%d has failed:\n %s", address, port, SDLNet_GetError());
		SDLremotes.pop();
		delete remote;
		return false;
	}

	if( !connected ) {
		SDLsocket = SDLNet_UDP_Open(0);
		connected = true;
	} else if( !hosting ) {
		mainEngine->fmsg(Engine::MSG_ERROR,"cannot connect to more than one server at once!");
		SDLremotes.pop();
		delete remote;
		return false;
	}

	Uint32 clientID = numClients;
	remote->id = clientID;

	// send a connection request to the host
	Packet packet;
	packet.write32(clientID);
	packet.write32(localGID);
	packet.write(versionStr);
	packet.write("JOIN");
	signPacket(packet);

	// send it several times, just to make sure the server gets it!
	// this is the only place we have to do this, other locations are
	// handled by the packet guarantee system

	sendPacket(clientID,packet);
	sendPacket(clientID,packet);
	sendPacket(clientID,packet);
	sendPacket(clientID,packet);
	sendPacket(clientID,packet);

	++numClients;

	return true;
}

void NetSDL::completeConnection(void* data) {
	const sdlrequest_t* request = (sdlrequest_t*)data;
	if( !hosting ) {
		return;
	}

	// make sure we haven't connected to this person already.
	for( Uint32 c = 0; c < SDLremotes.getSize(); ++c ) {
		sdlremote_t* remote = SDLremotes[c];
		if( remote->gid == request->gid ) {
			// don't allow the same person to connect more than once.
			return;
		}
	}

	const char* address = SDLNet_ResolveIP(&request->ip);

	sdlremote_t* remote = new sdlremote_t();
	strcpy(remote->address, address);
	remote->host = request->ip;
	remote->port = request->ip.port;
	remote->gid = request->gid;
	SDLremotes.push(remote);
	remotes.push(remote);
	remote->parent = this;

	Uint32 clientID = numClients;
	remote->id = clientID;

	// send a connection request to the host
	Packet packet;
	packet.write32(clientID);
	packet.write32(localGID);
	packet.write(versionStr);
	packet.write("JOIN");
	signPacket(packet);

	// send it several times, just to make sure the server gets it!
	// this is the only place we have to do this, other locations are
	// handled by the packet guarantee system

	sendPacket(clientID,packet);
	sendPacket(clientID,packet);
	sendPacket(clientID,packet);
	sendPacket(clientID,packet);
	sendPacket(clientID,packet);

	++numClients;

	mainEngine->fmsg(Engine::MSG_INFO,"completed connection to host at %s:%d", address, request->ip.port);

	if( parent ) {
		parent->onEstablishConnection(clientID);
	}
}

bool NetSDL::disconnect(Uint32 remoteID, bool inform) {
	Uint32 index = getRemoteWithID(remoteID);
	if( index == UINT32_MAX )
		return false;

	sdlremote_t* remote = SDLremotes[index];

	if( inform ) {
		// send disconnect packet to host
		Packet packet;
		packet.write("QUIT");
		signPacket(packet);

		sendPacket(remoteID,packet);
		sendPacket(remoteID,packet);
		sendPacket(remoteID,packet);
		sendPacket(remoteID,packet);
		sendPacket(remoteID,packet);
	}

	mainEngine->fmsg(Engine::MSG_INFO, "disconnected from host at '%s'",remote->address);
	SDLremotes.remove(index);
	delete remote;

	if( SDLremotes.getSize()==0 && !hosting ) {
		SDLNet_UDP_Close(SDLsocket);
		connected = false;
		localID = invalidID;
		numClients = 0;
	}

	if( parent ) {
		parent->onDisconnect(remoteID);
	}

	return true;
}

bool NetSDL::disconnectHost() {
	if( !hosting || !connected )
		return false;

	for( Uint32 c = 0; c < SDLremotes.getSize(); ++c ) {
		sdlremote_t* remote = SDLremotes[c];
		disconnect(remote->id);
	}

	hosting = false;
	if( SDLremotes.getSize()==0 ) {
		SDLNet_UDP_Close(SDLsocket);
		connected = false;
		localID = invalidID;
		numClients = 0;
	}

	mainEngine->fmsg(Engine::MSG_INFO, "closed localhost to inbound connections");

	return true;
}

bool NetSDL::disconnectAll() {
	// this locks the net thread! therefore this should NEVER be called by:
	// Game::handleNetworkMessages(), or the net thread!
	while( !lockThread() );

	bool result = false;
	if( connected ) {
		mainEngine->fmsg(Engine::MSG_INFO, "closing network connection(s)");

		for( Uint32 c = 0; c < SDLremotes.getSize(); ++c ) {
			sdlremote_t* remote = SDLremotes[c];
			if( disconnect(remote->id) ) {
				result = true;
				--c;
			}
		}
		if( disconnectHost() ) {
			result = true;
		}
		localID = invalidID;
	}
	return result;
}

const char* NetSDL::getHostname(Uint32 remoteID) const {
	Uint32 index = getRemoteWithID(remoteID);
	if( index != UINT32_MAX ) {
		const sdlremote_t* remote = SDLremotes[index];
		return remote->address;
	} else {
		return nullptr;
	}
}

bool NetSDL::sendPacket(Uint32 remoteID, const Packet& packet) {
	Uint32 index = getRemoteWithID(remoteID);
	if( index == UINT32_MAX ) {
		mainEngine->fmsg(Engine::MSG_WARN,"tried to send packet to invalid remote host! (%d)",remoteID);
		return false;
	}

	const sdlremote_t* remote = SDLremotes[index];

	SDLsendPacket->channel = -1;
	SDLsendPacket->len = min((int)packet.offset,SDLsendPacket->maxlen);
	SDLsendPacket->address = remote->host;
	memcpy(SDLsendPacket->data, packet.data, packet.offset);

	if( SDLNet_UDP_Send(SDLsocket, -1, SDLsendPacket)!=0 ) {
		return true;
	} else {
		mainEngine->fmsg(Engine::MSG_WARN,"failed to send SDL_Net UDP packet:\n %s", SDLNet_GetError());
		return false;
	}
}

bool NetSDL::sendPacketSafe(Uint32 remoteID, const Packet& packet) {
	Uint32 index = getRemoteWithID(remoteID);
	if( index == UINT32_MAX ) {
		mainEngine->fmsg(Engine::MSG_WARN,"tried to send packet to invalid remote host! (%d)",remoteID);
		return false;
	}

	if( localID == invalidID ) {
		mainEngine->fmsg(Engine::MSG_WARN,"tried to send safe packet with an invalid local id!");
		return false;
	}

	sdlremote_t* remote = SDLremotes[index];

	safepacket_t* safePacket = new safepacket_t();

	safePacket->id = remote->safePacketsSent;
	safePacket->lastTimeSent = SDL_GetTicks();
	safePacket->packet.copy(packet);

	safePacket->packet.write32(safePacket->id);
	safePacket->packet.write("SAFE");
	signPacket(safePacket->packet);

	if( sendPacket(remoteID, safePacket->packet) ) {
		remote->resendStack.push(safePacket);
		++remote->safePacketsSent;
		return true;
	} else {
		return false;
	}
}

bool NetSDL::broadcast(Packet& packet) {
	bool result = true;
	for( Uint32 c = 0; c < SDLremotes.getSize(); ++c ) {
		sdlremote_t* remote = SDLremotes[c];
		result = sendPacket(remote->id, packet) ? result : false;
	}
	return result;
}

bool NetSDL::broadcastSafe(Packet& packet) {
	bool result = true;
	for( Uint32 c = 0; c < SDLremotes.getSize(); ++c ) {
		sdlremote_t* remote = SDLremotes[c];
		result = sendPacketSafe(remote->id, packet) ? result : false;
	}
	return result;
}

Packet* NetSDL::recvPacket(unsigned int remoteIndex) {
	if( remoteIndex < 0 || remoteIndex >= (unsigned int)SDLremotes.getSize() ) {
		mainEngine->fmsg(Engine::MSG_WARN,"tried to recv packet via invalid remote index!");
		return nullptr;
	}
	sdlremote_t* remote = SDLremotes[remoteIndex];

	if( remote->packetStack.getSize() > 0 ) {
		Packet* packet = remote->packetStack.pop();
		return packet;
	} else {
		return nullptr;
	}
}

void NetSDL::update() {
	if( !connected ) {
		return;
	}

	runThread(this);

	// complete connection requests
	//SDL_LockMutex(connectLock);
	while( SDLrequests.getSize() > 0 ) {
		sdlrequest_t SDLrequest = SDLrequests.pop();
		completeConnection((void*)&SDLrequest);
	}
	//SDL_UnlockMutex(connectLock);

	// do resending of safe packets
	Net::update();
}

int NetSDL::runThread(void* data) {
	NetSDL* net = (NetSDL*)data;
	//SDL_SetThreadPriority(SDL_THREAD_PRIORITY_LOW);

	//while( 1 ) {
	{
		// check whether we should kill the thread
		/*SDL_LockMutex(net->killLock);
		if( net->kill ) {
			SDL_UnlockMutex(net->killLock);
			break;
		} else {
			SDL_UnlockMutex(net->killLock);
		}*/

		// receive packets
		int result = 0;
		do {
			// check again whether we should kill the thread
			/*SDL_LockMutex(net->killLock);
			if( net->kill ) {
				SDL_UnlockMutex(net->killLock);
				break;
			} else {
				SDL_UnlockMutex(net->killLock);
			}*/

			result = 0;
			if( net->lockThread() ) {
				if( net->SDLrecvPacket == nullptr || !net->connected ) {
					// this probably means we're disconnected!
					break;
				}

				Uint32 remoteIndex = UINT32_MAX;
				Packet* packet = new Packet();

				net->SDLrecvPacket->channel = -1;
				if( (result = SDLNet_UDP_Recv(net->SDLsocket, net->SDLrecvPacket)) == -1 ) {
					mainEngine->fmsg(Engine::MSG_WARN,"failed to recv SDL_Net UDP packet:\n %s", SDLNet_GetError());
				} else if( result==1 ) {
					memcpy(packet->data, net->SDLrecvPacket->data, net->SDLrecvPacket->len);
					packet->offset = net->SDLrecvPacket->len;
					Packet readPacket(*packet);

					Uint32 id;
					Uint32 timestamp;
					if( readPacket.read32(id) && readPacket.read32(timestamp) ) {
						remoteIndex = net->getRemoteWithID(id);
						if( remoteIndex == UINT32_MAX ) {
							char type[4];
							readPacket.read(type, 4);
							if( strncmp( (const char*)type, "JOIN", 4) == 0 ) {
								char version[16] = { 0 };
								if( readPacket.read(version,(Uint32)strlen(versionStr)) ) {
									if( strcmp(versionStr,version) ) {
										mainEngine->fmsg(Engine::MSG_WARN, "connection attempted by a client with version %s (mismatch)");
									} else {
										Uint32 gid;
										if( readPacket.read32(gid) ) {
											if( gid==net->localGID ) {
												mainEngine->fmsg(Engine::MSG_ERROR, "I tried to connect to myself!");
											} else {
												// store off connection request
												sdlrequest_t request;
												request.ip = net->SDLrecvPacket->address;
												request.gid = gid;
												//SDL_LockMutex(net->connectLock);
												net->SDLrequests.push(request);
												//SDL_UnlockMutex(net->connectLock);
											}
										}
									}
								}
							} else {
								mainEngine->fmsg(Engine::MSG_DEBUG, "message received from client with bad id (%d)", id);
							}
						} else {
							sdlremote_t* remote = net->SDLremotes[remoteIndex];
							remote->packetStack.push(packet);
						}
					}
				}

				if( remoteIndex == UINT32_MAX && packet ) {
					delete packet;
					packet = nullptr;
				}
			}

			// mark thread as finished
			net->unlockThread();
		} while( result==1 );
	}

	return 0;
}

int NetSDL::handleNetworkPacket(Packet& packet, const char* type, Uint32 remoteID) {
	if( type == nullptr ) {
		return 0;
	}

	// join completion
	else if( strncmp( type, "JOIN", 4) == 0 ) {
		char version[16] = { 0 };
		if( packet.read(version,(Uint32)strlen(versionStr)) ) {
			if( strcmp(versionStr,version) ) {
				mainEngine->fmsg(Engine::MSG_WARN, "connection attempted by a client with version %s (mismatch)", version);
			} else {
				Uint32 gid;
				if( packet.read32(gid) ) {
					Uint32 myID;
					if( packet.read32(myID) && localID == invalidID ) {
						localID = myID;
						sdlremote_t* remote = SDLremotes[0];
						mainEngine->fmsg(Engine::MSG_INFO,"connected to host at %s:%d", remote->address, remote->port);
						mainEngine->fmsg(Engine::MSG_INFO, "received new local id from connection: %d", myID);

						if( parent ) {
							parent->onEstablishConnection(0);
						}
					}
				}
			}
		}

		return 1;
	}

	// disconnects
	else if( strncmp( type, "QUIT", 4) == 0 ) {
		disconnect(remoteID, false);

		return 2;
	}

	// safe message -- queue and respond with ack
	else if( strncmp( type, "SAFE", 4) == 0 ) {
		Uint32 remoteIndex = getRemoteWithID(remoteID);
		if( remoteIndex == UINT32_MAX ) {
			mainEngine->fmsg(Engine::MSG_DEBUG, "message received from client with bad id (%d)", remoteID);
		} else {
			Uint32 packetID;
			packet.read32(packetID); // get the packet id

			sdlremote_t* remote = SDLremotes[remoteIndex];
			Uint32 hashIndex = packetID%128;

			bool foundPacket = false;
			for( Uint32 c = 0; c < remote->safeRcvdHash[hashIndex].getSize(); ++c ) {
				Uint32 id = remote->safeRcvdHash[hashIndex][c];

				if( id == packetID ) {
					foundPacket = true;
					break;
				}
			}

			if( !foundPacket ) {
				// put the packet back onto the stack
				Packet* newPacket = new Packet(packet);
				remote->packetStack.push(newPacket);
				remote->safeRcvdHash[hashIndex].push(packetID);
			}

			// now ack
			Packet ack;
			ack.write32(packetID);
			ack.write("ACKN");
			signPacket(ack);
			sendPacket(remoteID, ack);
		}

		return 3;
	}

	// safe message -- ack
	else if( strncmp( type, "ACKN", 4) == 0 ) {
		Uint32 remoteIndex = getRemoteWithID(remoteID);

		Uint32 packetID;
		if( packet.read32(packetID) ) {
			sdlremote_t* remote = SDLremotes[remoteIndex];

			for( Uint32 c = 0; c < remote->resendStack.getSize(); ++c ) {
				safepacket_t* safePacket = remote->resendStack[c];

				if( safePacket->id == packetID ) {
					remote->resendStack.remove(c);
					delete safePacket;
					break;
				}
			}
		}

		return 4;
	}

	return 0;
}

Uint32 NetSDL::numRemoteHosts() const {
	return (Uint32)SDLremotes.getSize();
}
