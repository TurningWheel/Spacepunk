// Player.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "Player.hpp"
#include "Random.hpp"
#include "World.hpp"
#include "Client.hpp"
#include "Input.hpp"
#include "Console.hpp"
#include "Mixer.hpp"
#include "Model.hpp"
#include "BBox.hpp"
#include "Camera.hpp"
#include "Frame.hpp"
#include "Renderer.hpp"

const char* Player::defaultName = "Player";

static Cvar cvar_mouseYInvert("player.mouselook.inverty", "invert y-axis on mouse look", "0");
static Cvar cvar_mouseSmooth("player.mouselook.smooth", "smooth mouse look over multiple frames", "0");
static Cvar cvar_mouseSpeed("player.mouselook.speed", "adjusts mouse sensitivity", "0.1");
static Cvar cvar_gravity("player.gravity", "gravity that players are subjected to", "9");
static Cvar cvar_speed("player.speed", "player movement speed", "28");
static Cvar cvar_airControl("player.aircontrol", "movement speed modifier while in the air", ".02");
static Cvar cvar_jumpPower("player.jump.power", "player jump strength", "4.0");
static Cvar cvar_canCrouch("player.crouch.enabled", "whether player can crouch at all or not", "1");
static Cvar cvar_crouchSpeed("player.crouch.speed", "movement speed modifier while crouching", ".25");
static Cvar cvar_wallWalk("player.wallwalk.enabled", "enable wall-walking ability on the player", "0");
static Cvar cvar_wallWalkLimit("player.wallwalk.limit", "the maximum difference in slope that can be overcome with wall-walking", "45");
static Cvar cvar_zeroGravity("player.zerog.enabled", "enable zero-g effects on the player", "0");
static Cvar cvar_slopeLimit("player.slope.limit", "the maximum slope of a floor traversible by a player", "45");
static Cvar cvar_stepHeight("player.step.height", "maximum step height that a player can ascend", "8");
static Cvar cvar_enableBob("player.bob.enabled", "enable view bobbing", "1");
static Cvar cvar_enableHeadBoneCamera("player.headbonecamera.enabled", "bind the camera to the player's headbone", "0");
static Cvar cvar_defaultController("player.default.enabled", "enable built-in first-person player controller", "1");

Cvar cvar_fov("player.fov", "field of view", "70.0");
Cvar cvar_mouselook("player.mouselook.playernum", "assigns mouse control to the given player", "0");

Player::Player() {
	Random& rand = mainEngine->getRandom();

	float hairR = rand.getFloat();
	float hairG = rand.getFloat();
	float hairB = rand.getFloat();

	float suitR = rand.getFloat();
	float suitG = rand.getFloat();
	float suitB = rand.getFloat();

	// setup a random color

	colors.headRChannel = { .7f, .5f, .2f, 1.f };
	colors.headGChannel = { hairR, hairG, hairB, 1.f };
	colors.headBChannel = { suitR, suitG, suitB, 1.f };

	colors.torsoRChannel = { suitR, suitG, suitB, 1.f };
	colors.torsoGChannel = { .5f, .5f, .5f, 1.f };
	colors.torsoBChannel = { .2f, .2f, .2f, 1.f };

	colors.armsRChannel = { suitR, suitG, suitB, 1.f };
	colors.armsGChannel = { .5f, .5f, .5f, 1.f };
	colors.armsBChannel = { .2f, .2f, .2f, 1.f };

	colors.feetRChannel = { suitR, suitG, suitB, 1.f };
	colors.feetGChannel = { .5f, .5f, .5f, 1.f };
	colors.feetBChannel = { .2f, .2f, .2f, 1.f };
}

Player::Player(const char* _name) : Player() {
	name = _name;
}

Player::Player(const char* _name, colors_t _colors) {
	name = _name;
	colors = _colors;
}

