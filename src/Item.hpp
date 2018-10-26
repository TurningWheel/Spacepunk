// Item.hpp

#pragma once

#include "Main.hpp"
#include "String.hpp"
#include "ArrayList.hpp"

class Item {
public:
	enum type_t {
		ITEMFLAG_HELMET				= 1 << 1,
		ITEMFLAG_SUIT				= 1 << 2,
		ITEMFLAG_BOOTS				= 1 << 3,
		ITEMFLAG_GLOVES				= 1 << 4,
		ITEMFLAG_AMULET				= 1 << 5,
		ITEMFLAG_RING				= 1 << 6,
		ITEMFLAG_SCROLL				= 1 << 7,
		ITEMFLAG_SPELLBOOK			= 1 << 8,
		ITEMFLAG_BOOK				= 1 << 9,
		ITEMFLAG_MELEE				= 1 << 10,
		ITEMFLAG_RANGED				= 1 << 11,
		ITEMFLAG_SPELL				= 1 << 12,
		ITEMFLAG_CONSUMABLE			= 1 << 13,
		ITEMFLAG_TOOL				= 1 << 14,

		ITEMFLAG_SLOTRESTRICT		= 1 << 8,
		ITEMFLAG_TAPFIRE			= 1 << 9,
		ITEMFLAG_HOLDFIRE			= 1 << 10,
		ITEMFLAG_HOLDRELEASE		= 1 << 11,
		ITEMFLAG_THROWN				= 1 << 12,
		ITEMFLAG_DETONATES			= 1 << 13,
		ITEMFLAG_VOLATILE			= 1 << 14,
		ITEMFLAG_RECHARGES			= 1 << 15
	};

	// fire type
	enum fire_t {
		TAPFIRE,
		HOLDFIRE,
		HOLDRELEASE,
		FIRE_NUM
	};
	static const char* fireStr[static_cast<int>(fire_t::FIRE_NUM)];

	// damage types
	enum damage_t {
		DAMAGE_BASIC,
		DAMAGE_BLUNT,
		DAMAGE_PIERCE,
		DAMAGE_SLASH,
		DAMAGE_FIRE,
		DAMAGE_ICE,
		DAMAGE_PLASMA,
		DAMAGE_LASER,
		DAMAGE_WATER,
		DAMAGE_EARTH,
		DAMAGE_AIR,
		DAMAGE_ETHER,
		DAMAGE_KINETIC,
		DAMAGE_ELECTRIC,
		DAMAGE_RADIATION,
		DAMAGE_CAUSTIC,
		DAMAGE_CRYO,
		DAMAGE_POISON,
		DAMAGE_HEAL,
		DAMAGE_NUM
	};
	static const char* damageStr[static_cast<int>(damage_t::DAMAGE_NUM)];

	// damage info
	struct damageinfo_t {
		damage_t type;
		Sint32 amount; // positive hurts, negative heals
		float radius;
	};

private:
	Sint32		weight				= 1;				// weight
	Sint32		value				= 1;				// base price
	Uint32		flags				= 0;				// flags

	float		distance			= 0.f;				// effect distance
	float		spread				= 0.f;				// effect spread cone arc
	float		radius				= 0.f;				// effect radius

	float		animLeadSpeed		= 0.f;				// pre-effect animation time
	float		animRecovSpeed		= 0.f;				// post-effect animation time

	Sint32		spendCharges		= 0;				// ammo to spend per use
	Sint32		currCharges			= 0;				// current ammo capacity
	Sint32		maxCharges			= 0;				// max ammo capacity
	Sint32		rechargeRate		= 0;				// recharge rate

	float		currCooldown		= 0.f;				// current cooldown time between uses
	float		maxCooldown			= 0.f;				// max cooldown time between uses

	ArrayList<damageinfo_t> damages;

	// in script:
	// passiveQuick() function
	// passiveHold() function
	// passiveEquip() function
};