//! @file Net.hpp

#pragma once

#include "Main.hpp"
#include "Packet.hpp"

class Game;

//! Defines a network interface.
//! If hosting a server, host() should be called first, followed by connect() to connect to clients.
//! If acting as client, term() should be called to close any existing connection(s), followed by connect() to join a server.
class Net {
public:
	Net() = delete;
	Net(Game& _parent);
	Net(const Net&) = delete;
	Net(Net&&) = delete;
	virtual ~Net() = default;

	Net& operator=(const Net&) = delete;
	Net& operator=(Net&&) = delete;

	//! invalid client id
	static const Uint32 invalidID = UINT32_MAX;

	//! maximum players
	static const Uint32 maxPlayers = 4;

	//! default server port
	static const Uint16 defaultPort = 12916;

	//! max packet send retries
	static const Uint16 maxPacketRetries = 10;

	//! milliseconds between safe packet retries
	static const Uint32 msBeforeResend = 200;

	//! network connection type
	enum kind_t {
		UNKNOWN,
		SDL_NET,
		STEAM,
		KIND_TYPE_LENGTH
	};

	//! safe packet
	struct safepacket_t {
		Packet packet;
		Uint32 resends = 0;
		Uint32 lastTimeSent = 0;
		Uint32 id = invalidID;
	};

	//! remote host
	struct remote_t {
		virtual ~remote_t() {
			while (packetStack.getSize() > 0) {
				Packet* packet = packetStack.pop();
				delete packet;
			}
			while (resendStack.getSize() > 0) {
				safepacket_t* safePacket = resendStack.pop();
				delete safePacket;
			}
		}

		ArrayList<Packet*> packetStack;			//! packets received from the host
		ArrayList<Uint32> safeRcvdHash[128];	//! hash list of safe packets received

		char address[256] = { 0 };				//! address (hostname or ip address)
		Uint16 port = 0;						//! port number

		Uint32 id = invalidID;					//! universal id number
		Uint32 gid = invalidID;					//! client's generated id
		Uint32 timestamp = 0;					//! the latest timestamp from this host; earlier packets might not be read

		Uint32 safePacketsSent = 0;				//! total number of safe packets sent TO this host
		ArrayList<safepacket_t*> resendStack;	//! list of packets due for resend
	};

	//! connection request
	struct request_t {
		Uint32 gid = 0;
	};

	//! inits the net interface
	virtual void init() = 0;

	//! closes the network connection
	virtual void term() = 0;

	//! hosts a new open connection
	//! @param port the port to host the connection on
	//! @return true when the port is successfully opened, false on failure
	virtual bool host(Uint16 port) = 0;

	//! connects to the given address, forming a new remote connection
	//! @param address the address to connect to
	//! @param port the port to open the connection on
	//! @return true on connection success, false on failure
	virtual bool connect(const char* address, Uint16 port) = 0;

	//! disconnects a remote connection
	//! @param remoteID the remote host to disconnect from
	//! @param inform if true, remote host will be notified of disconnect; otherwise it will not
	//! @return true if the disconnect succeeded, false otherwise
	virtual bool disconnect(Uint32 remoteID, bool inform = true) = 0;

	//! shuts down an open localhost connection, if any are open
	//! @return true if the disconnect succeeded, false otherwise
	virtual bool disconnectHost() = 0;

	//! shuts down any and all remote connections
	//! @return true if the disconnect succeeded, false otherwise
	virtual bool disconnectAll() = 0;

	//! find the name of the given remote host
	//! @param remoteID the id of the remote host we wish to query
	//! @return the name of a remote host in a string
	virtual const char* getHostname(Uint32 remoteID) const = 0;

	//! @return the type of Net layer this is
	virtual const kind_t getKind() const = 0;

	//! sends a packet to a remote recipient
	//! @param packet the packet to send
	//! @param remoteID the id of the recipient
	//! @return true if the send succeeded, false otherwise
	virtual bool sendPacket(Uint32 remoteID, const Packet& packet) = 0;

	//! just like sendPacket, except guarantees delivery
	//! @param packet the packet to send
	//! @param remoteID the id of the recipient
	//! @return true if the send succeeded, false otherwise
	virtual bool sendPacketSafe(Uint32 remoteID, const Packet& packet) = 0;

	//! broadcasts a packet to all remote hosts
	//! @param packet the packet to send
	//! @return true if the send succeeded, false otherwise
	virtual bool broadcast(Packet& packet) = 0;

	//! just like broadcast, except guarantees delivery
	//! @param packet the packet to send
	//! @return true if the send succeeded, false otherwise
	virtual bool broadcastSafe(Packet& packet) = 0;

	//! pops a packet from the stack and returns it
	//! @param remoteIndex the index of the remote host to read a packet from (not the id!)
	//! @return the highest packet on the stack, or nullptr if no packets are left
	virtual Packet* recvPacket(unsigned int remoteIndex) = 0;

	//! detects and completes any active connection requests, resends guaranteed packets
	virtual void update();

	//! @return the number of remote hosts we have connections with
	virtual Uint32 numRemoteHosts() const = 0;

	//! signs a packet with user id and timestamp info
	//! @param packet the packet to sign
	//! @return true if packet was successfully signed, false on failure
	bool signPacket(Packet& packet);

	//! handles network level packets
	//! @param packet the packet data
	//! @param type the 4 char packet type string
	//! @param remoteID the remote id that the packet came from
	//! @return positive number if the packet was interpreted here, 0 otherwise
	virtual int handleNetworkPacket(Packet& packet, const char* type, Uint32 remoteID) = 0;

	//! attempts to lock the net thread
	//! @return true if we've locked the thread, otherwise false
	bool lockThread();

	//! unlocks the net thread so it can be used again
	void unlockThread();

	bool				    	isConnected() const { return connected; }
	bool			      		isHosting() const { return hosting; }
	Uint32			        	getLocalID() const { return localID; }
	Uint32		        		getLocalGID() const { return localGID; }
	ArrayList<remote_t*>&		getRemoteHosts() { return remotes; }

	void					setParent(Game* _parent) { parent = _parent; }

protected:
	Game* parent = nullptr;
	bool hosting = false;
	bool connected = false;

	Uint32 localID = invalidID;		//! given by server to client, acts as official client number
	Uint32 localGID = invalidID;	//! generated by every client on startup, acts as pseudo-unique client id

	Uint32 numClients = 0;			//! increments by 1 with each connection

	ArrayList<remote_t*> remotes;

	//! completes a connection to a host
	//! @param data the request data
	virtual void completeConnection(void* data) = 0;

	//! threading
	String threadName;
	SDL_Thread* thread = nullptr;
	SDL_mutex* connectLock = nullptr;
	SDL_mutex* execLock = nullptr;
	SDL_mutex* killLock = nullptr;
	bool exec = false;
	bool kill = false;

	//! gets the remote host with the given id
	//! @param remoteID the id of the remote host to get
	//! @return an index to the remote host, or UINT32_MAX if they could not be found
	Uint32 getRemoteWithID(const Uint32 remoteID);

	//! gets the remote host with the given id
	//! @param remoteID the id of the remote host to get
	//! @return an index to the remote host, or UINT32_MAX if they could not be found
	Uint32 getRemoteWithID(const Uint32 remoteID) const;
};