void Player::setEntity(Entity* _entity) {
	entity = _entity;
	if (entity && cvar_defaultController.toInt()) {
		models = entity->findComponentByName<Component>("models");
		head = entity->findComponentByName<Model>("Head");
		torso = entity->findComponentByName<Model>("Torso");
		arms = entity->findComponentByName<Model>("Arms");
		feet = entity->findComponentByName<Model>("Feet");
		bbox = entity->findComponentByName<BBox>("physics");
		camera = entity->findComponentByName<Camera>("Camera");
		rTool = entity->findComponentByName<Model>("RightTool");
		lTool = entity->findComponentByName<Model>("LeftTool");
		lamp = entity->findComponentByName<Light>("Lamp");
		if (!models || !bbox || !camera) {
			mainEngine->fmsg(Engine::MSG_WARN, "failed to setup player for third party client: missing bodypart");
		}
	}
}

bool Player::spawn(World& _world, const Vector& pos, const Rotation& ang) {
	if (entity) {
		mainEngine->fmsg(Engine::MSG_ERROR, "failed to spawn player: already spawned");
		return false;
	}

	Uint32 uid;
	if (_world.isClientObj() && clientID == invalidID) {
		// this is for clients, and causes the entity to get a non-canonical uid.
		uid = UINT32_MAX - 1;
		while (_world.getEntities().find(uid) != nullptr) {
			--uid;
		}
	} else {
		// this is for the server, and causes the entity to get a new canonical uid.
		uid = UINT32_MAX;
	}

	// create 
	const Entity::def_t* def = Entity::findDef("Player");
	if (def) {
		entity = Entity::spawnFromDef(&_world, *def, pos, ang, uid);
	} else {
		entity = nullptr;
	}
	if (!entity) {
		mainEngine->fmsg(Engine::MSG_ERROR, "failed to spawn player: spawn from def failed");
		return false;
	}
	entity->setShouldSave(false);
	entity->setPlayer(this);

	// get body parts
	models = entity->findComponentByName<Component>("models");
	head = entity->findComponentByName<Model>("Head");
	torso = entity->findComponentByName<Model>("Torso");
	arms = entity->findComponentByName<Model>("Arms");
	feet = entity->findComponentByName<Model>("Feet");
	bbox = entity->findComponentByName<BBox>("physics");
	camera = entity->findComponentByName<Camera>("Camera");
	rTool = entity->findComponentByName<Model>("RightTool");
	lTool = entity->findComponentByName<Model>("LeftTool");
	lamp = entity->findComponentByName<Light>("Lamp");
	if (!models || !bbox || !camera) {
		mainEngine->fmsg(Engine::MSG_ERROR, "failed to spawn player: missing bodypart");
		entity->remove();
		entity = nullptr;
		return false;
	}

	Quaternion q(entity->getLookDir());
	camera->setLocalAng(q);

	// update colors
	updateColors(colors);

	// setup collision mesh and check that we spawned in a valid position
	entity->update();
	if (bbox->checkCollision()) {
		mainEngine->fmsg(Engine::MSG_ERROR, "failed to spawn player: no room at spawn location");
		entity->remove();
		return false;
	}

	entity->setFlag(static_cast<int>(Entity::flag_t::FLAG_UPDATE));
	if (_world.isClientObj()) {
		if (clientID == invalidID) {
			Rect<Sint32> rect;
			rect.x = 0;
			rect.y = 0;
			rect.w = Frame::virtualScreenX;
			rect.h = Frame::virtualScreenY;
			camera->setWin(rect);

			setupGUI();
		} else {
			// we don't own this player and shouldn't see their camera
			Rect<Sint32> rect;
			rect.x = 0;
			rect.y = 0;
			rect.w = 0;
			rect.h = 0;
			camera->setWin(rect);
		}

		mainEngine->fmsg(Engine::MSG_INFO, "Client spawned player (%d) at (%1.f, %.1f, %.1f)", serverID, entity->getPos().x, entity->getPos().y, entity->getPos().z);
	} else {
		mainEngine->fmsg(Engine::MSG_INFO, "Server spawned player (%d) at (%1.f, %.1f, %.1f)", serverID, entity->getPos().x, entity->getPos().y, entity->getPos().z);
	}

	playerAng = entity->getAng();

	// set crouch scaling vars
	standScale = bbox->getLocalScale();
	standOrigin = Vector(0.f);
	crouchScale = standScale;
	crouchOrigin = standOrigin;
	crouchScale.z = entity->getKeyValueAsFloat("crouchBBoxScale");
	crouchOrigin.z += standScale.z - crouchScale.z;
	crouchModelOffsetFactor = entity->getKeyValueAsFloat("crouchModelOffsetFactor");
	originalModelsPos = models->getLocalPos();
	originalCameraPos = camera->getLocalPos();

	return true;
}

