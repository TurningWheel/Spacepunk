// Player.hpp

// Multiple players can exist per client. This way we can do splitscreen multiplayer

#pragma once

#include "Angle.hpp"
#include "Vector.hpp"
#include "Entity.hpp"

class Model;
class BBox;
class Camera;

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
	~Player();

	// static properties
	static const Uint32 invalidID = UINT32_MAX;
	static const char* defaultName;
	static const float standFeetHeight;
	static const Vector standScale;
	static const Vector standOrigin;
	static const float crouchFeetHeight;
	static const Vector crouchScale;
	static const Vector crouchOrigin;

	// spawns the player if they have not already been spawned
	// @param _world the world to spawn in
	// @param pos the location to spawn at
	// @param ang the orientation to spawn with
	// @param _uid the uid that our entity will have
	// @return true if successfully spawned the player, false otherwise
	bool spawn(World& _world, const Vector& pos, const Angle& ang);

	// despawns the player, removing their presence from the world
	// @return true if successfully despawned, false otherwise
	bool despawn();

	// control the player (preprocess)
	void control();

	// move the camera (postprocess)
	void updateCamera();

	// when one of my entities is deleted, this gets called so I can clear the pointer
	// @param entity the entity that was deleted
	void onEntityDeleted(Entity* entity);

	// puts the player in a crouch or standing position
	// @param crouch if true, player will crouch, otherwise player will stand
	void putInCrouch(bool crouch);

	// updates the player's colors
	// @param _colors the colors to use
	void updateColors(const colors_t& _colors);

	// used by other clients to set the entity for this player
	void setEntity(Entity* _entity);

	// getters & setters
	const char*				getName() const			{ return name.get(); }
	const Uint32			getServerID() const		{ return serverID; }
	const Uint32			getLocalID() const		{ return localID; }
	const Uint32			getClientID() const		{ return clientID; }
	Entity*					getEntity()				{ return entity; }
	const colors_t&			getColors() const		{ return colors; }
	Camera*					getCamera() const		{ return camera; }
	bool					isCrouching() const		{ return crouching; }
	bool					isMoving() const		{ return moving; }
	bool					hasJumped() const		{ return jumped; }
	const Angle&			getLookDir() const		{ return lookDir; }

	void	setName(const char* _name)				{ name = _name; }
	void	setServerID(Uint32 id)					{ serverID = id; }
	void	setLocalID(Uint32 id)					{ localID = id; }
	void	setClientID(Uint32 id)					{ clientID = id; }
	void	setMoving(bool b)						{ moving = b; }
	void	setJumped(bool b)						{ jumped = b; }
	void	setLookDir(const Angle& ang)			{ lookDir = ang; }

private:
	StringBuf<64> name = defaultName;	// the player's name
	Uint32 serverID = invalidID;		// canonical player number. this is 0 for player 1, 1 for player 2, etc.
	Uint32 localID = invalidID;			// local player number. for the first player on a client, this is 0, regardless of serverID
	Uint32 clientID = invalidID;		// id number of the client associated with this player
	colors_t colors;					// cosmetic

	// when despawned, this is null
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

	// player controller vars
	float buttonRight = 0.f;
	float buttonLeft = 0.f;
	float buttonForward = 0.f;
	float buttonBackward = 0.f;
	bool buttonJump = false;
	bool buttonCrouch = false;

	bool moving = false;
	bool crouching = false;
	bool jumped = false;
	Angle lookDir;
	Angle oldLookDir;
	Vector originalVel;
};