// Net.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "Net.hpp"
#include "Random.hpp"
#include "Game.hpp"
#include "Console.hpp"

Net::Net(Game& _parent) {
	parent = &_parent;
	localGID = mainEngine->getRandom().getUint32();
}

Net::~Net() {
}

bool Net::signPacket(Packet& packet) {
	return packet.sign(mainEngine->getTicks(), localID);
}

bool Net::lockThread() {
	return true;

	/*if( !execLock )
		return true;

	SDL_LockMutex(execLock);
	if( exec ) {
		SDL_UnlockMutex(execLock);
		return false;
	} else {
		exec = true;
		SDL_UnlockMutex(execLock);
		return true;
	}*/
}

void Net::unlockThread() {
	/*if( !execLock )
		return;

	SDL_LockMutex(execLock);
	exec = false;
	SDL_UnlockMutex(execLock);*/
}

void Net::update() {
	for( Uint32 remoteIndex = 0; remoteIndex < remotes.getSize(); ++remoteIndex ) {
		remote_t* remote = remotes[remoteIndex];

		for( Uint32 c = 0; c < remote->resendStack.getSize(); ++c ) {
			safepacket_t* safepacket = remote->resendStack[c];

			if( SDL_GetTicks() - safepacket->lastTimeSent >= msBeforeResend ) {
				if( sendPacket(remote->id,safepacket->packet) ) {
					++safepacket->resends;
					if( safepacket->resends >= maxPacketRetries ) {
						remote->resendStack.remove(c);
						delete safepacket;
						--c;
					} else {
						safepacket->lastTimeSent = SDL_GetTicks();
					}
				}
			}
		}
	}
}

Uint32 Net::getRemoteWithID(const Uint32 remoteID) {
	for( Uint32 c = 0; c < remotes.getSize(); ++c ) {
		remote_t* remote = remotes[c];
		if( remote->id == remoteID ) {
			return c;
		}
	}

	return UINT32_MAX;
}

Uint32 Net::getRemoteWithID(const Uint32 remoteID) const {
	for( Uint32 c = 0; c < remotes.getSize(); ++c ) {
		const remote_t* remote = remotes[c];
		if( remote->id == remoteID ) {
			return c;
		}
	}

	return UINT32_MAX;
}

static int console_join(int argc, const char** argv) {
	if( argc < 1 ) {
		mainEngine->fmsg(Engine::MSG_ERROR,"A remote address is needed. ex: join localhost");
		return 1;
	}
	mainEngine->joinServer(argv[0]);
	return 0;
}

static int console_say(int argc, const char** argv) {
	if( argc < 1 ) {
		mainEngine->fmsg(Engine::MSG_ERROR,"A message is needed. ex: say Hello World!");
		return 1;
	}
	mainEngine->chat(argv[0]);
	return 0;
}

static Ccmd ccmd_join("join","connects to a remote server",&console_join);
static Ccmd ccmd_say("say","transmit a chat message to the remote server",&console_say);