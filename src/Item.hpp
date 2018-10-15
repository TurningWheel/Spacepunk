// Item.hpp

#pragma once

#include "Main.hpp"
#include "String.hpp"

class Item {
public:
	enum flag_t {
		ITEMFLAG_HELMET				= 1 << 0,
		ITEMFLAG_SUIT				= 1 << 1,
		ITEMFLAG_BOOTS				= 1 << 2,
		ITEMFLAG_GLOVES				= 1 << 3,
		ITEMFLAG_GEAR				= 1 << 4,		// catch-all for the above

		ITEMFLAG_HANDRIGHT			= 1 << 5,
		ITEMFLAG_HANDLEFT			= 1 << 6,
		ITEMFLAG_TOOL				= 1 << 7,		// catch-all for the above

		ITEMFLAG_SLOTRESTRICT		= 1 << 8,
		ITEMFLAG_TAPFIRE			= 1 << 9,
		ITEMFLAG_HOLDFIRE			= 1 << 10,
		ITEMFLAG_HOLDRELEASE		= 1 << 11,
		ITEMFLAG_THROWN				= 1 << 12,
		ITEMFLAG_DETONATES			= 1 << 13,
		ITEMFLAG_VOLATILE			= 1 << 14,
		ITEMFLAG_RECHARGES			= 1 << 15
	};

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
		DAMAGE_POISON
	};

	struct vars_t {
		float		distance			= 0.f;				// effect distance
		float		spread				= 0.f;				// effect spread cone arc
		float		radius				= 0.f;				// effect radius

		float		animLeadSpeed		= 0.f;				// pre-effect animation time
		float		animRecovSpeed		= 0.f;				// post-effect animation time

		float		damageValue			= 0.f;				// damage value
		damage_t	damageType			= DAMAGE_BASIC;		// damage type
		Sint32		ammoSpend			= 0;				// ammo spend

		Sint32		currCharges			= 0;				// internal ammo capacity
		Sint32		maxCharges			= 0;				// max ammo capacity

		float		currCooldown		= 0.f;				// current cooldown time between uses
		float		maxCooldown			= 0.f;				// max cooldown time between uses
	};

	// in script:
	// passiveQuick() function
	// passiveHold() function
	// passiveEquip() function
};