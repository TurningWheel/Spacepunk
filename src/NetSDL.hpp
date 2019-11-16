// NetSDL.hpp

#pragma once

#include "Main.hpp"
#include "Packet.hpp"
#include "Net.hpp"

class NetSDL : public Net {
public:
	NetSDL(Game& _parent);
	virtual ~NetSDL();

	// remote host
	struct sdlremote_t : remote_t {
		NetSDL* parent = nullptr;

		IPaddress host;

		virtual ~sdlremote_t() {
			for( Uint32 c = 0; c < parent->remotes.getSize(); ++c ) {
				if( parent->remotes[c]->id == id ) {
					parent->remotes.remove(c);
					break;
				}
			}
		}
	};

	// connection request
	struct sdlrequest_t : request_t {
		IPaddress ip;
	};

	// inits the net interface
	virtual void init() override;

	// closes the network connection
	virtual void term() override;

	// hosts a new open connection
	// @param port the port to host the connection on
	// @return true when the port is successfully opened, false on failure
	virtual bool host(Uint16 port) override;

	// terminates any existing connection, then connects to the given address
	// @param address the address to connect to, or nullptr to host an open connection
	// @return true on connection success, false on failure
	virtual bool connect(const char* address, Uint16 port) override;

	// disconnects a remote host
	// @param remoteID the remote host to disconnect from
	// @param inform if true, remote host will be notified of disconnect; otherwise it will not
	// @return true if the disconnect succeeded, false otherwise
	virtual bool disconnect(Uint32 remoteID, bool inform=true) override;

	// shuts down an open localhost connection, if any are open
	// @return true if the disconnect succeeded, false otherwise
	virtual bool disconnectHost() override;

	// shuts down any and all remote connections
	// @return true if the disconnect succeeded, false otherwise
	virtual bool disconnectAll() override;

	// find the name of the given remote host
	// @param remoteID the id of the remote host we wish to query
	// @return the name of a remote host in a string
	virtual const char* getHostname(Uint32 remoteID) const override;

	// @return the type of Net layer this is
	virtual const kind_t getKind() const override { return SDL_NET; }

	// sends a packet to a remote recipient
	// @param packet the packet to send
	// @param remoteID the id of the recipient
	// @return true if the send succeeded, false otherwise
	virtual bool sendPacket(Uint32 remoteID, const Packet& packet) override;

	// just like sendPacket, except guarantees delivery
	// @param packet the packet to send
	// @param remoteID the id of the recipient
	// @return true if the send succeeded, false otherwise
	virtual bool sendPacketSafe(Uint32 remoteID, const Packet& packet) override;

	// broadcasts a packet to all remote hosts
	// @param packet the packet to send
	// @return true if the send succeeded, false otherwise
	virtual bool broadcast(Packet& packet) override;

	// just like broadcast, except guarantees delivery
	// @param packet the packet to send
	// @return true if the send succeeded, false otherwise
	virtual bool broadcastSafe(Packet& packet) override;

	// pops a packet from the stack and returns it
	// @param remoteIndex the index of the remote host to read a packet from (not the id!)
	// @return the highest packet on the stack, or nullptr if no packets are left
	virtual Packet* recvPacket(unsigned int remoteIndex) override;

	// @return the number of remote hosts we have connections with
	virtual Uint32 numRemoteHosts() const override;

	// detects and completes any active connection requests
	virtual void update() override;

	// handles network level packets
	// @param packet the packet data
	// @param type the 4 char packet type string
	// @param remoteID the remote id that the packet came from
	// @return positive number if the packet was interpreted here, 0 otherwise
	virtual int handleNetworkPacket(Packet& packet, const char* type, Uint32 remoteID) override;

	// getters & setters
	const ArrayList<sdlremote_t*>&	getSDLRemoteHosts() const	{ return SDLremotes; }

protected:
	UDPpacket* SDLsendPacket = nullptr;
	UDPpacket* SDLrecvPacket = nullptr;
	UDPsocket SDLsocket;
	ArrayList<sdlremote_t*> SDLremotes;
	ArrayList<sdlrequest_t> SDLrequests;

	// collects any available packets from the remote hosts and place them on a local stack
	// @param data the NetSDL* obj to process
	// @return 0 on success, non-zero on error
	static int runThread(void* data);

	// completes a connection to a host
	// @param data the request data
	virtual void completeConnection(void* data) override;
};