void Player::process() {
	updateGUI();
}

void Player::setupGUI() {
	Client* client = mainEngine->getLocalClient(); assert(client);
	Frame* gui = client->getGUI(); assert(gui);
	Renderer* renderer = client->getRenderer(); assert(renderer);

	// create reticle
	char name[9] = "reticle"; name[7] = '0' + localID; name[8] = '\0';
	gui->addImage(Rect<Sint32>(), WideVector(1.f), "images/gui/reticle_1.png", name);
}

void Player::updateGUI() {
	if (!entity) {
		return;
	}
	Client* client = mainEngine->getLocalClient(); assert(client);
	Frame* gui = client->getGUI(); assert(gui);

	// update reticle position (in-case it moves)
	Rect<Sint32> pos;
	char name[9] = "reticle"; name[7] = '0' + localID; name[8] = '\0';
	auto reticle = gui->findImage(name); assert(reticle);
	auto img = mainEngine->getImageResource().dataForString(reticle->path.get());
	if (img) {
		pos.w = img->getWidth();
		pos.h = img->getHeight();
	}
	pos.x = camera->getWin().x + camera->getWin().w / 2 - pos.w / 2;
	pos.y = camera->getWin().y + camera->getWin().h / 2 - pos.h / 2;
	reticle->pos = pos;
}

void Player::updateColors(const colors_t& _colors) {
	colors = _colors;

	Mesh::shadervars_t shaderVars;

	// load head colors
	if (head) {
		shaderVars = head->getShaderVars();
		shaderVars.customColorR = colors.headRChannel;
		shaderVars.customColorG = colors.headGChannel;
		shaderVars.customColorB = colors.headBChannel;
		shaderVars.customColorA = colors.headGChannel;
		head->setShaderVars(shaderVars);
	}

	// load torso colors
	if (torso) {
		shaderVars = torso->getShaderVars();
		shaderVars.customColorR = colors.torsoRChannel;
		shaderVars.customColorG = colors.torsoGChannel;
		shaderVars.customColorB = colors.torsoBChannel;
		shaderVars.customColorA = colors.headGChannel;
		torso->setShaderVars(shaderVars);
	}

	// load arm colors
	if (arms) {
		shaderVars = arms->getShaderVars();
		shaderVars.customColorR = colors.armsRChannel;
		shaderVars.customColorG = colors.armsGChannel;
		shaderVars.customColorB = colors.armsBChannel;
		shaderVars.customColorA = colors.headGChannel;
		arms->setShaderVars(shaderVars);
	}

	// load feet colors
	if (feet) {
		shaderVars = feet->getShaderVars();
		shaderVars.customColorR = colors.feetRChannel;
		shaderVars.customColorG = colors.feetGChannel;
		shaderVars.customColorB = colors.feetBChannel;
		shaderVars.customColorA = colors.headGChannel;
		feet->setShaderVars(shaderVars);
	}
}

void Player::putInCrouch(bool crouch) {
	if (!cvar_canCrouch.toInt() || !bbox || !entity) {
		return;
	}
	crouching = crouch;
	Vector scale = bbox->getLocalScale();
	Vector origin = originOffset;
	const Vector& targetScale = crouching ? crouchScale : standScale;
	const Vector& targetOrigin = crouching ? crouchOrigin : standOrigin;
	if (scale.z < targetScale.z) {
		scale.z = min(scale.z + 1.f, targetScale.z);
	} else if (scale.z > targetScale.z) {
		scale.z = max(scale.z - 1.f, targetScale.z);
	}
	if (origin.z < targetOrigin.z) {
		origin.z = min(origin.z + 1.f, targetOrigin.z);
	} else if (origin.z > targetOrigin.z) {
		origin.z = max(origin.z - 1.f, targetOrigin.z);
	}
	bbox->setLocalScale(scale);
	originOffset = origin;
	float stepHeight = cvar_stepHeight.toFloat();
	models->setLocalPos(originalModelsPos + originOffset * crouchModelOffsetFactor + Vector(0.f, 0.f, stepHeight));
}

