// Item.hpp

#pragma once

#include "Main.hpp"
#include "String.hpp"
#include "ArrayList.hpp"
#include "Map.hpp"
#include "Inventory.hpp"
#include "WideVector.hpp"

class Item {
public:
	Item() {
	}

	typedef ArrayList<String> StringList;
	typedef Map<String, String> StringMap;

	struct Action {
		StringList	comboItems;							// a list of items which must be simultaneously activated to trigger this action

		float		damage = 0.f;						// damage value
		String		damageType;							// damage type

		float		leadInTime = 0.f;					// lead-in time in seconds
		float		recoverTime = 0.f;					// recovery time in seconds

		float		radius;								// spherical radius from point of contact
		bool		radiusFalloff = false;				// measured in conical degrees with the player’s reticle acting as a conical center-point

		bool		shootBullet = false;				// fire a hitscan bullet
		float		distance = 0.f;						// used by lasers as well, measured in meters
		float		spread = 0.f;						// measured in conical degrees centered on player reticle

		bool		shootLaser = false;					// shoots a laser
		WideVector	laserColor;							// color for laser bullet
		float		laserSize = 1.f;					// width of the laser

		bool		shootProjectile = false;			// shoot a projectile instead of a hitscan
		float		gravity = 0.f;						// projectile gravity in m/s
		float		speed = 0.f;						// projectile speed in m/s
		//Entity	projectile;							// projectile cosmetics n stuff

		// save/load this object to a file
		// @param file interface to serialize with
		void serialize(FileInterface * file);
	};

	// save/load this object to a file
	// @param file interface to serialize with
	void serialize(FileInterface * file);

	// init inventory slots
	void InitInventory();

	// deposit item into inventory slot
	void depositItem(Entity* itemToDeposit, String invSlot);

	// check if a slot is currently filled
	bool isSlotFilled(String invSlot);

	//returns entity filling a slot
	class Entity* getSlottedItem(String invSlot);

private:
	/*struct ConditionalEffect {
		String effect;
		float ammoSpendRate = 0.f;
		float ammoSpendUse = 0.f;

		// save/load this object to a file
		// @param file interface to serialize with
		void serialize(FileInterface * file);
	};
	typedef Map<ArrayList<String>, ConditionalEffect> Conditions;*/

	String		icon				= "null.png";		// icon

	float		weight				= 1.f;				// weight
	float		value				= 1.f;				// base price

	// a thrown item calls thrown(item, victim) on contact
	bool		throwable			= false;			// hold action + drop button throws the item
	float		throwLeadInTime		= 1.f;				// time it takes to animate a throw in secs
	float		throwRecovery		= 1.f;				// time after throwing before another action can be done
	bool		throwDestroys		= false;			// whether it destroys on contact after throwing

	bool		takesDamage			= false;			// the item can be damaged and destroyed
	float		health				= 0.f;				// amount of damage before item is destroyed
	StringList	damageImmunities;						// a list of damage types the item is immune to

	bool		detonates			= false;			// item explodes when destroyed
	String		detonationDamageType;					// what kind of damage the explosion does
	float		detonationRadius	= 0.f;				// how big the explosion radius is
	float		detonationDamage	= 0.f;				// base damage applied to targets within the explosion radius
	bool		detonationFalloff	= false;			// if true, damage falls off for distance

	StringMap	slotEffects;							// effect to apply while item is in a given inventory slot

	float		distance			= 0.f;				// effect distance
	float		spread				= 0.f;				// effect spread cone arc
	float		radius				= 0.f;				// effect radius

	float		animLeadSpeed		= 0.f;				// pre-effect animation time
	float		animRecovSpeed		= 0.f;				// post-effect animation time

	Sint32		spendCharges		= 0;				// ammo to spend per use
	Sint32		currCharges			= 0;				// current ammo capacity
	Sint32		maxCharges			= 0;				// max ammo capacity
	float		rechargeRate		= 0.f;				// recharge rate (charges/sec)

	float		currCooldown		= 0.f;				// current cooldown time between uses
	float		maxCooldown			= 0.f;				// max cooldown time between uses

	StringList	slotRestrictions;						// list of slots that this item cannot be in

	//Conditions	conditionalEffects;

	Map<String, Action> actions;						// map actions by name (eg "tap", "hold")

	Inventory Inventory;
};