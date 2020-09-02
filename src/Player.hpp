//! @file Player.hpp

#pragma once

#include "Rotation.hpp"
#include "Vector.hpp"
#include "Entity.hpp"

class Model;
class BBox;
class Camera;

//! A Player is an object that contains all the info to describe a Player.
//! A Player may be linked to an entity to provide the player direct controls to it.
//! Multiple players can exist per client. This way you can do splitscreen multiplayer
class Player {
public:
	struct colors_t {
		ArrayList<GLfloat> headRChannel;
		ArrayList<GLfloat> headGChannel;
		ArrayList<GLfloat> headBChannel;
		ArrayList<GLfloat> armsRChannel;
		ArrayList<GLfloat> armsGChannel;
		ArrayList<GLfloat> armsBChannel;
		ArrayList<GLfloat> feetRChannel;
		ArrayList<GLfloat> feetGChannel;
		ArrayList<GLfloat> feetBChannel;
		ArrayList<GLfloat> torsoRChannel;
		ArrayList<GLfloat> torsoGChannel;
		ArrayList<GLfloat> torsoBChannel;
	};

	Player();
	Player(const char* _name);
	Player(const char* _name, colors_t _colors);
	Player(const Player&) = default;
	Player(Player&&) = default;
	~Player() = default;

	Player& operator=(const Player&) = default;
	Player& operator=(Player&&) = default;

	//! static properties
	static const Uint32 invalidID = UINT32_MAX;
	static const char* defaultName;

	//! spawns the player if they have not already been spawned
	//! @param _world the world to spawn in
	//! @param pos the location to spawn at
	//! @param ang the orientation to spawn with
	//! @param _uid the uid that our entity will have
	//! @return true if successfully spawned the player, false otherwise
	bool spawn(World& _world, const Vector& pos, const Rotation& ang);

	//! despawns the player, removing their presence from the world
	//! @return true if successfully despawned, false otherwise
	bool despawn();

	//! control the player (preprocess)
	void control();

	//! update the player (process)
	void process();

	//! move the camera (postprocess)
	void updateCamera();

	//! when one of my entities is deleted, this gets called so I can clear the pointer
	//! @param entity the entity that was deleted
	void onEntityDeleted(Entity* entity);

	//! puts the player in a crouch or standing position
	//! @param crouch if true, player will crouch, otherwise player will stand
	void putInCrouch(bool crouch);

	//! updates the player's colors
	//! @param _colors the colors to use
	void updateColors(const colors_t& _colors);

	//! used by other clients to set the entity for this player
	void setEntity(Entity* _entity);

	//! setup player gui
	void setupGUI();

	//! update player gui
	void updateGUI();

	const char*				getName() const { return name.get(); }
	const Uint32			getServerID() const { return serverID; }
	const Uint32			getLocalID() const { return localID; }
	const Uint32			getClientID() const { return clientID; }
	Entity*					getEntity() { return entity; }
	const colors_t&			getColors() const { return colors; }
	Camera*					getCamera() const { return camera; }
	bool					isCrouching() const { return crouching; }
	bool					isMoving() const { return moving; }
	bool					hasJumped() const { return jumped; }
	bool					isInvVisible() const { return inventoryVisible; }

	void	setName(const char* _name) { name = _name; }
	void	setServerID(Uint32 id) { serverID = id; }
	void	setLocalID(Uint32 id) { localID = id; }
	void	setClientID(Uint32 id) { clientID = id; }
	void	setMoving(bool b) { moving = b; }
	void	setJumped(bool b) { jumped = b; }

private:
	StringBuf<64> name = defaultName;	//! the player's name
	Uint32 serverID = invalidID;		//! canonical player number. this is 0 for player 1, 1 for player 2, etc.
	Uint32 localID = invalidID;			//! local player number. for the first player on a client, this is 0, regardless of serverID
	Uint32 clientID = invalidID;		//! id number of the client associated with this player
	colors_t colors;					//! cosmetic

	//! when despawned, this is null
	Entity* entity = nullptr;
	Component* models = nullptr;
	Model* head = nullptr;
	Model* torso = nullptr;
	Model* arms = nullptr;
	Model* feet = nullptr;
	BBox* bbox = nullptr;
	Camera* camera = nullptr;
	Model* lTool = nullptr;
	Model* rTool = nullptr;
	Light* lamp = nullptr;

	//! input vars
	float buttonRight = 0.f;
	float buttonLeft = 0.f;
	float buttonForward = 0.f;
	float buttonBackward = 0.f;
	bool buttonJump = false;
	bool buttonCrouch = false;
	float buttonLeanLeft = 0.f;
	float buttonLeanRight = 0.f;
	float mouseX = 0.f;
	float mouseY = 0.f;

	//! inventory vars
	bool inventoryVisible = false;
	bool holdingInteract = false;
	float interactHoldTime = 0;
	const float HOLD_TO_PICKUP_TIME = 0;
	Entity* previousInteractedEntity = nullptr;

	//! controller vars
	bool moving = false;
	bool crouching = false;
	bool jumped = false;
	Rotation oldLookDir;
	Vector originalVel;
	Quaternion playerAng;
	bool orienting = false;
	float orient = 0.f;
	Rotation rot;
	Vector crouchScale;
	Vector crouchOrigin;
	Vector standScale;
	Vector standOrigin;
	Vector originOffset;
	float crouchModelOffsetFactor = 0.f;
	Vector originalModelsPos;
	float bobAngle = 0.f;
	float bobLength = 0.f;
	Vector originalCameraPos;
};

extern Cvar cvar_fov;