Cvar cvar_maxHTurn("player.turn.horizontal", "maximum turn range for player, horizontal", "0.0");
Cvar cvar_maxVTurn("player.turn.vertical", "maximum turn range for player, vertical", "90.0");

void Player::control() {
	Client* client = mainEngine->getLocalClient();
	if (!entity || !client) {
		return;
	}
	World* world = entity->getWorld();
	if (!world) {
		return;
	}

	if (client->isConsoleActive() || cvar_mouselook.toInt() < 0) {
		mainEngine->setMouseRelative(false);
	} else {
		mainEngine->setMouseRelative(true);
	}

	Vector vel = originalVel;
	//Vector vel = entity->getVel();
	bool warpNeeded = false;

	World::hit_t floorHit, ceilingHit;
	float totalHeight = standScale.z + originOffset.z;
	float nearestCeiling = entity->nearestCeiling(ceilingHit);
	float nearestFloor = entity->nearestFloor(floorHit);

	Entity* entityStandingOn = floorHit.manifest ? floorHit.manifest->entity : nullptr;
	Entity* entityAbove = ceilingHit.manifest ? ceilingHit.manifest->entity : nullptr;

	// collect movement inputs
	Input& input = mainEngine->getInput(localID);
	buttonRight = 0.f;
	buttonLeft = 0.f;
	buttonForward = 0.f;
	buttonBackward = 0.f;
	buttonLeanLeft = 0.f;
	buttonLeanRight = 0.f;
	buttonJump = false;
	buttonCrouch = cvar_canCrouch.toInt() ? nearestCeiling <= totalHeight && !entity->isFalling() : false;
	if (!client->isConsoleActive()) {
		buttonCrouch |= input.binary("MoveDown");

		if (input.binary("MoveUp") && !jumped) {
			if (nearestCeiling > totalHeight && !buttonCrouch) {
				buttonJump = true;
			}
		} else if (!input.binary("MoveUp") && jumped) {
			jumped = false;
		}

		if (!input.binary("Lean")) {
			buttonRight = input.analog("MoveRight");
			buttonLeft = input.analog("MoveLeft");
			buttonForward = input.analog("MoveForward");
			buttonBackward = input.analog("MoveBackward");
			buttonLeanLeft = input.analog("LeanLeft");
			buttonLeanRight = input.analog("LeanRight");

			// restrict inputs to circle
			float dir = atan2f(buttonForward - buttonBackward, buttonRight - buttonLeft);
			float cosDir = fabs(cosf(dir));
			float sinDir = fabs(sinf(dir));
			buttonRight = min(cosDir, buttonRight);
			buttonLeft = min(cosDir, buttonLeft);
			buttonForward = min(sinDir, buttonForward);
			buttonBackward = min(sinDir, buttonBackward);
		} else {
			buttonLeanLeft = input.analog("MoveLeft");
			buttonLeanRight = input.analog("MoveRight");
		}
	}
	if (buttonRight || buttonLeft || buttonForward || buttonBackward) {
		moving = true;
	} else {
		moving = false;
	}

	// animation
	if (buttonCrouch) {
		crouching = true;
	} else {
		crouching = false;
	}
	if (cvar_canCrouch.toInt() == 0) {
		crouching = false;
	}

	// direction vectors
	Vector forward = playerAng.toVector();
	Vector right = (playerAng * Quaternion(Rotation(PI / 2.f, 0.f, 0.f))).toVector();
	Vector down = (playerAng * Quaternion(Rotation(0.f, PI / 2.f, 0.f))).toVector();
	Vector up = (playerAng * Quaternion(Rotation(0.f, -PI / 2.f, 0.f))).toVector();

	// time and speed
	float speedFactor = (crouching ? cvar_crouchSpeed.toFloat() : 1.f) * (entity->isFalling() ? cvar_airControl.toFloat() : 1.f) * cvar_speed.toFloat();
	float timeFactor = 1.f / 60.f;

	// set bbox and origin
	if (cvar_zeroGravity.toInt()) {
		putInCrouch(false);
	} else {
		putInCrouch(crouching);
	}

	// set falling state and do attach-to-ground
	float stepHeight = cvar_stepHeight.toFloat();
	float middleOfBody = standScale.z + stepHeight;
	if (cvar_zeroGravity.toInt()) {
		entity->setFalling(true);
		vel += up * buttonJump * speedFactor * timeFactor;
		vel -= up * buttonCrouch * speedFactor * timeFactor;
	} else {
		if (entity->isFalling()) {
			if (nearestFloor <= middleOfBody && vel.normal().dot(down) > 0.f) {
				const float slope = cosf(cvar_slopeLimit.toFloat() * PI / 180.f);
				if (up.dot(floorHit.normal) > slope) {
					entity->setFalling(false);
					vel -= vel * down.absolute();
				}
			} else {
				vel += down * cvar_gravity.toFloat() * timeFactor;
			}
		} else {
			if (buttonJump) {
				jumped = true;
				entity->setFalling(true);
				vel += up * cvar_jumpPower.toFloat();
			} else {
				if (nearestFloor > middleOfBody + 1.f) {
					entity->setFalling(true);
				} else {
					float feetHeight = bbox->getLocalScale().z + stepHeight;
					float rebound = min(1.f, feetHeight - nearestFloor);
					Vector pos = entity->getPos();
					pos = pos + up * rebound;
					entity->setPos(pos);
					warpNeeded = true;
				}
			}
		}
	}

	// calculate movement vectors
	vel += forward * buttonForward * speedFactor * timeFactor;
	vel -= forward * buttonBackward * speedFactor * timeFactor;
	vel += right * buttonRight * speedFactor * timeFactor;
	vel -= right * buttonLeft * speedFactor * timeFactor;

	// friction
	if (!entity->isFalling()) {
		vel *= .9;
	}
	rot.yaw *= .5;
	rot.pitch *= .5;
	rot.roll *= .5;

	// looking
	if (!client->isConsoleActive()) {
		if (mainEngine->isMouseRelative() && cvar_mouselook.toInt() == localID) {
			if (cvar_mouseSmooth.toInt()) {
				mouseX = mouseX * .75f + mainEngine->getMouseMoveX() / 4.f;
				mouseY = mouseY * .75f + (cvar_mouseYInvert.toInt() ? mainEngine->getMouseMoveY() * -.25f : mainEngine->getMouseMoveY() / 4.f);
			} else {
				mouseX = mainEngine->getMouseMoveX();
				mouseY = cvar_mouseYInvert.toInt() ? mainEngine->getMouseMoveY() * -1 : mainEngine->getMouseMoveY();
			}

			rot.yaw += mouseX * timeFactor * cvar_mouseSpeed.toFloat();
			rot.pitch += mouseY * timeFactor  * cvar_mouseSpeed.toFloat() * (cvar_zeroGravity.toInt() ? -1.f : 1.f);
		}
		rot.yaw += (input.analog("LookRight") - input.analog("LookLeft")) * timeFactor * 2.f;
		rot.pitch += (input.analog("LookDown") - input.analog("LookUp")) * timeFactor * 2.f * (cvar_zeroGravity.toInt() ? -1.f : 1.f);
		if (cvar_zeroGravity.toInt()) {
			rot.roll += (buttonLeanRight - buttonLeanLeft) * timeFactor * .5f;
		} else {
			rot.roll = 0.f;
		}
	}
	rot.wrapAngles();

	// change look dir

	if (cvar_zeroGravity.toInt()) {
		Rotation lookDir = entity->getLookDir();
		if (lookDir.yaw > 0.f) {
			lookDir.yaw = max(0.f, lookDir.yaw - 0.01f);
		}
		if (lookDir.yaw < 0.f) {
			lookDir.yaw = min(0.f, lookDir.yaw + 0.01f);
		}
		if (lookDir.pitch > 0.f) {
			lookDir.pitch = max(0.f, lookDir.pitch - 0.01f);
		}
		if (lookDir.pitch < 0.f) {
			lookDir.pitch = min(0.f, lookDir.pitch + 0.01f);
		}
		entity->setLookDir(lookDir);
	} else {
		float hLimit = cvar_maxHTurn.toFloat() * PI / 180.f;
		float vLimit = cvar_maxVTurn.toFloat() * PI / 180.f;
		Rotation lookDir = entity->getLookDir();
		lookDir.yaw += rot.yaw;
		lookDir.yaw = fmod(lookDir.yaw, PI);
		if (lookDir.yaw > hLimit) {
			float diff = lookDir.yaw - hLimit;
			lookDir.yaw = hLimit;
			rot.yaw = diff;
		} else if (lookDir.yaw < -hLimit) {
			float diff = lookDir.yaw + hLimit;
			lookDir.yaw = -hLimit;
			rot.yaw = diff;
		} else {
			rot.yaw = 0.f;
		}
		lookDir.pitch += rot.pitch;
		lookDir.pitch = fmod(lookDir.pitch, PI);
		if (lookDir.pitch > vLimit) {
			lookDir.pitch = vLimit;
		} else if (lookDir.pitch < -vLimit) {
			lookDir.pitch = -vLimit;
		}
		entity->setLookDir(lookDir);

		// don't actually turn the entity vertically
		rot.pitch = 0.f;
	}

	// orient to ground
	if (cvar_wallWalk.toInt() && !cvar_zeroGravity.toInt()) {
		if (nearestFloor <= standScale.z + 16.f && !up.close(floorHit.normal)) {
			if (floorHit.normal.lengthSquared() > 0.f) {
				const float dot = up.dot(floorHit.normal);
				const float slopeLimit = cosf(cvar_wallWalkLimit.toFloat() * PI / 180.f);
				if (dot > slopeLimit) {
					Vector Y(floorHit.normal.x, -floorHit.normal.z, floorHit.normal.y);
					Vector X = Y.cross(Vector(right.x, -right.z, right.y));
					Vector Z = X.cross(Y);
					glm::mat4 m = glm::mat4(glm::mat3(
						X.x, X.y, X.z,
						Y.x, Y.y, Y.z,
						Z.x, Z.y, Z.z
					));
					Quaternion q(m);
					playerAng = q;
					orienting = true;
					orient = 0.f;

					Vector down = (playerAng * Quaternion(Rotation(0.f, PI / 2.f, 0.f))).toVector();
					vel -= vel * down.absolute();
				}
			}
		}
	}
	if (orienting) {
		playerAng = playerAng * Quaternion(rot);
		orient += timeFactor;
		if (orient > 1.f) {
			orienting = false;
			orient = 1.f;
		}
		entity->setAng(entity->getAng().slerp(playerAng, orient));
		warpNeeded = true;
	} else {
		playerAng = entity->getAng();
	}

	//Interacting with entities.
	if (!client->isConsoleActive()) {
		World* world = entity->getWorld();
		if (camera && world)
		{
			if (input.binaryToggle("Interact")) {
				if (holdingInteract)
				{
					// 60hz
					interactHoldTime += 1 / 60;
				}
				holdingInteract = true;
				input.consumeBinaryToggle("Interact");
				Vector start = camera->getGlobalPos();
				Vector dest = start + camera->getGlobalAng().toVector() * 128;
				World::hit_t hit = entity->lineTrace(start, dest);

				if (hit.manifest && hit.manifest->entity)
				{
					Entity* hitEntity = hit.manifest->entity;
					previousInteractedEntity = hitEntity;

					if (previousInteractedEntity->isPickupable())
					{
						entity->depositInAvailableSlot(previousInteractedEntity);
					}

					auto hitBBox = hit.manifest->bbox;
					if (hitBBox)
					{
						if (hitEntity->isFlag(Entity::flag_t::FLAG_INTERACTABLE))
						{
							mainEngine->fmsg(Engine::MSG_DEBUG, "clicked on entity '%s': UID %d", hitEntity->getName().get(), hitEntity->getUID());
							Packet packet;
							packet.write32(hitBBox->getUID());
							packet.write32(hitEntity->getUID());
							packet.write32(client->indexForWorld(world));
							packet.write32(localID);
							packet.write("ESEL");
							client->getNet()->signPacket(packet);
							client->getNet()->sendPacketSafe(0, packet);
						}
					}
				}
			} else
			{
				holdingInteract = false;

				interactHoldTime = 0;
			}
			// Toggling inventory
			if (input.binaryToggle("Status"))
			{
				input.consumeBinaryToggle("Status");
				entity->setInventoryVisibility(!inventoryVisible);
				inventoryVisible = !inventoryVisible;
			}
		}
	}

	// update entity vectors
	Vector standingOnVel;
	originalVel = vel;
	if (entityStandingOn) {
		standingOnVel = entityStandingOn->getVel();
	}
	entity->setVel(vel + standingOnVel);
	Rotation finalRot = orienting ? Rotation() : rot;
	entity->setRot(finalRot);
	entity->update();
	if (warpNeeded) {
		Quaternion ang = entity->getAng();
		ang = ang * Quaternion(finalRot);
		entity->setAng(ang);
		entity->warp();
	}

	// using hand items (shooting)
	if (lTool && input.binaryToggle("HandLeft")) {
		Uint32 bone = lTool->findBoneIndex("emitter");
		glm::mat4 mat = lTool->getGlobalMat();
		if (bone != UINT32_MAX) {
			mat *= lTool->findBone(bone);
		}
		auto red = WideVector(1.f, 0.f, 0.f, 1.f);
		lTool->shootLaser(mat, red, 8.f, 20.f);
	}
	if (rTool && input.binaryToggle("HandRight")) {
		Uint32 bone = rTool->findBoneIndex("emitter");
		glm::mat4 mat = rTool->getGlobalMat();
		if (bone != UINT32_MAX) {
			mat *= rTool->findBone(bone);
		}
		auto red = WideVector(1.f, 0.f, 0.f, 1.f);
		rTool->shootLaser(mat, red, 8.f, 20.f);
	}
	input.consumeBinaryToggle("HandLeft");
	input.consumeBinaryToggle("HandRight");

	// lamp
	if (lamp && input.binaryToggle("Inventory1")) {
		lamp->setIntensity(lamp->getIntensity() == 0.f ? 1.f : 0.f);
	}
	input.consumeBinaryToggle("Inventory1");
}

void Player::updateCamera() {
	Client* client = mainEngine->getLocalClient();
	if (!entity || !client) {
		return;
	}

	Input& input = mainEngine->getInput(localID);

	bobAngle += PI / 15.f;
	if (bobAngle > PI*2.f) {
		bobAngle -= PI * 2.f;
	}
	if (moving && cvar_enableBob.toInt()) {
		bobLength = min(bobLength + 0.1f, 1.f);
	} else {
		bobLength = max(bobLength - 0.1f, 0.f);
	}

	// move camera
	if (camera) {
		camera->setLocalPos(originalCameraPos);
		camera->setLocalAng(entity->getLookDir());

		// bobbing
		if (cvar_enableHeadBoneCamera.toInt()) {
			if (head) {
				head->updateSkin();
				Uint32 headBone = head->findBoneIndex("Bone_Head");
				if (headBone != UINT32_MAX) {
					auto mat = head->findBone(headBone);
					auto pos = Vector(mat[3][0], mat[3][2], -mat[3][1]);
					camera->setLocalPos(pos + models->getLocalPos());
					camera->translate(Vector(16.f, 0.f, 0.f));
				}
			}
		}
		if (cvar_enableBob.toInt()) {
			camera->translate(Vector(0.f, 0.f, sinf(bobAngle) * 4.f * bobLength));
		}

		// set listener for player 1
		if (localID == 0) {
			client->getMixer()->setListener(camera);
		}

		// set screen size
		Rect<Sint32> rect;
		int localPlayerCount = client->numLocalPlayers();
		if (localPlayerCount < 2) {
			rect.x = 0;
			rect.w = Frame::virtualScreenX;
			rect.y = 0;
			rect.h = Frame::virtualScreenY;
		} else if (localPlayerCount < 3) {
			rect.x = 0;
			rect.w = Frame::virtualScreenX;
			rect.y = (localID % 2) * (Frame::virtualScreenY / 2);
			rect.h = Frame::virtualScreenY / 2;
		} else {
			rect.x = (localID % 2) * (Frame::virtualScreenX / 2);
			rect.w = Frame::virtualScreenX / 2;
			rect.y = (localID > 1) * (Frame::virtualScreenY / 2);
			rect.h = Frame::virtualScreenY / 2;
		}
		camera->setWin(rect);

		camera->update();
	}
}

bool Player::despawn() {
	if (!entity) {
		return false;
	}
	entity->remove();
	entity = nullptr;
	return true;
}

void Player::onEntityDeleted(Entity* _entity) {
	if (entity == _entity) {
		entity = nullptr;
	}
	return;
}
